#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED

#include <string>
#include "libraries/autoinf.hpp"
#include <unordered_set>

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
struct Story {
  autoinf::Story _; // TODO compose by subclassing instead
  size_t scoreSignificantWordAddrI;
  std::vector<autofrotz::zword> initialIgnoredBytes;
};

int main (int argc, char *argv[]);
io::socket::TcpSocketAddress getFirstAddress (const core::u8string &nodeNameAndPort);
void runWorker (iu argsSize, char **args, Story story);
void runCmd (iu argsSize, char **args, Story story);
void runVelocityrun (iu argsSize, char **args, Story story);
bool runCommandLineTemplate (autoinf::Multiverse &r_multiverse, const core::u8string &r_in, iu roundCount, core::u8string &r_message, std::vector<std::tuple<core::u8string, iu>> &r_history);
void appendWordList (core::u8string &r_o, const bitset::Bitset &words, const autoinf::Multiverse &multiverse);
bool runCommandLine (autoinf::Multiverse &multiverse, const core::u8string &in, core::u8string &message);
void updateMultiverseDisplay (autoinf::Multiverse &multiverse, const char *outPathName, const core::u8string &message);
const char8_t *getOptionIcon (bool enabled);

class NodeMetricsListener : public autoinf::Multiverse::Node::Listener {
  pub static constexpr size_t VALUE_COUNT = 9;
  pub typedef is Value;
  pub static constexpr Value NON_VALUE = core::numeric_limits<Value>::max();
  pub static constexpr iu8f NON_BOOL = core::numeric_limits<iu8f>::max();
  pub static constexpr is16f NON_OUTPUTTAGE_VALUE = core::numeric_limits<is16f>::max();

  prv is16f scoreValue;

  prv size_t locationHash;
  prv Value visitageValue;

  prv bool novelOutputInParentArcDepthwise;
  prv is16f outputtageValue;
  prv is16f antioutputtageValue;

  prv iu8f novelOutputInParentArcPrimePathwise;

  pub NodeMetricsListener ();
  pub template<typename _Walker> void beWalked (_Walker &w);

  pub Value getValue (size_t i) const;

  friend class MultiverseMetricsListener;
};

class MultiverseMetricsListener : public autoinf::Multiverse::Listener {
  pub typedef NodeMetricsListener::Value Value;

  prv static const Value PER_TURN_VISITAGE_MODIFIER;
  prv static const std::vector<Value> NEW_LOCATION_VISITAGE_MODIFIERS;
  prv static const Value OLD_LOCATION_VISITAGE_MODIFIER;

  prv static const size_t OUTPUTTAGE_CHILD_OUTPUT_PRESKIP;

  prv const size_t scoreSignificantWordAddrI;
  prv bitset::Bitset interestingChildActionWords;
  prv bool interestingChildActionWordsIsDirty;

  pub MultiverseMetricsListener (size_t scoreSignificantWordAddrI);

  pub virtual void nodeReached (const autoinf::Multiverse &multiverse, autoinf::Multiverse::Node::Listener *listener, autoinf::ActionSet::Size parentActionId, const core::u8string &output, const autoinf::Signature &signature, const std::vector<autofrotz::zword> &significantWords) override;
  prv void setScoreValue (NodeMetricsListener *listener, const std::vector<autofrotz::zword> &significantWords);
  prv void setVisitageData (NodeMetricsListener *listener, const core::u8string &output);
  pub virtual void subtreePrimeAncestorsUpdated (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node) override;
  prv class VisitageChain {
    prv std::vector<size_t> visitedLocationHashes;
    prv size_t locationHash;
    prv std::vector<Value>::const_iterator newLocationVisitageModifiersI;
    prv Value visitageValue;

    pub VisitageChain ();

    pub void pushNode (size_t nodeLocationHash);
    pub Value getVisitageValue () const;
  };
  prv class PrimePathwiseOutputtageChain {
    prv bitset::Bitset strings;
    prv autoinf::MultiList<std::vector<iu>, iu> stringSetStack;

    pub void pushArc (const autoinf::StringSet<char8_t>::String &childOutput);
    pub bool novel () const;
    pub void popArc ();
  };
  prv void doPrimePathwisePassHead (const autoinf::Multiverse::Node *node, VisitageChain &r_visitageChain, PrimePathwiseOutputtageChain &r_outputtageChain);
  prv void doPrimePathwisePass (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, VisitageChain visitageChain, PrimePathwiseOutputtageChain &r_outputtageChain);
  prv void setVisitageValue (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, const VisitageChain &chain);
  prv void setPrimePathwiseOutputtageValue (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, const PrimePathwiseOutputtageChain &chain);
  prv size_t checkVisitageValueRecursively (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, VisitageChain chain);
  pub virtual void nodeProcessed (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node, size_t processedCount, size_t totalCount) override;
  prv void setWordData (const autoinf::Multiverse::Node *node, const autoinf::ActionSet &actionSet);
  pub virtual void nodesProcessed (const autoinf::Multiverse &multiverse) override;
  pub virtual void multiverseChanged (const autoinf::Multiverse &multiverse);
  prv void doDepthwisePass (const autoinf::Multiverse &multiverse);
  prv void doPostDepthwisePrimePathwisePass (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, NodeMetricsListener *parentListener);
  prv void setDepthwiseOutputtageValues (const autoinf::Multiverse::Node *node, NodeMetricsListener *listener, NodeMetricsListener *parentListener);
  pub virtual void nodeCollapsed (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node, bool childrenUpdated) override;
  pub virtual void nodesCollapsed (const autoinf::Multiverse &multiverse) override;
  pub virtual void loaded (const autoinf::Multiverse &multiverse) override;

