/** @file */
/* -----------------------------------------------------------------------------
   AutoInf Core Engine Library
   © Geoff Crossland 2006, 2013, 2014
----------------------------------------------------------------------------- */
#ifndef AUTOINF_ALREADYINCLUDED
#define AUTOINF_ALREADYINCLUDED

#include <autofrotz.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <map>

namespace autoinf {

extern const core::Version VERSION;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
extern DC();

// XXXX move to core (along with core::hash())
static size_t hashImpl (const iu8f *i, const iu8f *end) {
  size_t r = 0;
  for (; i != end; ++i) {
    r = 31 * r + *i;
  }
  return r;
}

template<typename _T> class Hasher {
  pub size_t operator() (const _T &s) const noexcept {
    return s.hash();
  }
};



// TODO proper checking (in debug mode) output iterator framework
class FileOutputIterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
  prv FILE *h;

  pub explicit FileOutputIterator (FILE *h);
  pub FileOutputIterator (const FileOutputIterator &) = default;
  pub FileOutputIterator &operator= (const FileOutputIterator &) = default;
  pub FileOutputIterator (FileOutputIterator &&o);
  pub FileOutputIterator &operator= (FileOutputIterator &&o);

  pub FileOutputIterator &operator= (iu8f v);
  pub FileOutputIterator &operator* ();
  pub FileOutputIterator &operator++ ();
  pub FileOutputIterator &operator++ (int);
};

class FileInputIterator : public std::iterator<std::input_iterator_tag, iu8f, void> {
  prv FILE *h;
  prv iu8f v;

  pub explicit FileInputIterator (FILE *h);
  pub FileInputIterator ();
  prv FileInputIterator (iu8f v);
  pub FileInputIterator (const FileInputIterator &) = default;
  pub FileInputIterator &operator= (const FileInputIterator &) = default;
  pub FileInputIterator (FileInputIterator &&o);
  pub FileInputIterator &operator= (FileInputIterator &&o);

  prv void advance ();
  pub const iu8f &operator* () const noexcept;
  pub FileInputIterator &operator++ ();
  pub FileInputIterator operator++ (int);
  friend bool operator== (const FileInputIterator &l, const FileInputIterator &r) noexcept;
  friend bool operator!= (const FileInputIterator &l, const FileInputIterator &r) noexcept;
};

class SerialiserBase {
  prt typedef size_t id;
  prt static constexpr id NON_ID = 0;
  prt static constexpr id NULL_ID = 1;
  pub typedef size_t SubtypeId;
};

template<typename _OutputIterator> class Serialiser : public SerialiserBase {
  prv _OutputIterator i;
  prv std::map<void *, std::tuple<id, void *>> allocations;
  prv id nextId;

  pub explicit Serialiser (_OutputIterator &&i);
  Serialiser (const Serialiser &) = delete;
  Serialiser &operator= (const Serialiser &) = delete;
  pub Serialiser (Serialiser &&) = default;
  pub Serialiser &operator= (Serialiser &&) = default;

  pub constexpr bool isSerialising () const;

  prv void write (iu32 value);
  prv void write (is32 value);
  prv void write (char8_t value);
  prv void write (const char8_t *begin, const char8_t *end);

  pub void process (iu16f &r_value);
  pub void process (is16f &r_value);
  pub void process (iu32f &r_value);
  pub void process (is32f &r_value);
  pub void process (core::u8string &r_value);
  // TODO do we really want custom walking code for each vector? surely we should just call process() for each member
  pub template<typename _T, typename _WalkingFunctor, iff(
    std::is_convertible<_WalkingFunctor, std::function<void (_T &, Serialiser<_OutputIterator> &)>>::value
  )> void process (std::vector<_T> &r_value, const _WalkingFunctor &walkElement);
  pub void process (bitset::Bitset &r_value);
  pub void process (autofrotz::State &r_value);
  pub template<typename _Walkable> void process (_Walkable &r_value);
  prv typedef decltype(allocations) decltype_allocations;
  prv typename decltype_allocations::value_type *findAllocationStart (void *ptr);
  // (serialising a ptr to a nonarray object allocation (or such a ptr cast to a superclass), which may or may not have been seen before)
  pub template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
    std::is_convertible<_TypeDeductionFunctor, std::function<std::tuple<SubtypeId, void *, size_t> (_T *)>>::value &&
    std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SubtypeId, Serialiser<_OutputIterator> &)>>::value
  )> void derefAndProcess (_T *&o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &, const _WalkingFunctor &walkReferent);
  pub template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
    std::is_convertible<_TypeDeductionFunctor, std::function<std::tuple<SubtypeId, void *, size_t> (_T *)>>::value &&
    std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SubtypeId, Serialiser<_OutputIterator> &)>>::value
  )> void derefAndProcess (std::unique_ptr<_T> &o, const _TypeDeductionFunctor &deduceReferentType, const _ConstructionFunctor &, const _WalkingFunctor &walkReferent);
  // (a variant for when _T isn't polymorphic)
  pub template<typename _T> void derefAndProcess (_T *&o);
  pub template<typename _T> void derefAndProcess (std::unique_ptr<_T> &o);
  // (serialising a ptr to an array object allocation, which may or may not have been seen before)
  pub template<typename _T> void derefAndProcess (_T *&o, size_t count);
  pub template<typename _T> void derefAndProcess (std::unique_ptr<_T []> &o, size_t count);
  // (ptr into a nonarray or array object allocation, which must already have been seen)
  pub template<typename _T, typename _P> void process (_T *&o, _P *parent);
  // (ditto, but with parent = o)
  pub template<typename _T> void process (_T *&o);
  pub template<typename _T> void process (std::unique_ptr<_T> &o);
};

