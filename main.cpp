#include "header.hpp"
#include <cstring>
#include <unordered_set>

using autofrotz::Vm;
using autoinf::Multiverse;
using std::vector;
using core::u8string;
using NodePath = autoinf::Multiverse::NodePath;
using Node = autoinf::Multiverse::Node;
using ActionId = autoinf::Multiverse::ActionId;
using ActionWord = autoinf::Multiverse::ActionWord;
using ActionTemplate = autoinf::Multiverse::ActionTemplate;
using std::unordered_set;
using std::get;
using core::numeric_limits;
using std::exception;
using std::unordered_map;
using std::move;
using core::PlainException;
using std::copy;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

void terminator () {
  core::dieHard();
}

FILE *out;

int main (int argc, char *argv[]) {
  std::set_terminate(&terminator);
  try {
    std::shared_ptr<core::debug::Stream> errs(new core::debug::Stream("LOG.TXT"));
    DOPEN(, errs);
    autoinf::DOPEN(, errs);
    //autofrotz::DOPEN(, errs);
    //autofrotz::vmlink::DOPEN(, errs);

    const char *outPathName = nullptr;
    if (argc == 2) {
      outPathName = argv[1];
    }

    const iu height = 64;
    const iu undoDepth = 0;
    u8string output;
    Vm vm("104/104.z5", 70, height, undoDepth, true, output);
    Multiverse multiverse(
      vm, u8("verbose\nfullscore\n"), output, u8("save\n\1\n"), u8("restore\n\1\n"),
      vector<vector<u8string>> {
        // {u8("z\n"), u8("z. z. z. z. z. z. z. z.\n")},
        // {u8("verbitudeise the tangerine monstrosity. verbitudeise the tangerine monstrosity.\n"), u8("")},
        // {u8("turn wheel. pull wheel.\n"), u8("turn wheel. pull wheel. east. west.\n"), u8("turn wheel. pull wheel. west. east.\n")},
        // {u8(""), u8("east\n"), u8("west\n"), u8("take red sphere\n"), u8("take blue sphere\n"), u8("drop red sphere\n"), u8("drop blue sphere\n"), u8("open red sphere\n"), u8("open blue sphere\n"), u8("enter light\n")}
      },
      vector<ActionWord> {
        {u8("red sphere"), 0b011},
        {u8("blue sphere"), 0b011},
        {u8("green sphere"), 0b001},
        {u8("wheel"), 0b101}
      },
      vector<ActionTemplate> {
        {u8("examine "), 0b001, u8("\n")},
      },
      vector<ActionTemplate> {
        {u8("east\n")},
        {u8("west\n")},
        {u8("take "), 0b010, u8("\n")},
        {u8("drop "), 0b010, u8("\n")},
        {u8("open "), 0b010, u8("\n")},
        {u8("turn "), 0b100, u8(". pull "), 0b100, u8("\n")},
        {u8("enter light\n")}
      },
      [] (const Vm &vm, const u8string &output) -> bool {
        return output.find(u8("You can't see")) != std::string::npos;
      }
    );
    /*
    Vm vm("103/103.z5", 70, height, undoDepth, true, output);
    Multiverse multiverse(
      vm, u8(""), output, u8("s\n\1\n"), u8("r\n\1\n"),
      vector<vector<u8string>> {
      },
      vector<u8string> {
        u8("0"), u8("1"), u8("2"), u8("3"), u8("5"), u8("6"), u8("B"), u8("D")
      },
      vector<vector<u8string>> {
        {u8("xxxxxx\n")},
        {u8("yyyyyyy\n")},
        {u8("i"), u8("\n")},
        {u8("b01\n")},
        {u8("b23\n")}
      }
    );
    */
    /*
    Vm vm("advent/advent.z5", 70, height, undoDepth, true, output);
    const ActionWord::CategorySet holdable = 1 << 0;
    const ActionWord::CategorySet lightable = 1 << 1;
    const ActionWord::CategorySet wearable = 1 << 2;
    const ActionWord::CategorySet imbibable = 1 << 3;
    const ActionWord::CategorySet edible = 1 << 4;
    const ActionWord::CategorySet container_or_supporter = 1 << 5;
    const ActionWord::CategorySet creature = 1 << 6;
    const ActionWord::CategorySet openable = 1 << 7;
    const ActionWord::CategorySet opener = 1 << 8;
    Multiverse multiverse(
      vm, u8("verbose\nfullscore\n"), output, u8("save\n\1\n"), u8("restore\n\1\n"),
      vector<vector<u8string>> {
      },
      vector<ActionWord> {
        {u8("hill"), 0},
        {u8("other side of hill"), 0},
        {u8("spring"), imbibable},
        {u8("pipes"), 0},
        {u8("keys"), holdable | opener},
        {u8("tasty food"), holdable | edible},
        {u8("brass lantern"), holdable | lightable},
        {u8("bottle"), holdable | imbibable},
        {u8("streambed"), imbibable},
        {u8("slit"), 0},
        {u8("depression"), 0},
        {u8("grate"), openable},
        {u8("cobbles"), 0},
        {u8("wicker cage"), holdable | container_or_supporter},
        {u8("debris"), 0},
        {u8("note"), holdable},
        {u8("rod"), holdable | opener},
        {u8("bird"), holdable | creature},
        {u8("pit"), container_or_supporter},
        {u8("crack"), container_or_supporter},
        {u8("staircase"), 0},
        {u8("steps"), 0},
        {u8("dome"), 0},
        {u8("large gold nugget"), holdable},
        {u8("diamonds"), holdable},
        {u8("bridge"), container_or_supporter},
        {u8("fissure"), container_or_supporter},
        {u8("crossover"), container_or_supporter},
        {u8("snake"), creature},
        {u8("bars of silver"), holdable},
        {u8("precious jewelry"), holdable | wearable},
        {u8("rare coins"), holdable},
        {u8("Y2"), container_or_supporter},
        {u8("window"), container_or_supporter | openable},
        {u8("marks"), 0},
        {u8("shadowy figure"), creature},
        {u8("rocks"), 0},
        {u8("orange column"), 0},
        {u8("bedrock block"), container_or_supporter},
        {u8("beanstalk"), container_or_supporter},
        {u8("hole above pit"), container_or_supporter},
        {u8("plant"), container_or_supporter},
        {u8("thin rock slabs"), 0},
        {u8("pool of oil"), lightable},
        {u8("slab"), 0},
        {u8("boulders"), 0},
        {u8("stalactite"), container_or_supporter},
        {u8("dragon"), creature},
        {u8("rug"), holdable | container_or_supporter},
        {u8("treasure chest"), container_or_supporter},
        {u8("leaves"), 0},
        {u8("scrawled inscription"), 0},
        {u8("nest of golden eggs"), holdable},
        {u8("rusty door"), openable},
        {u8("waterfall"), imbibable | container_or_supporter},
        {u8("trident"), holdable},
        {u8("carpet"), holdable | container_or_supporter},
        {u8("curtains"), container_or_supporter | openable},
        {u8("moss"), edible},
        {u8("pillow"), holdable},
        {u8("drawings"), holdable},
        {u8("vase"), holdable},
        {u8("shards"), holdable},
        {u8("emerald"), holdable},
        {u8("tablet"), holdable},
        {u8("platinum pyramid"), holdable},
        {u8("clam"), holdable},
        {u8("pearl"), holdable},
        {u8("sign"), holdable},
        {u8("Spelunker Today"), holdable},
        {u8("mirror"), 0},
        {u8("troll"), creature},
        {u8("volcano"), container_or_supporter},
        {u8("sparks of ash"), 0},
        {u8("jagged roof"), 0},
        {u8("gorge"), container_or_supporter},
        {u8("river of fire"), container_or_supporter},
        {u8("geyser"), 0},
        {u8("boulders"), 0},
        {u8("rare spices"), holdable},
        {u8("limestone formations"), 0},
        {u8("dust"), 0},
        {u8("bear"), creature},
        {u8("golden chain"), holdable},
        {u8("message"), holdable},
        {u8("vending machine"), container_or_supporter},
        {u8("batteries"), holdable},
        {u8("dwarf"), creature},
        {u8("axe"), holdable | opener},
        {u8("collection of adventure game materials"), holdable}
      },
      vector<ActionTemplate> {
        {u8("examine "), 0, u8("\n")},
      },
      vector<ActionTemplate> {
        {u8("north\n")},
        {u8("south\n")},
        {u8("east\n")},
        {u8("west\n")},
        {u8("northeast\n")},
        {u8("northwest\n")},
        {u8("southeast\n")},
        {u8("southwest\n")},
        {u8("up above\n")},
        {u8("ground\n")},
        {u8("inside\n")},
        {u8("outside\n")},
        {u8("attack "), 0, u8("\n")},
        {u8("blow "), holdable, u8("\n")},
        {u8("burn "), 0, u8(" with "), lightable, u8("\n")},
        {u8("buy "), 0, u8("\n")},
        {u8("climb "), 0, u8("\n")},
        {u8("close "), openable, u8("\n")},
        {u8("chop "), 0, u8("\n")},
        {u8("dig "), 0, u8("\n")},
        {u8("disrobe "), wearable, u8("\n")},
        {u8("drink "), imbibable, u8("\n")},
        {u8("discard "), holdable, u8("\n")},
        {u8("put down "), holdable, u8("\n")},
        {u8("eat "), edible, u8("\n")},
        {u8("empty "), 0, u8(" onto "), 0, u8("\n")},
        {u8("cross "), 0, u8("\n")},
        {u8("get into "), container_or_supporter, u8("\n")},
        {u8("go into "), 0, u8("\n")},
        {u8("leave into "), 0, u8("\n")},
        {u8("lie in "), container_or_supporter, u8("\n")},
        {u8("stand in "), container_or_supporter, u8("\n")},
        {u8("check "), 0, u8("\n")},
        {u8("read "), 0, u8("\n")},
        {u8("exit\n")},
        {u8("exit "), 0, u8("\n")},
        {u8("get out\n")},
        {u8("leave "), 0, u8("\n")},
        {u8("stand\n")},
        {u8("fill "), container_or_supporter, u8("\n")},
        {u8("feed "), edible, u8(" to "), creature, u8("\n")},
        {u8("discard "), holdable, u8(" into "), container_or_supporter, u8("\n")},
        {u8("put "), holdable, u8(" inside "), container_or_supporter, u8("\n")},
        {u8("hop\n")},
        {u8("hop over "), 0, u8("\n")},
        {u8("attach "), holdable, u8("\n")},
        {u8("attach "), holdable, u8(" to "), 0, u8("\n")},
        {u8("lock "), openable, u8(" with "), opener, u8("\n")},
        {u8("look under "), 0, u8("\n")},
        {u8("open "), openable, u8("\n")},
        {u8("drag "), 0, u8("\n")},
        {u8("clear "), 0, u8("\n")},
        {u8("rotate "), 0, u8("\n")},
        {u8("clear "), 0, u8(" "), 0, u8("\n")},
        {u8("discard "), holdable, u8(" onto "), container_or_supporter, u8("\n")},
        {u8("put "), holdable, u8(" onto "), container_or_supporter, u8("\n")},
        {u8("get "), 0, u8(" from "), container_or_supporter, u8("\n")},
        {u8("remove "), 0, u8(" from "), container_or_supporter, u8("\n")},
        {u8("carry "), 0, u8(" from "), container_or_supporter, u8("\n")},
        {u8("clean "), 0, u8("\n")},
        {u8("look in "), container_or_supporter, u8("\n")},
        {u8("search "), container_or_supporter, u8("\n")},
        {u8("adjust "), 0, u8("\n")},
        {u8("display "), holdable, u8(" to "), creature, u8("\n")},
        {u8("sing\n")},
        {u8("nap\n")},
        {u8("smell\n")},
        {u8("smell "), 0, u8("\n")},
        {u8("squash "), 0, u8("\n")},
        {u8("dive\n")},
        {u8("swing "), 0, u8("\n")},
        {u8("swing on "), 0, u8("\n")},
        {u8("close off "), 0, u8("\n")},
        {u8("switch off "), 0, u8("\n")},
        {u8("rotate off "), 0, u8("\n")},
        {u8("switch on "), 0, u8("\n")},
        {u8("rotate on "), 0, u8("\n")},
        {u8("get "), 0, u8("\n")},
        {u8("peel "), 0, u8("\n")},
        {u8("peel off "), 0, u8("\n")},
        {u8("pick up "), 0, u8("\n")},
        {u8("pick "), 0, u8(" up\n")},
        {u8("remove "), 0, u8("\n")},
        {u8("carry "), 0, u8("\n")},
        {u8("taste "), edible, u8("\n")},
        {u8("discard "), holdable, u8(" onto "), container_or_supporter, u8("\n")},
        {u8("feel "), 0, u8("\n")},
        {u8("open "), openable, u8(" with "), opener, u8("\n")},
        {u8("force "), 0, u8(" with "), holdable, u8("\n")},
        {u8("unlock "), openable, u8(" with "), opener, u8("\n")},
        {u8("go\n")},
        {u8("leave\n")},
        {u8("awake "), creature, u8("\n")},
        {u8("wave "), holdable, u8("\n")},
        {u8("wave\n")},
        {u8("put on "), wearable, u8("\n")},
        {u8("ask "), creature, u8(" for "), 0, u8("\n")},
        {u8("xyzzy\n")},
        {u8("plugh\n")},
        {u8("count "), 0, u8("\n")},
        {u8("empty "), container_or_supporter, u8("\n")},
        {u8("douse "), 0, u8("\n")},
        {u8("free "), creature, u8("\n")},
        {u8("capture "), creature, u8("\n")},
        {u8("capture "), creature, u8(" with "), holdable, u8("\n")},
        {u8("plover\n")},
        {u8("water "), 0, u8("\n")},
        {u8("douse water on "), 0, u8("\n")},
        {u8("grease "), 0, u8("\n")},
        {u8("douse oil on "), 0, u8("\n")},
        {u8("kick "), 0, u8("\n")},
        {u8("cross\n")},
        {u8("blast\n")},
        {u8("blast "), 0, u8(" with "), holdable, u8("\n")},
        {u8("fee\n")},
        {u8("fie\n")},
        {u8("foe\n")},
        {u8("foo\n")},
        {u8("abracadab\n")}
      },
      [] (const Vm &vm, const u8string &output) -> bool {
        return output.find(u8("You can't see")) != std::string::npos;
      }
    );
    */

    unordered_set<Node *> selectedNodes;
    unordered_set<Node *> verboseNodes;
    vector<NodeData> nodesByIndex;
    unordered_map<Node *, size_t> nodeIndices;
    bool elideDeadEndNodes = false;
    u8string line;
    while (true) {
      system("cls");

      if (nodesByIndex.empty()) {
        nodeIndices.clear();
        studyNodes(multiverse, nodesByIndex, nodeIndices);
        for (unordered_set<Node *> *nodes: {&selectedNodes, &verboseNodes}) {
          nodes->clear();
        }
      }

      {
        FILE *out = stdout;
        if (outPathName) {
          out = fopen(outPathName, "wb");
          if (!out) {
            throw PlainException(u8("unable to open output file"));
          }
        }
        auto _ = autoinf::finally([&] {
          if (outPathName) {
            fclose(out);
          }
        });

        printNode(multiverse.getNode(NodePath()), multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, out);
      }

      printf(
        "_Quit\n"
        "Show All _Nodes     Hide All _Dead End Nodes\n"
        "Select _All         _Clear Selection    _Invert Selection\n"
        "_Show Output        _Hide Output\n"
        "_Process            Co_llapse           _Terminate\n"
        ">"
      );
      do {
        line.clear();
        readLine(line);
      } while (line.empty() || line[0] == '#');
      if (line == u8("Q") || line == u8("q")) {
        break;
      } else if (line == u8("N") || line == u8("n")) {
        elideDeadEndNodes = false;
      } else if (line == u8("D") || line == u8("d")) {
        elideDeadEndNodes = true;
      } else if (line == u8("A") || line == u8("a")) {
        selectedNodes.clear();
        for (const auto &d : nodesByIndex) {
          selectedNodes.insert(d.node);
        }
      } else if (line == u8("C") || line == u8("c")) {
        selectedNodes.clear();
      } else if (line == u8("I") || line == u8("i")) {
        decltype(selectedNodes) nextSelectedNodes;
        for (const auto &d : nodesByIndex) {
          if (selectedNodes.find(d.node) == selectedNodes.end()) {
            nextSelectedNodes.insert(d.node);
          }
        }
        selectedNodes = move(nextSelectedNodes);
      } else if (line == u8("S") || line == u8("s")) {
        verboseNodes.insert(selectedNodes.cbegin(), selectedNodes.cend());
      } else if (line == u8("H") || line == u8("h")) {
        for (Node *node : selectedNodes) {
          verboseNodes.erase(node);
        }
      } else if (line == u8("P") || line == u8("p")) {
        Node *t[nodesByIndex.size()];
        Node **tI = t;
        for (const auto &d : nodesByIndex) {
          Node *n = d.node;
          if (selectedNodes.find(n) != selectedNodes.end()) {
            *(tI++) = n;
          }
        }
        multiverse.processNodes(t, tI, vm);
        nodesByIndex.clear();
      } else if (line == u8("L") || line == u8("l")) {
        if (selectedNodes.cbegin() != selectedNodes.cend()) {
          multiverse.collapseNodes(selectedNodes.cbegin(), selectedNodes.cend(), vm);
        }
        nodesByIndex.clear();
      } else if (line == u8("T") || line == u8("t")) {
        for (Node *node : selectedNodes) {
          node->clearState();
        }
        nodesByIndex.clear();
      } else {
        const char8_t *lineI = line.data();
        const char8_t *lineEnd = lineI + line.size();
        while (true) {
          for (; lineI != lineEnd && *lineI == ' '; ++lineI);
          const char8_t *numBegin = lineI;
          if (numBegin == lineEnd) {
            break;
          }
          for (; lineI != lineEnd && *lineI != ' '; ++lineI);
          const char8_t *numEnd = lineI;

          is n = getNaturalNumber(numBegin, numEnd);
          size_t nn;
          if (n >= 0 && (nn = static_cast<size_t>(n)) < nodesByIndex.size()) {
            Node *node = nodesByIndex[nn].node;
            auto pos = selectedNodes.find(node);
            if (pos == selectedNodes.end()) {
              selectedNodes.insert(node);
            } else {
              selectedNodes.erase(pos);
            }
          }
        }
      }
    }

    printf("Time spent in VM (over and above init): %f secs\n", vm.getTime());

    return 0;
  } catch (exception &e) {
    fprintf(stderr, "Error: %s\n", core::createExceptionMessage(e, false).c_str());
    return 1;
  }
}

