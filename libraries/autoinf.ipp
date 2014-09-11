#include "autoinf.using"

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */


// XXXX move this lot to somewhere more core!!

template<typename _F> class Finally {
  prv _F functor;

  pub Finally (_F &&functor) : functor(move(functor)) {
  }

  pub ~Finally () noexcept {
    try {
      functor();
    } catch (...) {
      // XXXX
    }
  }
};

template<typename _F> Finally<_F> finally (_F &&functor) {
  return Finally<_F>(move(functor));
}

template<typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator, typename Key_> const T *find (const unordered_map<Key, T, Hash, KeyEqual, Allocator> &map, const Key_ &key) {
  auto e = map.find(key);
  return e == map.end() ? nullptr : &get<1>(*e);
}

template<typename Key, typename T, typename Compare, typename Allocator, typename Key_> const T *find (const map<Key, T, Compare, Allocator> &map, const Key_ &key) {
  auto e = map.find(key);
  return e == map.end() ? nullptr : &get<1>(*e);
}

template<typename Key, typename Hash, typename KeyEqual, typename Allocator, typename Key_> bool contains (const unordered_set<Key, Hash, KeyEqual, Allocator> &map, const Key_ &key) {
  auto e = map.find(key);
  return e != map.end();
}



template<typename _OutputIterator> Serialiser<_OutputIterator>::Serialiser (_OutputIterator &&i) : i(move(i)), nextId(NULL_ID + 1) {
  allocations.emplace(nullptr, tuple<id, void *>(NULL_ID, static_cast<char *>(nullptr) + 1));
  DA(!!findAllocationStart(nullptr));
}

