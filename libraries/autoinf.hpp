/** @file */
/* -----------------------------------------------------------------------------
   AutoInf Core Engine Library
   © Geoff Crossland 2006, 2013-2016
----------------------------------------------------------------------------- */
#ifndef AUTOINF_ALREADYINCLUDED
#define AUTOINF_ALREADYINCLUDED

#include <autofrotz.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <map>

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
extern DC();

// TODO proper checking (in debug mode) output iterator framework
class FileOutputIterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
  prv FILE *h;

  pub explicit FileOutputIterator (FILE *h);

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

  prv void writeIu (iu64 value);
  prv void writeIs (is64 value);
  prv void writeOctet (iu8f value);
  prv void writeOctets (const iu8f *begin, const iu8f *end);

  pub void process (bool &r_value);
  pub void process (iu8f &r_value);
  pub void process (is8f &r_value);
  pub void process (iu16f &r_value);
  pub void process (is16f &r_value);
  pub void process (iu32f &r_value);
  pub void process (is32f &r_value);
  pub void process (iu64f &r_value);
  pub void process (is64f &r_value);
  pub void process (core::string<iu8f> &r_value);
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
  prv iu8f readOctet ();
  prv void readOctets (iu8f *begin, size_t size);

  pub void process (bool &r_value);
  pub void process (iu8f &r_value);
  pub void process (is8f &r_value);
  pub void process (iu16f &r_value);
  pub void process (is16f &r_value);
  pub void process (iu32f &r_value);
  pub void process (is32f &r_value);
  pub void process (iu64f &r_value);
  pub void process (is64f &r_value);
  pub void process (core::string<iu8f> &r_value);
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

  pub Signature ();
  pub explicit Signature (size_t sizeHint);
  pub template<typename _InputIterator> explicit Signature (const Deserialiser<_InputIterator> &);
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub size_t getSizeHint () const noexcept;
  friend size_t hashSlow (const Signature &o) noexcept;
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

    friend class ActionSet;
  };

  pub class ActionTemplate {
    prv std::vector<core::u8string> segments;
    prv std::vector<ActionWord::CategorySet> words;

    pub template<typename ..._Ts> ActionTemplate (_Ts &&...ts);
    prv void init (core::u8string &&segment);
    prv template<typename ..._Ts> void init (core::u8string &&segment, ActionWord::CategorySet word, _Ts &&...ts);

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

    pub Index getWordsSize () const;
    pub const core::u8string &getWord (Index i) const;
    pub ActionId getSize () const;
    pub Action get (ActionId id) const;

    friend class Action;
    pub class Action {
      prv const ActionSet &actionSet;
      prv ActionId id;
      prv const std::vector<core::u8string> &segments;
      prv const Index *specWordsBegin;

      prv Action (const ActionSet &actionSet, ActionId id, const Index *specI);

      pub ActionId getDewordingTarget () const;
      pub size_t getWordCount () const;
      pub Index getWord (size_t i) const;
      pub void getInput (core::u8string &r_out) const;
      pub bool includesAnyWords (const bitset::Bitset &words) const;

      friend class ActionSet;
    };
  };

  prv struct RangesetPart {
    iu16f setSize;
    iu16f clearSize;
  };

  prv class Rangeset : public std::vector<RangesetPart> {
    pub Rangeset (const bitset::Bitset &bitset, iu16 size);
  };

  pub class Node {
    pub class Listener;

    pub static Node *const UNPARENTED;

    prv std::unique_ptr<Listener> listener;
    prv Node *primeParentNode;
    prv bool primeParentNodeInvalid;
    prv core::HashWrapper<Signature> signature;
    prv std::unique_ptr<autofrotz::State> state;
    prv std::vector<std::tuple<ActionId, core::u8string, Node *>> children;

    pub Node (std::unique_ptr<Listener> &&listener, Node *primeParentNode, core::HashWrapper<Signature> &&signature, autofrotz::State &&state);
    Node (const Node &) = delete;
    Node &operator= (const Node &) = delete;
    Node (Node &&) = delete;
    Node &operator= (Node &&) = delete;
    pub template<typename _InputIterator> explicit Node (const Deserialiser<_InputIterator> &);
    pub template<typename _Walker> void beWalked (_Walker &w);

    pub Listener *getListener () const;
    pub Node *getPrimeParentNode () const;
    pub size_t getPrimeParentArcChildIndex () const;
    pub const core::HashWrapper<Signature> &getSignature () const;
    pub core::HashWrapper<Signature> setSignature (core::HashWrapper<Signature> &&signature);
    pub const autofrotz::State *getState () const;
    pub void clearState ();
    pub size_t getChildrenSize () const;
    pub const std::tuple<ActionId, core::u8string, Node *> &getChild (size_t i) const;
    pub size_t getChildIndex (ActionId id) const;
    pub void addChild (ActionId actionId, core::u8string &&output, Node *node, const Multiverse &multiverse);
    prv bool updatePrimeParent (Node *newParentNode, bool changedAbove);
    pub void removeChild (size_t i);
    pub void changeChild (size_t i, Node *node);
    pub void childrenUpdated (const Multiverse &multiverse);
    pub void invalidatePrimeParent ();
    pub static void rebuildPrimeParents (Multiverse &multiverse);

    pub template<typename _F, iff(std::is_convertible<_F, std::function<bool (Node *)>>::value)> void forEach (const _F &f);

    pub class Listener {
      prt Listener ();
      Listener (const Listener &) = delete;
      Listener &operator= (const Listener &) = delete;
      Listener (Listener &&) = delete;
      Listener &operator= (Listener &&) = delete;
      pub virtual ~Listener ();
    };
  };

  pub class NodeIterator {
    prv std::unordered_map<std::reference_wrapper<const core::HashWrapper<Signature>>, Node *>::const_iterator i;

    pub NodeIterator ();
    prv NodeIterator (decltype(i) &&i);

    pub Node *const &operator* () const;
    pub NodeIterator &operator++ ();
    pub NodeIterator operator++ (int);
    friend bool operator== (const NodeIterator &l, const NodeIterator &r);
    friend bool operator!= (const NodeIterator &l, const NodeIterator &r);

    friend class Multiverse;
  };

  pub class Listener;

  prv const std::function<bool (autofrotz::Vm &r_vm)> saver;
  prv const std::function<bool (autofrotz::Vm &r_vm)> restorer;
  prv const ActionSet actionSet;
  prv const std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> deworder;
  prv std::unique_ptr<Listener> listener;
  prv bitset::Bitset ignoredBytes;
  prv Rangeset ignoredByteRangeset;
  prv Node *rootNode;
  prv std::unordered_map<std::reference_wrapper<const core::HashWrapper<Signature>>, Node *> nodes; // XXXX make Node * unique_ptr?

  pub Multiverse (
    autofrotz::Vm &r_vm, const core::u8string &initialInput, core::u8string &r_initialOutput,
    std::function<bool (autofrotz::Vm &r_vm)> &&saver, std::function<bool (autofrotz::Vm &r_vm)> &&restorer,
    const std::vector<std::vector<core::u8string>> &equivalentActionInputsSet,
    std::vector<ActionWord> &&words, std::vector<ActionTemplate> &&dewordingTemplates, std::vector<ActionTemplate> &&otherTemplates,
    std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> &&deworder, std::unique_ptr<Listener> &&listener
  );
  prv static bitset::Bitset initIgnoredBytes (const autofrotz::Vm &vm);
  Multiverse (const Multiverse &) = delete;
  Multiverse &operator= (const Multiverse &) = delete;
  Multiverse (Multiverse &&) = delete;
  Multiverse &operator= (Multiverse &&) = delete;
  pub ~Multiverse () noexcept;

  pub const ActionSet &getActionSet () const;
  pub Listener *getListener () const;
  pub static void doAction (autofrotz::Vm &r_vm, core::u8string::const_iterator inputBegin, core::u8string::const_iterator inputEnd, core::u8string &r_output, const char8_t *deathExceptionMsg);
  pub static void doAction (autofrotz::Vm &r_vm, const core::u8string &input, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv void doSaveAction (autofrotz::Vm &r_vm, autofrotz::State &r_state);
  prv void doRestoreAction (autofrotz::Vm &r_vm, const autofrotz::State &state);
  prv static Signature createSignature (const autofrotz::Vm &vm, const Rangeset &ignoredByteRangeset);
  prv static Signature recreateSignature (const Signature &oldSignature, const Rangeset &extraIgnoredByteRangeset);
  pub size_t size () const;
  pub NodeIterator begin () const;
  pub NodeIterator end () const;
  pub Node *getRoot () const;
  pub template<typename _I> void processNodes (_I nodesBegin, _I nodesEnd, autofrotz::Vm &r_vm);
  prv template<typename _I> bitset::Bitset createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, const autofrotz::Vm &vm);
  pub template<typename _I> void collapseNodes (_I nodesBegin, _I nodesEnd, const autofrotz::Vm &vm);
  prv Node *collapseNode (
    Node *node, const Rangeset &extraIgnoredByteRangeset,
    std::unordered_map<std::reference_wrapper<const core::HashWrapper<Signature>>, Node *> &r_survivingNodes,
    std::unordered_map<Node *, Node *> &r_nodeCollapseTargets,
    std::unordered_map<Node *, core::HashWrapper<Signature>> &r_survivingNodePrevSignatures
  );
  pub void save (const char *pathName, const autofrotz::Vm &vm);
  pub void load (const char *pathName, autofrotz::Vm &r_vm);
  prv template<typename _Walker> void derefAndProcessNodeListener (Node::Listener *&listener, _Walker &w);

  pub class Listener {
    prt Listener ();
    Listener (const Listener &) = delete;
    Listener &operator= (const Listener &) = delete;
    Listener (Listener &&) = delete;
    Listener &operator= (Listener &&) = delete;
    pub virtual ~Listener ();

    pub virtual std::tuple<void *, size_t> deduceNodeListenerType (Node::Listener *listener) = 0;
    pub virtual std::tuple<Node::Listener *, void *, size_t> constructNodeListener () = 0;
    pub virtual void walkNodeListener (Node::Listener *listener, Serialiser<FileOutputIterator> &s) = 0;
    pub virtual void walkNodeListener (Node::Listener *listener, Deserialiser<FileInputIterator> &s) = 0;

    pub virtual std::unique_ptr<Node::Listener> createNodeListener () = 0;
    pub virtual void nodeReached (const Multiverse &multiverse, Node::Listener *listener, ActionId parentActionId, const core::u8string &output, const Signature &signature, const autofrotz::Vm &vm) = 0;
    pub virtual void subtreePrimeAncestorsUpdated (const Multiverse &multiverse, const Node *node) = 0;
    pub virtual void nodeChildrenUpdated (const Multiverse &multiverse, const Node *node) = 0;
    pub virtual void nodesProcessed (const Multiverse &multiverse) = 0;
    pub virtual void nodesCollapsed (const Multiverse &multiverse) = 0;
    pub virtual void loaded (const Multiverse &multiverse) = 0;
  };
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}

#include "autoinf.ipp"
#endif