void studyNodes (const Multiverse &multiverse, vector<NodeData> &r_nodesByIndex, unordered_map<Node *, size_t> &r_nodeIndices) {
  DPRE(r_nodesByIndex.empty(), "");
  DPRE(r_nodeIndices.empty(), "");
  DS();

  iu depth = 0;
  size_t s0;
  size_t s1 = 0;
  Node *rootNode = multiverse.getNode(NodePath());
  do {
    DW(, "studying nodes at depth ",depth);
    s0 = s1;
    studyNode(0, depth, rootNode, nullptr, Multiverse::NON_ID, r_nodesByIndex, r_nodeIndices);
    s1 = r_nodesByIndex.size();
    DW(, "after studying nodes at depth ",depth,", we know about ",s1," nodes");
    ++depth;
  } while (s0 != s1);

  markDeadEndNodes(r_nodesByIndex, r_nodeIndices);
}

void studyNode (
  iu depth, iu targetDepth, Node *node, Node *parentNode, ActionId actionId,
  vector<NodeData> &r_nodesByIndex, unordered_map<Node *, size_t> &r_nodeIndices
) {
  DS();
  DW(, "looking at node with sig of hash ",node->getSignature().hash());
  DPRE(targetDepth >= depth, "");
  if (depth != targetDepth) {
    DW(, " not yet at the right depth");
    DPRE(find(r_nodeIndices, node) != nullptr, "");
    for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
      DW(, " looking at child ",i);
      auto &child = node->getChild(i);
      ActionId childActionId = get<0>(child);
      Node *childNode = get<2>(child);

      auto c = find(r_nodeIndices, childNode);
      bool skip = true;
      if (!c) {
        skip = false;
      } else {
        const NodeData &childNodeData = r_nodesByIndex[*c];
        if (childNodeData.primeParent.node == node && childNodeData.primeParent.actionId == childActionId) {
          skip = false;
        }
      }
      if (!skip) {
        studyNode(depth + 1, targetDepth, childNode, node, childActionId, r_nodesByIndex, r_nodeIndices);
      }
    }

    return;
  }

  DW(, " this node is at the right depth");
  DPRE(find(r_nodeIndices, node) == nullptr, "");
  size_t nodeIndex = r_nodesByIndex.size();
  r_nodesByIndex.push_back(NodeData{node, {parentNode, actionId}, false});
  r_nodeIndices.emplace(node, nodeIndex);
  return;
}

