/** @file */
/* -----------------------------------------------------------------------------
   AutoInf Core Engine Library
   © Geoff Crossland 2006, 2013
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

class Signature {
  // XXXX debug-only stuff!!
  prv static iu64 BYTE_COUNTS[256];
  prv static const iu8f ESCAPE = 200;
  // TODO still with the not using std::basic_string
  prv std::basic_string<iu8f> b;
  prv size_t h;

  pub Signature ();
  pub Signature (const Signature &) = default;
  pub Signature &operator= (const Signature &) = default;
  pub Signature (Signature &&) = default;
  pub Signature &operator= (Signature &&) = default;

  pub size_t hash () const noexcept;

  friend bool operator== (const Signature &l, const Signature &r) noexcept;
  friend bool operator< (const Signature &l, const Signature &r) noexcept;

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
    pub void close();
  };
};

bool operator== (const Signature &l, const Signature &r) noexcept;
bool operator< (const Signature &l, const Signature &r) noexcept;

class Multiverse {
  pub typedef iu16f ActionId;

  pub class Node {
    prv iu id; // XXXX needed?
    prv Signature signature;
    prv std::unique_ptr<autofrotz::State> state;
    prv std::vector<std::tuple<ActionId, std::string, Node *>> children;

    pub Node (iu id, Signature &&signature, autofrotz::State &&state);
    Node (const Node &) = delete;
    Node &operator= (const Node &) = delete;
    Node (Node &&) = delete;
    Node &operator= (Node &&) = delete;

    pub iu getId () const;
    pub const Signature &getSignature () const;
    pub const autofrotz::State *getState () const;
    pub void addChild (ActionId actionId, std::string &&output, Node *node);
    pub void allChildrenAdded ();
    pub size_t getChildCount () const;
    pub const std::tuple<ActionId, std::string, Node *> &getChild (size_t i) const;
    pub const std::tuple<ActionId, std::string, Node *> *getChildByActionId (ActionId id) const;
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

  /* XXXX
  pub class Memset {
    pub struct Range {
      iu16f begin;
      iu16f end;
    };

    // XXXX sorted
    prv std::vector<Range> ranges;

    pub Memset ();
    pub Memset (const Memset &) = default;
    pub Memset &operator= (const Memset &) = default;
    pub Memset (Memset &&) = default;
    pub Memset &operator= (Memset &&) = default;
    pub ~Memset () noexcept;

    pub void append (iu16f begin, iu16f end);
    pub void add (iu16 begin, iu16 end);
    pub void add (const Memset &o); // XXXX or go for boolean operators? yes!
    pub std::vector<Range>::const_iterator begin () const;
    pub std::vector<Range>::const_iterator end () const;
  };
  */

  prv std::string saveActionInput;
  prv std::string restoreActionInput;
  prv std::string actionInputs;
  prv std::vector<size_t> actionInputBegins;
  /* XXXX
  prv Memset ignored;
  */
  prv Node *rootNode;
  prv std::unordered_map<std::reference_wrapper<const Signature>, Node *, Hasher<Signature>> nodesBySignature; // XXXX make Node * unique_ptr?

  pub Multiverse (autofrotz::Vm &vm, const std::string &initialInput, std::string &r_initialOutput, const std::string &saveActionInput, const std::string &restoreActionInput, const std::vector<std::string> &words, const std::vector<std::vector<std::string>> &actionTemplates);
  prv static void initActionInputs (const std::vector<std::string> &words, const std::vector<std::vector<std::string>> &actionTemplates, std::string &r_actionInputs, std::vector<size_t> &r_actionInputBegins);
  prv static void initActionInputsImpl (const std::vector<std::string> &words, std::vector<std::string>::const_iterator actionTemplateI, std::vector<std::string>::const_iterator actionTemplateEnd, std::string &r_actionInputs, std::vector<size_t> &r_actionInputBegins, std::string &r_actionInput);
  Multiverse (const Multiverse &) = delete;
  Multiverse &operator= (const Multiverse &) = delete;
  Multiverse (Multiverse &&) = delete;
  Multiverse &operator= (Node &&) = delete;
  pub ~Multiverse () noexcept;

  pub std::tuple<std::string::const_iterator, std::string::const_iterator> getActionInput (ActionId id) const;
  prv static void doAction (autofrotz::Vm &vm, std::string::const_iterator inputBegin, std::string::const_iterator inputEnd, std::string &r_output, const char *deathExceptionMsg);
  prv static void doAction (autofrotz::Vm &vm, const std::string &input, std::string &r_output, const char *deathExceptionMsg);
  prv void doSaveAction (autofrotz::Vm &vm, autofrotz::State *state);
  prv void doRestoreAction (autofrotz::Vm &vm, const autofrotz::State *state);
  prv static Signature createSignature (autofrotz::Vm &vm);
  pub Node *getNode (const NodePath &nodePath) const;
  pub template<typename _I> void processNodes (_I nodesBegin, _I nodesEnd, autofrotz::Vm &vm);
  // XXXX pub template<typename _I> void compactNodes (_I nodesBegin, _I nodesEnd);
  // have the parent 'model' class have a) the vm b) the multiverse c) the set of memsets
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}

#include "autoinf.ipp"
#endif
