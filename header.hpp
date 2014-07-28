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
  const std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices, bool elideDeadEndNodes, FILE *out
);
void printNodeAsLeaf (
  const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices, bool elideDeadEndNodes, core::u8string &r_prefix, FILE *out
);
void printNodeAsNonleaf (
  const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<NodeData> &nodesByIndex, const std::unordered_map<autoinf::Multiverse::Node *, size_t> &nodeIndices, bool elideDeadEndNodes, core::u8string &r_prefix, FILE *out
);

void readLine (core::u8string &r_out);
const char8_t *skipSpaces (const char8_t *i, const char8_t *end);
const char8_t *skipNonSpaces (const char8_t *i, const char8_t *end);
is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd);

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
#endif