void markDeadEndNodes (vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices) {
  for (auto i = nodesByIndex.rbegin(), end = nodesByIndex.rend(); i != end; ++i) {
    NodeData &nodeData = *i;
    Node *node = nodeData.node;
    if (node->getState()) {
      continue;
    }

    bool interesting = false;
    for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
      auto &child = node->getChild(i);
      ActionId childActionId = get<0>(child);
      Node *childNode = get<2>(child);

      NodeData &childNodeData = nodesByIndex[*find(nodeIndices, childNode)];
      if (childNodeData.primeParent.node == node && childNodeData.primeParent.actionId == childActionId && !childNodeData.allChildrenAreNonPrime) {
        interesting = true;
        break;
      }
    }
    if (interesting) {
      continue;
    }

    nodeData.allChildrenAreNonPrime = true;
  }
}

void printNode (
  Node *rootNode,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, FILE *out
) {
  u8string prefix;
  printNonleafNode(nullptr, rootNode, nullptr, Multiverse::NON_ID, multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, prefix, out);
}

u8string renderActionInput (ActionId actionId, const Multiverse &multiverse) {
  u8string actionInput;
  if (actionId != Multiverse::NON_ID) {
    actionInput.push_back(U'"');
    multiverse.getActionInput(actionId, actionInput);
    actionInput.erase(actionInput.size() - 1);
    actionInput.append(u8("\" -> "));
  }
  return actionInput;
}

