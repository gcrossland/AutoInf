/** @file */
/* -----------------------------------------------------------------------------
   AutoInf Core Engine Library
   © Geoff Crossland 2006-2022
----------------------------------------------------------------------------- */
#ifndef AUTOINF_ALREADYINCLUDED
#define AUTOINF_ALREADYINCLUDED

#include <autofrotz.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <map>
#include <unordered_set>
#include <iterators.hpp>
#include <io_file.hpp>
#include <io_socket.hpp>

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
extern DC();
extern DC(c);

typedef iterators::InputStreamIterator<io::file::FileStream> FileInputIterator;
typedef iterators::InputStreamEndIterator<io::file::FileStream> FileInputEndIterator;
typedef iterators::OutputStreamIterator<io::file::FileStream> FileOutputIterator;

class SerialiserBase {
  prt typedef size_t id;
  prt static constexpr id nonId = 0;
  prt static constexpr id nullId = 1;
  pub typedef size_t SubtypeId;
};

template<typename _OutputIterator> class Serialiser : public SerialiserBase {
  prv _OutputIterator &i;
  prv std::map<void *, std::tuple<id, void *>> allocations;
  prv id nextId;

  pub explicit Serialiser (_OutputIterator &r_i);
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
  pub template<typename _T, typename _WalkingFunctor, iff(
    std::is_convertible<_WalkingFunctor, std::function<void (_T &, Serialiser<_OutputIterator> &)>>::value
  )> void process (std::vector<_T> &r_value, const _WalkingFunctor &walkElement);
  pub template<typename _T> void process (std::vector<_T> &r_value);
  pub void process (bitset::Bitset &r_value);
  pub void process (autofrotz::State &r_value);
  pub template<typename _Walkable> void process (core::HashWrapper<_Walkable> &r_value);
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

template<typename _InputIterator, typename _InputEndIterator> class Deserialiser : public SerialiserBase {
  prv _InputIterator &i;
  prv const _InputEndIterator &end;
  prv std::vector<void *> allocations;

  pub Deserialiser (_InputIterator &r_i, const _InputEndIterator &end);
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
    std::is_convertible<_WalkingFunctor, std::function<void (_T &, Deserialiser<_InputIterator, _InputEndIterator> &)>>::value
  )> void process (std::vector<_T> &r_value, const _WalkingFunctor &walkElement);
  pub template<typename _T> void process (std::vector<_T> &r_value);
  prv template<typename _T, iff(std::is_constructible<_T, Deserialiser<_InputIterator, _InputEndIterator>>::value)> void emplaceBack (std::vector<_T> &r_value);
  prv template<typename _T, iff(!std::is_constructible<_T, Deserialiser<_InputIterator, _InputEndIterator>>::value)> void emplaceBack (std::vector<_T> &r_value);
  pub void process (bitset::Bitset &r_value);
  pub void process (autofrotz::State &r_value);
  pub template<typename _Walkable> void process (core::HashWrapper<_Walkable> &r_value);
  pub template<typename _Walkable> void process (_Walkable &r_value);
  prv id readAllocationId ();
  pub template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
    std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SubtypeId)>>::value &&
    std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SubtypeId, Deserialiser<_InputIterator, _InputEndIterator> &)>>::value
  )> void derefAndProcess (_T *&o, const _TypeDeductionFunctor &, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent);
  pub template<typename _T, typename _TypeDeductionFunctor, typename _ConstructionFunctor, typename _WalkingFunctor, iff(
    std::is_convertible<_ConstructionFunctor, std::function<std::tuple<_T *, void *, size_t> (SubtypeId)>>::value &&
    std::is_convertible<_WalkingFunctor, std::function<void (_T *, void *, SubtypeId, Deserialiser<_InputIterator, _InputEndIterator> &)>>::value
  )> void derefAndProcess (std::unique_ptr<_T> &o, const _TypeDeductionFunctor &, const _ConstructionFunctor &constructReferent, const _WalkingFunctor &walkReferent);
  pub template<typename _T> void derefAndProcess (_T *&o);
  prv template<typename _T, iff(std::is_constructible<_T, Deserialiser<_InputIterator, _InputEndIterator>>::value)> _T *construct ();
  prv template<typename _T, iff(!std::is_constructible<_T, Deserialiser<_InputIterator, _InputEndIterator>>::value)> _T *construct ();
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

  prv static const iu8f escape = 200;
  prv typedef iu32f RunSize;

  prv core::string<iu8f> b;

  pub Signature ();
  pub explicit Signature (size_t sizeHint);
  pub template<typename _InputIterator, typename _InputEndIterator> explicit Signature (const Deserialiser<_InputIterator, _InputEndIterator> &);
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub bool empty () const noexcept;
  pub size_t getSizeHint () const noexcept;
  pub size_t hashSlow () const noexcept;
  pub bool operator== (const Signature &r) const noexcept;
  pub Iterator begin () const;
  pub Iterator end () const;

  friend class Writer;
  pub class Writer {
    prv Signature &signature;
    prv RunSize zeroByteCount;

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

  friend class Iterator;
  pub class Iterator : public std::iterator<std::forward_iterator_tag, iu8f> {
    prv const iu8f *i;
    prv const iu8f *end;
    prv iu8f currentByte;
    prv RunSize zeroByteCount;

    pub Iterator () noexcept;
    pub explicit Iterator (const Signature &signature) noexcept;

    prv void advance () noexcept;
    prv void inc () noexcept;
    pub const iu8f &operator* () const noexcept;
    pub Iterator &operator++ () noexcept;
    pub Iterator operator++ (int) noexcept;
    pub bool operator== (const Iterator &r) const noexcept;
    pub Iterator &operator+= (size_t r) noexcept;
    friend Iterator operator+ (Iterator l, size_t r) noexcept;
    pub void copy (Writer &r_writer, size_t r);

    friend class Writer;
  };
};

