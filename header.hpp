#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED

#include <string>
#include "libraries/autoinf.hpp"
#include <unordered_set>

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
int main (int argc, char *argv[]);
void runCommandLine (autofrotz::Vm &vm, autoinf::Multiverse &multiverse, const char *outPathName, const core::u8string &in);

class NodeMetricsListener : public autoinf::Multiverse::Node::Listener {
  pub static constexpr size_t VALUE_COUNT = 3;
  pub static constexpr size_t NON_VALUE = static_cast<size_t>(-1);

  prv size_t scoreValue;

  prv bitset::Bitset interestingChildActionWords;
  prv size_t wordValue;

  prv size_t locationHash;
  prv size_t visitageValue;

  pub NodeMetricsListener ();
  NodeMetricsListener (const NodeMetricsListener &) = delete;
  NodeMetricsListener &operator= (const NodeMetricsListener &) = delete;
  NodeMetricsListener (NodeMetricsListener &&) = delete;
  NodeMetricsListener &operator= (NodeMetricsListener &&) = delete;
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub size_t getValue (size_t i) const;

  friend class MultiverseMetricsListener;
};

class MultiverseMetricsListener : public autoinf::Multiverse::Listener {
  prv static const size_t INITIAL_VISITAGE_VALUE;
  prv static const std::vector<size_t> NEW_LOCATION_VISITAGE_MODIFIERS;
  prv static const ptrdiff_t OLD_LOCATION_VISITAGE_MODIFIER;

  prv const autofrotz::zword scoreAddr;

  pub MultiverseMetricsListener (autofrotz::zword scoreAddr);
  MultiverseMetricsListener (const MultiverseMetricsListener &) = delete;
  MultiverseMetricsListener &operator= (const MultiverseMetricsListener &) = delete;
  MultiverseMetricsListener (MultiverseMetricsListener &&) = delete;
  MultiverseMetricsListener &operator= (MultiverseMetricsListener &&) = delete;

  pub virtual void nodeReached (const autoinf::Multiverse &multiverse, autoinf::Multiverse::Node::Listener *listener, autoinf::Multiverse::ActionId parentActionId, const core::u8string &output, const autoinf::Signature &signature, const autofrotz::Vm &vm) override;
  prv void setScoreValue (NodeMetricsListener *listener, const autofrotz::Vm &vm);
  prv void setVisitageData (NodeMetricsListener *listener, const core::u8string &output);
  pub virtual void subtreePrimeAncestorsUpdated (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node) override;
  prv std::tuple<std::unordered_set<size_t>, size_t, std::vector<size_t>::const_iterator, size_t> getVisitageChain (const autoinf::Multiverse::Node *node);
  prv void incrementVisitageChain (NodeMetricsListener *listener, std::tuple<std::unordered_set<size_t>, size_t, std::vector<size_t>::const_iterator, size_t> &r_chain);
  prv void setVisitageValueRecursively (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, std::tuple<std::unordered_set<size_t>, size_t, std::vector<size_t>::const_iterator, size_t> chain);
  prv void setVisitageValue (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, std::tuple<std::unordered_set<size_t>, size_t, std::vector<size_t>::const_iterator, size_t> &r_chain);
  pub virtual void nodeChildrenUpdated (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node) override;
  prv void setWordData (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, const autoinf::Multiverse::ActionSet &actionSet);
  pub virtual void nodesProcessed (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *rootNode, const std::unordered_map<std::reference_wrapper<const autoinf::Signature>, autoinf::Multiverse::Node *, autoinf::Hasher<autoinf::Signature>> &nodes) override;
  prv template<typename F> std::unique_ptr<size_t []> getWordStats (const std::unordered_map<std::reference_wrapper<const autoinf::Signature>, autoinf::Multiverse::Node *, autoinf::Hasher<autoinf::Signature>> &nodes, const F &nodeFunctor, const autoinf::Multiverse::ActionSet &actionSet);
  prv void setWordValueRecursively (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, size_t nodesSize, const size_t *stats, size_t wordValue);
  prv void setWordValue (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, size_t nodesSize, const size_t *stats, size_t &r_wordValue);
  pub void virtual nodesCollapsed (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *rootNode, const std::unordered_map<std::reference_wrapper<const autoinf::Signature>, autoinf::Multiverse::Node *, autoinf::Hasher<autoinf::Signature>> &nodes) override;

  friend class NodeMetricsListener;
};

class NodeView : public NodeMetricsListener {
  pub size_t index;
  pub autoinf::Multiverse::ActionId primeParentChildIndex;
  pub bool allChildrenAreNonPrime;

  pub NodeView ();
  pub template<typename _Walker> void beWalked (_Walker &w);
};

class MultiverseView : public MultiverseMetricsListener {
  pub std::vector<autoinf::Multiverse::Node *> nodesByIndex;
  pub std::unordered_set<autoinf::Multiverse::Node *> selectedNodes;
  pub std::unordered_set<autoinf::Multiverse::Node *> verboseNodes;
  pub bool elideDeadEndNodes = false;
  pub size_t maxDepth = core::numeric_limits<size_t>::max();

  pub MultiverseView (autofrotz::zword scoreAddr);

  pub virtual std::tuple<void *, size_t> deduceNodeListenerType (autoinf::Multiverse::Node::Listener *listener) override;
  pub virtual std::tuple<autoinf::Multiverse::Node::Listener *, void *, size_t> constructNodeListener () override;
  pub virtual void walkNodeListener (autoinf::Multiverse::Node::Listener *listener, autoinf::Serialiser<autoinf::FileOutputIterator> &s) override;
  pub virtual void walkNodeListener (autoinf::Multiverse::Node::Listener *listener, autoinf::Deserialiser<autoinf::FileInputIterator> &s) override;

  pub virtual std::unique_ptr<autoinf::Multiverse::Node::Listener> createNodeListener () override;
  pub void multiverseChanged (const autoinf::Multiverse &multiverse);
  prv void studyNodes (const autoinf::Multiverse &multiverse);
  prv void studyNode (
    iu depth, iu targetDepth, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId childIndex
  );
  prv void markDeadEndNodes ();
  pub void printNodes (const autoinf::Multiverse &multiverse, FILE *out);
  prv void printNodeHeader (
    char8_t nodeIndexRenderingPrefix, char8_t nodeIndexRenderingSuffix, autoinf::Multiverse::Node *node, autoinf::Multiverse::ActionId actionId,
    const autoinf::Multiverse::ActionSet &actionSet, FILE *out
  );
  prv core::u8string renderActionInput (autoinf::Multiverse::ActionId actionId, const autoinf::Multiverse::ActionSet &actionSet);
  prv void printNodeOutput (const core::u8string *output, const core::u8string &prefix, FILE *out);
  prv void printNodeAsLeaf (
    size_t depth, const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
    const autoinf::Multiverse::ActionSet &actionSet, core::u8string &r_prefix, FILE *out
  );
  prv void printNodeAsNonleaf (
    size_t depth, const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
    const autoinf::Multiverse::ActionSet &actionSet, core::u8string &r_prefix, FILE *out
  );
};

void readLine (core::u8string &r_out);
const char8_t *skipSpaces (const char8_t *i, const char8_t *end);
const char8_t *skipNonSpaces (const char8_t *i, const char8_t *end);
is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd);

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
#endif
