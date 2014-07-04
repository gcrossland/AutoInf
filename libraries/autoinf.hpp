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

  // XXXX debug-only stuff!!
  prv static iu64 BYTE_COUNTS[256];
  prv static const iu8f ESCAPE = 200;
  prv core::string<iu8f> b;
  prv size_t h;

  pub core::u8string wrXXXX () const;
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

  prv struct BitrangesPart {
    iu16f setSize;
    iu16f clearSize;
  };

  // XXXX gaah! naming classes with plurals!!!
  prv class Bitranges : public std::vector<BitrangesPart> {
    pub Bitranges (const bitset::Bitset &bitset, iu16 size);
    Bitranges (const Bitranges &) = default;
    Bitranges &operator= (const Bitranges &) = default;
    Bitranges (Bitranges &&) = default;
    Bitranges &operator= (Bitranges &&) = default;
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

  prv core::u8string saveActionInput;
  prv core::u8string restoreActionInput;
  prv core::u8string actionInputs;
  prv std::vector<size_t> actionInputBegins;
  prv bitset::Bitset ignoredBytes;
  prv Bitranges ignoredByteRanges;
  prv Node *rootNode;
  prv std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> nodes; // XXXX make Node * unique_ptr?

  pub Multiverse (autofrotz::Vm &vm, const core::u8string &initialInput, core::u8string &r_initialOutput, const core::u8string &saveActionInput, const core::u8string &restoreActionInput, const std::vector<std::vector<core::u8string>> &equivalentActionInputsSet, const std::vector<core::u8string> &words, const std::vector<std::vector<core::u8string>> &actionTemplates);
  prv static bitset::Bitset initIgnoredBytes (autofrotz::Vm &vm);
  prv static void initActionInputs (const std::vector<core::u8string> &words, const std::vector<std::vector<core::u8string>> &actionTemplates, core::u8string &r_actionInputs, std::vector<size_t> &r_actionInputBegins);
  prv static void initActionInputsImpl (const std::vector<core::u8string> &words, std::vector<core::u8string>::const_iterator actionTemplateI, std::vector<core::u8string>::const_iterator actionTemplateEnd, core::u8string &r_actionInputs, std::vector<size_t> &r_actionInputBegins, core::u8string &r_actionInput);
  Multiverse (const Multiverse &) = delete;
  Multiverse &operator= (const Multiverse &) = delete;
  Multiverse (Multiverse &&) = delete;
  Multiverse &operator= (Multiverse &&) = delete;
  pub ~Multiverse () noexcept;

  pub std::tuple<core::u8string::const_iterator, core::u8string::const_iterator> getActionInput (ActionId id) const;
  prv static void doAction (autofrotz::Vm &vm, core::u8string::const_iterator inputBegin, core::u8string::const_iterator inputEnd, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv static void doAction (autofrotz::Vm &vm, const core::u8string &input, core::u8string &r_output, const char8_t *deathExceptionMsg);
  prv void doSaveAction (autofrotz::Vm &vm, autofrotz::State *state);
  prv void doRestoreAction (autofrotz::Vm &vm, const autofrotz::State *state);
  prv static Signature createSignature (const autofrotz::Vm &vm, const Bitranges &ignoredByteRanges);
  prv static Signature recreateSignature (const Signature &oldSignature, const Bitranges &extraIgnoredByteRanges);
  pub Node *getNode (const NodePath &nodePath) const;
  pub template<typename _I> void processNodes (_I nodesBegin, _I nodesEnd, autofrotz::Vm &vm);
  pub template<typename _I> bitset::Bitset createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, const autofrotz::Vm &vm);
  pub template<typename _I> void collapseNodes (_I nodesBegin, _I nodesEnd, const autofrotz::Vm &vm);
  prv static Node *collapseNode (
    Node *node, const Bitranges &extraIgnoredByteRanges,
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