  pub Value getMaxScoreValue (const autoinf::Multiverse &multiverse) const;
  pub const bitset::Bitset &getInterestingChildActionWords (const autoinf::Multiverse &multiverse);

  friend class NodeMetricsListener;
};

class NodeView : public NodeMetricsListener {
  pub size_t index;
  pub static constexpr size_t NON_INDEX = static_cast<size_t>(-1);
  pub autoinf::ActionSet::Size primeParentChildIndex;
  pub bool isDeadEnd;
  pub bool isAntiselected;

  pub NodeView ();
  pub template<typename _Walker> void beWalked (_Walker &w);
};

class MultiverseView : public MultiverseMetricsListener {
  pub time_t nodeProcessingStarted;
  pub time_t prevNodeProcessingProgressReport;
  pub int prevNodeProcessingProgressReportSize;
  pub std::vector<autoinf::Multiverse::Node *> nodesByIndex;
  pub std::unordered_set<autoinf::Multiverse::Node *> selectedNodes;
  pub std::unordered_set<autoinf::Multiverse::Node *> verboseNodes;
  pub bool elideDeadEndNodes;
  pub bool elideAntiselectedNodes;
  pub size_t maxDepth;
  pub bool combineSimilarSiblings;
  prv bool deadEndnessIsDirty;
  prv bool antiselectednessIsDirty;

  pub MultiverseView (size_t scoreSignificantWordAddrI);

  pub virtual std::tuple<void *, size_t> deduceNodeListenerType (autoinf::Multiverse::Node::Listener *listener) override;
  pub virtual std::tuple<autoinf::Multiverse::Node::Listener *, void *, size_t> constructNodeListener () override;
  pub virtual void walkNodeListener (autoinf::Multiverse::Node::Listener *listener, autoinf::Serialiser<autoinf::FileOutputIterator> &s) override;
  pub virtual void walkNodeListener (autoinf::Multiverse::Node::Listener *listener, autoinf::Deserialiser<autoinf::FileInputIterator, autoinf::FileInputEndIterator> &s) override;

  pub virtual std::unique_ptr<autoinf::Multiverse::Node::Listener> createNodeListener () override;
  pub void nodeProcessingStarting ();
  pub virtual void nodeProcessed (const autoinf::Multiverse &multiverse, const autoinf::Multiverse::Node *node, size_t processedCount, size_t totalCount) override;
  pub void multiverseMayHaveChanged ();
  pub virtual void multiverseChanged (const autoinf::Multiverse &multiverse) override;
  pub void selectionChanged ();
  prv void studyNodes (const autoinf::Multiverse &multiverse);
  prv void studyNode (autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::ActionSet::Size childIndex);
  prv void markDeadEndAndAntiselectedNodes ();
  prv void markDeadEndNode (autoinf::Multiverse::Node *node, NodeView *nodeView);
  prv void markAntiselectedNode (autoinf::Multiverse::Node *node, NodeView *nodeView);
  pub void printNodes (const autoinf::Multiverse &multiverse, FILE *out);
  prv void printNodeHeader (
    char8_t nodeIndexRenderingPrefix, char8_t nodeIndexRenderingSuffix, autoinf::Multiverse::Node *node,
    autoinf::ActionSet::Size *actionIdsI, autoinf::ActionSet::Size *actionIdsEnd,
    const autoinf::Multiverse &multiverse, FILE *out
  );
  prv core::u8string renderActionInput (autoinf::ActionSet::Size *actionIdsI, autoinf::ActionSet::Size *actionIdsEnd, const autoinf::ActionSet &actionSet);
  prv void printNodeOutput (const autoinf::StringSet<char8_t>::String *output, const autoinf::Multiverse &multiverse, const core::u8string &prefix, FILE *out);
  prv void printNodeAsLeaf (
    size_t depth, const autoinf::StringSet<char8_t>::String *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode,
    autoinf::ActionSet::Size *actionIdsI, autoinf::ActionSet::Size *actionIdsEnd,
    const autoinf::Multiverse &multiverse, core::u8string &r_prefix, FILE *out
  );
  prv void printNodeAsNonleaf (
    size_t depth, const autoinf::StringSet<char8_t>::String *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode,
    autoinf::ActionSet::Size *actionIdsI, autoinf::ActionSet::Size *actionIdsEnd,
    const autoinf::Multiverse &multiverse, core::u8string &r_prefix, FILE *out
  );
};

void readLine (core::u8string &r_out);
const char8_t *skipSpaces (const char8_t *i, const char8_t *end);
const char8_t *skipNonSpaces (const char8_t *i, const char8_t *end);
is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd);
core::u8string hexise (iu v);
char narrowise (char8_t c);
const char *narrowise (const char8_t *s);
const char *narrowise (const core::u8string &s);

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
#endif