// XXXX move out?
template<typename _L, typename _Size = typename _L::size_type> class MultiList {
  pub class SubList {
    pub typedef typename _L::const_iterator Iterator;
    pub typedef typename _L::size_type Size;

    prv Iterator b;
    prv Iterator e;

    pub SubList (Iterator &&begin, Iterator &&end);

    pub Size size () const noexcept;
    pub Iterator begin () const noexcept;
    pub Iterator end () const noexcept;
  };

  pub class Iterator : public iterators::RevaluedIterator<Iterator, SubList, typename std::vector<typename SubList::Size>::const_iterator> {
    prv typename SubList::Iterator listBegin;

    pub Iterator ();
    pub Iterator (const MultiList<_L, _Size> &multiList);

    pub SubList operator_ind_ () const;
  };

  pub typedef Iterator const_iterator;
  pub typedef _Size size_type;

  prv _L list;
  prv std::vector<typename SubList::Size> bounds;

  pub MultiList ();
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub _Size size () const noexcept;
  pub SubList get (_Size i) const noexcept;
  pub Iterator begin () const;
  pub Iterator end () const;
  pub _L &subList () noexcept;
  pub _Size push ();
  pub void pop ();
  pub void reserve (_Size capacity);
  pub _L &compact ();
};

template<typename _c, typename _Size = iu> class StringSet {
  prv class Key {
    pub static constexpr _Size proposed = core::numeric_limits<_Size>::max();
    pub static thread_local MultiList<core::string<_c>, _Size> *list;
    pub static thread_local const _c *proposedBegin;
    pub static thread_local const _c *proposedEnd;

    pub _Size i;

    pub explicit Key (_Size i);

    pub size_t hashSlow () const noexcept;
    pub bool operator== (const Key &r) const noexcept;
  };

  prv MultiList<core::string<_c>, _Size> list;
  prv std::unordered_set<core::HashWrapper<Key>> set;

  pub StringSet ();
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub _Size size () const noexcept;
  pub _Size push (const _c *begin, const _c *end);
  pub typename MultiList<core::string<_c>, _Size>::SubList get (_Size i) const noexcept;
  pub typedef std::vector<_Size> String;
  pub void createString (const core::string<_c> &o, const core::string<_c> &terminator, String &r_out);
  pub void rebuildString (const String &o, core::string<_c> &r_out) const;
};

class ActionSet {
  pub typedef iu16f Size;
  pub static constexpr Size nonId = static_cast<Size>(-1);
  pub typedef iu8f SubSize;
  pub class Action;

  pub class Word {
    pub typedef iu64 CategorySet;

    prv core::u8string word;
    prv CategorySet categories;

    pub Word (core::u8string &&word, CategorySet categories);

    friend class ActionSet;
  };

  pub class Template {
    prv std::vector<core::u8string> segments;
    prv std::vector<Word::CategorySet> words;

    pub template<typename ..._Ts> Template (_Ts &&...ts);
    prv void init (core::u8string &&segment);
    prv template<typename ..._Ts> void init (core::u8string &&segment, Word::CategorySet word, _Ts &&...ts);

    friend class ActionSet;
  };

  prv MultiList<core::u8string, SubSize> words;
  prv MultiList<MultiList<core::u8string, iu16f>, SubSize> templates;
  prv Size dewordingActionCount;
  prv MultiList<core::string<SubSize>, Size> specs;

