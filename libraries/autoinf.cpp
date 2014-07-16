#include "autoinf.hpp"
#include "autoinf.using"

namespace autoinf {

const core::Version VERSION{LIB_MAJ, LIB_MIN}; DEPENDENCIES;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

// XXXX move to core
/*
// XXXX -> spanto templ fn with fwd precondition
// XXXX -> can we overload ptr + size_t? it's not normally defined?
ptrdiff_t ptrdiff (size_t s) {
  return static_cast<ptrdiff_t>(s);
}
size_t size (ptrdiff_t s) {
  DPRE(s >= 0, "s must be non-negative");
  return static_cast<size_t>(s);
}*/
template<typename _I> _I operator+ (_I i, size_t o) {
  return i + static_cast<ptrdiff_t>(o);
}

iu64 Signature::BYTE_COUNTS[256] = {0,};

u8string Signature::wrXXXX () const {
  const char8_t HEX_DIGITS[] = "0123456789ABCDEF";

  u8string o;

  o.append(u8("{"));

  bool first = true;
  const iu8f *i = &b[0];
  const iu8f *end = i + b.size();
  while (i != end) {
    if (!first) {
      o.push_back(U' ');
    }
    first = false;

    iu8 b = *(i++);
    if (b != ESCAPE) {
      o.push_back(HEX_DIGITS[b >> 4]);
      o.push_back(HEX_DIGITS[b & 0b1111]);
    } else {
      auto v = getValidIeu<iu32>(i);
      DA(v != 1 && v != 2);
      if (v == 0) {
        o.push_back(HEX_DIGITS[ESCAPE >> 4]);
        o.push_back(HEX_DIGITS[ESCAPE & 0b1111]);
      } else {
        std::basic_ostringstream<char> os;
        os << v;
        os.flush();

        o.append(reinterpret_cast<const char8_t *>(os.str().c_str()));
        o.append(u8("*0"));
      }
    }
  }

  o.push_back(U'}');

  return o;
}

Signature::Signature () : h(0) {
}

size_t Signature::hash () const noexcept {
  return h;
}

bool operator== (const Signature &l, const Signature &r) noexcept {
  return l.h == r.h && l.b == r.b;
}

Signature::Iterator Signature::begin () const {
  return Iterator(*this);
}

Signature::Iterator Signature::end () const {
  return Iterator();
}

Signature::Writer::Writer (Signature &signature) noexcept :
  signature(signature), zeroByteCount(0)
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
  /*DW(, "SSSS updated signature stats:");
  for (iu i = 0; i != 256; ++i) {
    if (BYTE_COUNTS[i] != 0) {
      DW(, "SSSS   byte value ", i, " count = ", BYTE_COUNTS[i]);
    }
  }*/
}

Signature::Iterator::Iterator () noexcept : i(nullptr), end(nullptr), currentByte(0), zeroByteCount(0) {
}

Signature::Iterator::Iterator (const Signature &signature) noexcept : i(signature.b.data()), end(i + signature.b.size()) {
  advance();
}

void Signature::Iterator::advance () noexcept {
  if (i == end) {
    i = end = nullptr;
    currentByte = 0;
    zeroByteCount = 0;
    return;
  }

  iu8f b = *(i++);
  if (b != ESCAPE) {
    currentByte = b;
    zeroByteCount = 0;
  } else {
    auto v = getValidIeu<decltype(zeroByteCount)>(i);
    DA(v != 1 && v != 2);
    if (v == 0) {
      currentByte = ESCAPE;
      zeroByteCount = 0;
    } else {
      currentByte = 0;
      zeroByteCount = v;
    }
  }
}

void Signature::Iterator::inc () noexcept {
  if (zeroByteCount != 0) {
    DA(currentByte == 0);
    --zeroByteCount;
    if (zeroByteCount != 0) {
      return;
    }
  }

  advance();
}