void printLeafNode (
  const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, u8string &r_prefix, FILE *out
) {
  // XXXX 'map contains' fn as well? also 'find an entry known to be present'? just subclass unordered_*<> (as we will soon do for basic_string) ???
  bool selected = selectedNodes.find(node) != selectedNodes.end();
  if (selected) {
    fprintf(out, "* ");
  }

  fprintf(
    out,
    "%ssig of hash %u (elsewhere)\n",
    renderActionInput(actionId, multiverse).c_str(),
    node->getSignature().hash()
  );
}

void printNonleafNode (
  const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, u8string &r_prefix, FILE *out
) {
  bool selected = selectedNodes.find(node) != selectedNodes.end();
  if (selected) {
    fprintf(out, "* ");
  }

  size_t nodeIndex = *find(nodeIndices, node);

  fprintf(
    out,
    "(%u) %ssig of hash %u / state size %d / %u children\n",
    nodeIndex,
    renderActionInput(actionId, multiverse).c_str(),
    /* node->getSignature().wrXXXX().c_str(), */
    node->getSignature().hash(),
    node->getState() != nullptr ? node->getState()->getSize() : static_cast<size_t>(-1),
    node->getChildrenSize()
  );
  if (output) {
    u8string o(*output);
    char8_t c;
    while ((c = o.back()) == U'\n' || c == U'>') {
      o.pop_back();
    }
    o.push_back(U'\n');

    auto lineStart = o.cbegin();
    for (auto i = o.cbegin(), end = o.cend(); i != end; ++i) {
      if (*i == U'\n') {
        fprintf(out, "%s  %s\n", r_prefix.c_str(), u8string(lineStart, i).c_str());
        lineStart = i + 1;
      }
    }
  }

  enum {
    NONE,
    LEAF,
    NONLEAF
  } fmts[node->getChildrenSize()];
  size_t fmtsEnd = 0;
  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    auto &child = node->getChild(i);
    ActionId childActionId = get<0>(child);
    Node *childNode = get<2>(child);
    DA(node->getChildByActionId(childActionId) == &child);
    const NodeData &childNodeData = nodesByIndex[*find(nodeIndices, childNode)];

    auto fmt = NONE;
    if (elideDeadEndNodes) {
      if (childNodeData.primeParent.node != node || childNodeData.primeParent.actionId != childActionId || childNodeData.allChildrenAreNonPrime) {
        fmt = NONE;
      } else {
        fmt = NONLEAF;
      }
    } else {
      if (childNodeData.primeParent.node != node || childNodeData.primeParent.actionId != childActionId) {
        fmt = LEAF;
      } else {
        fmt = NONLEAF;
      }
    }
    fmts[i] = fmt;
    if (fmt != NONE) {
      fmtsEnd = i + 1;
    }
  }
  for (size_t i = 0; i != fmtsEnd; ++i) {
    if (fmts[i] == NONE) {
      continue;
    }
    bool last = i == fmtsEnd - 1;

    auto &child = node->getChild(i);
    ActionId childActionId = get<0>(child);
    const u8string &childOuput = get<1>(child);
    Node *childNode = get<2>(child);

    fprintf(out, "%s%c-> ", r_prefix.c_str(), last ? '+' : '|');
    r_prefix.append(last ? u8("    ") : u8("|   "));
    (fmts[i] == LEAF ? printLeafNode : printNonleafNode)(verboseNodes.find(childNode) != verboseNodes.end() ? &childOuput : nullptr, childNode, node, childActionId, multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, r_prefix, out);
    r_prefix.resize(r_prefix.size() - 4);
  }
}

size_t readLine (char8_t *b, size_t bSize) {
  // TODO convert input from native
  fgets(reinterpret_cast<char *>(b), static_cast<int>(bSize), stdin);
  size_t size = 0;
  for (; b[size] != 0; ++size) {
    if (b[size] >= 128) {
      b[size] = '?';
    }
  }
  if (size > 0 && b[size - 1] == '\n') {
    b[--size] = '\0';
  }
  return size;
}

void readLine (u8string &r_out) {
  char8_t b[1024];
  size_t size = readLine(b, sizeof(b));
  r_out.append(b, size);
}

is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd) {
  char8_t in[iEnd - iBegin + 1];
  copy(iBegin, iEnd, in);
  in[iEnd - iBegin] = U'\0';

  const char8_t *inBegin = in;
  const char8_t *inEnd = inBegin + (iEnd - iBegin);
  char8_t *numberEnd;
  long number = strtol(reinterpret_cast<const char *>(inBegin), reinterpret_cast<char **>(&numberEnd), 10);

  if (numberEnd != inEnd || number < 0 || number == LONG_MAX || number > numeric_limits<is>::max()) {
    return -1;
  }
  return static_cast<is>(number);
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