  pub ActionSet (const std::vector<Word> &words, const std::vector<Template> &dewordingTemplates, const std::vector<Template> &otherTemplates);
  prv static void init (
    const std::vector<Word> &words, SubSize nextTemplateI, const std::vector<Template> &templates,
    MultiList<core::string<SubSize>, Size> &specs
  );
  prv static void initImpl (
    const std::vector<Word> &words, const Template &templ, SubSize templateWordI,
    MultiList<core::string<SubSize>, Size> &specs, core::string<SubSize> &r_spec
  );

  pub SubSize getWordsSize () const;
  pub MultiList<core::u8string, SubSize>::SubList getWord (SubSize i) const;
  pub Size getSize () const;
  pub Action get (Size id) const;

  pub class Action {
    prv const ActionSet &actionSet;
    prv const MultiList<core::string<SubSize>, Size>::SubList spec;
    prv bool dewording;

    prv Action (const ActionSet &actionSet, Size id);
    prv Action (const ActionSet &actionSet, Size id, MultiList<core::string<SubSize>, Size>::SubList &&spec);

    pub Size getDewordingTarget () const;
    pub size_t getWordCount () const;
    pub SubSize getWord (size_t i) const;
    pub void getInput (core::u8string &r_out) const;
    pub bool includesAnyWords (const bitset::Bitset &words) const;

    friend class ActionSet;
  };
};

struct Story {
  const char *zcodeFileName;
  iu screenWidth;
  iu screenHeight;
  core::u8string prologueInput;
  std::function<bool (autofrotz::Vm &r_vm)> saver;
  std::function<bool (autofrotz::Vm &r_vm)> restorer;
  std::vector<ActionSet::Word> words;
  std::vector<ActionSet::Template> dewordingTemplates;
  std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> deworder;
  std::vector<ActionSet::Template> otherTemplates;
  std::vector<autofrotz::zword> significantWordAddrs;
};

struct RangesetPart {
  iu16f setSize;
  iu16f clearSize;
};

class Rangeset : public std::vector<RangesetPart> {
  pub Rangeset (const bitset::Bitset &bitset, iu16 size);
  pub Rangeset ();
  pub template<typename _Walker> void beWalked (_Walker &w);
};

class ActionExecutor {
  pub virtual ~ActionExecutor () {};

  pub virtual void clearWordSet () = 0;
  pub virtual void setIgnoredByteRangeset (Rangeset ignoredByteRangeset) = 0;
  pub struct ActionResult {
    ActionSet::Size id;
    core::u8string output;
    size_t similarSiblingReverseOffset;
    core::HashWrapper<Signature> signature;
    autofrotz::State state;
    std::vector<autofrotz::zword> significantWords;

    ActionResult (ActionSet::Size id, core::u8string output, size_t similarSiblingReverseOffset);
    ActionResult (ActionSet::Size id, core::u8string output, core::HashWrapper<Signature> signature);
    ActionResult (ActionSet::Size id, core::u8string output, core::HashWrapper<Signature> signature, autofrotz::State state, std::vector<autofrotz::zword> significantWords);
    template<typename _InputIterator, typename _InputEndIterator> explicit ActionResult (const Deserialiser<_InputIterator, _InputEndIterator> &);
    template<typename _Walker> void beWalked (_Walker &w);
  };
  pub virtual void processNode (
    std::vector<ActionResult> &r_results, bitset::Bitset *extraWordSet,
    const core::HashWrapper<Signature> &parentSignature, const autofrotz::State &parentState
  ) = 0;
};

class LocalActionExecutor : public ActionExecutor {
  prv autofrotz::Vm vm;
  prv const std::function<bool (autofrotz::Vm &r_vm)> saver;
  prv const std::function<bool (autofrotz::Vm &r_vm)> restorer;
  prv const ActionSet actionSet;
  prv const std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> deworder;
  prv const std::vector<autofrotz::zword> significantWordAddrs;
  prv Rangeset ignoredByteRangeset;

  pub LocalActionExecutor (Story &&story, core::u8string &r_initialOutput);