template<typename _InputIterator> class Deserialiser : public SerialiserBase {
  prv _InputIterator i;
  prv _InputIterator end;
  prv std::vector<void *> allocations;

  pub Deserialiser (_InputIterator &&i, _InputIterator &&end);
  Deserialiser (const Deserialiser &) = delete;
  Deserialiser &operator= (const Deserialiser &) = delete;
  pub Deserialiser (Deserialiser &&) = default;
  pub Deserialiser &operator= (Deserialiser &&) = default;

  pub constexpr bool isSerialising () const;

  prv template<typename _i> _i readIu ();
  prv template<typename _i> _i readIs ();
  prv char8_t readChar8 ();
  prv void readChar8s (char8_t *begin, size_t size);

  pub void process (iu16f &r_value);
  pub void process (is16f &r_value);
  pub void process (iu32f &r_value);
  pub void process (is32f &r_value);
  pub void process (core::u8string &r_value);
  pub template<typename _T, typename _WalkingFunctor, iff(
    std::is_convertible<_WalkingFunctor, std::function<void (_T &, Deserialiser<_InputIterator> &)>>::value
  )> void process (std::vector<_T> &r_value, const _WalkingFunctor &walkElement);
  prv template<typename _T, iff(std::is_constructible<_T, Deserialiser<_InputIterator>>::value)> void emplaceBack (std::vector<_T> &r_value);
  prv template<typename _T, iff(!std::is_constructible<_T, Deserialiser<_InputIterator>>::value)> void emplaceBack (std::vector<_T> &r_value);
  pub void process (bitset::Bitset &r_value);
  pub void process (autofrotz::State &r_value);
  pub template<typename _Walkable> void process (_Walkable &r_value);
  prv id readAllocationId ();
  pub template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
    std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SubtypeId)>>::value &&
    std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SubtypeId, Deserialiser<_InputIterator> &)>>::value
  )> void derefAndProcess (_T *&o, const _TypeDeductionFunctor &, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent);
  pub template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
    std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SubtypeId)>>::value &&
    std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SubtypeId, Deserialiser<_InputIterator> &)>>::value
  )> void derefAndProcess (std::unique_ptr<_T> &o, const _TypeDeductionFunctor &, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent);
  pub template<typename _T> void derefAndProcess (_T *&o);
  prv template<typename _T, iff(std::is_constructible<_T, Deserialiser<_InputIterator>>::value)> _T *construct ();
  prv template<typename _T, iff(!std::is_constructible<_T, Deserialiser<_InputIterator>>::value)> _T *construct ();
  pub template<typename _T> void derefAndProcess (std::unique_ptr<_T> &o);
  pub template<typename _T> void derefAndProcess (_T *&o, size_t count);
  pub template<typename _T> void derefAndProcess (std::unique_ptr<_T []> &o, size_t count);
  pub template<typename _T, typename _P> void process (_T *&o, _P *parent);
  pub template<typename _T> void process (_T *&o);
  pub template<typename _T> void process (std::unique_ptr<_T> &o);
};

