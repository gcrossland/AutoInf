#include "autoinf.using"

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */


// XXXX move this lot to somewhere more core!!

template<typename _F> class Finally {
  prv _F functor;
  prv bool live;

  pub Finally (_F &&functor) : functor(move(functor)), live(true) {
  }

  Finally (const Finally &) = delete;

  Finally &operator= (const Finally &) = delete;

  pub Finally (Finally &&o) : functor(move(o.functor)), live(o.live) {
    o.live = false;
  }

  Finally &operator= (Finally &&) = delete;

  pub ~Finally () noexcept {
    if (live) {
      try {
        functor();
      } catch (...) {
        // XXXX
      }
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

template<typename _OutputIterator> void Serialiser<_OutputIterator>::writeIu (iu64 value) {
  DW(, "writing iu value ",value);
  writeIeu(i, value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::writeIs (is64 value) {
  DW(, "writing is value ",value);
  writeIes(i, value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::writeOctet (iu8f value) {
  *(i++) = value;
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::writeOctets (const iu8f *begin, const iu8f *end) {
  copy(begin, end, i);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (bool &r_value) {
  writeIu(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (iu8f &r_value) {
  writeIu(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (is8f &r_value) {
  writeIs(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (iu16f &r_value) {
  writeIu(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (is16f &r_value) {
  writeIs(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (iu32f &r_value) {
  writeIu(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (is32f &r_value) {
  writeIs(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (iu64f &r_value) {
  writeIu(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (is64f &r_value) {
  writeIs(r_value);
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (string<iu8f> &r_value) {
  writeIu(r_value.size());
  writeOctets(r_value.data(), r_value.data() + r_value.size());
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (u8string &r_value) {
  auto data = reinterpret_cast<const iu8f *>(r_value.data());
  writeIu(r_value.size());
  DW(, "writing char8_t values ", r_value.c_str());
  writeOctets(data, data + r_value.size());
}

template<typename _OutputIterator> template<typename _T, typename _WalkingFunctor, iff(
  std::is_convertible<_WalkingFunctor, std::function<void (_T &, Serialiser<_OutputIterator> &)>>::value
)> void Serialiser<_OutputIterator>::process (vector<_T> &r_value, const _WalkingFunctor &walkElement) {
  writeIu(r_value.size());
  for (_T &e : r_value) {
    walkElement(e, *this);
  }
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (Bitset &r_value) {
  // TODO pull out
  auto p = reinterpret_cast<string<iu> *>(&r_value);
  writeIu(p->size());
  for (iu *i = p->data(), *end = i + p->size(); i != end; ++i) {
    writeIu(*i);
  }
}

template<typename _OutputIterator> void Serialiser<_OutputIterator>::process (State &r_value) {
  // TODO pull out
  string<zbyte> &v = *reinterpret_cast<string<zbyte> *>(&r_value);
  this->process(v);
}

template<typename _OutputIterator> template<typename _Walkable> void Serialiser<_OutputIterator>::process (_Walkable &r_value) {
  r_value.beWalked(*this);
}

template<typename _OutputIterator> typename Serialiser<_OutputIterator>::decltype_allocations::value_type *Serialiser<_OutputIterator>::findAllocationStart (void *ptr) {
  auto i = allocations.upper_bound(ptr);
  DA(i != allocations.begin());
  --i;
  void *allocationStart = get<0>(*i);
  void *allocationEnd = get<1>(get<1>(*i));
  return (ptr >= allocationStart && ptr < allocationEnd) ? &(*i) : nullptr;
}

template<typename _OutputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
  std::is_convertible<_TypeDeductionFunctor, std::function<std::tuple<SerialiserBase::SubtypeId, void *, size_t> (_T *)>>::value &&
  std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Serialiser<_OutputIterator> &)>>::value
)> void Serialiser<_OutputIterator>::derefAndProcess (_T *&o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &, const _WalkingFunctor &walkReferent) {
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

    writeIu(NON_ID);
    writeIu(type);
    walkReferent(o, allocationStart, type, *this);
  } else {
    writeIu(get<0>(get<1>(*allocationEntry)));
    void *allocationStart = get<0>(*allocationEntry);
    size_t offset = core::offset(static_cast<char *>(allocationStart), static_cast<char *>(ptr));
    writeIu(offset);
  }
}

template<typename _OutputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
  std::is_convertible<_TypeDeductionFunctor, std::function<std::tuple<SerialiserBase::SubtypeId, void *, size_t> (_T *)>>::value &&
  std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Serialiser<_OutputIterator> &)>>::value
)> void Serialiser<_OutputIterator>::derefAndProcess (unique_ptr<_T> &o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent) {
  _T *p = o.get();
  this->derefAndProcess(p, deduceReferentType, constructReferent, walkReferent);
}

template<typename _OutputIterator> template<typename _T> void Serialiser<_OutputIterator>::derefAndProcess (_T *&o) {
  void *ptr = o;

  auto allocation = find(allocations, ptr);
  if (!allocation) {
    DW(, "added allocation ", nextId, " - nonarray, nonpolymorphic, size ", sizeof(_T));
    allocations.emplace(ptr, tuple<id, void *>(nextId++, static_cast<char *>(ptr) + sizeof(_T)));

    writeIu(NON_ID);
    this->process(*o);
  } else {
    writeIu(get<0>(*allocation));
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

    writeIu(NON_ID);
    writeIu(count);
    for (_T *i = o, *end = o + count; i != end; ++i) {
      this->process(*i);
    }
  } else {
    writeIu(get<0>(*allocation));
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
    offset = core::offset(static_cast<char *>(allocationStart), static_cast<char *>(ptr));
  }
  writeIu(allocationId);
  writeIu(offset);
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

template<typename _InputIterator> iu8f Deserialiser<_InputIterator>::readOctet () {
  if (i == end) {
    throw PlainException(u8("input was truncated"));
  }
  iu8f value = *(i++);
  return value;
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::readOctets (iu8f *begin, size_t size) {
  for (iu8f *i = begin, *end = i + size; i != end; ++i) {
    if (this->i == this->end) {
      throw PlainException(u8("input was truncated"));
    }
    *i = *(this->i++);
  }
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (bool &r_value) {
  r_value = static_cast<bool>(readIu<iu8f>());
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (iu8f &r_value) {
  r_value = readIu<iu8f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (is8f &r_value) {
  r_value = readIs<is8f>();
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

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (iu64f &r_value) {
  r_value = readIu<iu64f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (is64f &r_value) {
  r_value = readIs<is64f>();
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (string<iu8f> &r_value) {
  size_t size = readIu<size_t>();
  r_value.resize_any(size);
  readOctets(r_value.data(), size);
}

template<typename _InputIterator> void Deserialiser<_InputIterator>::process (u8string &r_value) {
  size_t size = readIu<size_t>();
  r_value.resize_any(size);
  readOctets(reinterpret_cast<iu8f *>(r_value.data()), size);
  DW(, "reading char8_t values ", r_value.c_str());
}

template<typename _InputIterator> template<typename _T, typename _WalkingFunctor, iff(
  std::is_convertible<_WalkingFunctor, std::function<void (_T &, Deserialiser<_InputIterator> &)>>::value
)> void Deserialiser<_InputIterator>::process (vector<_T> &r_value, const _WalkingFunctor &walkElement) {
  size_t size = readIu<size_t>();
  r_value.clear();
  r_value.reserve(size);
  for (size_t i = 0; i != size; ++i) {
    emplaceBack(r_value);
    walkElement(r_value[i], *this);
  }
}

template<typename _InputIterator> template<typename _T, iff(
  std::is_constructible<_T, Deserialiser<_InputIterator>>::value
)> void Deserialiser<_InputIterator>::emplaceBack (vector<_T> &r_value) {
  DW(, "emplacing_back a ",typeid(_T).name()," object with the custom deserialisation constructor");
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

template<typename _InputIterator> template<typename _Walkable> void Deserialiser<_InputIterator>::process (_Walkable &r_value) {
  r_value.beWalked(*this);
}

template<typename _InputIterator> SerialiserBase::id Deserialiser<_InputIterator>::readAllocationId () {
  id allocationId = readIu<id>();
  if (allocationId >= allocations.size()) {
    throw PlainException(u8("as-yet-unseen allocation referenced"));
  }
  return allocationId;
}

template<typename _InputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
  std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SerialiserBase::SubtypeId)>>::value &&
  std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Deserialiser<_InputIterator> &)>>::value
)> void Deserialiser<_InputIterator>::derefAndProcess (_T *&o, const _TypeDeductionFunctor &, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent) {
  DPRE(!o, "o must be null");
  id allocationId = readAllocationId();
  if (allocationId == NON_ID) {
    SubtypeId type = readIu<SubtypeId>();

    void *allocationStart;
    size_t size;
    tie(o, allocationStart, size) = constructReferent(type);
    DW(, "added allocation ", allocations.size(), " - nonarray, polymorphic, size ", size);
    allocations.emplace_back(allocationStart);

    walkReferent(o, allocationStart, type, *this);
  } else {
    size_t offset = readIu<size_t>();
    o = static_cast<_T *>(static_cast<void *>(static_cast<char *>(allocations[allocationId]) + offset));
  }
}

template<typename _InputIterator> template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
  std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SerialiserBase::SubtypeId)>>::value &&
  std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SerialiserBase::SubtypeId, Deserialiser<_InputIterator> &)>>::value
)> void Deserialiser<_InputIterator>::derefAndProcess (unique_ptr<_T> &o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent) {
  _T *p = nullptr;
  auto _ = finally([&] () {
    o.reset(p);
  });
  this->derefAndProcess(p, deduceReferentType, constructReferent, walkReferent);
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
  DW(, "constructing a ",typeid(_T).name()," object with the custom deserialisation constructor");
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
  if (allocationId == NON_ID) {
    throw PlainException(u8("serialisation of an as-yet-unseen allocation appears in the input where only a reference to an already-seen allocation was expected"));
  }
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

template<typename _Walker> void Signature::beWalked (_Walker &w) {
  DS();
  w.process(b);
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::RevaluedIterator () : i() {
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::RevaluedIterator (_Iterator &&i) : i(move(i)) {
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> const _T *RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator-> () noexcept(noexcept(*(std::declval<_Class>()))) {
  return &(*(*static_cast<const _Class *>(this)));
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> _Class &RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator++ () noexcept(noexcept(++i)) {
  ++i;
  return *static_cast<_Class *>(this);
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> _Class RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator++ (int) noexcept(std::is_nothrow_copy_constructible<_Class>::value && noexcept(++i)) {
  _Class o(*static_cast<_Class *>(this));
  ++i;
  return o;
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> _Class &RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator-- () noexcept(noexcept(--i)) {
  --i;
  return *static_cast<_Class *>(this);
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> _Class RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator-- (int) noexcept(std::is_nothrow_copy_constructible<_Class>::value && noexcept(--i)) {
  _Class o(*static_cast<_Class *>(this));
  --i;
  return o;
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> _Class &RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator+= (const Distance &r) noexcept(noexcept(i += r)) {
  i += r;
  return *static_cast<_Class *>(this);
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> _Class &RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator-= (const Distance &r) noexcept(noexcept(i -= r)) {
  i -= r;
  return *static_cast<_Class *>(this);
}

template<
  typename _Class, typename _T, typename _Iterator, typename _IteratorClass
> const _T &RevaluedIterator<_Class, _T, _Iterator, _IteratorClass>::operator[] (const Distance &r) noexcept(noexcept(*(std::declval<_Class>() + r))) {
  return *(*static_cast<const _Class *>(this) + r);
}

template<typename _L, typename _Size> MultiList<_L, _Size>::SubList::SubList (Iterator &&begin, Iterator &&end) : b(begin), e(end) {
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::SubList::Size MultiList<_L, _Size>::SubList::size () const noexcept {
  return static_cast<Size>(e - b);
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::SubList::Iterator MultiList<_L, _Size>::SubList::begin () const noexcept {
  return b;
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::SubList::Iterator MultiList<_L, _Size>::SubList::end () const noexcept {
  return e;
}

template<typename _L, typename _Size> MultiList<_L, _Size>::Iterator::Iterator () :
  RevaluedIterator<Iterator, SubList, typename vector<typename SubList::Size>::const_iterator>(), listBegin()
{
}

template<typename _L, typename _Size> MultiList<_L, _Size>::Iterator::Iterator (const MultiList<_L, _Size> &multiList) :
  RevaluedIterator<Iterator, SubList, typename vector<typename SubList::Size>::const_iterator>(multiList.bounds.cbegin()), listBegin(static_cast<const _L &>(multiList.list).begin())
{
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::SubList MultiList<_L, _Size>::Iterator::operator* () const {
  return SubList(listBegin + *this->i, listBegin + *(this->i + 1));
}

template<typename _L, typename _Size> MultiList<_L, _Size>::MultiList () {
  bounds.emplace_back(0);
}

template<typename _L, typename _Size> template<typename _Walker> void MultiList<_L, _Size>::beWalked (_Walker &w) {
  w.process(list);
  w.process(bounds, [] (typename SubList::Size &e, _Walker &w) {
    w.process(e);
  });
}

template<typename _L, typename _Size> _Size MultiList<_L, _Size>::size () const noexcept {
  return static_cast<_Size>(bounds.size() - 1);
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::SubList MultiList<_L, _Size>::get (_Size i) const noexcept {
  DPRE(i < size());
  typename SubList::Iterator listBegin = static_cast<const _L &>(list).begin();
  auto boundsI = bounds.begin() + i;
  return SubList(listBegin + *boundsI, listBegin + *(boundsI + 1));
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::Iterator MultiList<_L, _Size>::begin () const {
  return Iterator(*this);
}

template<typename _L, typename _Size> typename MultiList<_L, _Size>::Iterator MultiList<_L, _Size>::end () const {
  return begin() + size();
}

template<typename _L, typename _Size> _L &MultiList<_L, _Size>::subList () noexcept {
  return list;
}

template<typename _L, typename _Size> _Size MultiList<_L, _Size>::push () {
  _Size i = size();
  if (i == numeric_limits<_Size>::max()) {
    throw PlainException(u8("list at maximum size"));
  }
  DPRE(bounds.back() <= list.size());
  bounds.emplace_back(list.size());
  return i;
}

template<typename _L, typename _Size> void MultiList<_L, _Size>::reserve (_Size capacity) {
  bounds.reserve(static_cast<typename vector<typename SubList::Size>::size_type>(capacity) + 1);
}

template<typename _L, typename _Size> _L &MultiList<_L, _Size>::compact () {
  bounds.shrink_to_fit();
  return list;
}

template<typename _c, typename _Size> constexpr _Size StringSet<_c, _Size>::Key::PROPOSED;
template<typename _c, typename _Size> thread_local MultiList<core::string<_c>, _Size> *StringSet<_c, _Size>::Key::list;
template<typename _c, typename _Size> thread_local const _c *StringSet<_c, _Size>::Key::proposedBegin;
template<typename _c, typename _Size> thread_local const _c *StringSet<_c, _Size>::Key::proposedEnd;

template<typename _c, typename _Size> StringSet<_c, _Size>::Key::Key (_Size i) : i(i) {
}

template<typename _c, typename _Size> size_t StringSet<_c, _Size>::Key::hashSlow () const noexcept {
  DPRE(list != nullptr);

  const _c *begin;
  const _c *end;
  if (i == PROPOSED) {
    DPRE(proposedBegin != nullptr);
    DPRE(proposedEnd != nullptr);
    begin = proposedBegin;
    end = proposedEnd;
  } else {
    auto s = list->get(i);
    begin = &*s.begin();
    end = &*s.end();
    DA(offset(begin, end) == s.size());
  }

  return hash(reinterpret_cast<const iu8f *>(begin), reinterpret_cast<const iu8f *>(end));
}

template<typename _c, typename _Size> bool StringSet<_c, _Size>::Key::operator== (const Key &r) const noexcept {
  DPRE(list != nullptr);

  if (i == r.i) {
    return true;
  }

  if (i != PROPOSED && r.i != PROPOSED) {
    return false;
  }

  _Size realI = i + r.i - PROPOSED;
  DA((i == realI && r.i == PROPOSED) || (r.i == realI && i == PROPOSED));
  DPRE(proposedBegin != nullptr);
  DPRE(proposedEnd != nullptr);
  auto s = list->get(realI);
  return proposedEnd - proposedBegin == s.end() - s.begin() && equal(proposedBegin, proposedEnd, s.begin());
}

template<typename _c, typename _Size> StringSet<_c, _Size>::StringSet () {
}

template<typename _c, typename _Size> template<typename _Walker> void StringSet<_c, _Size>::beWalked (_Walker &w) {
  w.process(list);
  if (!w.isSerialising()) {
    DPRE(Key::list == nullptr);
    Key::list = &list;
    DI(
      auto _ = finally([] () {
        Key::list = nullptr;
      });
    )

    DPRE(set.empty());
    for (_Size i = 0, end = list.size(); i != end; ++i) {
      set.emplace(i);
    }
  }
}

template<typename _c, typename _Size> _Size StringSet<_c, _Size>::size () const noexcept {
  return static_cast<_Size>(list.size());
}

template<typename _c, typename _Size> _Size StringSet<_c, _Size>::push (const _c *begin, const _c *end) {
  DPRE(Key::list == nullptr);
  Key::list = &list;
  Key::proposedBegin = begin;
  Key::proposedEnd = end;
  DI(
    auto _ = finally([] () {
      Key::list = nullptr;
      Key::proposedBegin = nullptr;
      Key::proposedEnd = nullptr;
    });
  )

  HashWrapper<Key> proposed(Key::PROPOSED);

  auto e = set.find(proposed);
  if (e != set.end()) {
    return e->get().i;
  }

  list.subList().append(begin, end);
  _Size i = list.push();
  set.emplace(i);
  return i;
}

template<typename _c, typename _Size> typename MultiList<string<_c>, _Size>::SubList StringSet<_c, _Size>::get (_Size i) const noexcept {
  return list.get(i);
}

template<typename _c, typename _Size> void StringSet<_c, _Size>::createString (const string<_c> &o, const string<_c> &terminator, String &r_out) {
  const _c *data = o.data();
  typename string<_c>::size_type begin = 0, end;

  while ((end = o.find(terminator, begin)) != string<_c>::npos) {
    end += terminator.size();
    r_out.emplace_back(push(data + begin, data + end));
    begin = end;
  }

  end = o.size();
  if (begin != end) {
    r_out.emplace_back(push(data + begin, data + end));
  }
}

template<typename _c, typename _Size> void StringSet<_c, _Size>::rebuildString (const String &o, string<_c> &r_out) const {
  for (const auto &i : o) {
    auto s = get(i);
    r_out.append(s.begin(), s.end());
  }
}

template<typename ..._Ts> ActionSet::Template::Template (_Ts &&...ts) {
  DS();
  segments.reserve((sizeof...(_Ts) + 1) / 2);
  words.reserve((sizeof...(_Ts) - 1) / 2);
  init(forward<_Ts>(ts)...);
  DI(DW(,"created an action template with segments"); for (auto &s : segments) { DW(,"  ", s.c_str()); })
  DI(DW(,"and words"); for (auto &w : words) { DW(,"  ", w); })
}

template<typename ..._Ts> void ActionSet::Template::init (u8string &&segment, Word::CategorySet word, _Ts &&...ts) {
  segments.push_back(move(segment));
  words.push_back(word);
  init(forward<_Ts>(ts)...);
}

template<typename _InputIterator> Multiverse::Node::Node (const Deserialiser<_InputIterator> &) : primeParentNode(nullptr), primeParentNodeInvalid(false) {
}

template<typename _Walker> void Multiverse::Node::beWalked (_Walker &w) {
  DS();
  w.process(listener);
  w.derefAndProcess(primeParentNode);
  if (w.isSerialising()) {
    w.process(const_cast<Signature &>(signature.get()));
  } else {
    Signature t;
    w.process(t);
    signature = hashed(move(t));
  }
  w.derefAndProcess(state);
  w.process(children, [] (tuple<ActionSet::Size, StringSet<char8_t>::String, Node *> &o, _Walker &w) {
    DS();
    w.process(get<0>(o));
    w.process(get<1>(o), [] (iu &e, _Walker &w) {
      w.process(e);
    });
    w.derefAndProcess(get<2>(o));
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

template<typename _I> void Multiverse::processNodes (_I nodesBegin, _I nodesEnd) {
  DS();
  deque<tuple<Node *, ActionSet::Size, u8string, HashWrapper<Signature>, State, unique_ptr<Node::Listener>>> resultQueue;
  mutex resultQueueLock;
  condition_variable resultQueueCondVar;

  packaged_task<void ()> dispatcher([&] () {
    DS();
    auto _ = finally([&] () {
      DS();
      DW(, "adding done result to queue");
      unique_lock<mutex> l(resultQueueLock);
      resultQueue.emplace_back(nullptr, 0, u8string(), HashWrapper<Signature>(), State(), nullptr);
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
      DW(, "node has sig of hash ", parentNode->getSignature().hashFast(), " and ", parentNode->getChildrenSize(), " children");
      const State *parentState = parentNode->getState();
      if (!parentState) {
        DW(, "  node has already been processed, so skipping");
        continue;
      }
      DA(parentNode->getChildrenSize() == 0);

      Bitset dewordedWords;
      for (ActionSet::Size id = 0, end = actionSet.getSize(); id != end; ++id) {
        ActionSet::Action action = actionSet.get(id);

        if (action.includesAnyWords(dewordedWords)) {
          DW(, "was about to process action of id ",id,", but at least one of the words used in the action is in the deworded set");
          continue;
        }

        input.clear();
        action.getInput(input);
        DW(, "processing action **",input.c_str(),"** (id ",id,")");
        output.clear();

        doRestoreAction(*parentState);
        doAction(vm, input, output, u8("VM died while doing action"));
        HashWrapper<Signature> signature(createSignature(vm, ignoredByteRangeset));

        auto dewordingWord = action.getDewordingTarget();
        if (dewordingWord != ActionSet::NON_ID) {
          DW(, "this action is a dewording one (for word of id ",dewordingWord,")");
          if (deworder(vm, output)) {
            DW(, "word of id ",dewordingWord," is missing!");
            dewordedWords.setBit(dewordingWord);
          }
        }

        if (signature == parentNode->getSignature()) {
          DW(, "the resultant VM state is the same as the parent's, so skipping");
          continue;
        }

        unique_ptr<Node::Listener> nodeListener = listener->createNodeListener();
        listener->nodeReached(*this, nodeListener.get(), id, output, signature.get(), vm);

        DW(, "output from the action is **", output.c_str(), "**");
        try {
          // XXXX better response from doSaveAction on 'can't save'?
          doSaveAction(postState);
        } catch (...) {
          postState.clear();
        }
        {
          DW(, "adding the result to queue");
          unique_lock<mutex> l(resultQueueLock);
          resultQueue.emplace_back(parentNode, id, output, move(signature), postState, move(nodeListener));
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
    auto rs = move(resultQueue.front());
    resultQueue.pop_front();
    l.unlock();

    Node *parentNode = get<0>(rs);
    if (prevParentNode && parentNode != prevParentNode) {
      prevParentNode->clearState();
      prevParentNode->childrenUpdated();
      listener->nodeProcessed(*this, prevParentNode);
    }
    prevParentNode = parentNode;
    if (!parentNode) {
      DW(, "M it's the end of the queue!");
      break;
    }
    DW(, "M the result is a child for the node with sig of hash ", parentNode->getSignature().hashFast());
    ActionSet::Size parentActionId = get<1>(rs);
    u8string &resultOutput = get<2>(rs);
    HashWrapper<Signature> &resultSignature = get<3>(rs);
    State &resultState = get<4>(rs);
    unique_ptr<Node::Listener> &resultListener = get<5>(rs);
    DW(, "M the child is for the action of id ",parentActionId,"; the sig is of hash ", resultSignature.hashFast());
    DA(!(resultSignature == parentNode->getSignature()));

    Node *resultNode;
    auto v = find(nodes, cref(resultSignature));
    if (!v) {
      DW(, "M this is a new node for this multiverse!");
      unique_ptr<Node> n(new Node(move(resultListener), Node::UNPARENTED, move(resultSignature), move(resultState)));
      resultNode = n.get();
      nodes.emplace(ref(resultNode->getSignature()), resultNode);
      n.release();
    } else {
      DW(, "M there already exists a node of this signature");
      resultNode = *v;
      DA(resultSignature == resultNode->getSignature());
    }
    parentNode->addChild(parentActionId, resultOutput, resultNode, *this);
  }

  dispatcherFuture.get();

  listener->nodesProcessed(*this);
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

template<typename _I> void Multiverse::collapseNodes (_I nodesBegin, _I nodesEnd) {
  DS();
  DPRE(nodesBegin != nodesEnd);
  DW(, "Collapsing nodes:");

  // Build the set of extra ignored bytes (those which aren't the same across all
  // of the given nodes).

  Node *firstNode = *(nodesBegin++);
  DW(, "first node has sig of hash", firstNode->getSignature().hashFast());
  const Signature &firstSignature = firstNode->getSignature().get();
  vector<Signature::Iterator> otherSignatureIs;
  for (; nodesBegin != nodesEnd; ++nodesBegin) {
    Node *node = *nodesBegin;
    DW(, "other node has sig of hash", node->getSignature().hashFast());
    otherSignatureIs.emplace_back(node->getSignature().get().begin());
  }
  Bitset extraIgnoredBytes = createExtraIgnoredBytes(firstSignature, otherSignatureIs.begin(), otherSignatureIs.end(), vm);

  // Walk through the node tree, processing each node once (unless all its parents
  // are collapsed away, in which case it'll never be reached), and rebuild the
  // node map with only those nodes which have survived (and with their new
  // signatures).

  Rangeset extraIgnoredByteRangeset(extraIgnoredBytes, vm.getDynamicMemorySize());
  decltype(nodes) survivingNodes(nodes.bucket_count());
  unordered_map<Node *, Node *> nodeCollapseTargets(nodes.bucket_count());
  unordered_map<Node *, HashWrapper<Signature>> survivingNodePrevSignatures(nodes.bucket_count());
  // XXXX should have an obj per surviving node to hold this + the child undo data i.e. 'surviving node undo object'

  DI(Node *t =)
  collapseNode(rootNode, extraIgnoredByteRangeset, survivingNodes, nodeCollapseTargets, survivingNodePrevSignatures);
  DA(t == rootNode);
  DW(, "after collapsing all ",nodes.size()," nodes, there are ",survivingNodes.size()," surviving nodes");

  ignoredBytes |= move(extraIgnoredBytes);
  ignoredByteRangeset = Rangeset(ignoredBytes, vm.getDynamicMemorySize());
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

  Node::rebuildPrimeParents(*this);

  listener->nodesCollapsed(*this);
}

template<typename _Walker> void Multiverse::derefAndProcessNodeListener (Node::Listener *&nodeListener, _Walker &w) {
  w.derefAndProcess(nodeListener, [&] (Node::Listener *nodeListener) -> tuple<SerialiserBase::SubtypeId, void *, size_t> {
    auto r = listener->deduceNodeListenerType(nodeListener);
    return tuple<SerialiserBase::SubtypeId, void *, size_t>(0, get<0>(r), get<1>(r));
  }, [&] (SerialiserBase::SubtypeId type) -> tuple<Node::Listener *, void *, size_t> {
    if (type != 0) {
      throw PlainException(u8("invalid node listener type found"));
    }
    return listener->constructNodeListener();
  }, [&] (Node::Listener *nodeListener, void *, SerialiserBase::SubtypeId, _Walker &w) {
    listener->walkNodeListener(nodeListener, w);
  });
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