  pub iu16 getDynamicMemorySize () const noexcept;
  pub bitset::Bitset &getWordSet () noexcept;
  pub void clearWordSet () noexcept override;
  pub const ActionSet &getActionSet () const noexcept;
  pub const Rangeset &getIgnoredByteRangeset () const noexcept;
  pub void setIgnoredByteRangeset (Rangeset ignoredByteRangeset) noexcept override;
  pub static void doAction (autofrotz::Vm &r_vm, core::u8string::const_iterator inputBegin, core::u8string::const_iterator inputEnd, core::u8string &r_output, const char8_t *deathExceptionMsg);
  pub static void doAction (autofrotz::Vm &r_vm, const core::u8string &input, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv void doSaveAction (autofrotz::State &r_state);
  prv void doRestoreAction (const autofrotz::State &state);
  prv std::vector<autofrotz::zword> getSignificantWords () const;
  pub ActionResult getActionResult ();
  pub void processNode (
    std::vector<ActionResult> &r_results, bitset::Bitset *extraWordSet,
    const core::HashWrapper<Signature> &parentSignature, const autofrotz::State &parentState
  ) override;
  pub void processNode (
    std::vector<ActionResult> &r_results, bitset::Bitset *extraWordSet,
    const core::HashWrapper<Signature> &parentSignature, const autofrotz::State &parentState,
    const std::function<bool (const core::HashWrapper<Signature> &signature)> &signatureKnown
  );
};

class RemoteActionExecutor : public ActionExecutor {
  pub static constexpr size_t bufferSize = 65535;

  prv io::socket::TcpSocketStream stream;
  prv iterators::OutputStreamIterator<io::socket::TcpSocketStream> out;
  prv iterators::InputStreamIterator<io::socket::TcpSocketStream> in;

  pub static core::string<iu8f> getBuildId ();

  pub RemoteActionExecutor (const io::socket::TcpSocketAddress &addr);

  prv void beginRequest (iu8f requestId);
  prv void endRequest (iu8f requestId);
  prv iu8f read ();
  prv void check (iu expected, iu received);
  prv void beginResponse (iu8f requestId);
  prv void endResponse (iu8f requestId);

  pub void clearWordSet () override;
  pub void setIgnoredByteRangeset (Rangeset ignoredByteRangeset) override;
  pub void processNode (
    std::vector<ActionResult> &r_results, bitset::Bitset *extraWordSet,
    const core::HashWrapper<Signature> &parentSignature, const autofrotz::State &parentState
  ) override;
};

class ActionExecutorServer {
  pub static constexpr iu8f initId = 0;
  pub static constexpr iu8f clearWordSetId = 1;
  pub static constexpr iu8f setIgnoredByteRangesetId = 2;
  pub static constexpr iu8f processNodeId = 3;

  prv LocalActionExecutor &r_e;
  prv io::socket::PassiveTcpSocket listeningSocket;

  pub ActionExecutorServer (LocalActionExecutor &r_e, const io::socket::TcpSocketAddress &addr);

  prv iu8f read (iterators::InputStreamIterator<io::socket::TcpSocketStream> &r_in);
  prv void check (iu expected, iu received);
  prv iu8f beginRequest (iterators::InputStreamIterator<io::socket::TcpSocketStream> &r_in);
  prv void endRequest (iterators::InputStreamIterator<io::socket::TcpSocketStream> &r_in, iu8f requestId);
  prv void beginResponse (iterators::OutputStreamIterator<io::socket::TcpSocketStream> &r_out, iu8f requestId);
  prv void endResponse (iterators::OutputStreamIterator<io::socket::TcpSocketStream> &r_out, iu8f requestId);

  pub void accept ();
};

class Multiverse {
  pub class Node {
    pub class Listener;

    pub static Node *const unparented;
    prv static const core::u8string outputLineTerminator;

    prv std::unique_ptr<Listener> listener;
    prv Node *primeParentNode;
    prv bool primeParentNodeInvalid;
    prv core::HashWrapper<Signature> signature;
    prv std::unique_ptr<autofrotz::State> state;
    prv std::vector<std::tuple<ActionSet::Size, StringSet<char8_t>::String, Node *>> children;

    pub Node (std::unique_ptr<Listener> &&listener, Node *primeParentNode, core::HashWrapper<Signature> &&signature, autofrotz::State &&state);
    Node (const Node &) = delete;
    Node &operator= (const Node &) = delete;
    Node (Node &&) = delete;
    Node &operator= (Node &&) = delete;
    pub template<typename _InputIterator, typename _InputEndIterator> explicit Node (const Deserialiser<_InputIterator, _InputEndIterator> &);
    pub template<typename _Walker> void beWalked (_Walker &w);