const iu8f &Signature::Iterator::operator* () const noexcept {
  return currentByte;
}

Signature::Iterator &Signature::Iterator::operator++ () noexcept {
  inc();
  return *this;
}

Signature::Iterator Signature::Iterator::operator++ (int) noexcept {
  Iterator v(*this);
  ++*this;
  return v;
}

bool operator== (const Signature::Iterator &l, const Signature::Iterator &r) noexcept {
  return l.i == r.i;
}

bool operator!= (const Signature::Iterator &l, const Signature::Iterator &r) noexcept {
  return !(l == r);
}

Signature::Iterator &Signature::Iterator::operator+= (size_t r) noexcept {
  while (r != 0) {
    DPRE(i, "r must not exceed bounds");
    if (zeroByteCount != 0) {
      if (zeroByteCount > r) {
        zeroByteCount -= r;
        break;
      }
      r -= zeroByteCount;
    } else {
      --r;
    }
    advance();
  }

  return *this;
}

Signature::Iterator operator+ (Signature::Iterator l, size_t r) noexcept {
  l += r;
  return l;
}


void Signature::Iterator::copy (Writer &r_writer, size_t r) {
  while (r != 0) {
    DPRE(i, "r must not exceed bounds");
    if (zeroByteCount != 0) {
      if (zeroByteCount > r) {
        zeroByteCount -= r;
        r_writer.appendZeroBytes(r);
        break;
      }
      r -= zeroByteCount;
      r_writer.appendZeroBytes(zeroByteCount);
    } else {
      --r;
      r_writer.appendByte(currentByte);
    }
    advance();
  }
}

Multiverse::Bitranges::Bitranges (const Bitset &bitset, iu16 rangesEnd) : vector() {
  DS();
  DW(, "creating Bitranges from a bitset:");
  DPRE(bitset.getNextSetBit(rangesEnd) == Bitset::NON_INDEX);

  for (size_t end = 0; end != rangesEnd;) {
    size_t prevEnd = end;
    size_t begin = bitset.getNextClearBit(end);
    end = bitset.getNextSetBit(begin + 1);
    if (end == Bitset::NON_INDEX) {
      end = rangesEnd;
    }
    this->push_back(BitrangesPart{static_cast<iu16f>(begin - prevEnd), static_cast<iu16f>(end - begin)});
    DA(end <= rangesEnd);
  }
  DA(std::accumulate(this->begin(), this->end(), static_cast<size_t>(0), [] (size_t a, const BitrangesPart &o) -> size_t {
    return a + o.setSize + o.clearSize;
  }) == rangesEnd);
  DW(, "a total of ", std::accumulate(this->begin(), this->end(), static_cast<size_t>(0), [] (size_t a, const BitrangesPart &o) -> size_t {
    return a + o.setSize;
  }), " bits are set");
}

Multiverse::Node::Node (Signature &&signature, State &&state) :
  signature(move(signature)), state(), children(0)
{
  if (!state.isEmpty()) {
    this->state.reset(new State(move(state)));
  }
  DW(, "created new Node with sig of hash ", this->signature.hash(), " and state size ", this->state ? static_cast<is64>(this->state->getSize()) : -1);
}

const Signature &Multiverse::Node::getSignature () const {
  return signature;
}

const State *Multiverse::Node::getState () const {
  return state.get();
}

void Multiverse::Node::clearState () {
  DW(, "done with trying to process Node with sig of hash ", signature.hash());
  state.reset();
}

void Multiverse::Node::addChild (ActionId actionId, u8string &&output, Node *node) {
  DW(, "adding to Node with sig of hash ", signature.hash(), " child of action id ", actionId, ":");
  DW(, "  output is **", output.c_str(), "**");
  DW(, "  dest. Node has sig of hash ", node->getSignature().hash());
  DPRE(!!state, "children cannot be added after all have been added");
  DPRE(children.empty() || get<0>(children.back()) < actionId, "children must be added in order of actionId");

  children.emplace_back(actionId, move(output), node);
}

