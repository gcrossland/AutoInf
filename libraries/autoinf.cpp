#include "autoinf.hpp"

LIB_DEPENDENCIES

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();
DC(c);

constexpr SerialiserBase::id SerialiserBase::nonId;
constexpr SerialiserBase::id SerialiserBase::nullId;

Signature::Signature () {
  DA(empty());
}

Signature::Signature (size_t sizeHint) : b(sizeHint) {
}

Signature::Signature (const SerialiserBase &) : Signature() {
}

bool Signature::empty () const noexcept {
  return b.empty();
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
  DSA(escape != 0, "");
  if (zeroByteCount < 3) {
    iu8f b[2] = {0, 0};
    signature.b.append(b, zeroByteCount);
  } else {
    iu8f b[1 + numeric_limits<decltype(zeroByteCount)>::max_ie_octets] = {escape,};
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
    if (byte == escape) {
      iu8f b[2] = {escape, 0};
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
  if (b != escape) {
    currentByte = b;
    zeroByteCount = 0;
  } else {
    auto v = readValidIeu<decltype(zeroByteCount)>(i);
    DA(v != 1 && v != 2);
    if (v == 0) {
      currentByte = escape;
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

ActionSet::Word::Word (u8string &&word, CategorySet categories) : word(move(word)), categories(categories) {
}

void ActionSet::Template::init (u8string &&segment) {
  segments.push_back(move(segment));
}

ActionSet::ActionSet (const vector<Word> &words, const vector<Template> &dewordingTemplates, const vector<Template> &otherTemplates) {
  DS();
  for (const Template &templ : dewordingTemplates) {
    if (templ.words.size() != 1) {
      throw PlainException(u8"dewording action templates must contain exactly one action word");
    }
  }

  this->words.reserve(words.size());
  for (const Word &word : words) {
    this->words.subList().append(word.word);
    this->words.push();
  }
  this->words.compact().shrink_to_fit();
  DI(DW(,"words are:"); for (const auto &w : this->words) { DW(,"  **",u8string(w.begin(), w.end()).c_str(),"**"); })
  this->templates.reserve(dewordingTemplates.size() + otherTemplates.size());
  auto l = {&dewordingTemplates, &otherTemplates};
  for (const vector<Template> *templates : l) {
    for (const Template &templ : *templates) {
      auto &e = this->templates.subList();
      for (auto &segment : templ.segments) {
        e.subList().append(segment);
        e.push();
      }
      this->templates.push();
    }
  }
  this->templates.compact().compact().shrink_to_fit();
  DI(DW(,"templates are:"); for (const auto &t : this->templates) { DWP(," "); for (const auto &w : t) { DWP(," *",u8string(w.begin(), w.end()).c_str(),"*"); } DWP(,""); })

  init(words, 0, dewordingTemplates, specs);
  dewordingActionCount = specs.size();
  init(words, static_cast<SubSize>(dewordingTemplates.size()), otherTemplates, specs);
  specs.compact().shrink_to_fit();
  DI(
    DW(,"hence, action inputs are:");
    for (Size i = 0; i != getSize(); ++i) {
      u8string o;
      get(i).getInput(o);
      DW(,"  **",o.c_str(),"**");
    }
  )
}

void ActionSet::init (
  const vector<Word> &words, SubSize nextTemplateI, const vector<Template> &templates,
  MultiList<string<SubSize>, Size> &specs
) {
  DS();
  string<SubSize> spec;
  for (const Template &templ : templates) {
    spec.clear();
    spec.push_back(nextTemplateI++);
    initImpl(words, templ, 0, specs, spec);
  }
}

void ActionSet::initImpl (
  const vector<Word> &words, const Template &templ, SubSize templateWordI,
  MultiList<string<SubSize>, Size> &specs, string<SubSize> &r_spec
) {
  if (templateWordI == templ.words.size()) {
    DA(r_spec.size() == static_cast<size_t>(templateWordI) + 1);
    DWP(, "adding action ",specs.size()," (template ",r_spec[0]," with words"); for (size_t i = 1; i != r_spec.size(); ++i) { DWP(, " ", r_spec[i]); } DW(, ")");
    specs.subList().append(r_spec);
    specs.push();
    return;
  }

  Word::CategorySet categories = templ.words[templateWordI];
  size_t specPreSize = r_spec.size();
  for (SubSize i = 0, end = static_cast<SubSize>(words.size()); i != end; ++i) {
    const Word &word = words[i];
    if ((categories & word.categories) == categories) {
      r_spec.push_back(i);
      initImpl(words, templ, templateWordI + 1, specs, r_spec);
      r_spec.erase(specPreSize);
    }
  }
}

ActionSet::SubSize ActionSet::getWordsSize () const {
  return words.size();
}

MultiList<core::u8string, ActionSet::SubSize>::SubList ActionSet::getWord (SubSize i) const {
  return words.get(i);
}

ActionSet::Size ActionSet::getSize () const {
  return specs.size();
}

ActionSet::Action ActionSet::get (Size id) const {
  return Action(*this, id);
}

ActionSet::Action::Action (const ActionSet &actionSet, Size id) : Action(actionSet, id, actionSet.specs.get(id)) {
}

ActionSet::Action::Action (const ActionSet &actionSet, Size id, MultiList<core::string<SubSize>, Size>::SubList &&spec) :
  actionSet(actionSet), spec(move(spec)), dewording(id < actionSet.dewordingActionCount)
{
  DA(actionSet.templates.get(*this->spec.begin()).size() == this->spec.size());
}

ActionSet::Size ActionSet::Action::getDewordingTarget () const {
  if (!dewording) {
    return nonId;
  }

  DA(getWordCount() == 1);
  return getWord(0);
}

size_t ActionSet::Action::getWordCount () const {
  return spec.size() - 1;
}

ActionSet::SubSize ActionSet::Action::getWord (size_t i) const {
  DA(i < getWordCount());
  return *(spec.begin() + 1 + i);
}

void ActionSet::Action::getInput (u8string &r_out) const {
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

bool ActionSet::Action::includesAnyWords (const Bitset &words) const {
  for (size_t i = 0, end = getWordCount(); i != end; ++i) {
    if (words.getBit(getWord(i))) {
      return true;
    }
  }
  return false;
}

Rangeset::Rangeset (const Bitset &bitset, iu16 rangesEnd) : vector() {
  DS();
  DW(, "creating Rangeset from a bitset:");
  DPRE(bitset.getNextSetBit(rangesEnd) == Bitset::nonIndex);

  for (size_t end = 0; end != rangesEnd;) {
    size_t prevEnd = end;
    size_t begin = bitset.getNextClearBit(end);
    end = bitset.getNextSetBit(begin + 1);
    if (end == Bitset::nonIndex) {
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

Rangeset::Rangeset () : vector() {
}

ActionExecutor::ActionResult::ActionResult (ActionSet::Size id, u8string output, size_t similarSiblingReverseOffset) :
  id(id), output(move(output)), similarSiblingReverseOffset(similarSiblingReverseOffset)
{
}

ActionExecutor::ActionResult::ActionResult (ActionSet::Size id, u8string output, HashWrapper<Signature> signature) :
  id(id), output(move(output)), similarSiblingReverseOffset(0), signature(move(signature))
{
}

ActionExecutor::ActionResult::ActionResult (ActionSet::Size id, u8string output, HashWrapper<Signature> signature, State state, vector<zword> significantWords) :
  id(id), output(move(output)), similarSiblingReverseOffset(0), signature(move(signature)), state(move(state)), significantWords(move(significantWords))
{
}

ActionExecutor::ActionResult::ActionResult (const SerialiserBase &) {
}

LocalActionExecutor::LocalActionExecutor (Story &&story, u8string &r_initialOutput) :
  vm(story.zcodeFileName, story.screenWidth, story.screenHeight, 0, true, r_initialOutput),
  saver(move(story.saver)), restorer(move(story.restorer)), actionSet(story.words, story.dewordingTemplates, story.otherTemplates),
  deworder(move(story.deworder)), significantWordAddrs(move(story.significantWordAddrs)), ignoredByteRangeset()
{
  if (!vm.isAlive()) {
    throw PlainException(u8"VM died while initialising");
  }
  doAction(vm, story.prologueInput, r_initialOutput, u8"VM died while running the prologue input");
}

iu16 LocalActionExecutor::getDynamicMemorySize () const noexcept {
  return vm.getDynamicMemorySize();
}

Bitset &LocalActionExecutor::getWordSet () noexcept {
  return *vm.getWordSet();
}

void LocalActionExecutor::clearWordSet () noexcept {
  return getWordSet().clear();
}

const ActionSet &LocalActionExecutor::getActionSet () const noexcept {
  return actionSet;
}

const Rangeset &LocalActionExecutor::getIgnoredByteRangeset () const noexcept {
  return ignoredByteRangeset;
}

void LocalActionExecutor::setIgnoredByteRangeset (Rangeset ignoredByteRangeset) noexcept {
  this->ignoredByteRangeset = move(ignoredByteRangeset);
}

void LocalActionExecutor::doAction (Vm &r_vm, u8string::const_iterator inputBegin, u8string::const_iterator inputEnd, u8string &r_output, const char8_t *deathExceptionMsg) {
  DPRE(r_vm.isAlive());

  r_vm.doAction(inputBegin, inputEnd, r_output);
  if (!r_vm.isAlive()) {
    throw PlainException(deathExceptionMsg);
  }
}

void LocalActionExecutor::doAction (Vm &r_vm, const u8string &input, u8string &r_output, const char8_t *deathExceptionMsg) {
  doAction(r_vm, input.begin(), input.end(), r_output, deathExceptionMsg);
}

void LocalActionExecutor::doSaveAction (State &r_state) {
  vm.setSaveState(&r_state);
  finally([&] () {
    vm.setSaveState(nullptr);
  });

  bool succeeded = saver(vm);
  DPRE(vm.isAlive());

  if (!succeeded) {
    throw PlainException(u8"save action didn't cause saving");
  }
}

void LocalActionExecutor::doRestoreAction (const State &state) {
  vm.setRestoreState(&state);
  finally([&] () {
    vm.setRestoreState(nullptr);
  });

  bool succeeded = restorer(vm);
  DPRE(vm.isAlive());

  if (!succeeded) {
    throw PlainException(u8"restore action didn't cause restoration");
  }
}

vector<zword> LocalActionExecutor::getSignificantWords () const {
  vector<zword> significantWords;
  significantWords.reserve(significantWordAddrs.size());

  const zbyte *m = vm.getDynamicMemory();
  for (zword significantWordAddr : significantWordAddrs) {
    DPRE((static_cast<iu>(significantWordAddr) + 1) < vm.getDynamicMemorySize() + 0);
    auto w = static_cast<zword>(static_cast<zword>(m[significantWordAddr] << 8) | m[significantWordAddr + 1]);
    significantWords.emplace_back(w);
  }

  return significantWords;
}

ActionExecutor::ActionResult LocalActionExecutor::getActionResult () {
  HashWrapper<Signature> signature(Multiverse::Node::createSignature(vm, ignoredByteRangeset));
  vector<zword> significantWords = getSignificantWords();
  State state;
  doSaveAction(state);
  return ActionResult(0, u8string(), move(signature), move(state), move(significantWords));
}

void LocalActionExecutor::processNode (
  vector<ActionResult> &r_results, bitset::Bitset *extraWordSet,
  const HashWrapper<Signature> &parentSignature, const State &parentState
) {
  processNode(r_results, extraWordSet, parentSignature, parentState, [] (const HashWrapper<Signature> &signature) -> bool {
    return false;
  });
}

void LocalActionExecutor::processNode (
  vector<ActionResult> &r_results, bitset::Bitset *extraWordSet,
  const HashWrapper<Signature> &parentSignature, const State &parentState,
  const function<bool (const HashWrapper<Signature> &signature)> &signatureKnown
) {
  DS();

  Bitset initialWordSet;
  if (extraWordSet) {
    initialWordSet = getWordSet();
  }

  Bitset dewordedWords;
  unordered_map<reference_wrapper<const HashWrapper<Signature>>, size_t> signaturesToResultsI;
  r_results.reserve(r_results.size() + actionSet.getSize());

  u8string input;
  u8string output;
  State state;
  DW(, "processing node with sig of hash ", parentSignature.hashFast(), ":");
  for (ActionSet::Size id = 0, end = actionSet.getSize(); id != end; ++id) {
    ActionSet::Action action = actionSet.get(id);

    if (action.includesAnyWords(dewordedWords)) {
      DW(, "was about to process action of id ",id,", but at least one of the words used in the action is in the deworded set");
      continue;
    }

    input.clear();
    output.clear();
    state.clear();

    action.getInput(input);
    DW(, "processing action **",input.c_str(),"** (id ",id,")");

    doRestoreAction(parentState);
    doAction(vm, input, output, u8"VM died while doing action");
    HashWrapper<Signature> signature(Multiverse::Node::createSignature(vm, ignoredByteRangeset));

    auto dewordingWord = action.getDewordingTarget();
    if (dewordingWord != ActionSet::nonId) {
      DW(, "this action is a dewording one (for word of id ",dewordingWord,")");
      if (deworder(vm, output)) {
        DW(, "word of id ",dewordingWord," is missing!");
        dewordedWords.setBit(dewordingWord);
      }
    }

    if (signature == parentSignature) {
      DW(, "the resultant VM state is the same as the parent's, so skipping");
      continue;
    }
    auto v = find(signaturesToResultsI, cref(signature));
    if (v) {
      size_t similarSiblingResultI = *v;
      DA(similarSiblingResultI < r_results.size());
      DW(, "the resultant VM state was also reached by the sibling with action of id ", r_results[similarSiblingResultI].id);
      r_results.emplace_back(id, output, r_results.size() - similarSiblingResultI);
      continue;
    }
    if (signatureKnown(signature)) {
      DW(, "the resultant VM state was also reached by a prior processing run");
      r_results.emplace_back(id, output, move(signature));
      signaturesToResultsI.emplace(cref(r_results.back().signature), r_results.size() - 1);
      continue;
    }
    DW(, "the resultant VM state is new (to us, right now, at least)");

    vector<zword> significantWords = getSignificantWords();

    DW(, "output from the action is **", output.c_str(), "**");
    try {
      doSaveAction(state);
    } catch (...) {
      state.clear();
    }

    r_results.emplace_back(id, output, move(signature), state, move(significantWords));
    signaturesToResultsI.emplace(cref(r_results.back().signature), r_results.size() - 1);
  }

  if (extraWordSet) {
    *extraWordSet = Bitset::andNot(getWordSet(), move(initialWordSet));
    extraWordSet->compact(); // TODO automatically compact after operations? (but we need to be clear on when that can happen, for the *Existing*()s' sake)
  }
}

string<iu8f> RemoteActionExecutor::getBuildId () {
  const char *raw = __TIMESTAMP__; // DODGY close enough
  return string<iu8f>(reinterpret_cast<const iu8f *>(raw), strlen(raw));
}

RemoteActionExecutor::RemoteActionExecutor (const TcpSocketAddress &addr) : stream(addr, true), out(stream, bufferSize), in(stream, bufferSize) {
  beginRequest(ActionExecutorServer::initId);
  Serialiser<OutputStreamIterator<TcpSocketStream>> s(out);
  string<iu8f> buildId = getBuildId();
  s.process(buildId);
  endRequest(ActionExecutorServer::initId);

  beginResponse(ActionExecutorServer::initId);
  check(1, read());
  endResponse(ActionExecutorServer::initId);
}

void RemoteActionExecutor::beginRequest (iu8f requestId) {
  DW(c, "beginRequest ", requestId);
  *(out++) = requestId;
}

void RemoteActionExecutor::endRequest (iu8f requestId) {
  DW(c, "endRequest ", requestId);
  requestId = ~requestId;
  *(out++) = requestId;

  out.flushToStream();
}

iu8f RemoteActionExecutor::read () {
  if (in == InputStreamEndIterator<TcpSocketStream>()) {
    throw PlainException(u8"received incomplete data from server");
  }
  return *(in++);
}

void RemoteActionExecutor::check (iu expected, iu received) {
  if (expected != received) {
    throw PlainException(u8"received incorrect data from server");
  }
}

void RemoteActionExecutor::beginResponse (iu8f requestId) {
  DW(c, "beginResponse ", requestId);
  check(requestId, read());
}

void RemoteActionExecutor::endResponse (iu8f requestId) {
  DW(c, "endResponse ", requestId);
  requestId = ~requestId;
  check(requestId, read());
}

void RemoteActionExecutor::clearWordSet () {
  beginRequest(ActionExecutorServer::clearWordSetId);
  endRequest(ActionExecutorServer::clearWordSetId);

  beginResponse(ActionExecutorServer::clearWordSetId);
  endResponse(ActionExecutorServer::clearWordSetId);
}

void RemoteActionExecutor::setIgnoredByteRangeset (Rangeset ignoredByteRangeset) {
  beginRequest(ActionExecutorServer::setIgnoredByteRangesetId);
  Serialiser<OutputStreamIterator<TcpSocketStream>> s(out);
  s.process(ignoredByteRangeset);
  endRequest(ActionExecutorServer::setIgnoredByteRangesetId);

  beginResponse(ActionExecutorServer::setIgnoredByteRangesetId);
  endResponse(ActionExecutorServer::setIgnoredByteRangesetId);
}

void RemoteActionExecutor::processNode (
  vector<ActionResult> &r_results, Bitset *extraWordSet,
  const HashWrapper<Signature> &parentSignature, const State &parentState
) {
  beginRequest(ActionExecutorServer::processNodeId);
  Serialiser<OutputStreamIterator<TcpSocketStream>> s(out);
  bool hasExtraWordSet = !!extraWordSet;
  s.process(hasExtraWordSet);
  s.process(const_cast<HashWrapper<Signature> &>(parentSignature));
  s.process(const_cast<State &>(parentState)); // TODO sort out const support for read-only Walkers
  endRequest(ActionExecutorServer::processNodeId);

  beginResponse(ActionExecutorServer::processNodeId);
  InputStreamEndIterator<TcpSocketStream> end;
  Deserialiser<InputStreamIterator<TcpSocketStream>, InputStreamEndIterator<TcpSocketStream>> d(in, end);
  if (r_results.empty()) {
    d.process(r_results);
  } else {
    thread_local vector<ActionResult> results;
    d.process(results);
    for (ActionResult *i = results.data(), *end = i + results.size(); i != end; ++i) {
      r_results.emplace_back(move(*i));
    }
    results.clear();
  }
  if (hasExtraWordSet) {
    d.process(*extraWordSet);
  }
  endResponse(ActionExecutorServer::processNodeId);
}

ActionExecutorServer::ActionExecutorServer (LocalActionExecutor &r_e, const TcpSocketAddress &addr) : r_e(r_e), listeningSocket(addr, 1) {
}

iu8f ActionExecutorServer::read (InputStreamIterator<TcpSocketStream> &r_in) {
  if (r_in == InputStreamEndIterator<TcpSocketStream>()) {
    throw PlainException(u8"received incomplete data from client");
  }
  return *(r_in++);
}

void ActionExecutorServer::check (iu expected, iu received) {
  if (expected != received) {
    throw PlainException(u8"received incorrect data from client");
  }
}

iu8f ActionExecutorServer::beginRequest (InputStreamIterator<TcpSocketStream> &r_in) {
  DW(c, "beginRequest");
  if (r_in == InputStreamEndIterator<TcpSocketStream>()) {
    return numeric_limits<iu8f>::max();
  }
  return read(r_in);
}

void ActionExecutorServer::endRequest (InputStreamIterator<TcpSocketStream> &r_in, iu8f requestId) {
  DW(c, "endRequest ", requestId);
  requestId = ~requestId;
  check(requestId, read(r_in));
}

void ActionExecutorServer::beginResponse (OutputStreamIterator<TcpSocketStream> &r_out, iu8f requestId) {
  DW(c, "beginResponse ", requestId);
  *(r_out++) = requestId;
}

void ActionExecutorServer::endResponse (OutputStreamIterator<TcpSocketStream> &r_out, iu8f requestId) {
  DW(c, "endResponse ", requestId);
  requestId = ~requestId;
  *(r_out++) = requestId;

  r_out.flushToStream();
}

void ActionExecutorServer::accept () {
  TcpSocketStream stream = listeningSocket.accept(true);
  finally([&] () {
    stream.close();
  });
  InputStreamIterator<TcpSocketStream> in(stream, RemoteActionExecutor::bufferSize);
  InputStreamEndIterator<TcpSocketStream> end;
  OutputStreamIterator<TcpSocketStream> out(stream, RemoteActionExecutor::bufferSize);

  unordered_set<HashWrapper<Signature>> knownSignatures;
  while (true) {
    iu8f requestId = beginRequest(in);
    DW(c, "- request is ", requestId);
    switch (requestId) {
      case numeric_limits<iu8f>::max():
        return;
      case ActionExecutorServer::initId: {
        Deserialiser<InputStreamIterator<TcpSocketStream>, InputStreamEndIterator<TcpSocketStream>> d(in, end);
        string<iu8f> buildId;
        d.process(buildId);
        endRequest(in, requestId);

        bool clientValid = buildId == RemoteActionExecutor::getBuildId();

        beginResponse(out, requestId);
        *(out++) = clientValid;
        endResponse(out, requestId);

        if (!clientValid) {
          return;
        }
      } break;
      case ActionExecutorServer::clearWordSetId: {
        endRequest(in, requestId);

        r_e.clearWordSet();

        beginResponse(out, requestId);
        endResponse(out, requestId);
      } break;
      case ActionExecutorServer::setIgnoredByteRangesetId: {
        Deserialiser<InputStreamIterator<TcpSocketStream>, InputStreamEndIterator<TcpSocketStream>> d(in, end);
        thread_local Rangeset ignoredByteRangeset;
        d.process(ignoredByteRangeset);
        endRequest(in, requestId);

        r_e.setIgnoredByteRangeset(ignoredByteRangeset);

        beginResponse(out, requestId);
        endResponse(out, requestId);

        knownSignatures.clear();
      } break;
      case ActionExecutorServer::processNodeId: {
        Deserialiser<InputStreamIterator<TcpSocketStream>, InputStreamEndIterator<TcpSocketStream>> d(in, end);
        bool hasExtraWordSet;
        d.process(hasExtraWordSet);
        thread_local HashWrapper<Signature> parentSignature;
        d.process(parentSignature);
        thread_local State parentState;
        d.process(parentState);
        endRequest(in, requestId);

        thread_local vector<ActionExecutor::ActionResult> results;
        results.clear();
        Bitset extraWordSet;
        r_e.processNode(results, hasExtraWordSet ? &extraWordSet : nullptr, parentSignature, parentState, [&] (const HashWrapper<Signature> &signature) -> bool {
          return contains(knownSignatures, signature);
        });

        beginResponse(out, requestId);
        Serialiser<OutputStreamIterator<TcpSocketStream>> s(out);
        s.process(results);
        if (hasExtraWordSet) {
          s.process(extraWordSet);
        }
        endResponse(out, requestId);

        for (auto &result : results) {
          auto &signature = result.signature;
          if (!signature.get().empty()) {
            knownSignatures.emplace(move(signature));
          }
        }
        results.clear();
      } break;
    }
  }
}

Multiverse::Node *const Multiverse::Node::unparented = static_cast<Node *>(nullptr) + 1;
const u8string Multiverse::Node::outputLineTerminator = u8"\n\n";

Multiverse::Node::Node (unique_ptr<Listener> &&listener, Node *primeParentNode, HashWrapper<Signature> &&signature, State &&state) :
  listener(move(listener)), primeParentNode(primeParentNode), primeParentNodeInvalid(false), signature(move(signature)), state(), children()
{
  if (!state.isEmpty()) {
    this->state.reset(new State(move(state)));
  }
  DW(, "created new Node with sig of hash ", this->signature.hashFast());
}

Multiverse::Node::Node (const SerialiserBase &) : primeParentNode(nullptr), primeParentNodeInvalid(false) {
}

Signature Multiverse::Node::createSignature (const Vm &vm, const Rangeset &ignoredByteRangeset) {
  DS();
  DPRE(vm.isAlive());

  Signature signature;

  const zbyte *mem = vm.getDynamicMemory();
  const zbyte *initialMem = vm.getInitialDynamicMemory();
  Signature::Writer writer(signature);
  DI(size_t ec = 0, ms = 0;)
  for (const auto &part : ignoredByteRangeset) {
    //DW(, "part has ignored len ",part.setSize," followed by used size ",part.clearSize);
    writer.appendZeroBytes(part.setSize);
    mem += part.setSize;
    initialMem += part.setSize;

    for (iu16 i = 0; i != part.clearSize; ++i) {
      // TODO something like memcmp, but that returns the first point where the two blocks don't match
      DI(ec += *mem == *initialMem;)
      writer.appendByte(*mem ^ *initialMem);
      ++mem;
      ++initialMem;
    }
    DI(ms += part.clearSize;)
  }
  DA(mem == vm.getDynamicMemory() + vm.getDynamicMemorySize());
  DW(, "unchanged (of the non-ignoreds): ", ec, " / ", ms, " bytes");
  writer.close();

  return signature;
}

Signature Multiverse::Node::recreateSignature (const Signature &oldSignature, const Rangeset &extraIgnoredByteRangeset) {
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

Multiverse::Node::Listener *Multiverse::Node::getListener () const {
  return listener.get();
}

Multiverse::Node *Multiverse::Node::getPrimeParentNode () const {
  DPRE(!primeParentNodeInvalid);
  DPRE(primeParentNode != unparented);
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
  DW(, "done with trying to process Node with sig of hash ", signature.hashFast());
  state.reset();
}

size_t Multiverse::Node::getChildrenSize () const {
  return children.size();
}

const tuple<ActionSet::Size, StringSet<char8_t>::String, Multiverse::Node *> &Multiverse::Node::getChild (size_t i) const {
  return children[i];
}

size_t Multiverse::Node::getChildIndex (ActionSet::Size id) const {
  auto i = lower_bound(children.begin(), children.end(), id, [] (const tuple<ActionSet::Size, StringSet<char8_t>::String, Node *> &elmt, const ActionSet::Size &target) {
    return get<0>(elmt) < target;
  });
  return (i == children.end() || get<0>(*i) != id) ? numeric_limits<size_t>::max() : offset(children.begin(), i);
}

void Multiverse::Node::addChild (ActionSet::Size actionId, const u8string &output, Node *node, Multiverse &multiverse) {
  DW(, "adding to Node with sig of hash ", signature.hashFast(), " child of action id ", actionId, ":");
  DW(, "  output is **", output.c_str(), "**");
  DW(, "  dest. Node has sig of hash ", node->getSignature().hashFast());
  DPRE(!!state, "children cannot be added after all have been added");
  DPRE(children.empty() || get<0>(children.back()) < actionId, "children must be added in order of actionId");

  thread_local StringSet<char8_t>::String o;
  DA(o.empty());
  multiverse.outputStringSet.createString(output, outputLineTerminator, o);
  children.emplace_back(actionId, o, node);
  o.clear();

  bool primeParentChanged = node->updatePrimeParent(this, false);
  if (primeParentChanged) {
    multiverse.listener->subtreePrimeAncestorsUpdated(multiverse, node);
  }
}

bool Multiverse::Node::updatePrimeParent (Node *newParentNode, bool changedAbove) {
  DS();
  DW(, "checking if Node with sig of hash ", signature.hashFast(), " needs its prime parent updated");
  DPRE(!!newParentNode);
  DPRE(!primeParentNodeInvalid);

  bool changed = false;

  if (primeParentNode == newParentNode) {
    DW(, "  same parent");
  } else if (!primeParentNode) {
    DW(, "  this Node is already the root!");
  } else if (primeParentNode == unparented) {
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
      bool emplaced = get<1>(seenNodes.emplace(childNode));
      if (!emplaced) {
        continue;
      }

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

void Multiverse::Node::childrenUpdated () {
  DW(, "finished updating children (new count ", children.size(), ") on Node with sig of hash ", signature.hashFast());
  DPRE(!primeParentNodeInvalid);
  children.shrink_to_fit();
}

void Multiverse::Node::invalidatePrimeParent () {
  DA(!primeParentNodeInvalid);
  primeParentNodeInvalid = true;
}

void Multiverse::Node::rebuildPrimeParents (Multiverse &multiverse) {
  DS();
  DW(, "number of surviving nodes is ", multiverse.nodes.size());
  DI(
    for (Node *node : multiverse) {
      DA(node->primeParentNodeInvalid);
    }
  )

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

          DA(childNode->primeParentNode != unparented);
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

Multiverse::Node *const &Multiverse::NodeIterator::operator_ind_ () const {
  return get<1>(*i);
}

Multiverse::Multiverse (Story &&story, u8string &r_initialOutput, unique_ptr<Listener> &&listener, const vector<zword> &initialIgnoredBytes) :
  localExecutor(move(story), r_initialOutput), listener(move(listener)), ignoredBytes(initIgnoredBytes(initialIgnoredBytes)), rootNode(nullptr)
{
  DS();
  DPRE(!!this->listener);

  ignoredBytesChanged();

  ActionExecutor::ActionResult actionResult = localExecutor.getActionResult();

  unique_ptr<Node::Listener> nodeListener(this->listener->createNodeListener());
  this->listener->nodeReached(*this, nodeListener.get(), ActionSet::nonId, r_initialOutput, actionResult.signature.get(), actionResult.significantWords);
  unique_ptr<Node> node(new Node(move(nodeListener), nullptr, move(actionResult.signature), move(actionResult.state)));
  rootNode = node.get();
  nodes.emplace(ref(rootNode->getSignature()), rootNode);
  this->listener->subtreePrimeAncestorsUpdated(*this, rootNode);
  node.release();
}

Bitset Multiverse::initIgnoredBytes (const vector<zword> &initialIgnoredBytes) {
  Bitset ignoredBytes;
  for (const auto &addr : initialIgnoredBytes) {
    ignoredBytes.setBit(addr);
  }

  return ignoredBytes;
}

Multiverse::~Multiverse () noexcept {
  for (const auto &e : nodes) {
    delete get<1>(e);
  }
}

const ActionSet &Multiverse::getActionSet () const {
  return localExecutor.getActionSet();
}

Multiverse::Listener *Multiverse::getListener () const {
  return listener.get();
}

const Bitset &Multiverse::getIgnoredBytes () const {
  return ignoredBytes;
}

const StringSet<char8_t> &Multiverse::getOutputStringSet () const {
  return outputStringSet;
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

void Multiverse::ignoredBytesChanged () {
  localExecutor.setIgnoredByteRangeset(Rangeset(ignoredBytes, localExecutor.getDynamicMemorySize()));
  remoteExecutorIgnoredBytesCleans.clear();
}

void Multiverse::addRemoteExecutor (const TcpSocketAddress &addr) {
  remoteExecutors.emplace_back(unique_ptr<RemoteActionExecutor>(new RemoteActionExecutor(addr)));
  DA(!remoteExecutorIgnoredBytesCleans.getBit(remoteExecutors.size() - 1));
}

void Multiverse::removeRemoteExecutors () {
  remoteExecutors.clear();
  remoteExecutorIgnoredBytesCleans.clear();
}

Multiverse::Node *Multiverse::collapseNode (
  Node *node, const Rangeset &extraIgnoredByteRangeset,
  unordered_map<reference_wrapper<const HashWrapper<Signature>>, Node *> &r_survivingNodes,
  unordered_map<Node *, Node *> &r_nodeCollapseTargets,
  unordered_map<Node *, HashWrapper<Signature>> &r_survivingNodePrevSignatures
) {
  DS();
  DW(, "checking for collapse of node with sig of prev hash ", node->getSignature().hashFast());

  // If we've already processed the node (be it surviving or collapsed away),
  // just return its new target.
  auto v = find(r_nodeCollapseTargets, node);
  if (v) {
    DW(, " we've already processed this one (and it ", (*v == node) ? "survived" : "was collapsed away", ")");
    return *v;
  }

  const HashWrapper<Signature> &prevSignature = node->getSignature();
  HashWrapper<Signature> signature(Node::recreateSignature(prevSignature.get(), extraIgnoredByteRangeset));
  DW(, " this node now has signature ", signature.hashFast());

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
      node->childrenUpdated();
    }
    listener->nodeCollapsed(*this, node, mutated);
    node->invalidatePrimeParent();

    return node;
  }
}

void Multiverse::save (const u8string &pathName) {
  DS();
  FileStream h(pathName, FileStream::Mode::readWriteRecreate);
  FileOutputIterator i(h);
  auto s = Serialiser<FileOutputIterator>(i);

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

  s.process(localExecutor.getWordSet());
  s.process(ignoredBytes);
  s.process(outputStringSet);
  s.derefAndProcess(rootNode);

  i.flushToStream();
}

void Multiverse::load (const u8string &pathName) {
  DS();
  FileStream h(pathName, FileStream::Mode::readExisting);
  FileInputIterator i(h);
  FileInputEndIterator end;
  auto s = Deserialiser<FileInputIterator, FileInputEndIterator>(i, end);

  ignoredBytes.clear();
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
    ignoredBytesChanged();
    s.process(outputStringSet);
    s.derefAndProcess(rootNode);

    unordered_set<Node *> seenNodes;
    rootNode->forEach([&] (Node *node) -> bool {
      bool emplaced = get<1>(seenNodes.emplace(node));
      if (!emplaced) {
        return false;
      }

      nodes.emplace(ref(node->getSignature()), node);
      return true;
    });

    localExecutor.getWordSet() = move(wordSet);
    // TODO validate result
  } catch (...) {
    nthrow(PlainException(u8"loading failed"));
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
