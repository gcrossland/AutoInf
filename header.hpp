#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED

#include <string>
#include "libraries/autoinf.hpp"
#include <unordered_set>

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
int main (int argc, char *argv[]);
class MultiverseView;
void runCmd (int argc, char **argv, autofrotz::Vm &vm, autoinf::Multiverse &multiverse, MultiverseView *view);
void runVelocityrun (int argc, char **argv, autofrotz::Vm &vm, autoinf::Multiverse &multiverse, MultiverseView *view);
bool runCommandLine (autofrotz::Vm &vm, autoinf::Multiverse &multiverse, const core::u8string &in, core::u8string &message);
void updateMultiverseDisplay (autoinf::Multiverse &multiverse, const char *outPathName, const core::u8string &message);

class NodeMetricsListener : public autoinf::Multiverse::Node::Listener {
  pub static constexpr size_t VALUE_COUNT = 3;
  pub typedef is Value;
  pub static constexpr Value NON_VALUE = core::numeric_limits<Value>::max();

  prv Value scoreValue;

  prv bitset::Bitset interestingChildActionWords;
  prv Value wordValue;

  prv size_t locationHash;
  prv Value visitageValue;

  pub NodeMetricsListener ();
  NodeMetricsListener (const NodeMetricsListener &) = delete;
  NodeMetricsListener &operator= (const NodeMetricsListener &) = delete;
  NodeMetricsListener (NodeMetricsListener &&) = delete;
  NodeMetricsListener &operator= (NodeMetricsListener &&) = delete;
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub Value getValue (size_t i) const;

  friend class MultiverseMetricsListener;
};

class MultiverseMetricsListener : public autoinf::Multiverse::Listener {
  pub typedef NodeMetricsListener::Value Value;

  prv static const Value PER_TURN_VISITAGE_MODIFIER;
  prv static const std::vector<Value> NEW_LOCATION_VISITAGE_MODIFIERS;
  prv static const Value OLD_LOCATION_VISITAGE_MODIFIER;

  prv const autofrotz::zword scoreAddr;
  prv Value maxScoreValue;

  pub MultiverseMetricsListener (autofrotz::zword scoreAddr);
  MultiverseMetricsListener (const MultiverseMetricsListener &) = delete;
  MultiverseMetricsListener &operator= (const MultiverseMetricsListener &) = delete;
  MultiverseMetricsListener (MultiverseMetricsListener &&) = delete;
  MultiverseMetricsListener &operator= (MultiverseMetricsListener &&) = delete;

  pub virtual void nodeReached (const autoinf::Multiverse &multiverse, autoinf::Multiverse::Node::Listener *listener, autoinf::Multiverse::ActionId parentActionId, const core::u8string &output, const autoinf::Signature &signature, const autofrotz::Vm &vm) override;
  prv void setScoreValue (NodeMetricsListener *listener, const autofrotz::Vm &vm);
  prv void setVisitageData (NodeMetricsListener *listener, const core::u8string &output);
  pub virtual void subtreePrimeAncestorsUpdated (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node) override;
  prv class VisitageChain {
    prv std::unordered_set<size_t> visitedLocationHashes;
    prv size_t locationHash;
    prv std::vector<Value>::const_iterator newLocationVisitageModifiersI;
    prv Value visitageValue;

    pub VisitageChain (size_t locationHash, std::vector<Value>::const_iterator newLocationVisitageModifiersI, Value visitageValue);
    pub VisitageChain (const VisitageChain &) = default;
    pub VisitageChain &operator= (const VisitageChain &) = default;
    pub VisitageChain (VisitageChain &&) = default;
    pub VisitageChain &operator= (VisitageChain &&) = default;

    pub void increment (NodeMetricsListener *listener);
    pub Value getVisitageValue () const;
  };
  prv VisitageChain getVisitageChain (const autoinf::Multiverse::Node *node);
  prv void setVisitageValueRecursively (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, VisitageChain chain);
  prv void setVisitageValue (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, VisitageChain &r_chain);
  pub virtual void nodeChildrenUpdated (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node) override;
  prv void setWordData (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, const autoinf::Multiverse::ActionSet &actionSet);
  pub virtual void nodesProcessed (const autoinf::Multiverse &multiverse) override;
  prv template<typename F> std::unique_ptr<size_t []> getWordStats (const autoinf::Multiverse &multiverse, const F &nodeFunctor);
  prv void setWordValueRecursively (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, size_t nodesSize, const size_t *stats, Value wordValue);
  prv void setWordValue (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, size_t nodesSize, const size_t *stats, Value &r_wordValue);
  pub void virtual nodesCollapsed (const autoinf::Multiverse &multiverse) override;
  pub void virtual loaded (const autoinf::Multiverse &multiverse) override;

  pub Value getMaxScoreValue () const;

  friend class NodeMetricsListener;
};

class NodeView : public NodeMetricsListener {
  pub size_t index;
  pub static constexpr size_t NON_INDEX = static_cast<size_t>(-1);
  pub autoinf::Multiverse::ActionId primeParentChildIndex;
  pub bool isDeadEnd;

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
  prv void studyNode (autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId childIndex);
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