void Multiverse::Node::batchOfChildChangesCompleted () {
  DW(, "finished changing all ", children.size(), " children on Node with sig of hash ", signature.hash());
  children.shrink_to_fit();
}

size_t Multiverse::Node::getChildrenSize () const {
  return children.size();
}

const tuple<Multiverse::ActionId, u8string, Multiverse::Node *> &Multiverse::Node::getChild(size_t i) const {
  return children[i];
}

const tuple<Multiverse::ActionId, u8string, Multiverse::Node *> *Multiverse::Node::getChildByActionId (ActionId id) const {
  auto i = lower_bound(children.begin(), children.end(), id, [] (const tuple<ActionId, u8string, Node *> &elmt, const ActionId &target) {
    return get<0>(elmt) < target;
  });
  if (i == children.end() || get<0>(*i) != id) {
    return nullptr;
  }
  return &(*i);
}

Signature Multiverse::Node::setSignature (Signature &&signature) {
  Signature oldSignature(move(this->signature));
  this->signature = move(signature);
  return oldSignature;
}

void Multiverse::Node::removeChild (size_t i) {
  children.erase(children.begin() + i);
}

void Multiverse::Node::changeChild (size_t i, Node *node) {
  get<2>(children[i]) = node;
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
  Vm &vm, const u8string &initialInput, u8string &r_initialOutput, const u8string &saveActionInput, const u8string &restoreActionInput,
  const vector<u8string> &words, const vector<vector<u8string>> &actionTemplates
) : saveActionInput(saveActionInput), restoreActionInput(restoreActionInput), ignoredBytes(initIgnoredBytes(vm)), ignoredByteRanges(ignoredBytes, vm.getDynamicMemorySize()), rootNode(nullptr) {
  DS();
  DPRE(vm.isAlive());

  initActionInputs(words, actionTemplates, actionInputs, actionInputBegins);
  DW(, "all actions are this: **", actionInputs.c_str(), "**");

  doAction(vm, initialInput, r_initialOutput, u8("VM died while running the initial input"));
  Signature signature = createSignature(vm, ignoredByteRanges);
  State state;
  doSaveAction(vm, &state);
  DW(, "root node save state has size ",state.getSize());

  unique_ptr<Node> node(new Node(move(signature), move(state)));
  rootNode = node.get();
  nodes.emplace(ref(rootNode->getSignature()), rootNode);
  node.release();
}

// XXXX trial - handy for keeping signatures clean with 103
Bitset Multiverse::initIgnoredBytes (Vm &vm) {
  Bitset ignoredBytes;

  /*
  auto size = vm.getDynamicMemorySize();
  const zbyte *o0 = vm.getInitialDynamicMemory();
  const zbyte *o1 = vm.getDynamicMemory();
  const iu8f *vmWordSet = vm.getWordSet();
  for (size_t i = 0; i != size; ++i, ++o0, ++o1) {
    if (*o0 != *o1) {
      ignoredBytes.setBit(i);

      auto addr = i;
      if (((vmWordSet[addr >> 3] >> (addr & 0x7)) & 0b1) != 0) {
        ignoredBytes.setBit(addr + 1);
      }
      if (addr > 0 && ((vmWordSet[(addr-1) >> 3] >> ((addr-1) & 0x7)) & 0b1) != 0) {
        ignoredBytes.setBit(addr - 1);
      }
    }
  }
  */

  return ignoredBytes;
}

