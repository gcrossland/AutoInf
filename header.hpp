#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED

#include <string>
#include "libraries/autoinf.hpp"
#include <unordered_set>

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
int main (int argc, char *argv[]);

void studyNodes (const autoinf::Multiverse &multiverse, std::vector<autoinf::Multiverse::Node *> &r_nodesByIndex);
void studyNode (
  iu depth, iu targetDepth, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId childIndex,
  std::vector<autoinf::Multiverse::Node *> &r_nodesByIndex
);
void markDeadEndNodes (const std::vector<autoinf::Multiverse::Node *> &nodesByIndex);
void printNode (
  autoinf::Multiverse::Node *rootNode,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<autoinf::Multiverse::Node *> &nodesByIndex, bool elideDeadEndNodes, size_t maxDepth, FILE *out
);
void printNodeAsLeaf (
  size_t depth, const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<autoinf::Multiverse::Node *> &nodesByIndex, bool elideDeadEndNodes, size_t maxDepth, core::u8string &r_prefix, FILE *out
);
void printNodeAsNonleaf (
  size_t depth, const core::u8string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::Node *parentNode, autoinf::Multiverse::ActionId actionId,
  const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes,
  const std::vector<autoinf::Multiverse::Node *> &nodesByIndex, bool elideDeadEndNodes, size_t maxDepth, core::u8string &r_prefix, FILE *out
);

void readLine (core::u8string &r_out);
const char8_t *skipSpaces (const char8_t *i, const char8_t *end);
const char8_t *skipNonSpaces (const char8_t *i, const char8_t *end);
is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd);

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
#endif