template<typename _OutputIterator> constexpr bool Serialiser<_OutputIterator>::isSerialising () const {
  return true;
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::write (iu32 value) {
  DW(, "writing iu value ",value);
  writeIeu(i, value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::write (is32 value) {
  DW(, "writing is value ",value);
  writeIes(i, value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::write (char8_t value) {
  DW(, "writing char8_t value ", u8string(1, value).c_str());
  *(i++) = value;
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::write (const char8_t *begin, const char8_t *end) {
  DW(, "writing char8_t values ", u8string(begin, end).c_str());
  copy(begin, end, i);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (iu16f &r_value) {
  write(static_cast<iu32>(r_value));
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (is16f &r_value) {
  write(static_cast<is32>(r_value));
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (iu32f &r_value) {
  write(static_cast<iu32>(r_value));
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (is32f &r_value) {
  write(static_cast<is32>(r_value));
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (u8string &r_value) {
  write(r_value.size());
  write(r_value.data(), r_value.data() + r_value.size());
}

template<typename _OutputIterator> template<typename _T, typename _SerialisationFunctor, iff(
  std::is_convertible<_SerialisationFunctor, std::function<void (_T &, Serialiser<_OutputIterator> &)>>::value
)> void Serialiser<_OutputIterator>::process (vector<_T> &r_value, const _SerialisationFunctor &serialiseElement) {
  write(r_value.size());
  for (_T &e : r_value) {
    serialiseElement(e, *this);
  }
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (Bitset &r_value) {
  // TODO pull out
  auto p = reinterpret_cast<string<iu> *>(&r_value);
  write(p->size());
  for (iu *i = p->data(), *end = i + p->size(); i != end; ++i) {
    write(*i);
  }
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (State &r_value) {
  // TODO pull out
  string<zbyte> &v = *reinterpret_cast<string<zbyte> *>(&r_value);
  this->process(v);
}

template<typename _OutputIterator> template<typename _Serialisable> void Serialiser<_OutputIterator>::process (_Serialisable &r_value) {
  r_value.serialise(*this);
}

template<typename _OutputIterator> typename Serialiser<_OutputIterator>::decltype_allocations::value_type *Serialiser<_OutputIterator>::findAllocationStart (void *ptr) {
  auto i = allocations.upper_bound(ptr);
  DA(i != allocations.begin());
  --i;
  void *allocationStart = get<0>(*i);
  void *allocationEnd = get<1>(get<1>(*i));
  if (!(ptr >= allocationStart && ptr < allocationEnd)) {
    return nullptr;
  }

  return &(*i);
}

template<typename _OutputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _SerialisationFunctor, iff(
  std::is_convertible<_TypeDeductionFunctor, std::function<std::tuple<SerialiserBase::SubtypeId, void *, size_t> (_T *)>>::value &&
  std::is_convertible<_SerialisationFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Serialiser<_OutputIterator> &)>>::value
)> void Serialiser<_OutputIterator>::derefAndProcess (_T *&o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &, const _SerialisationFunctor &serialiseReferent) {
  void *ptr = o;

  auto allocationEntry = findAllocationStart(ptr);
  if (!allocationEntry) {
    SubtypeId type;
    void *allocationStart;
    size_t size;
    tie(type, allocationStart, size) = deduceReferentType(o);
    DW(, "added allocation ", nextId, " - nonarray, polymorphic, size ", size);
    DPRE(allocationStart <= ptr, "allocation pointers must be no greater than that for any superclass");
    DPRE(size != 0, "object sizes must be non-zero");
    allocations.emplace(allocationStart, tuple<id, void *>(nextId++, static_cast<char *>(allocationStart) + size));
    // TODO check that new entries to the allocations set don't overlap?

    write(NON_ID);
    write(type);
    serialiseReferent(o, allocationStart, type, *this);
  } else {
    write(get<0>(get<1>(*allocationEntry)));
    void *allocationStart = get<0>(*allocationEntry);
    size_t offset = static_cast<size_t>(static_cast<char *>(ptr) - static_cast<char *>(allocationStart));
    write(offset);
  }
}

template<typename _OutputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _SerialisationFunctor, iff(
  std::is_convertible<_TypeDeductionFunctor, std::function<std::tuple<SerialiserBase::SubtypeId, void *, size_t> (_T *)>>::value &&
  std::is_convertible<_SerialisationFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Serialiser<_OutputIterator> &)>>::value
)> void Serialiser<_OutputIterator>::derefAndProcess (unique_ptr<_T> &o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &constructReferent, const _SerialisationFunctor &serialiseReferent) {
  _T *p = o.get();
  this->derefAndProcess(p, deduceReferentType, constructReferent, serialiseReferent);
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::derefAndProcess (_T *&o) {
  void *ptr = o;

  auto allocation = find(allocations, ptr);
  if (!allocation) {
    DW(, "added allocation ", nextId, " - nonarray, nonpolymorphic, size ", sizeof(_T));
    allocations.emplace(ptr, tuple<id, void *>(nextId++, static_cast<char *>(ptr) + sizeof(_T)));

    write(NON_ID);
    this->process(*o);
  } else {
    write(get<0>(*allocation));
  }
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::derefAndProcess (unique_ptr<_T> &o) {
  _T *p = o.get();
  this->derefAndProcess(p);
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::derefAndProcess (_T *&o, size_t count) {
  void *ptr = o;

  auto allocation = find(allocations, ptr);
  if (!allocation) {
    size_t size = (count == 0 ? 1 : count * sizeof(_T));
    DW(, "adding allocation ", nextId, " - array, size ", count, " of ", sizeof(_T));
    allocations.emplace(ptr, tuple<id, void *>(nextId++, static_cast<char *>(ptr) + size));

    write(NON_ID);
    write(count);
    for (_T *i = o, *end = o + count; i != end; ++i) {
      this->process(*i);
    }
  } else {
    write(get<0>(*allocation));
  }
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::derefAndProcess (unique_ptr<_T []> &o, size_t count) {
  _T *p = o.get();
  this->derefAndProcess(p, count);
}

template<typename _OutputIterator> template<typename _T, typename _P> void Serialiser<_OutputIterator>::process (_T *&o, _P *parent) {
  id allocationId;
  size_t offset;
  if (!o) {
    allocationId = NULL_ID;
    offset = 0;
  } else {
    void *ptr = o;

    DPRE(!!parent, "parent must be non-null if o is");
    DPRE(static_cast<void *>(parent) <= ptr, "parent must be no greater than o");
    auto allocationEntry = findAllocationStart(parent);
    DPRE(!!allocationEntry, "the object occupying the allocation must already have been serialised");
    void *allocationStart = get<0>(*allocationEntry);
    DPRE(ptr <= get<1>(get<1>(*allocationEntry)), "o must be within parent's allocation");
    DA(allocationStart <= ptr);
    allocationId = get<0>(get<1>(*allocationEntry));
    offset = static_cast<size_t>(static_cast<char *>(ptr) - static_cast<char *>(allocationStart));
  }
  write(allocationId);
  write(offset);
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::process (_T *&o) {
  this->process(o, o);
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::process (unique_ptr<_T> &o) {
  _T *p = o.get();
  this->process(p);
}

template<typename _InputIterator> Deserialiser<_InputIterator>::Deserialiser (
  _InputIterator &&i, _InputIterator &&end
) : i(move(i)), end(move(end)), allocations() {
  DA(allocations.size() == NON_ID);
  allocations.emplace_back(nullptr);
  DA(allocations.size() == NULL_ID);
  allocations.emplace_back(nullptr);
}

template<typename _InputIterator> constexpr bool Deserialiser<_InputIterator>::isSerialising () const {
  return false;
}

template<typename _InputIterator> template<typename _i> _i Deserialiser<_InputIterator>::readIu () {
  _i value = readIeu<_i>(i, end);
  DW(, "reading u value ",value);
  return value;
}

template<typename _InputIterator> template<typename _i> _i Deserialiser<_InputIterator>::readIs () {
  _i value = readIes<_i>(i, end);
  DW(, "reading s value ",value);
  return value;
}

template<typename _InputIterator> char8_t Deserialiser<_InputIterator>::readChar8 () {
  if (i == end) {
    throw PlainException(u8("input was truncated"));
  }
  char8_t value = *(i++);
  DW(, "reading char8_t value ", u8string(1, value).c_str());
  return value;
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::readChar8s (char8_t *begin, size_t size) {
  for (char8_t *i = begin, *end = i + size; i != end; ++i) {
    if (this->i == this->end) {
      throw PlainException(u8("input was truncated"));
    }
    *i = *(this->i++);
  }
  DW(, "reading char8_t values ", u8string(begin, begin + size).c_str());
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (iu16f &r_value) {
  r_value = readIu<iu16f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (is16f &r_value) {
  r_value = readIs<is16f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (iu32f &r_value) {
  r_value = readIu<iu32f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (is32f &r_value) {
  r_value = readIs<is32f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (u8string &r_value) {
  size_t size = readIu<size_t>();
  r_value.resize_any(size);
  readChar8s(r_value.data(), size);
}

template<typename _InputIterator> template<typename _T, typename _SerialisationFunctor, iff(
  std::is_convertible<_SerialisationFunctor, std::function<void (_T &, Deserialiser<_InputIterator> &)>>::value
)> void Deserialiser<_InputIterator>::process (vector<_T> &r_value, const _SerialisationFunctor &serialiseElement) {
  size_t size = readIu<size_t>();
  r_value.clear();
  r_value.reserve(size);
  for (size_t i = 0; i != size; ++i) {
    emplaceBack(r_value);
    serialiseElement(r_value[i], *this);
  }
}

template<typename _InputIterator> template<typename _T, iff(
  std::is_constructible<_T, Deserialiser<_InputIterator>>::value
)> void Deserialiser<_InputIterator>::emplaceBack (vector<_T> &r_value) {
  DW(, "emplacing_back a ",typeid(_T).name()," object with the custom serialisation constructor");
  r_value.emplace_back(*this);
}

template<typename _InputIterator> template<typename _T, iff(
  !std::is_constructible<_T, Deserialiser<_InputIterator>>::value
)> void Deserialiser<_InputIterator>::emplaceBack (vector<_T> &r_value) {
  DW(, "emplacing_back a ",typeid(_T).name()," object with the default constructor");
  r_value.emplace_back();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (Bitset &r_value) {
  // TODO pull out
  auto p = reinterpret_cast<string<iu> *>(&r_value);
  size_t size = readIu<size_t>();
  p->resize_any(size);
  for (iu *i = p->data(), *end = i + p->size(); i != end; ++i) {
    *i = readIu<iu>();
  }
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (State &r_value) {
  // TODO pull out
  string<zbyte> &v = *reinterpret_cast<string<zbyte> *>(&r_value);
  this->process(v);
}

template<typename _InputIterator> template<typename _Serialisable> void Deserialiser<_InputIterator>::process (_Serialisable &r_value) {
  r_value.serialise(*this);
}

template<typename _InputIterator> SerialiserBase::id Deserialiser<_InputIterator>::readAllocationId () {
  id allocationId = readIu<id>();
  if (allocationId >= allocations.size()) {
    throw PlainException(u8("as-yet-unseen allocation referenced"));
  }
  return allocationId;
}

template<typename _InputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _SerialisationFunctor, iff(
  std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SerialiserBase::SubtypeId)>>::value &&
  std::is_convertible<_SerialisationFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Deserialiser<_InputIterator> &)>>::value
)> void Deserialiser<_InputIterator>::derefAndProcess (_T *&o, const _TypeDeductionFunctor &, const _ConstructionFunctor &constructReferent, const _SerialisationFunctor &serialiseReferent) {
  DPRE(!o, "o must be null");
  id allocationId = readAllocationId();
  if (allocationId == NON_ID) {
    SubtypeId type = readIu<SubtypeId>();

    void *allocationStart;
    size_t size;
    tie(o, allocationStart, size) = constructReferent(type);
    DW(, "added allocation ", allocations.size(), " - nonarray, polymorphic, size ", size);
    allocations.emplace_back(allocationStart);

    serialiseReferent(o, allocationStart, type, *this);
  } else {
    size_t offset = readIu<size_t>();
    o = static_cast<_T *>(static_cast<void *>(static_cast<char *>(allocations[allocationId]) + offset));
  }
}

template<typename _InputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _SerialisationFunctor, iff(
  std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SerialiserBase::SubtypeId)>>::value &&
  std::is_convertible<_SerialisationFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Deserialiser<_InputIterator> &)>>::value
)> void Deserialiser<_InputIterator>::derefAndProcess (unique_ptr<_T> &o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &constructReferent, const _SerialisationFunctor &serialiseReferent) {
  _T *p = nullptr;
  auto _ = finally([&] () {
    o.reset(p);
  });
  this->derefAndProcess(p, deduceReferentType, constructReferent, serialiseReferent);
}

template<typename _InputIterator> template<typename _T> void Deserialiser<_InputIterator>::derefAndProcess (_T *&o) {
  DPRE(!o, "o must be null");
  id allocationId = readAllocationId();
  if (allocationId == NON_ID) {
    o = construct<_T>();
    DW(, "added allocation ", allocations.size(), " - nonarray, nonpolymorphic, size ", sizeof(_T));
    allocations.emplace_back(o);

    this->process(*o);
  } else {
    o = static_cast<_T *>(allocations[allocationId]);
  }
}

template<typename _InputIterator> template<typename _T, iff(
  std::is_constructible<_T, Deserialiser<_InputIterator>>::value
)> _T *Deserialiser<_InputIterator>::construct () {
  DW(, "constructing a ",typeid(_T).name()," object with the custom serialisation constructor");
  return new _T(*this);
}

template<typename _InputIterator> template<typename _T, iff(
  !std::is_constructible<_T, Deserialiser<_InputIterator>>::value
)> _T *Deserialiser<_InputIterator>::construct () {
  DW(, "constructing a ",typeid(_T).name()," object with the default constructor");
  return new _T();
}

template<typename _InputIterator> template<typename _T> void Deserialiser<_InputIterator>::derefAndProcess (unique_ptr<_T> &o) {
  _T *p = nullptr;
  auto _ = finally([&] () {
    o.reset(p);
  });
  this->derefAndProcess(p);
}

template<typename _InputIterator> template<typename _T> void Deserialiser<_InputIterator>::derefAndProcess (_T *&o, size_t count) {
  DPRE(!o, "o must be null");
  id allocationId = readAllocationId();
  if (allocationId == NON_ID) {
    size_t count = readIu<size_t>();

    o = new _T[count];
    DW(, "added allocation ", allocations.size(), " - array, size ", count, " of ", sizeof(_T));
    allocations.emplace_back(o);
    for (_T *i = o, *end = i + count; i != end; ++i) {
      this->process(*i);
    }
  } else {
    o = static_cast<_T *>(allocations[allocationId]);
  }
}

template<typename _InputIterator> template<typename _T> void Deserialiser<_InputIterator>::derefAndProcess (unique_ptr<_T []> &o, size_t count) {
  _T *p = nullptr;
  auto _ = finally([&] () {
    o.reset(p);
  });  this->derefAndProcess(p, count);
}

template<typename _InputIterator> template<typename _T, typename _P> void Deserialiser<_InputIterator>::process (_T *&o, _P *parent) {
  id allocationId = readAllocationId();
  size_t offset = readIu<size_t>();
  o = static_cast<_T *>(static_cast<void *>(static_cast<char *>(allocations[allocationId]) + offset));
}

template<typename _InputIterator> template<typename _T> void Deserialiser<_InputIterator>::process (_T *&o) {
  this->process(o, o);
}

template<typename _InputIterator> template<typename _T> void Deserialiser<_InputIterator>::process (unique_ptr<_T> &o) {
  _T *p;
  this->process(p);
  o.reset(p);
}

template<typename _InputIterator> Signature::Signature (const Deserialiser<_InputIterator> &) : Signature() {
}

template<typename _Serialiser> void Signature::serialise (_Serialiser &s) {
  DS();
  s.process(b);
  s.process(h);
  // TODO validate result
}

template<typename ..._Ts> Multiverse::ActionTemplate::ActionTemplate (_Ts &&...ts) {
  DS();
  segments.reserve((sizeof...(_Ts) + 1) / 2);
  words.reserve((sizeof...(_Ts) - 1) / 2);
  init(forward<_Ts>(ts)...);
  DW(,"created an action template with segments");
  for (auto &s : segments) {
    DW(,"  ", s.c_str());
  }
  DW(,"and words");
  for (auto &w : words) {
    DW(,"  ", w);
  }
}

template<typename ..._Ts> void Multiverse::ActionTemplate::init (u8string &&segment, ActionWord::CategorySet word, _Ts &&...ts)
{
  segments.push_back(move(segment));
  words.push_back(word);
  init(forward<_Ts>(ts)...);
}

template<typename _InputIterator> Multiverse::Node::Node (const Deserialiser<_InputIterator> &) {
}

template<typename _Serialiser> void Multiverse::Node::serialise (_Serialiser &s) {
  DS();
  s.process(signature);
  s.derefAndProcess(state);
  s.process(metricState);
  s.process(children, [] (tuple<ActionId, u8string, Node *> &o, _Serialiser &s) {
    DS();
    s.process(get<0>(o));
    s.process(get<1>(o));
    s.derefAndProcess(get<2>(o));
  });
  // TODO validate result
}

template<typename _F, iff(std::is_convertible<_F, std::function<bool (Multiverse::Node *)>>::value)> void Multiverse::Node::forEach (const _F &f) {
  bool recurse = f(this);
  if (recurse) {
    for (const auto &e : children) {
      get<2>(e)->forEach(f);
    }
  }
}

template<typename _I> void Multiverse::processNodes (_I nodesBegin, _I nodesEnd, Vm &r_vm) {
  typedef tuple<Node *, ActionId, u8string, Signature, State> result;
  DS();

  deque<result> resultQueue;
  mutex resultQueueLock;
  condition_variable resultQueueCondVar;

  packaged_task<void ()> dispatcher([&] () {
    DS();
    auto _ = finally([&] () {
      DS();
      DW(, "adding done result to queue");
      unique_lock<mutex> l(resultQueueLock);
      resultQueue.emplace_back(nullptr, 0, u8string(), Signature(), State());
      resultQueueCondVar.notify_one();
      DW(, "done adding done result to queue");
    });

    u8string input;
    u8string output;
    State postState;
    for (; nodesBegin != nodesEnd; ++nodesBegin) {
      DW(, "looking at the next node:");
      DS();
      Node *parentNode = *nodesBegin;
      DW(, "node has sig of hash ", parentNode->getSignature().hash(), " and ", parentNode->getChildrenSize(), " children");
      const State *parentState = parentNode->getState();
      if (!parentState) {
        DW(, "  node has already been processed, so skipping");
        continue;
      }
      DA(parentNode->getChildrenSize() == 0);

      Bitset dewordedWords;
      for (ActionId id = 0, end = actionSet.getSize(); id != end; ++id) {
        ActionSet::Action action = actionSet.get(id);

        if (action.includesAnyWords(dewordedWords)) {
          DW(, "was about to process action of id ",id,", but at least one of the words used in the action is in the deworded set");
          continue;
        }

        input.clear();
        action.getInput(input);
        DW(, "processing action **",input.c_str(),"** (id ",id,")");
        output.clear();

        doRestoreAction(r_vm, *parentState);
        doAction(r_vm, input, output, u8("VM died while doing action"));
        Signature signature = createSignature(r_vm, ignoredByteRangeset);

        auto dewordingWord = action.getDewordingTarget();
        if (dewordingWord != NON_ID) {
          DW(, "this action is a dewording one (for word of id ",dewordingWord,")");
          if (deworder(r_vm, output)) {
            DW(, "word of id ",dewordingWord," is missing!");
            dewordedWords.setBit(dewordingWord);
          }
        }

        if (signature == parentNode->getSignature()) {
          DW(, "the resultant VM state is the same as the parent's, so skipping");
          continue;
        }

        DW(, "output from the action is **", output.c_str(), "**");
        try {
          // XXXX better response from doSaveAction on 'can't save'?
          doSaveAction(r_vm, postState);
        } catch (...) {
          postState.clear();
        }
        {
          DW(, "adding the result to queue");
          unique_lock<mutex> l(resultQueueLock);
          resultQueue.emplace_back(parentNode, id, output, move(signature), postState);
          resultQueueCondVar.notify_one();
          DW(, "done adding the result to queue");
        }
        // system("sleep 1"); // XXXX
      }
    }
  });
  auto dispatcherFuture = dispatcher.get_future();
  thread dispatcherThread(move(dispatcher));
  auto _ = finally([&dispatcherThread] () {
    dispatcherThread.join();
  });

  Node *prevParentNode = nullptr;
  while (true) {
    unique_lock<mutex> l(resultQueueLock);
    resultQueueCondVar.wait(l, [&] () {
      return !resultQueue.empty();
    });
    DW(, "M getting the next result:");
    result rs = move(resultQueue.front());
    resultQueue.pop_front();
    l.unlock();

    Node *parentNode = get<0>(rs);
    if (prevParentNode && parentNode != prevParentNode) {
      prevParentNode->batchOfChildChangesCompleted();
      prevParentNode->clearState();
      metric.get()->nodeProcessed(*this, *prevParentNode);
    }
    prevParentNode = parentNode;
    if (!parentNode) {
      DW(, "M it's the end of the queue!");
      break;
    }
    DW(, "M the result is a child for the node with sig of hash ", parentNode->getSignature().hash());
    ActionId parentActionId = get<1>(rs);
    u8string &resultOutput = get<2>(rs);
    Signature &resultSignature = get<3>(rs);
    State &resultState = get<4>(rs);
    DW(, "M the child is for the action of id ",parentActionId,"; the sig is of hash ", resultSignature.hash());
    DA(!(resultSignature == parentNode->getSignature()));

    Node *resultNode;
    auto v = find(nodes, resultSignature);
    if (!v) {
      DW(, "M this is a new node for this multiverse!");
      unique_ptr<Metric::State> metricState(metric.get()->nodeCreated(*this, parentActionId, resultOutput, resultSignature));
      unique_ptr<Node> n(new Node(move(resultSignature), move(resultState), move(metricState)));
      resultNode = n.get();
      nodes.emplace(ref(resultNode->getSignature()), resultNode);
      n.release();
    } else {
      DW(, "M there already exists a node of this signature");
      resultNode = *v;
      DA(resultSignature == resultNode->getSignature());
    }
    parentNode->addChild(parentActionId, move(resultOutput), resultNode);
  }

  dispatcherFuture.get();

  metric.get()->nodesProcessed(*this, *rootNode, nodes);
}

template<typename _I> Bitset Multiverse::createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, const Vm &vm) {
  Bitset extraIgnoredBytes;

  iu16 addr = 0;
  const Bitset &vmWordSet = *vm.getWordSet();
  for (auto byte : firstSignature) {
    bool same = true;
    for (auto otherSignatureIsI = otherSignatureIsBegin; otherSignatureIsI != otherSignatureIsEnd; ++otherSignatureIsI) {
      Signature::Iterator &i = *otherSignatureIsI;
      if (*(i++) != byte) {
        same = false;
      }
    }

    if (!same) {
      DW(, "the signatures don't all match at dynmem addr ", addr);
      extraIgnoredBytes.setBit(addr);

      if (vmWordSet.getExistingBit(addr)) {
        DW(, "  (it's the first byte of a zword)");
        extraIgnoredBytes.setBit(addr + 1);
      }
      if (addr > 0 && vmWordSet.getExistingBit(addr - 1)) {
        DW(, "  (it's the second byte of a zword)");
        extraIgnoredBytes.setBit(addr - 1);
      }
    }

    ++addr;
  }
  DA(addr == vm.getDynamicMemorySize());

  return extraIgnoredBytes;
}

template<typename _I> void Multiverse::collapseNodes (_I nodesBegin, _I nodesEnd, const Vm &vm) {
  DS();
  DPRE(nodesBegin != nodesEnd);
  DW(, "Collapsing nodes:");

  // Build the set of extra ignored bytes (those which aren't the same across all
  // of the given nodes).

  Node *firstNode = *(nodesBegin++);
  DW(, "first node has sig of hash", firstNode->getSignature().hash());
  const Signature &firstSignature = firstNode->getSignature();
  vector<Signature::Iterator> otherSignatureIs;
  for (; nodesBegin != nodesEnd; ++nodesBegin) {
    Node *node = *nodesBegin;
    DW(, "other node has sig of hash", node->getSignature().hash());
    otherSignatureIs.emplace_back(node->getSignature().begin());
  }
  Bitset extraIgnoredBytes = createExtraIgnoredBytes(firstSignature, otherSignatureIs.begin(), otherSignatureIs.end(), vm);

  // Walk through the node tree, processing each node once (unless all its parents
  // are collapsed away, in which case it'll never be reached), and rebuild the
  // node map with only those nodes which have survived (and with their new
  // signatures).

  Rangeset extraIgnoredByteRangeset(extraIgnoredBytes, vm.getDynamicMemorySize());
  decltype(nodes) survivingNodes(nodes.bucket_count());
  unordered_map<Node *, Node *> nodeCollapseTargets(nodes.bucket_count());
  unordered_map<Node *, Signature> survivingNodePrevSignatures(nodes.bucket_count());
  // XXXX should have an obj per surviving node to hold this + the child undo data i.e. 'surviving node undo object'

  Node *t = collapseNode(rootNode, extraIgnoredByteRangeset, survivingNodes, nodeCollapseTargets, survivingNodePrevSignatures);
  DW(, "after collapsing all ",nodes.size()," nodes, there are ",survivingNodes.size()," surviving nodes");

  ignoredBytes |= move(extraIgnoredBytes);
  ignoredByteRangeset = Rangeset(ignoredBytes, vm.getDynamicMemorySize());
  DA(t == rootNode);
  size_t dc = 0;
  for (const auto &i : nodes) {
    Node *node = get<1>(i);
    auto s = find(survivingNodePrevSignatures, node);
    if (!s) {
      delete node;
      ++dc;
    }
  }
  DW(, dc, " old nodes were deleted");
  nodes = move(survivingNodes);

  metric.get()->nodesCollapsed(*this, *rootNode, nodes);
}

template<typename _Serialiser> void Multiverse::derefAndProcessMetricState (Metric::State *&state, _Serialiser &s) {
  s.derefAndProcess(state, [&] (Metric::State *state) -> tuple<SerialiserBase::SubtypeId, void *, size_t> {
    auto r = metric->deduceStateType(state);
    return tuple<SerialiserBase::SubtypeId, void *, size_t>(0, get<0>(r), get<1>(r));
  }, [&] (SerialiserBase::SubtypeId type) -> tuple<Metric::State *, void *, size_t> {
    if (type != 0) {
      throw PlainException(u8("invalid metric state type found"));
    }
    return metric->constructState();
  }, [&] (Metric::State *state, void *, SerialiserBase::SubtypeId, _Serialiser &s) {
    metric->serialiseState(state, s);
  });
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