// XXXX move out?
class Signature {
  pub class Iterator;

  prv static const iu8f ESCAPE = 200;
  prv core::string<iu8f> b;
  prv size_t h;

  pub Signature ();
  pub explicit Signature (size_t sizeHint);
  pub Signature (const Signature &) = default;
  pub Signature &operator= (const Signature &) = default;
  pub Signature (Signature &&) = default;
  pub Signature &operator= (Signature &&) = default;
  pub template<typename _InputIterator> explicit Signature (const Deserialiser<_InputIterator> &);
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub size_t hash () const noexcept;
  pub size_t getSizeHint () const noexcept;
  friend bool operator== (const Signature &l, const Signature &r) noexcept;
  pub Iterator begin () const;
  pub Iterator end () const;

  friend class Writer;
  pub class Writer {
    prv Signature &signature;
    prv iu32f zeroByteCount;

    pub Writer (Signature &signature) noexcept;
    Writer (const Writer &) = delete;
    Writer &operator= (const Writer &) = delete;
    pub Writer (Writer &&) = default;
    pub Writer &operator= (Writer &&) = default;

    prv void flushZeroBytes ();
    pub void appendByte (iu8 byte);
    pub void appendZeroBytes (iu byteCount);
    pub void close ();
  };

  // XXXX non-mutable ForwardIterator
  friend class Iterator;
  pub class Iterator {
    prv const iu8f *i;
    prv const iu8f *end;
    prv iu8f currentByte;
    prv decltype(Writer::zeroByteCount) zeroByteCount;

    pub Iterator () noexcept;
    pub Iterator (const Signature &signature) noexcept;
    pub Iterator (const Iterator &) = default;
    pub Iterator &operator= (const Iterator &) = default;
    pub Iterator (Iterator &&) = default;
    pub Iterator &operator= (Iterator &&) = default;

    prv void advance () noexcept;
    prv void inc () noexcept;
    pub const iu8f &operator* () const noexcept;
    pub Iterator &operator++ () noexcept;
    pub Iterator operator++ (int) noexcept;
    friend bool operator== (const Iterator &l, const Iterator &r) noexcept;
    friend bool operator!= (const Iterator &l, const Iterator &r) noexcept;
    pub Iterator &operator+= (size_t r) noexcept;
    friend Iterator operator+ (Iterator l, size_t r) noexcept;
    pub void copy (Writer &r_writer, size_t r);

    friend class Writer;
  };
};

class Multiverse {
  pub typedef iu16f ActionId;
  pub static constexpr ActionId NON_ID = static_cast<ActionId>(-1);

  pub class ActionSet;

  pub class ActionWord {
    pub typedef iu64 CategorySet;

    prv core::u8string word;
    prv CategorySet categories;

    pub ActionWord (core::u8string &&word, CategorySet categories);
    pub ActionWord (const ActionWord &) = default;
    pub ActionWord &operator= (const ActionWord &) = default;
    pub ActionWord (ActionWord &&) = default;
    pub ActionWord &operator= (ActionWord &&) = default;

    friend class ActionSet;
  };

  pub class ActionTemplate {
    prv std::vector<core::u8string> segments;
    prv std::vector<ActionWord::CategorySet> words;

    pub template<typename ..._Ts> ActionTemplate (_Ts &&...ts);
    prv void init (core::u8string &&segment);
    prv template<typename ..._Ts> void init (core::u8string &&segment, ActionWord::CategorySet word, _Ts &&...ts);
    pub ActionTemplate (const ActionTemplate &) = default;
    pub ActionTemplate &operator= (const ActionTemplate &) = default;
    pub ActionTemplate (ActionTemplate &&) = default;
    pub ActionTemplate &operator= (ActionTemplate &&) = default;

    friend class ActionSet;
  };

  pub class ActionSet {
    pub typedef iu8f Index;
    pub class Action;

