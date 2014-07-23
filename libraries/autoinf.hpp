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

// XXXX move out?
class Signature {
  pub class Iterator;

  prv static const iu8f ESCAPE = 200;
  prv core::string<iu8f> b;
  prv size_t h;

  pub Signature ();
  pub Signature (const Signature &) = default;
  pub Signature &operator= (const Signature &) = default;
  pub Signature (Signature &&) = default;
  pub Signature &operator= (Signature &&) = default;

  pub size_t hash () const noexcept;
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

  prv class ActionSet;

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

  prv class ActionSet {
    prv typedef iu8f Index;

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

    pub ActionId getSize () const;
    pub ActionId getDewordingWord (ActionId id) const;
    pub bool includesAnyWords (ActionId id, const bitset::Bitset &words) const;
    pub void getInput (ActionId id, core::u8string &r_out) const;
  };

  prv struct RangesetPart {
    iu16f setSize;
    iu16f clearSize;
  };

  prv class Rangeset : public std::vector<RangesetPart> {
    pub Rangeset (const bitset::Bitset &bitset, iu16 size);
    Rangeset (const Rangeset &) = default;
    Rangeset &operator= (const Rangeset &) = default;
    Rangeset (Rangeset &&) = default;
    Rangeset &operator= (Rangeset &&) = default;
  };

  pub class Node {
    prv Signature signature;
    prv std::unique_ptr<autofrotz::State> state;
    prv std::vector<std::tuple<ActionId, core::u8string, Node *>> children;

    pub Node (Signature &&signature, autofrotz::State &&state);
    Node (const Node &) = delete;
    Node &operator= (const Node &) = delete;
    Node (Node &&) = delete;
    Node &operator= (Node &&) = delete;

    pub const Signature &getSignature () const;
    pub const autofrotz::State *getState () const;
    pub void clearState ();
    pub void addChild (ActionId actionId, core::u8string &&output, Node *node);
    pub void batchOfChildChangesCompleted ();
    pub size_t getChildrenSize () const;
    pub const std::tuple<ActionId, core::u8string, Node *> &getChild (size_t i) const;
    pub const std::tuple<ActionId, core::u8string, Node *> *getChildByActionId (ActionId id) const;
    // XXXX get index by action id?
    pub Signature setSignature (Signature &&signature);
    pub void removeChild (size_t i);
    pub void changeChild (size_t i, Node *node);
  };

  pub class NodePath {
    prv std::vector<ActionId> path;

    pub NodePath ();
    pub NodePath (const NodePath &) = default;
    pub NodePath &operator= (const NodePath &) = default;
    pub NodePath (NodePath &&) = default;
    pub NodePath &operator= (NodePath &&) = default;

    pub void append (ActionId child);
    pub void pop ();
    pub Node *resolve (Node *node) const;
  };

  prv const core::u8string saveActionInput;
  prv const core::u8string restoreActionInput;
  prv const ActionSet actionSet;
  prv const std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> deworder;
  prv bitset::Bitset ignoredBytes;
  prv Rangeset ignoredByteRangeset;
  prv Node *rootNode;
  prv std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> nodes; // XXXX make Node * unique_ptr?

  pub Multiverse (
    autofrotz::Vm &vm, const core::u8string &initialInput, core::u8string &r_initialOutput,
    const core::u8string &saveActionInput, const core::u8string &restoreActionInput,
    const std::vector<std::vector<core::u8string>> &equivalentActionInputsSet,
    std::vector<ActionWord> &&words, std::vector<ActionTemplate> &&dewordingTemplates, std::vector<ActionTemplate> &&otherTemplates,
    std::function<bool (const autofrotz::Vm &vm, const core::u8string &output)> deworder
  );
  prv static bitset::Bitset initIgnoredBytes (autofrotz::Vm &vm);
  Multiverse (const Multiverse &) = delete;
  Multiverse &operator= (const Multiverse &) = delete;
  Multiverse (Multiverse &&) = delete;
  Multiverse &operator= (Multiverse &&) = delete;
  pub ~Multiverse () noexcept;

  pub void getActionInput (ActionId id, core::u8string &r_out) const;
  prv static void doAction (autofrotz::Vm &vm, core::u8string::const_iterator inputBegin, core::u8string::const_iterator inputEnd, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv static void doAction (autofrotz::Vm &vm, const core::u8string &input, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv void doSaveAction (autofrotz::Vm &vm, autofrotz::State &r_state);
  prv void doRestoreAction (autofrotz::Vm &vm, const autofrotz::State &state);
  prv static Signature createSignature (const autofrotz::Vm &vm, const Rangeset &ignoredByteRangeset);
  prv static Signature recreateSignature (const Signature &oldSignature, const Rangeset &extraIgnoredByteRangeset);
  pub Node *getNode (const NodePath &nodePath) const;
  pub template<typename _I> void processNodes (_I nodesBegin, _I nodesEnd, autofrotz::Vm &vm);
  pub template<typename _I> bitset::Bitset createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, const autofrotz::Vm &vm);
  pub template<typename _I> void collapseNodes (_I nodesBegin, _I nodesEnd, const autofrotz::Vm &vm);
  prv static Node *collapseNode (
    Node *node, const Rangeset &extraIgnoredByteRangeset,
    std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> &r_survivingNodes,
    std::unordered_map<Node *, Node *> &r_nodeCollapseTargets,
    std::unordered_map<Node *, Signature> &r_survivingNodePrevSignatures
  );
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}

#include "autoinf.ipp"
#endif
