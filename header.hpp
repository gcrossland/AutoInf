#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED

#include <string>
#include "libraries/autoinf.hpp"
#include <unordered_set>

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
int main (int argc, char *argv[]);

struct NodeData {
  autoinf::Multiverse::Node *node;
  struct {
    autoinf::Multiverse::Node *node;
    autoinf::Multiverse::ActionId actionId;
  } primeParent;
  bool allChildrenAreNonPrime;
};

void studyNodes (const autoinf::Multiverse &multiverse, std::vector<NodeData> &r_nodesByIndex, std::unordered_map<autoinf::Multiverse::Node *, size_t> &r_nodeIndices);
void studyNode (
  iu depth, iu targetDepth, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  std::vector<NodeData> &r_nodesByIndex, std::unordered_map<autoinf::Multiverse::Node *, size_t> &r_nodeIndices
);
void markDeadEndNodes (std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices);
void printNode (
  autoinf::Multiverse::Node *rootNode,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices, bool elideDeadEndNodes
);
void printLeafNode (
  const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices, bool elideDeadEndNodes, core::u8string &r_prefix
);
void printNonleafNode (
  const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices, bool elideDeadEndNodes, core::u8string &r_prefix
);

void readLine(core::u8string &r_out);
is getNaturalNumber(const core::u8string &in);

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
#endif
