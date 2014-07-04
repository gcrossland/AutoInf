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
using std::unordered_set;
using std::get;
using core::numeric_limits;
using std::exception;
using std::unordered_map;
using std::move;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

void terminator () {
  core::dieHard();
}

int main (int argc, char *argv[]) {
  std::set_terminate(&terminator);
  try {
    std::shared_ptr<core::debug::Stream> errs(new core::debug::Stream("LOG.TXT"));
    DOPEN(, errs);
    autoinf::DOPEN(, errs);
    //autofrotz::DOPEN(, errs);
    //autofrotz::vmlink::DOPEN(, errs);

    u8string output;
    Vm vm("104/104.z5", 70, 128, 1, true, output);
    Multiverse multiverse(
      vm, u8("verbose\nfullscore\n"), output, u8("save\n\1\n"), u8("restore\n\1\n"),
      vector<vector<u8string>> {
        // {u8("z\n"), u8("z. z. z. z. z. z. z. z.\n")},
        // {u8("verbitudeise the tangerine monstrosity. verbitudeise the tangerine monstrosity.\n"), u8("")},
        // {u8("turn wheel. pull wheel.\n"), u8("turn wheel. pull wheel. east. west.\n"), u8("turn wheel. pull wheel. west. east.\n")},
        // {u8("\n"), u8("east\n"), u8("west\n"), u8("take red sphere\n"), u8("take blue sphere\n"), u8("drop red sphere\n"), u8("drop blue sphere\n"), u8("open red sphere\n"), u8("open blue sphere\n"), u8("enter light\n")}
      },
      vector<u8string> {
        u8("red sphere"), u8("blue sphere")
      },
      vector<vector<u8string>> {
        {u8("east\n")},
        {u8("west\n")},
        {u8("take "), u8("\n")},
        {u8("drop "), u8("\n")},
        {u8("open "), u8("\n")},
        {u8("turn wheel. pull wheel\n")},
        {u8("enter light\n")}
      }
    );
    /*
    Vm vm("103/103.z5", 70, 128, 1, true, output);
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

      printNode(multiverse.getNode(NodePath()), multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes);

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
        is n = getNaturalNumber(line);
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

    printf("Time spent in VM (over and above init): %f secs\n", vm.getTime());

    return 0;
  } catch (exception &e) {
    fprintf(stderr, "Error: %s\n", core::createExceptionMessage(e, false).c_str());
    return 1;
  }
}

static ActionId NO_ID = static_cast<ActionId>(-1);

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
    studyNode(0, depth, rootNode, nullptr, NO_ID, r_nodesByIndex, r_nodeIndices);
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
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes
) {
  u8string prefix;
  printNonleafNode(nullptr, rootNode, nullptr, NO_ID, multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, prefix);
}

u8string renderActionInput (ActionId actionId, const Multiverse &multiverse) {
  u8string actionInput;
  if (actionId != NO_ID) {
    auto actionInputRange = multiverse.getActionInput(actionId);
    actionInput.push_back(U'"');
    actionInput.append(get<0>(actionInputRange), get<1>(actionInputRange) - 1);
    actionInput.append(u8("\" -> "));
  }
  return actionInput;
}

void printLeafNode (
  const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, u8string &r_prefix
) {
  // XXXX 'map contains' fn as well? also 'find an entry known to be present'? just subclass unordered_*<> (as we will soon do for basic_string) ???
  bool selected = selectedNodes.find(node) != selectedNodes.end();
  if (selected) {
    printf("* ");
  }

  printf(
    "%ssig of hash %u (elsewhere)\n",
    renderActionInput(actionId, multiverse).c_str(),
    node->getSignature().hash()
  );
}

void printNonleafNode (
  const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, u8string &r_prefix
) {
  bool selected = selectedNodes.find(node) != selectedNodes.end();
  if (selected) {
    printf("* ");
  }

  size_t nodeIndex = *find(nodeIndices, node);

  printf(
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
        printf("%s  %s\n", r_prefix.c_str(), u8string(lineStart, i).c_str());
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

    printf("%s%c-> ", r_prefix.c_str(), last ? '+' : '|');
    r_prefix.append(last ? u8("    ") : u8("|   "));
    (fmts[i] == LEAF ? printLeafNode : printNonleafNode)(verboseNodes.find(childNode) != verboseNodes.end() ? &childOuput : nullptr, childNode, node, childActionId, multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, r_prefix);
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

is getNaturalNumber (const u8string &in) {
  const char8_t *inBegin = in.c_str();
  char8_t *numberEnd;
  long number = strtol(reinterpret_cast<const char *>(inBegin), reinterpret_cast<char **>(&numberEnd), 10);

  if (numberEnd != inBegin + in.size() || number < 0 || number == LONG_MAX || number > numeric_limits<is>::max()) {
    return -1;
  }
  return static_cast<is>(number);
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