    pub static Signature createSignature (const autofrotz::Vm &vm, const Rangeset &ignoredByteRangeset);
    pub static Signature recreateSignature (const Signature &oldSignature, const Rangeset &extraIgnoredByteRangeset);
    pub Listener *getListener () const;
    pub Node *getPrimeParentNode () const;
    pub size_t getPrimeParentArcChildIndex () const;
    pub const core::HashWrapper<Signature> &getSignature () const;
    pub core::HashWrapper<Signature> setSignature (core::HashWrapper<Signature> &&signature);
    pub const autofrotz::State *getState () const;
    pub void clearState ();
    pub size_t getChildrenSize () const;
    pub const std::tuple<ActionSet::Size, StringSet<char8_t>::String, Node *> &getChild (size_t i) const;
    pub size_t getChildIndex (ActionSet::Size id) const;
    pub void addChild (ActionSet::Size actionId, const core::u8string &output, Node *node, Multiverse &multiverse);
    prv bool updatePrimeParent (Node *newParentNode, bool changedAbove);
    pub void removeChild (size_t i);
    pub void changeChild (size_t i, Node *node);
    pub void childrenUpdated ();
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

  pub class NodeIterator : public iterators::RevaluedIterator<NodeIterator, Node *const &, std::unordered_map<std::reference_wrapper<const core::HashWrapper<Signature>>, Node *>::const_iterator> {
    pub NodeIterator ();
    prv NodeIterator (decltype(i) &&i);

    pub Node *const &operator_ind_ () const;

    friend class Multiverse;
  };

  pub class Listener;

  prv LocalActionExecutor localExecutor;
  prv std::unique_ptr<Listener> listener;
  prv bitset::Bitset ignoredBytes;
  prv StringSet<char8_t> outputStringSet;
  prv Node *rootNode;
  prv std::unordered_map<std::reference_wrapper<const core::HashWrapper<Signature>>, Node *> nodes; // XXXX make Node * unique_ptr?
  prv std::vector<std::unique_ptr<RemoteActionExecutor>> remoteExecutors;
  prv bitset::Bitset remoteExecutorIgnoredBytesCleans;

  pub Multiverse (Story &&story, core::u8string &r_initialOutput, std::unique_ptr<Listener> &&listener, const std::vector<autofrotz::zword> &initialIgnoredBytes);
  prv static bitset::Bitset initIgnoredBytes (const std::vector<autofrotz::zword> &initialIgnoredBytes);
  Multiverse (const Multiverse &) = delete;
  Multiverse &operator= (const Multiverse &) = delete;
  Multiverse (Multiverse &&) = delete;
  Multiverse &operator= (Multiverse &&) = delete;
  pub ~Multiverse () noexcept;

  pub const ActionSet &getActionSet () const;
  pub Listener *getListener () const;
  pub const bitset::Bitset &getIgnoredBytes () const;
  pub const StringSet<char8_t> &getOutputStringSet () const;
  pub size_t size () const;
  pub NodeIterator begin () const;
  pub NodeIterator end () const;
  pub Node *getRoot () const;
  prv void ignoredBytesChanged ();
  pub void addRemoteExecutor (const io::socket::TcpSocketAddress &addr);
  pub void removeRemoteExecutors ();
  pub template<typename _I> void processNodes (_I nodesBegin, _I nodesEnd);
  pub template<typename _I> void collapseNodes (_I nodesBegin, _I nodesEnd);
  prv template<typename _I> static bitset::Bitset createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, LocalActionExecutor &r_e);
  prv Node *collapseNode (
    Node *node, const Rangeset &extraIgnoredByteRangeset,
    std::unordered_map<std::reference_wrapper<const core::HashWrapper<Signature>>, Node *> &r_survivingNodes,
    std::unordered_map<Node *, Node *> &r_nodeCollapseTargets,
    std::unordered_map<Node *, core::HashWrapper<Signature>> &r_survivingNodePrevSignatures
  );
  pub void save (const core::u8string &pathName);
  pub void load (const core::u8string &pathName);
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
    pub virtual void walkNodeListener (Node::Listener *listener, Deserialiser<FileInputIterator, FileInputEndIterator> &s) = 0;

    pub virtual std::unique_ptr<Node::Listener> createNodeListener () = 0;
    pub virtual void nodeReached (const Multiverse &multiverse, Node::Listener *listener, ActionSet::Size parentActionId, const core::u8string &output, const Signature &signature, const std::vector<autofrotz::zword> &significantWords) = 0;
    pub virtual void subtreePrimeAncestorsUpdated (const Multiverse &multiverse, const Node *node) = 0;
    pub virtual void nodeProcessed (const Multiverse &multiverse, const Node *node, size_t processedCount, size_t totalCount) = 0;
    pub virtual void nodesProcessed (const Multiverse &multiverse) = 0;
    pub virtual void nodeCollapsed (const Multiverse &multiverse, const Node *node, bool childrenUpdated) = 0;
    pub virtual void nodesCollapsed (const Multiverse &multiverse) = 0;
    pub virtual void loaded (const Multiverse &multiverse) = 0;
  };
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}

#include "autoinf.ipp"
#endif
