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
  zeroByteCount = 0;
}

void Signature::Writer::appendByte (iu8 byte) {
  if (byte == 0) {
    appendZeroBytes(1);
  } else {
    flushZeroBytes();
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
  return l.i == r.i && l.zeroByteCount == r.zeroByteCount;
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

Multiverse::ActionWord::ActionWord (u8string &&word, CategorySet categories) : word(move(word)), categories(categories) {
}

void Multiverse::ActionTemplate::init (u8string &&segment) {
  segments.push_back(move(segment));
}

Multiverse::ActionSet::ActionSet (vector<ActionWord> &&words, vector<ActionTemplate> &&dewordingTemplates, vector<ActionTemplate> &&otherTemplates) {
  DS();
  if (words.size() >= numeric_limits<Index>::max()) {
    throw PlainException(u8("the limit for the number of action words has been reached"));
  }
  if (dewordingTemplates.size() + otherTemplates.size() >= numeric_limits<Index>::max()) {
    throw PlainException(u8("the limit for the number of action templates has been reached"));
  }
  for (const ActionTemplate &templ : dewordingTemplates) {
    DPRE(!templ.segments.empty(), "action templates must have at least one segment");
    DPRE(templ.segments.size() - 1 == templ.words.size());
    if (templ.words.size() != 1) {
      throw PlainException(u8("dewording action templates must contain exactly one action word"));
    }
  }

  init(words, 0, dewordingTemplates, specs, specBegins);
  dewordingTemplateCount = specBegins.size();
  init(words, static_cast<Index>(dewordingTemplates.size()), otherTemplates, specs, specBegins);
  specs.shrink_to_fit();
  specBegins.shrink_to_fit();

  this->words.reserve(words.size());
  for (ActionWord &word : words) {
    this->words.push_back(move(word.word));
  }
  DW(,"words are:"); for (auto &w : this->words) { DW(,"  **",w.c_str(),"**"); }
  this->templates.reserve(dewordingTemplates.size() + otherTemplates.size());
  for (vector<ActionTemplate> *templates : {&dewordingTemplates, &otherTemplates}) {
    for (ActionTemplate &templ : *templates) {
      this->templates.push_back(move(templ.segments));
    }
  }
  DW(,"templates are:"); for (auto &t : this->templates) { DWP(," "); for (auto &w : t) { DWP(," *",w.c_str(),"*"); } DWP(,""); }
  DW(,"hence, action inputs are:");
  for (ActionId i = 0; i != getSize(); ++i) {
    u8string o;
    getInput(i, o);
    DW(,"  **",o.c_str(),"**");
  }
}

void Multiverse::ActionSet::init (
  const vector<ActionWord> &words, Index nextTemplateI, const vector<ActionTemplate> &templates,
  string<Index> &r_specs, vector<size_t> &r_specBegins
) {
  DS();
  string<Index> spec;
  for (const ActionTemplate &templ : templates) {
    spec.clear();
    spec.push_back(nextTemplateI++);
    initImpl(words, templ, 0, r_specs, r_specBegins, spec);
  }
}

void Multiverse::ActionSet::initImpl (
  const vector<ActionWord> &words, const ActionTemplate &templ, Index templateWordI,
  string<Index> &r_specs, vector<size_t> &r_specBegins, string<Index> &r_spec
) {
  if (templateWordI == templ.words.size()) {
    DA(r_spec.size() == static_cast<size_t>(templateWordI) + 1);
    if (r_specBegins.size() == numeric_limits<ActionId>::max()) {
      throw PlainException(u8("the limit for the number of actions has been reached"));
    }
    DWP(, "adding action ",r_specBegins.size()," (template ",r_spec[0]," with words"); for (size_t i = 1; i != r_spec.size(); ++i) { DWP(, " ", r_spec[i]); } DW(, ")");
    r_specBegins.push_back(r_specs.size());
    r_specs.append(r_spec);
    return;
  }

  ActionWord::CategorySet categories = templ.words[templateWordI];
  size_t specPreSize = r_spec.size();
  for (Index i = 0, end = static_cast<Index>(words.size()); i != end; ++i) {
    const ActionWord &word = words[i];
    if ((categories & word.categories) == categories) {
      r_spec.push_back(i);
      initImpl(words, templ, templateWordI + 1, r_specs, r_specBegins, r_spec);
      r_spec.erase(specPreSize);
    }
  }
}

Multiverse::ActionId Multiverse::ActionSet::getSize () const {
  return specBegins.size();
}

Multiverse::ActionId Multiverse::ActionSet::getDewordingWord (ActionId id) const {
  if (id >= dewordingTemplateCount) {
    return NON_ID;
  }

  // XXXX tidy up
  const Index *specI = &specs[specBegins[id]];
  const vector<u8string> &segments = templates[*specI];
  ++specI;

  DA(segments.size() == 2);
  return *specI;
}

bool Multiverse::ActionSet::includesAnyWords (ActionId id, const Bitset &words) const {
  DPRE(id < specBegins.size());
  const Index *specI = &specs[specBegins[id]];
  const vector<u8string> &segments = templates[*specI];
  ++specI;

  for (const Index *specEnd = specI + segments.size() - 1; specI != specEnd; ++specI) {
    if (words.getBit(*specI)) {
      return true;
    }
  }
  return false;
}

void Multiverse::ActionSet::getInput (ActionId id, u8string &r_out) const {
  DPRE(id < specBegins.size());
  const Index *specI = &specs[specBegins[id]];
  const vector<u8string> &segments = templates[*specI];
  ++specI;

  const u8string *segmentsI = segments.data();
  r_out.append(*(segmentsI++));
  for (const Index *specEnd = specI + segments.size() - 1; specI != specEnd; ++specI, ++segmentsI) {
    r_out.append(words[*specI]);
    r_out.append(*segmentsI);
  }
}

Multiverse::Rangeset::Rangeset (const Bitset &bitset, iu16 rangesEnd) : vector() {
  DS();
  DW(, "creating Rangeset from a bitset:");
  DPRE(bitset.getNextSetBit(rangesEnd) == Bitset::NON_INDEX);

  for (size_t end = 0; end != rangesEnd;) {
    size_t prevEnd = end;
    size_t begin = bitset.getNextClearBit(end);
    end = bitset.getNextSetBit(begin + 1);
    if (end == Bitset::NON_INDEX) {
      end = rangesEnd;
    }
    this->push_back(RangesetPart{static_cast<iu16f>(begin - prevEnd), static_cast<iu16f>(end - begin)});
    DA(end <= rangesEnd);
  }
  DA(std::accumulate(this->begin(), this->end(), static_cast<size_t>(0), [] (size_t a, const RangesetPart &o) -> size_t {
    return a + o.setSize + o.clearSize;
  }) == rangesEnd);
  DW(, "a total of ", std::accumulate(this->begin(), this->end(), static_cast<size_t>(0), [] (size_t a, const RangesetPart &o) -> size_t {
    return a + o.setSize;
  }), " bits are set");
}

Multiverse::Node::Node (Signature &&signature, State &&state, vector<MetricValue> &&metricValues) :
  signature(move(signature)), state(), metricValues(metricValues), children(0)
{
  if (!state.isEmpty()) {
    this->state.reset(new State(move(state)));
  }
  DW(, "created new Node with sig of hash ", this->signature.hash());
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

Multiverse::MetricValue Multiverse::Node::getMetricValue (size_t i) const {
  return metricValues[i];
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
  const vector<vector<u8string>> &equivalentActionInputsSet,
  vector<ActionWord> &&words, vector<ActionTemplate> &&dewordingTemplates, vector<ActionTemplate> &&otherTemplates,
  function<bool (const Vm &vm, const u8string &output)> &&deworder, vector<Metric> &&metrics
) :
  saveActionInput(saveActionInput), restoreActionInput(restoreActionInput), actionSet(move(words), move(dewordingTemplates), move(otherTemplates)),
  deworder(deworder), metrics(metrics),
  ignoredBytes(initIgnoredBytes(vm)), ignoredByteRangeset(ignoredBytes, vm.getDynamicMemorySize()), rootNode(nullptr)
{
  DS();
  DPRE(vm.isAlive());

  doAction(vm, initialInput, r_initialOutput, u8("VM died while running the initial input"));
  Signature signature = createSignature(vm, ignoredByteRangeset);
  State state;
  doSaveAction(vm, state);

  Bitset extraIgnoredBytes;
  for (const auto &equivalentActionInputs : equivalentActionInputsSet) {
    DS();
    DW(, "running some equivalent actions to enrich the ignored byte set");
    DPRE(equivalentActionInputs.size() > 1, "equivalent actions must give multiple actions");
    Signature signatures[equivalentActionInputs.size()];
    Signature::Iterator signatureIs[equivalentActionInputs.size()];
    u8string tmp;
    for (size_t i = 0, end = equivalentActionInputs.size(); i != end; ++i) {
      const u8string &actionInput = equivalentActionInputs[i];
      DW(, " doing **",actionInput.c_str(),"**");
      doRestoreAction(vm, state);
      doAction(vm, actionInput, tmp, u8("VM was dead after doing action"));
      DW(, " output was **",tmp.c_str(),"**");
      tmp.clear();
      signatures[i] = createSignature(vm, ignoredByteRangeset);
      signatureIs[i] = signatures[i].begin();
    }
    extraIgnoredBytes |= createExtraIgnoredBytes(signatures[0], signatureIs + 1, signatureIs + equivalentActionInputs.size(), vm);
  }
  ignoredBytes |= move(extraIgnoredBytes);
  ignoredByteRangeset = Rangeset(ignoredBytes, vm.getDynamicMemorySize());
  signature = recreateSignature(signature, ignoredByteRangeset);

  unique_ptr<Node> node(new Node(move(signature), move(state), vector<MetricValue>(getMetricCount(), 0)));
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

Multiverse::~Multiverse () noexcept {
  for (const auto &e : nodes) {
    delete get<1>(e);
  }
}

void Multiverse::getActionInput (ActionId id, u8string &r_out) const {
  actionSet.getInput(id, r_out);
}

size_t Multiverse::getMetricCount () const {
  return metrics.size();
}

void Multiverse::doAction(Vm &vm, u8string::const_iterator inputBegin, u8string::const_iterator inputEnd, u8string &r_output, const char8_t *deathExceptionMsg) {
  DPRE(vm.isAlive());

  r_output.clear(); // XXXX really do this here? not let the caller do it? ...
  vm.doAction(inputBegin, inputEnd, r_output);
  if (!vm.isAlive()) {
    throw PlainException(deathExceptionMsg);
  }
}

void Multiverse::doAction(Vm &vm, const u8string &input, u8string &r_output, const char8_t *deathExceptionMsg) {
  doAction(vm, input.begin(), input.end(), r_output, deathExceptionMsg);
}

void Multiverse::doSaveAction (Vm &vm, State &r_state) {
  vm.setSaveState(&r_state);
  auto _ = finally([&] () {
    vm.setSaveState(nullptr);
  });

  u8string o;
  doAction(vm, saveActionInput, o, u8("VM died while saving a state"));

  if (vm.getSaveCount() == 0) {
    throw PlainException(u8("save action didn't cause saving"));
  }
}

void Multiverse::doRestoreAction (Vm &vm, const State &state) {
  vm.setRestoreState(&state);
  auto _ = finally([&] () {
    vm.setRestoreState(nullptr);
  });

  u8string o;
  doAction(vm, restoreActionInput, o, u8("VM died while restoring a state"));

  if (vm.getRestoreCount() == 0) {
    throw PlainException(u8("restore action didn't cause restoration"));
  }
}

Signature Multiverse::createSignature (const Vm &vm, const Rangeset &ignoredByteRangeset) {
  DS();
  DPRE(vm.isAlive());

  Signature signature;

  const zbyte *mem = vm.getDynamicMemory();
  const zbyte *initialMem = vm.getInitialDynamicMemory();
  Signature::Writer writer(signature);
  size_t ec = 0, ms = 0;
  for (const auto &part : ignoredByteRangeset) {
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

Signature Multiverse::recreateSignature (const Signature &oldSignature, const Rangeset &extraIgnoredByteRangeset) {
  DS();

  Signature signature; // XXXX init size? what units?

  auto i = oldSignature.begin();
  Signature::Writer writer(signature);
  for (const auto &part : extraIgnoredByteRangeset) {
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
  Node *node, const Rangeset &extraIgnoredByteRangeset,
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
  Signature signature = recreateSignature(prevSignature, extraIgnoredByteRangeset);
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
      Node *childTargetNode = collapseNode(childNode, extraIgnoredByteRangeset, r_survivingNodes, r_nodeCollapseTargets, r_survivingNodePrevSignatures);
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
