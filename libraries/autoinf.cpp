#include "autoinf.hpp"
#include "autoinf.using"

namespace autoinf {

const core::Version VERSION{LIB_MAJ, LIB_MIN}; DEPENDENCIES;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

iu64 Signature::BYTE_COUNTS[256] = {0,};

Signature::Signature () : h(0) {
}

size_t Signature::hash () const noexcept {
  return h;
}

bool operator== (const Signature &l, const Signature &r) noexcept {
  return l.h == r.h && l.b == r.b;
}

bool operator< (const Signature &l, const Signature &r) noexcept {
  return l.b < r.b;
}

Signature::Writer::Writer (Signature &signature) noexcept
  : signature(signature), zeroByteCount(0)
{
}

void Signature::Writer::flushZeroBytes () {
  if (zeroByteCount == 0) {
    return;
  }

  // Bytes appear literally, except for ESCAPE. ESCAPE + x as an ieu means a run
  // of x 0s when x != 0 and ESCAPE when x == 0.
  DSA(ESCAPE != 0, "");
  if (zeroByteCount < 3) {
    iu8f b[2] = {0, 0};
    signature.b.append(b, zeroByteCount);
  } else {
    iu8f b[1 + numeric_limits<decltype(zeroByteCount)>::max_ie_octets] = {ESCAPE,};
    iu8f *bEnd = b + 1;
    setIeu(bEnd, zeroByteCount);
    signature.b.append(b, bEnd);
  }
  BYTE_COUNTS[0] += zeroByteCount;
  zeroByteCount = 0;
}

void Signature::Writer::appendByte (iu8 byte) {
  if (byte == 0) {
    appendZeroBytes(1);
  } else {
    flushZeroBytes();
    ++BYTE_COUNTS[byte];
    if (byte == ESCAPE) {
      iu8f b[2] = {ESCAPE, 0};
      signature.b.append(b, 2);
    } else {
      signature.b.push_back(byte);
    }
  }
}

void Signature::Writer::appendZeroBytes (iu byteCount) {
  zeroByteCount += byteCount;
}

void Signature::Writer::close () {
  flushZeroBytes();
  signature.b.shrink_to_fit();

  const iu8f *i = signature.b.data();
  signature.h = hashImpl(i, i + signature.b.size());

  DW(, "finished writing sig of hash ", signature.h);
  DW(, "SSSS updated signature stats:");
  for (iu i = 0; i != 256; ++i) {
    if (BYTE_COUNTS[i] != 0) {
      DW(, "SSSS   byte value ", i, " count = ", BYTE_COUNTS[i]);
    }
  }
}

Multiverse::Node::Node (iu id, Signature &&signature, State &&state)
  : id(id), signature(move(signature)), state(new State(move(state))), children(0)
{
  DW(, "created new Node with sig of hash ", this->signature.hash(), " and state size ", this->state->getSize());
}

iu Multiverse::Node::getId () const {
  return id;
}

const Signature &Multiverse::Node::getSignature () const {
  return signature;
}

const State *Multiverse::Node::getState () const {
  return state.get();
}

void Multiverse::Node::addChild (ActionId actionId, string &&output, Node *node) {
  DW(, "adding to Node with sig of hash ", signature.hash(), " child of action id ", actionId, ":");
  DW(, "  output is **", output.c_str(), "**");
  DW(, "  dest. Node has sig of hash ", node->getSignature().hash());
  DPRE(!!state, "children cannot be added after all have been added");
  DPRE(children.empty() || get<0>(children.back()) < actionId, "children must be added in order of actionId");

  children.emplace_back(actionId, move(output), node);
}

void Multiverse::Node::allChildrenAdded () {
  DW(, "finished adding all ", children.size(), " children to Node with sig of hash ", signature.hash());
  state.reset();
  children.shrink_to_fit();
}

size_t Multiverse::Node::getChildCount () const {
  return children.size();
}

const tuple<Multiverse::ActionId, string, Multiverse::Node *> &Multiverse::Node::getChild(size_t i) const {
  return children[i];
}

const tuple<Multiverse::ActionId, string, Multiverse::Node *> *Multiverse::Node::getChildByActionId (ActionId id) const {
  auto i = lower_bound(children.begin(), children.end(), id, [] (const tuple<ActionId, string, Node *> &elmt, const ActionId &target) {
    return get<0>(elmt) < target;
  });
  if (i == children.end() || get<0>(*i) != id) {
    return nullptr;
  }
  return &(*i);
}

Multiverse::NodePath::NodePath () {
}

void Multiverse::NodePath::append (ActionId child) {
  path.push_back(child);
}

void Multiverse::NodePath::pop () {
  path.pop_back();
}

Multiverse::Node *Multiverse::NodePath::resolve (Node *node) const {
  for (const ActionId &child : path) {
    auto r = node->getChildByActionId(child);
    if (!r) {
      return nullptr;
    }
    node = get<2>(*r);
  }
  return node;
}

Multiverse::Multiverse (
  Vm &vm, const string &initialInput, string &r_initialOutput, const string &saveActionInput, const string &restoreActionInput,
  const vector<string> &words, const vector<vector<string>> &actionTemplates
) : saveActionInput(saveActionInput), restoreActionInput(restoreActionInput), rootNode(nullptr) {
  DS();
  DPRE(vm.isAlive());

  initActionInputs(words, actionTemplates, actionInputs, actionInputBegins);
  DW(, "all actions are this: **", actionInputs.c_str(), "**");

  doAction(vm, initialInput, r_initialOutput, "VM died while running the initial input");
  Signature signature = createSignature(vm);
  State state;
  doSaveAction(vm, &state);
  DW(, "root node save state has size ",state.getSize());

  unique_ptr<Node> node(new Node(42 /* XXXX */, move(signature), move(state)));
  rootNode = node.get();
  nodesBySignature.emplace(ref(rootNode->getSignature()), rootNode);
  node.release();
}

void Multiverse::initActionInputs (const vector<string> &words, const vector<vector<string>> &actionTemplates, string &r_actionInputs, vector<size_t> &r_actionInputBegins) {
  DS();
  DPRE(r_actionInputs.empty());
  DPRE(r_actionInputBegins.empty());

  string actionInput;
  for (auto &actionTemplate : actionTemplates) {
    DPRE(actionTemplate.size() != 0, "action templates must have at least one component");
    actionInput.clear();
    actionInput.append(actionTemplate[0]);
    initActionInputsImpl(words, actionTemplate.begin() + 1, actionTemplate.end(), r_actionInputs, r_actionInputBegins, actionInput);
  }
  r_actionInputBegins.push_back(r_actionInputs.size());
  r_actionInputs.shrink_to_fit();
  r_actionInputBegins.shrink_to_fit();
}

void Multiverse::initActionInputsImpl (const vector<string> &words, vector<string>::const_iterator actionTemplateI, vector<string>::const_iterator actionTemplateEnd, string &r_actionInputs, vector<size_t> &r_actionInputBegins, string &r_actionInput) {
  if (actionTemplateI == actionTemplateEnd) {
    if (r_actionInputBegins.size() - 1 == numeric_limits<ActionId>::max()) {
      throw GeneralException("the limit for the number of actions has been reached");
    }
    DW(, "adding action **", r_actionInput.c_str(), "**");
    r_actionInputBegins.push_back(r_actionInputs.size());
    r_actionInputs.append(r_actionInput);
    return;
  }

  size_t actionInputPreSize = r_actionInput.size();
  string nw = *(actionTemplateI++);
  for (const string &word : words) {
    r_actionInput.append(word);
    r_actionInput.append(nw);
    initActionInputsImpl(words, actionTemplateI, actionTemplateEnd, r_actionInputs, r_actionInputBegins, r_actionInput);
    r_actionInput.resize(actionInputPreSize);
  }
}

Multiverse::~Multiverse () noexcept {
  for (auto &e : nodesBySignature) {
    delete get<1>(e);
  }
}

// XXXX move to core
ptrdiff_t ptrdiff (size_t s) {
  return static_cast<ptrdiff_t>(s);
}
size_t size (ptrdiff_t s) {
  DPRE(s >= 0, "s must be non-negative");
  return static_cast<size_t>(s);
}

tuple<string::const_iterator, string::const_iterator> Multiverse::getActionInput (ActionId id) const {
  DPRE(id < actionInputBegins.size() - 1);

  auto begin = actionInputs.cbegin();
  // XXXX worry about this junk...
  return tuple<string::const_iterator, string::const_iterator>(begin + ptrdiff(actionInputBegins[id]), begin + ptrdiff(actionInputBegins[static_cast<size_t>(id) + 1]));
}

void Multiverse::doAction(Vm &vm, string::const_iterator inputBegin, string::const_iterator inputEnd, string &r_output, const char *deathExceptionMsg) {
  DPRE(vm.isAlive());

  r_output.clear();
  vm.doAction(inputBegin, inputEnd, r_output);
  if (!vm.isAlive()) {
    throw GeneralException(deathExceptionMsg);
  }
}

void Multiverse::doAction(Vm &vm, const string &input, string &r_output, const char *deathExceptionMsg) {
  doAction(vm, input.begin(), input.end(), r_output, deathExceptionMsg);
}

void Multiverse::doSaveAction (Vm &vm, State *state) { // XXXX referenceise these?
  string o;
  vm.setSaveState(state);
  doAction(vm, saveActionInput, o, "VM died while saving a state");
  if (vm.getSaveCount() == 0) {
    throw GeneralException("save action didn't cause saving");
  }
}

void Multiverse::doRestoreAction (Vm &vm, const State *state) {
  string o;
  vm.setRestoreState(state);
  doAction(vm, restoreActionInput, o, "VM died while restoring a state");
  if (vm.getRestoreCount() == 0) {
    throw GeneralException("restore action didn't cause restoration");
  }
}

Signature Multiverse::createSignature (Vm &vm /*, XXXX Memset ignored */) {
  DS();
  DPRE(vm.isAlive());

  iu16 memSize = vm.getDynamicMemorySize();
  const zbyte *mem = vm.getDynamicMemory();
  const zbyte *initialMem = vm.getInitialDynamicMemory();

  Signature signature;

  size_t equalCount = 0;
  Signature::Writer writer(signature);
  for (iu16 i = 0; i < memSize; ++i) {
    // XXXX something like memcmp, but that returns the first point where the two blocks don't match
    equalCount += mem[i] == initialMem[i];
    writer.appendByte(mem[i] ^ initialMem[i]);
    // XXXX stats on signature for numbers of each chr - to pick best choice for escape char?
  }
  DW(, "unchanged: ", equalCount, " / ", memSize, " bytes");
  writer.close();

  return signature;
}

Multiverse::Node *Multiverse::getNode (const NodePath &nodePath) const {
  return nodePath.resolve(rootNode);
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