void Multiverse::initActionInputs (const vector<u8string> &words, const vector<vector<u8string>> &actionTemplates, u8string &r_actionInputs, vector<size_t> &r_actionInputBegins) {
  DS();
  DPRE(r_actionInputs.empty());
  DPRE(r_actionInputBegins.empty());

  u8string actionInput;
  for (const auto &actionTemplate : actionTemplates) {
    DPRE(actionTemplate.size() != 0, "action templates must have at least one component");
    actionInput.clear();
    actionInput.append(actionTemplate[0]);
    initActionInputsImpl(words, actionTemplate.begin() + 1, actionTemplate.end(), r_actionInputs, r_actionInputBegins, actionInput);
  }
  r_actionInputBegins.push_back(r_actionInputs.size());
  r_actionInputs.shrink_to_fit();
  r_actionInputBegins.shrink_to_fit();
}

void Multiverse::initActionInputsImpl (const vector<u8string> &words, vector<u8string>::const_iterator actionTemplateI, vector<u8string>::const_iterator actionTemplateEnd, u8string &r_actionInputs, vector<size_t> &r_actionInputBegins, u8string &r_actionInput) {
  if (actionTemplateI == actionTemplateEnd) {
    if (r_actionInputBegins.size() == numeric_limits<ActionId>::max()) {
      throw PlainException(u8("the limit for the number of actions has been reached"));
    }
    DW(, "adding action **", r_actionInput.c_str(), "**");
    r_actionInputBegins.push_back(r_actionInputs.size());
    r_actionInputs.append(r_actionInput);
    return;
  }

  size_t actionInputPreSize = r_actionInput.size();
  u8string nw = *(actionTemplateI++);
  for (const u8string &word : words) {
    r_actionInput.append(word);
    r_actionInput.append(nw);
    initActionInputsImpl(words, actionTemplateI, actionTemplateEnd, r_actionInputs, r_actionInputBegins, r_actionInput);
    r_actionInput.resize(actionInputPreSize);
  }
}

Multiverse::~Multiverse () noexcept {
  for (const auto &e : nodes) {
    delete get<1>(e);
  }
}

tuple<u8string::const_iterator, u8string::const_iterator> Multiverse::getActionInput (ActionId id) const {
  DPRE(id < actionInputBegins.size() - 1);

  auto begin = actionInputs.cbegin();
  return tuple<u8string::const_iterator, u8string::const_iterator>(begin + actionInputBegins[id], begin + actionInputBegins[static_cast<size_t>(id) + 1]);
}

void Multiverse::doAction(Vm &vm, u8string::const_iterator inputBegin, u8string::const_iterator inputEnd, u8string &r_output, const char8_t *deathExceptionMsg) {
  DPRE(vm.isAlive());

  r_output.clear();
  vm.doAction(inputBegin, inputEnd, r_output);
  if (!vm.isAlive()) {
    throw PlainException(deathExceptionMsg);
  }
}

void Multiverse::doAction(Vm &vm, const u8string &input, u8string &r_output, const char8_t *deathExceptionMsg) {
  doAction(vm, input.begin(), input.end(), r_output, deathExceptionMsg);
}

void Multiverse::doSaveAction (Vm &vm, State *state) { // XXXX referenceise these?
  u8string o;
  vm.setSaveState(state);
  doAction(vm, saveActionInput, o, u8("VM died while saving a state"));
  if (vm.getSaveCount() == 0) {
    throw PlainException(u8("save action didn't cause saving"));
  }
}

void Multiverse::doRestoreAction (Vm &vm, const State *state) {
  u8string o;
  vm.setRestoreState(state);
  doAction(vm, restoreActionInput, o, u8("VM died while restoring a state"));
  if (vm.getRestoreCount() == 0) {
    throw PlainException(u8("restore action didn't cause restoration"));
  }
}