    prv std::vector<core::u8string> words;
    prv std::vector<std::vector<core::u8string>> templates;
    prv Index dewordingTemplateCount;
    prv core::string<Index> specs;
    prv std::vector<size_t> specBegins;

    pub ActionSet (std::vector<ActionWord> &&words, std::vector<ActionTemplate> &&dewordingTemplates, std::vector<ActionTemplate> &&otherTemplates);
    prv static void init (
      const std::vector<ActionWord> &words, Index nextTemplateI, const std::vector<ActionTemplate> &templates,
      core::string<Index> &r_specs, std::vector<size_t> &r_specBegins
    );
    prv static void initImpl (
      const std::vector<ActionWord> &words, const ActionTemplate &templ, Index templateWordI,
      core::string<Index> &r_specs, std::vector<size_t> &r_specBegins, core::string<Index> &r_spec
    );
    pub ActionSet (const ActionSet &) = default;
    pub ActionSet &operator= (const ActionSet &) = default;
    pub ActionSet (ActionSet &&) = default;
    pub ActionSet &operator= (ActionSet &&) = default;

    pub Index getWordsSize () const;
    pub const core::u8string &getWord (Index i) const;
    pub ActionId getSize () const;
    pub Action get (ActionId id) const;

    friend class Action;
    pub class Action {
      prv Action (const ActionSet &actionSet, ActionId id, const Index *specI);
      pub Action (const Action &) = default;
      pub Action &operator= (const Action &) = default;
      pub Action (Action &&) = default;
      pub Action &operator= (Action &&) = default;

      prv const ActionSet &actionSet;
      prv ActionId id;
      prv const std::vector<core::u8string> &segments;
      prv const Index *specWordsBegin;

      pub ActionId getDewordingTarget () const;
      pub size_t getWordCount () const;
      pub Index getWord (size_t i) const;
      pub void getInput (core::u8string &r_out) const;
      pub bool includesAnyWords (const bitset::Bitset &words) const;

      friend class ActionSet;
    };
  };

  pub class Node;

  pub class Metric {
    pub class State {
      prt State ();
      pub virtual ~State ();

      pub virtual size_t getValue (size_t i) = 0;
    };

    prt Metric ();
    pub virtual ~Metric ();

    pub virtual std::tuple<void *, size_t> deduceStateType (State *state) = 0;
    pub virtual std::tuple<State *, void *, size_t> constructState () = 0;
    pub virtual void walkState (State *state, Serialiser<FileOutputIterator> &s) = 0;
    pub virtual void walkState (State *state, Deserialiser<FileInputIterator> &s) = 0;

    pub virtual std::unique_ptr<State> nodeCreated (const Multiverse &multiverse, ActionId parentActionId, const core::u8string &output, const Signature &signature, const autofrotz::Vm &vm) = 0;
    pub virtual void nodeProcessed (const Multiverse &multiverse, Node &r_node) = 0;
    pub virtual void nodesProcessed (const Multiverse &multiverse, Node &r_rootNode, std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> &r_nodes) = 0;
    pub virtual void nodesCollapsed (const Multiverse &multiverse, Node &r_rootNode, std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> &r_nodes) = 0;

    pub virtual size_t getValueCount () const = 0;
  };

  prv struct RangesetPart {
    iu16f setSize;
    iu16f clearSize;
  };

  prv class Rangeset : public std::vector<RangesetPart> {
    pub Rangeset (const bitset::Bitset &bitset, iu16 size);
    pub Rangeset (const Rangeset &) = default;
    pub Rangeset &operator= (const Rangeset &) = default;
    pub Rangeset (Rangeset &&) = default;
    pub Rangeset &operator= (Rangeset &&) = default;
  };

  pub class Node {
    prv Signature signature;
    prv std::unique_ptr<autofrotz::State> state;
    prv std::unique_ptr<Metric::State> metricState;
    prv std::vector<std::tuple<ActionId, core::u8string, Node *>> children;

    pub Node (Signature &&signature, autofrotz::State &&state, std::unique_ptr<Metric::State> &&metricState);
    Node (const Node &) = delete;
    Node &operator= (const Node &) = delete;
    Node (Node &&) = delete;
    Node &operator= (Node &&) = delete;
    pub template<typename _InputIterator> explicit Node (const Deserialiser<_InputIterator> &);
    pub template<typename _Walker> void beWalked (_Walker &w);

