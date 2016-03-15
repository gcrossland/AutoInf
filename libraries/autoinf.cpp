#include "autoinf.hpp"

LIB_DEPENDENCIES

#include "autoinf.using"

namespace autoinf {

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



FileOutputIterator::FileOutputIterator (FILE *h) : h(h) {
}

FileOutputIterator &FileOutputIterator::operator= (iu8f v) {
  int r = fputc(v, h);
  if (r == EOF) {
    throw PlainException(u8("write failed"));
  }
  return *this;
}

FileOutputIterator &FileOutputIterator::operator* () {
  return *this;
}

FileOutputIterator &FileOutputIterator::operator++ () {
  return *this;
}

FileOutputIterator &FileOutputIterator::operator++ (int) {
  return *this;
}

FileInputIterator::FileInputIterator (FILE *h) : h(h) {
  advance();
}

FileInputIterator::FileInputIterator () : h(nullptr), v(42) {
}

FileInputIterator::FileInputIterator (iu8f v) : h(nullptr), v(v) {
}

void FileInputIterator::advance () {
  DPRE(!!h, "this must not be an end iterator");
  int r = fgetc(h);
  if (r == EOF) {
    h = nullptr;
  }
  v = static_cast<iu8f>(r);
}

const iu8f &FileInputIterator::operator* () const noexcept {
  return v;
}

FileInputIterator &FileInputIterator::operator++ () {
  advance();
  return *this;
}

FileInputIterator FileInputIterator::operator++ (int) {
  FileInputIterator v(this->v);
  ++(*this);
  return v;
}

bool FileInputIterator::operator== (const FileInputIterator &r) const noexcept {
  return h == r.h;
}

constexpr SerialiserBase::id SerialiserBase::NON_ID;
constexpr SerialiserBase::id SerialiserBase::NULL_ID;

Signature::Signature () {
}

Signature::Signature (size_t sizeHint) : b(sizeHint) {
}

size_t Signature::getSizeHint () const noexcept {
  return b.size();
}

size_t Signature::hashSlow () const noexcept {
  return b.hashSlow();
}

bool Signature::operator== (const Signature &r) const noexcept {
  return b == r.b;
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
    writeIeu(bEnd, zeroByteCount);
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
  DW(, "finished writing sig of hash ", signature.hashSlow());
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
    auto v = readValidIeu<decltype(zeroByteCount)>(i);
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

bool Signature::Iterator::operator== (const Iterator &r) const noexcept {
  return i == r.i && zeroByteCount == r.zeroByteCount;
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

Multiverse::ActionSet::ActionSet (const vector<ActionWord> &words, const vector<ActionTemplate> &dewordingTemplates, const vector<ActionTemplate> &otherTemplates) {
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

  init(words, 0, dewordingTemplates, specs);
  dewordingActionCount = specs.size();
  init(words, static_cast<Index>(dewordingTemplates.size()), otherTemplates, specs);
  specs.compact().shrink_to_fit();

  this->words.reserve(words.size());
  for (const ActionWord &word : words) {
    this->words.subList().append(word.word);
    this->words.push();
  }
  this->words.compact().shrink_to_fit();
  DW(,"words are:"); for (const auto &w : this->words) { DW(,"  **",u8string(w.begin(), w.end()).c_str(),"**"); }
  this->templates.reserve(dewordingTemplates.size() + otherTemplates.size());
  auto l = {&dewordingTemplates, &otherTemplates};
  for (const vector<ActionTemplate> *templates : l) {
    for (const ActionTemplate &templ : *templates) {
      auto &e = this->templates.subList();
      for (auto &segment : templ.segments) {
        e.subList().append(segment);
        e.push();
      }
      this->templates.push();
    }
  }
  this->templates.compact().compact().shrink_to_fit();
  DW(,"templates are:"); for (const auto &t : this->templates) { DWP(," "); for (const auto &w : t) { DWP(," *",u8string(w.begin(), w.end()).c_str(),"*"); } DWP(,""); }
  DW(,"hence, action inputs are:");
  for (ActionId i = 0; i != getSize(); ++i) {
    u8string o;
    get(i).getInput(o);
    DW(,"  **",o.c_str(),"**");
  }
}

void Multiverse::ActionSet::init (
  const vector<ActionWord> &words, Index nextTemplateI, const vector<ActionTemplate> &templates,
  MultiList<string<Index>> &specs
) {
  DS();
  string<Index> spec;
  for (const ActionTemplate &templ : templates) {
    spec.clear();
    spec.push_back(nextTemplateI++);
    initImpl(words, templ, 0, specs, spec);
  }
}

void Multiverse::ActionSet::initImpl (
  const vector<ActionWord> &words, const ActionTemplate &templ, Index templateWordI,
  MultiList<string<Index>> &specs, string<Index> &r_spec
) {
  if (templateWordI == templ.words.size()) {
    DA(r_spec.size() == static_cast<size_t>(templateWordI) + 1);
    if (specs.size() == numeric_limits<ActionId>::max()) {
      throw PlainException(u8("the limit for the number of actions has been reached"));
    }
    DWP(, "adding action ",specs.size()," (template ",r_spec[0]," with words"); for (size_t i = 1; i != r_spec.size(); ++i) { DWP(, " ", r_spec[i]); } DW(, ")");
    specs.subList().append(r_spec);
    specs.push();
    return;
  }

  ActionWord::CategorySet categories = templ.words[templateWordI];
  size_t specPreSize = r_spec.size();
  for (Index i = 0, end = static_cast<Index>(words.size()); i != end; ++i) {
    const ActionWord &word = words[i];
    if ((categories & word.categories) == categories) {
      r_spec.push_back(i);
      initImpl(words, templ, templateWordI + 1, specs, r_spec);
      r_spec.erase(specPreSize);
    }
  }
}

Multiverse::ActionSet::Index Multiverse::ActionSet::getWordsSize () const {
  return words.size();
}

MultiList<core::u8string>::SubList Multiverse::ActionSet::getWord (Index i) const {
  return words.get(i);
}

Multiverse::ActionId Multiverse::ActionSet::getSize () const {
  return specs.size();
}

Multiverse::ActionSet::Action Multiverse::ActionSet::get (ActionId id) const {
  return Action(*this, id);
}

Multiverse::ActionSet::Action::Action (const ActionSet &actionSet, ActionId id) : Action(actionSet, id, actionSet.specs.get(id)) {
}

Multiverse::ActionSet::Action::Action (const ActionSet &actionSet, ActionId id, MultiList<core::string<Index>>::SubList &&spec) :
  actionSet(actionSet), spec(move(spec)), dewording(id < actionSet.dewordingActionCount)
{
  DA(actionSet.templates.get(*this->spec.begin()).size() == this->spec.size());
}

Multiverse::ActionId Multiverse::ActionSet::Action::getDewordingTarget () const {
  if (!dewording) {
    return NON_ID;
  }

  DA(getWordCount() == 1);
  return getWord(0);
}

size_t Multiverse::ActionSet::Action::getWordCount () const {
  return spec.size() - 1;
}

Multiverse::ActionSet::Index Multiverse::ActionSet::Action::getWord (size_t i) const {
  DA(i < getWordCount());
  return *(spec.begin() + 1 + i);
}

void Multiverse::ActionSet::Action::getInput (u8string &r_out) const {
  auto segmentsI = actionSet.templates.get(*spec.begin()).begin();
  auto segment = *(segmentsI++);
  r_out.append(segment.begin(), segment.end());

  for (size_t i = 0, end = getWordCount(); i != end; ++i, ++segmentsI) {
    auto word = actionSet.words.get(getWord(i));
    r_out.append(word.begin(), word.end());
    auto segment = *segmentsI;
    r_out.append(segment.begin(), segment.end());
  }
  DA(segmentsI == actionSet.templates.get(*spec.begin()).end());
}

bool Multiverse::ActionSet::Action::includesAnyWords (const Bitset &words) const {
  for (size_t i = 0, end = getWordCount(); i != end; ++i) {
    if (words.getBit(getWord(i))) {
      return true;
    }
  }
  return false;
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

Multiverse::Node *const Multiverse::Node::UNPARENTED = static_cast<Node *>(nullptr) + 1;
const u8string Multiverse::Node::OUTPUT_LINE_TERMINATOR = u8("\n\n");

Multiverse::Node::Node (unique_ptr<Listener> &&listener, Node *primeParentNode, HashWrapper<Signature> &&signature, State &&state) :
  listener(move(listener)), primeParentNode(primeParentNode), primeParentNodeInvalid(false), signature(move(signature)), state(), children()
{
  if (!state.isEmpty()) {
    this->state.reset(new State(move(state)));
  }
  DW(, "created new Node with sig of hash ", this->signature.hash());
}

Multiverse::Node::Listener *Multiverse::Node::getListener () const {
  return listener.get();
}

Multiverse::Node *Multiverse::Node::getPrimeParentNode () const {
  DPRE(!primeParentNodeInvalid);
  DPRE(primeParentNode != UNPARENTED);
  return primeParentNode;
}

size_t Multiverse::Node::getPrimeParentArcChildIndex () const {
  auto &children = getPrimeParentNode()->children;
  for (size_t i = 0, end = children.size(); i != end; ++i) {
    if (get<2>(children[i]) == this) {
      return i;
    }
  }
  DA(false);
  return 0;
}

const HashWrapper<Signature> &Multiverse::Node::getSignature () const {
  return signature;
}

HashWrapper<Signature> Multiverse::Node::setSignature (HashWrapper<Signature> &&signature) {
  HashWrapper<Signature> oldSignature(move(this->signature));
  this->signature = move(signature);
  return oldSignature;
}

const State *Multiverse::Node::getState () const {
  return state.get();
}

void Multiverse::Node::clearState () {
  DW(, "done with trying to process Node with sig of hash ", signature.hash());
  state.reset();
}

size_t Multiverse::Node::getChildrenSize () const {
  return children.size();
}

const tuple<Multiverse::ActionId, StringSet<char8_t>::String, Multiverse::Node *> &Multiverse::Node::getChild (size_t i) const {
  return children[i];
}

size_t Multiverse::Node::getChildIndex (ActionId id) const {
  auto i = lower_bound(children.begin(), children.end(), id, [] (const tuple<ActionId, StringSet<char8_t>::String, Node *> &elmt, const ActionId &target) {
    return get<0>(elmt) < target;
  });
  return (i == children.end() || get<0>(*i) != id) ? numeric_limits<size_t>::max() : static_cast<size_t>(i - children.begin());
}

void Multiverse::Node::addChild (ActionId actionId, const u8string &output, Node *node, Multiverse &multiverse) {
  DW(, "adding to Node with sig of hash ", signature.hash(), " child of action id ", actionId, ":");
  DW(, "  output is **", output.c_str(), "**");
  DW(, "  dest. Node has sig of hash ", node->getSignature().hash());
  DPRE(!!state, "children cannot be added after all have been added");
  DPRE(children.empty() || get<0>(children.back()) < actionId, "children must be added in order of actionId");

  thread_local StringSet<char8_t>::String o;
  DA(o.empty());
  multiverse.outputStringSet.createString(output, OUTPUT_LINE_TERMINATOR, o);
  children.emplace_back(actionId, o, node);
  o.clear();

  bool primeParentChanged = node->updatePrimeParent(this, false);
  if (primeParentChanged) {
    multiverse.listener->subtreePrimeAncestorsUpdated(multiverse, node);
  }
}

bool Multiverse::Node::updatePrimeParent (Node *newParentNode, bool changedAbove) {
  DS();
  DW(, "checking if Node with sig of hash ", signature.hash(), " needs its prime parent updated");
  DPRE(!!newParentNode);
  DPRE(!primeParentNodeInvalid);

  bool changed = false;

  if (primeParentNode == newParentNode) {
    DW(, "  same parent");
  } else if (!primeParentNode) {
    DW(, "  this Node is already the root!");
  } else if (primeParentNode == UNPARENTED) {
    DW(, "  this Node is unparented");
    changed = true;
  } else {
    Node *n0 = primeParentNode;
    Node *n1 = newParentNode;
    while (true) {
      DW(, "  going up the graph...");
      Node *prevN0 = n0;
      n0 = n0->primeParentNode;
      DPRE(!n0 || !n0->primeParentNodeInvalid);
      Node *prevN1 = n1;
      n1 = n1->primeParentNode;
      DPRE(!n1 || !n1->primeParentNodeInvalid);

      if (!n0) {
        DW(, "  gone above the root node on the old prime parent's side");
        break;
      } else if (!n1) {
        DW(, "  gone above the root node on the new prime parent's side");
        changed = true;
        break;
      } else if (n0 == n1) {
        DW(, "  reached the same node from both sides");
        for (size_t i = 0;; ++i) {
          DA(i != n0->getChildrenSize());
          Node *c = get<2>(n0->getChild(i));
          if (c == prevN0) {
            DW(, "  old prime parent's side is hit first");
            break;
          } else if (c == prevN1) {
            DW(, "  new prime parent's side is hit first");
            changed = true;
            break;
          }
        }
        break;
      }
    }
  }

  if (changed) {
    primeParentNode = newParentNode;
    changedAbove = true;
  } else {
    if (changedAbove && primeParentNode != newParentNode) {
      changedAbove = false;
    }
  }

  if (changedAbove) {
    DW(, "  updating children (that have a different parent)...");
    unordered_set<Node *> seenNodes(children.size());
    for (auto &e : children) {
      Node *childNode = get<2>(e);
      if (contains(seenNodes, childNode)) {
        continue;
      }
      seenNodes.emplace(childNode);

      childNode->updatePrimeParent(this, changedAbove);
    }
  }

  return changed;
}

void Multiverse::Node::removeChild (size_t i) {
  DPRE(!primeParentNodeInvalid);
  children.erase(children.begin() + i);
}

void Multiverse::Node::changeChild (size_t i, Node *node) {
  DPRE(!primeParentNodeInvalid);
  get<2>(children[i]) = node;
}

void Multiverse::Node::childrenUpdated (const Multiverse &multiverse) {
  DW(, "finished updating children (new count ", children.size(), ") on Node with sig of hash ", signature.hash());
  DPRE(!primeParentNodeInvalid);
  children.shrink_to_fit();
  // DODGY might be part-way through collapsing nodes -> model tree is in some intermediate state (prime parent might be out of date etc.)
  multiverse.listener->nodeChildrenUpdated(multiverse, this);
}

void Multiverse::Node::invalidatePrimeParent () {
  DA(!primeParentNodeInvalid);
  primeParentNodeInvalid = true;
}

void Multiverse::Node::rebuildPrimeParents (Multiverse &multiverse) {
  DS();
  DW(, "number of surviving nodes is ", multiverse.nodes.size());
  #ifndef NDEBUG
  for (Node *node : multiverse) {
    DA(node->primeParentNodeInvalid);
  }
  #endif

  vector<Node *> reparentedNodes;

  {
    DW(, "walking the tree by bredth:");
    vector<Node *> nodes0, nodes1;
    vector<Node *> *nodesAtThisDepth = &nodes0, *nodesAtNextDepth = &nodes1;
    unordered_set<Node *> nodes2, nodes3;
    unordered_set<Node *> *enshadowedNodesAtThisDepth = &nodes2, *enshadowedNodesAtNextDepth = &nodes3;
    Node *rootNode = multiverse.getRoot();
    nodesAtThisDepth->emplace_back(rootNode);
    DA(rootNode->primeParentNode == nullptr);
    rootNode->primeParentNodeInvalid = false;
    do {
      DW(, "  count of nodes at this level: ", nodesAtThisDepth->size());
      for (Node *node : *nodesAtThisDepth) {
        bool enshadowed = contains(*enshadowedNodesAtThisDepth, node);

        for (auto &e : node->children) {
          Node *childNode = get<2>(e);
          if (!childNode->primeParentNodeInvalid) {
            continue;
          }
          childNode->primeParentNodeInvalid = false;

          DA(childNode->primeParentNode != UNPARENTED);
          if (childNode->primeParentNode != node) {
            childNode->primeParentNode = node;
            if (!enshadowed) {
              reparentedNodes.emplace_back(childNode);
              enshadowedNodesAtNextDepth->emplace(childNode);
            }
          }

          nodesAtNextDepth->emplace_back(childNode);
          if (enshadowed) {
            enshadowedNodesAtNextDepth->emplace(childNode);
          }
        }
      }
      nodesAtThisDepth->clear();
      swap(nodesAtThisDepth, nodesAtNextDepth);
      enshadowedNodesAtThisDepth->clear();
      swap(enshadowedNodesAtThisDepth, enshadowedNodesAtNextDepth);
    } while (!nodesAtThisDepth->empty());
    DW(, "number of reparented nodes is ", reparentedNodes.size());
  }

  for (Node *node : reparentedNodes) {
    multiverse.listener->subtreePrimeAncestorsUpdated(multiverse, node);
  }
}

Multiverse::Node::Listener::Listener () {
}

Multiverse::Node::Listener::~Listener () {
}

Multiverse::NodeIterator::NodeIterator () {
}

Multiverse::NodeIterator::NodeIterator (decltype(i) &&i) : RevaluedIterator(move(i)) {
}

Multiverse::Node *const &Multiverse::NodeIterator::operator* () const {
  return get<1>(*i);
}

Multiverse::Multiverse (
  Vm &r_vm, const u8string &initialInput, u8string &r_initialOutput, function<bool (Vm &r_vm)> &&saver, function<bool (Vm &r_vm)> &&restorer,
  const vector<vector<u8string>> &equivalentActionInputsSet,
  const vector<ActionWord> &words, const vector<ActionTemplate> &dewordingTemplates, const vector<ActionTemplate> &otherTemplates,
  function<bool (const Vm &vm, const u8string &output)> &&deworder, unique_ptr<Listener> &&listener
) :
  saver(saver), restorer(restorer), actionSet(words, dewordingTemplates, otherTemplates),
  deworder(move(deworder)), listener(move(listener)),
  ignoredBytes(initIgnoredBytes(r_vm)), ignoredByteRangeset(ignoredBytes, r_vm.getDynamicMemorySize()), rootNode(nullptr)
{
  DS();
  DPRE(r_vm.isAlive());
  DPRE(r_vm.getWordSet());
  DPRE(!!this->listener);

  doAction(r_vm, initialInput, r_initialOutput, u8("VM died while running the initial input"));
  Signature signature = createSignature(r_vm, ignoredByteRangeset);
  State state;
  doSaveAction(r_vm, state);

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
      doRestoreAction(r_vm, state);
      doAction(r_vm, actionInput, tmp, u8("VM was dead after doing action"));
      DW(, " output was **",tmp.c_str(),"**");
      tmp.clear();
      signatures[i] = createSignature(r_vm, ignoredByteRangeset);
      signatureIs[i] = signatures[i].begin();
    }
    extraIgnoredBytes |= createExtraIgnoredBytes(signatures[0], signatureIs + 1, signatureIs + equivalentActionInputs.size(), r_vm);
  }
  ignoredBytes |= move(extraIgnoredBytes);
  ignoredByteRangeset = Rangeset(ignoredBytes, r_vm.getDynamicMemorySize());
  signature = recreateSignature(signature, ignoredByteRangeset);

  unique_ptr<Node::Listener> nodeListener(this->listener->createNodeListener());
  this->listener->nodeReached(*this, nodeListener.get(), NON_ID, r_initialOutput, signature, r_vm);
  unique_ptr<Node> node(new Node(move(nodeListener), nullptr, hashed(move(signature)), move(state)));
  rootNode = node.get();
  nodes.emplace(ref(rootNode->getSignature()), rootNode);
  this->listener->subtreePrimeAncestorsUpdated(*this, rootNode);
  node.release();
}

// XXXX trial - handy for keeping signatures clean with 103
Bitset Multiverse::initIgnoredBytes (const Vm &vm) {
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

const Multiverse::ActionSet &Multiverse::getActionSet () const {
  return actionSet;
}

Multiverse::Listener *Multiverse::getListener () const {
  return listener.get();
}

const StringSet<char8_t> &Multiverse::getOutputStringSet () const {
  return outputStringSet;
}

void Multiverse::doAction(Vm &r_vm, u8string::const_iterator inputBegin, u8string::const_iterator inputEnd, u8string &r_output, const char8_t *deathExceptionMsg) {
  DPRE(r_vm.isAlive());

  r_vm.doAction(inputBegin, inputEnd, r_output);
  if (!r_vm.isAlive()) {
    throw PlainException(deathExceptionMsg);
  }
}

void Multiverse::doAction(Vm &r_vm, const u8string &input, u8string &r_output, const char8_t *deathExceptionMsg) {
  doAction(r_vm, input.begin(), input.end(), r_output, deathExceptionMsg);
}

void Multiverse::doSaveAction (Vm &r_vm, State &r_state) {
  r_vm.setSaveState(&r_state);
  auto _ = finally([&] () {
    r_vm.setSaveState(nullptr);
  });

  bool succeeded = saver(r_vm);
  DPRE(r_vm.isAlive());

  if (!succeeded) {
    throw PlainException(u8("save action didn't cause saving"));
  }
}

void Multiverse::doRestoreAction (Vm &r_vm, const State &state) {
  r_vm.setRestoreState(&state);
  auto _ = finally([&] () {
    r_vm.setRestoreState(nullptr);
  });

  bool succeeded = restorer(r_vm);
  DPRE(r_vm.isAlive());

  if (!succeeded) {
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

  Signature signature(oldSignature.getSizeHint());

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

size_t Multiverse::size () const {
  return nodes.size();
}

Multiverse::NodeIterator Multiverse::begin () const {
  return NodeIterator(nodes.cbegin());
}

Multiverse::NodeIterator Multiverse::end () const {
  return NodeIterator(nodes.cend());
}

Multiverse::Node *Multiverse::getRoot () const {
  return rootNode;
}

Multiverse::Node *Multiverse::collapseNode (
  Node *node, const Rangeset &extraIgnoredByteRangeset,
  unordered_map<reference_wrapper<const HashWrapper<Signature>>, Node *> &r_survivingNodes,
  unordered_map<Node *, Node *> &r_nodeCollapseTargets,
  unordered_map<Node *, HashWrapper<Signature>> &r_survivingNodePrevSignatures
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

  const HashWrapper<Signature> &prevSignature = node->getSignature();
  HashWrapper<Signature> signature(recreateSignature(prevSignature.get(), extraIgnoredByteRangeset));
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

    bool mutated = false;
    for (size_t size = node->getChildrenSize(), i = 0; i != size; ++i) {
      auto &child = node->getChild(i);
      Node *childNode = get<2>(child);
      DA(childNode != node);
      Node *childTargetNode = collapseNode(childNode, extraIgnoredByteRangeset, r_survivingNodes, r_nodeCollapseTargets, r_survivingNodePrevSignatures);
      if (childTargetNode == node) {
        node->removeChild(i);
        mutated = true;
        --i;
        --size;
        // XXXX preserve action id -> output string, old target node for undo
      } else if (childTargetNode != childNode) {
        node->changeChild(i, childTargetNode);
        mutated = true;
        // XXXX preserve action id -> old target node for undo
      }
    }
    if (mutated) {
      node->childrenUpdated(*this);
    }
    node->invalidatePrimeParent();

    return node;
  }
}

void Multiverse::save (const char *pathName, const Vm &vm) {
  DS();
  FILE *h = fopen(pathName, "wb");
  if (h == nullptr) {
    throw PlainException(u8("failed to open file"));
  }
  auto _ = finally([&] () {
    // TODO autocloser
    int r = fclose(h);
    if (r == EOF) {
      // TODO throw PlainException(u8("failed to finish writing to file"));
    }
  });

  auto s = Serialiser<FileOutputIterator>(FileOutputIterator(h));

  DA(s.isSerialising());
  for (auto nodeEntry : nodes) {
    Node *node = get<1>(nodeEntry);
    Node::Listener *listener = node->getListener();
    if (listener) {
      derefAndProcessNodeListener(listener, s);
    }
  }
  Node::Listener *listener = nullptr;
  derefAndProcessNodeListener(listener, s);

  s.process(*const_cast<Bitset *>(vm.getWordSet()));
  s.process(ignoredBytes);
  s.process(outputStringSet);
  s.derefAndProcess(rootNode);
}

void Multiverse::load (const char *pathName, Vm &r_vm) {
  DS();
  FILE *h = fopen(pathName, "rb");
  if (h == nullptr) {
    throw PlainException(u8("failed to open file"));
  }
  auto _ = finally([&] () {
    fclose(h);
  });

  auto s = Deserialiser<FileInputIterator>(FileInputIterator(h), FileInputIterator());

  ignoredBytes.clear();
  ignoredByteRangeset.clear();
  outputStringSet = StringSet<char8_t>();
  rootNode = nullptr;
  for (const auto &e : nodes) {
    delete get<1>(e);
  }
  nodes.clear();

  vector<Node::Listener *> listeners;
  try {
    DA(!s.isSerialising());
    while (true) {
      listeners.emplace_back(nullptr);
      Node::Listener *&listener = listeners.back();
      derefAndProcessNodeListener(listener, s);
      if (!listener) {
        break;
      }
    }

    Bitset wordSet;
    s.process(wordSet);
    s.process(ignoredBytes);
    s.process(outputStringSet);
    s.derefAndProcess(rootNode);

    ignoredByteRangeset = Rangeset(ignoredBytes, r_vm.getDynamicMemorySize());
    unordered_set<Node *> seenNodes;
    rootNode->forEach([&] (Node *node) -> bool {
      if (contains(seenNodes, node)) {
        return false;
      }
      seenNodes.emplace(node);

      nodes.emplace(ref(node->getSignature()), node);
      return true;
    });

    r_vm.setWordSet(move(wordSet));
    // TODO validate result
  } catch (...) {
    nthrow(PlainException(u8("loading failed")));
/*
    unordered_set<Node *> seenNodes;
    rootNode->forEach([&] (Node *node) -> bool {
      if (contains(seenNodes, node)) {
        return false;
      }
      seenNodes.emplace(node);

      node->listener.reset();
      return true;
    });
    rootNode = nullptr;
    nodes.clear();
    for (Node *node : seenNodes) {
      delete node;
    }

    if (!listeners.back()) {
      listeners.pop_back();
    }
    DA(!!listeners.back());
    for (Node::Listener *listener : listeners) {
      delete listener;
    }
*/
    throw;
  }

  listener->loaded(*this);
}

Multiverse::Listener::Listener () {
}

Multiverse::Listener::~Listener () {
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