Signature Multiverse::createSignature (const Vm &vm, const Bitranges &ignoredByteRanges) {
  DS();
  DPRE(vm.isAlive());

  Signature signature;

  const zbyte *mem = vm.getDynamicMemory();
  const zbyte *initialMem = vm.getInitialDynamicMemory();
  Signature::Writer writer(signature);
  size_t ec = 0, ms = 0;
  for (const auto &part : ignoredByteRanges) {
    //DW(, "part has ignored len ",part.setSize," followed by used size ",part.clearSize);
    writer.appendZeroBytes(part.setSize);
    mem += part.setSize;
    initialMem += part.setSize;

    for (iu16 i = 0; i != part.clearSize; ++i) {
      // XXXX something like memcmp, but that returns the first point where the two blocks don't match
      ec += *mem == *initialMem;
      writer.appendByte(*mem ^ *initialMem);
      ++mem;
      ++initialMem;
    }
    ms += part.clearSize;
  }
  DA(mem == vm.getDynamicMemory() + vm.getDynamicMemorySize());
  DW(, "unchanged (of the non-ignoreds): ", ec, " / ", ms, " bytes");
  writer.close();

  return signature;
}

Signature Multiverse::recreateSignature (const Signature &oldSignature, const Bitranges &extraIgnoredByteRanges) {
  DS();

  Signature signature; // XXXX init size? what units?

  auto i = oldSignature.begin();
  Signature::Writer writer(signature);
  for (const auto &part : extraIgnoredByteRanges) {
    writer.appendZeroBytes(part.setSize);
    i += part.setSize;

    i.copy(writer, part.clearSize);
  }
  DA(i == oldSignature.end());
  writer.close();

  return signature;
}

Multiverse::Node *Multiverse::getNode (const NodePath &nodePath) const {
  return nodePath.resolve(rootNode);
}

Multiverse::Node *Multiverse::collapseNode (
  Node *node, const Bitranges &extraIgnoredByteRanges,
  unordered_map<reference_wrapper<const Signature>, Node *, Hasher<Signature>> &r_survivingNodes,
  unordered_map<Node *, Node *> &r_nodeCollapseTargets,
  unordered_map<Node *, Signature> &r_survivingNodePrevSignatures
) {
  DS();
  DW(, "checking for collapse of node with sig of prev hash ", node->getSignature().hash());

  // If we've already processed the node (be it surviving or collapsed away),
  // just return its new target.
  auto v = find(r_nodeCollapseTargets, node);
  if (v) {
    DW(, " we've already processed this one (and it ", (*v == node) ? "survived" : "was collapsed away", ")");
    return *v;
  }

  const Signature &prevSignature = node->getSignature();
  Signature signature = recreateSignature(prevSignature, extraIgnoredByteRanges);
  DW(, " this node now has signature ", signature.hash());

  auto v1 = find(r_survivingNodes, signature);
  if (v1) {
    // This node ends up the same as another. Collapse it away.
    DW(, " another node also now has this signature, so this one is collapsed away");
    r_nodeCollapseTargets.emplace(node, *v1);
    return *v1;
  } else {
    // This node is different from all the others we've processed so far; it
    // survives. Update its signature, process its children and update any changed
    // child details (and preserve any old state needed to undo the collapsing).
    DW(, " this is the first node we've seen with this new signature!");

    r_survivingNodePrevSignatures.emplace(node, node->setSignature(move(signature)));
    r_survivingNodes.emplace(ref(node->getSignature()), node);
    r_nodeCollapseTargets.emplace(node, node);

    for (size_t size = node->getChildrenSize(), i = 0; i != size; ++i) {
      auto &child = node->getChild(i);
      Node *childNode = get<2>(child);
      DA(childNode != node);
      Node *childTargetNode = collapseNode(childNode, extraIgnoredByteRanges, r_survivingNodes, r_nodeCollapseTargets, r_survivingNodePrevSignatures);
      if (childTargetNode == node) {
        node->removeChild(i);
        --i;
        --size;
        // XXXX preserve action id -> output string, old target node for undo
      } else if (childTargetNode != childNode) {
        node->changeChild(i, childTargetNode);
        // XXXX preserve action id -> old target node for undo
      }
    }

    node->batchOfChildChangesCompleted();

    return node;
  }
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
