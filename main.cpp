#include "header.hpp"
#include <cstring>
#include <unordered_set>

using autofrotz::Vm;
using autoinf::Multiverse;
using std::vector;
using std::string;
using NodePath = autoinf::Multiverse::NodePath;
using Node = autoinf::Multiverse::Node;
using ActionId = autoinf::Multiverse::ActionId;
using std::unordered_set;
using std::get;
using core::numeric_limits;
using std::exception;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

void terminator () {
  core::dieHard();
}

int main (int argc, char *argv[]) {
  std::set_terminate(&terminator);
  try {
    std::shared_ptr<core::debug::Stream> errs(new core::debug::Stream());
    DOPEN(, errs);
    autoinf::DOPEN(, errs);
    //autofrotz::DOPEN(, errs);
    //autofrotz::vmlink::DOPEN(, errs);

    string output;
    Vm vm("104.z5", 70, 128, 1, true, output);
    auto vmEnder = autoinf::finally([&vm, &output] () {
      vm.doAction("quit\ny\n", output); // XXXX do properly
    });

    Multiverse multiverse(
      vm, "verbose\nfullscore\n", output, "save\n\1\n", "restore\n\1\n",
      vector<string> {
        "red sphere", "blue sphere"
      },
      vector<vector<string>> {
        {"east\n"},
        {"west\n"},
        {"take ", "\n"},
        {"drop ", "\n"},
        {"open ", "\n"},
        {"turn wheel. pull wheel\n"},
        {"enter light\n"}

        // verbitudeise the tangerine monstrosity. verbitudeise the tangerine monstrosity. ...
        // z. z. z. z. z. z. z. z.
      }
    );

    unordered_set<Node *> selectedNodes;
    unordered_set<Node *> verboseNodes;
    string line;
    while (true) {
      system("cls");

      vector<NodePath> nodesByOrder;
      printNode(NodePath(), multiverse, selectedNodes, verboseNodes, nodesByOrder);

      printf("_Quit\nSelect _All        _Clear Selection\n_Show Output  _Hide Output\n_Process\n>");
      line.clear();
      readTextLine(line);
      if (line == "Q" || line == "q") {
        break;
      } else if (line == "A" || line == "a") {
        selectedNodes.clear();
        for (NodePath &nodePath : nodesByOrder) {
          selectedNodes.insert(multiverse.getNode(nodePath));
        }
      } else if (line == "C" || line == "c") {
        selectedNodes.clear();
      } else if (line == "S" || line == "s") {
        verboseNodes.insert(selectedNodes.cbegin(), selectedNodes.cend());
      } else if (line == "H" || line == "h") {
        for (Node *node : selectedNodes) {
          verboseNodes.erase(node);
        }
      } else if (line == "P" || line == "p") {
        multiverse.processNodes(selectedNodes.cbegin(), selectedNodes.cend(), vm);
      } else {
        is n = getNaturalNumber(line);
        if (n >= 0 && static_cast<size_t>(n) < nodesByOrder.size()) {
          Node *node = multiverse.getNode(nodesByOrder[static_cast<size_t>(n)]);
          auto pos = selectedNodes.find(node);
          if (pos == selectedNodes.end()) {
            selectedNodes.insert(node);
          } else {
            selectedNodes.erase(pos);
          }
        }
      }
    }

    return 0;
  } catch (exception &e) {
    fprintf(stderr, "Failure: %s\n", e.what());
    return 1;
  }
}

static ActionId NO_ID = static_cast<ActionId>(-1);

void printNode (NodePath topNodePath, const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes, vector<NodePath> &r_nodesByOrder) {
  unordered_set<Node *> seenNodes;
  string prefix;
  printNode(NO_ID, nullptr, multiverse.getNode(topNodePath), topNodePath, multiverse, selectedNodes, verboseNodes, r_nodesByOrder, seenNodes, prefix);
}

void printNode (
  ActionId actionId, const string *output, Node *node, NodePath &r_nodePath,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  vector<NodePath> &r_nodesByOrder, unordered_set<Node *> &r_seenNodes, string &r_prefix
) {
  DPRE(multiverse.getNode(r_nodePath) == node);

  bool selected = selectedNodes.find(node) != selectedNodes.end();
  if (selected) {
    printf("* ");
  }

  string actionInput;
  if (actionId != NO_ID) {
    auto actionInputRange = multiverse.getActionInput(actionId);
    actionInput.push_back('"');
    actionInput.append(get<0>(actionInputRange), get<1>(actionInputRange) - 1);
    actionInput.append("\" -> ");
  }

  bool nodeAlreadyPrinted = r_seenNodes.find(node) != r_seenNodes.end();
  if (nodeAlreadyPrinted) {
    printf(
      "%ssig of hash %u (above)\n",
      actionInput.c_str(),
      node->getSignature().hash()
    );
    return;
  }

  printf(
    "(%u) %ssig of hash %u / state size %d / %u children\n",
    r_nodesByOrder.size(),
    actionInput.c_str(),
    node->getSignature().hash(),
    node->getState() != nullptr ? node->getState()->getSize() : static_cast<size_t>(-1),
    node->getChildCount()
  );
  if (output) {
    string o(*output);
    char c;
    while ((c = o.back()) == '\n' || c == '>') {
      o.pop_back();
    }
    o.push_back('\n');

    auto lineStart = o.cbegin();
    for (auto i = o.cbegin(), end = o.cend(); i != end; ++i) {
      if (*i == '\n') {
        printf("%s  %s\n", r_prefix.c_str(), string(lineStart, i).c_str());
        lineStart = i + 1;
      }
    }
  }
  r_nodesByOrder.push_back(r_nodePath);
  r_seenNodes.insert(node);

  size_t end = node->getChildCount();
  for (size_t i = 0; i != end; ++i) {
    bool last = i == end - 1;

    auto &child = node->getChild(i);
    ActionId childActionId = get<0>(child);
    const string &childOuput = get<1>(child);
    Node *childNode = get<2>(child);
    DA(node->getChildByActionId(childActionId) == &child);

    printf("%s%c-> ", r_prefix.c_str(), last ? '+' : '|');
    r_nodePath.append(childActionId);
    r_prefix.append(last ? "    " : "|   ");
    printNode(childActionId, verboseNodes.find(childNode) != verboseNodes.end() ? &childOuput : nullptr, childNode, r_nodePath, multiverse, selectedNodes, verboseNodes, r_nodesByOrder, r_seenNodes, r_prefix);
    r_prefix.resize(r_prefix.size() - 4);
    r_nodePath.pop();
  }
}

size_t readLine (char *b, size_t bSize) {
  fgets(b, static_cast<int>(bSize), stdin);
  size_t size = strlen(b);
  if (size > 0 && b[size - 1] == '\n') {
    --size;
  }
  return size;
}

void readTextLine(string &r_out) {
  char b[1024];
  size_t size = readLine(b, sizeof(b));
  r_out.append(b, size);
}

is getNaturalNumber(const string &in) {
  const char *inBegin = in.c_str();
  char *numberEnd;
  long number = strtol(inBegin, &numberEnd, 10);

  if (numberEnd != inBegin + in.size() || number < 0 || number == LONG_MAX || number > numeric_limits<is>::max()) {
    return -1;
  }
  return static_cast<is>(number);
}