    pub const Signature &getSignature () const;
    pub Signature setSignature (Signature &&signature);
    pub const autofrotz::State *getState () const;
    pub void clearState ();
    pub Metric::State *getMetricState () const;
    pub void addChild (ActionId actionId, core::u8string &&output, Node *node);
    pub void batchOfChildChangesCompleted ();
    pub size_t getChildrenSize () const;
    pub const std::tuple<ActionId, core::u8string, Node *> &getChild (size_t i) const;
    pub const std::tuple<ActionId, core::u8string, Node *> *getChildByActionId (ActionId id) const;
    // XXXX get index by action id?
    pub void removeChild (size_t i);
    pub void changeChild (size_t i, Node *node);

    pub template<typename _F, iff(std::is_convertible<_F, std::function<bool (Node *)>>::value)> void forEach (const _F &f);
  };

  prv const std::function<bool (autofrotz::Vm &r_vm)> saver;
  prv const std::function<bool (autofrotz::Vm &r_vm)> restorer;
  prv const ActionSet actionSet;
  prv const std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> deworder;
  prv const std::unique_ptr<Metric> metric;
  prv bitset::Bitset ignoredBytes;
  prv Rangeset ignoredByteRangeset;
  prv Node *rootNode;
  prv std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> nodes; // XXXX make Node * unique_ptr?

  pub Multiverse (
    autofrotz::Vm &r_vm, const core::u8string &initialInput, core::u8string &r_initialOutput,
    std::function<bool (autofrotz::Vm &r_vm)> &&saver, std::function<bool (autofrotz::Vm &r_vm)> &&restorer,
    const std::vector<std::vector<core::u8string>> &equivalentActionInputsSet,
    std::vector<ActionWord> &&words, std::vector<ActionTemplate> &&dewordingTemplates, std::vector<ActionTemplate> &&otherTemplates,
    std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> &&deworder, std::unique_ptr<Metric> &&metric
  );
  prv static bitset::Bitset initIgnoredBytes (const autofrotz::Vm &vm);
  Multiverse (const Multiverse &) = delete;
  Multiverse &operator= (const Multiverse &) = delete;
  Multiverse (Multiverse &&) = delete;
  Multiverse &operator= (Multiverse &&) = delete;
  pub ~Multiverse () noexcept;

  pub const ActionSet &getActionSet () const;
  pub const Metric *getMetric () const;
  pub static void doAction (autofrotz::Vm &r_vm, core::u8string::const_iterator inputBegin, core::u8string::const_iterator inputEnd, core::u8string &r_output, const char8_t *deathExceptionMsg);
  pub static void doAction (autofrotz::Vm &r_vm, const core::u8string &input, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv void doSaveAction (autofrotz::Vm &r_vm, autofrotz::State &r_state);
  prv void doRestoreAction (autofrotz::Vm &r_vm, const autofrotz::State &state);
  prv static Signature createSignature (const autofrotz::Vm &vm, const Rangeset &ignoredByteRangeset);
  prv static Signature recreateSignature (const Signature &oldSignature, const Rangeset &extraIgnoredByteRangeset);
  pub Node *getRootNode () const;
  pub template<typename _I> void processNodes (_I nodesBegin, _I nodesEnd, autofrotz::Vm &r_vm);
  prv template<typename _I> bitset::Bitset createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, const autofrotz::Vm &vm);
  pub template<typename _I> void collapseNodes (_I nodesBegin, _I nodesEnd, const autofrotz::Vm &vm);
  prv static Node *collapseNode (
    Node *node, const Rangeset &extraIgnoredByteRangeset,
    std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> &r_survivingNodes,
    std::unordered_map<Node *, Node *> &r_nodeCollapseTargets,
    std::unordered_map<Node *, Signature> &r_survivingNodePrevSignatures
  );
  pub void save (const char *pathName);
  pub void load (const char *pathName, const autofrotz::Vm &vm);
  prv template<typename _Walker> void derefAndProcessMetricState (Metric::State *&state, _Walker &w);
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}

#include "autoinf.ipp"
#endif
