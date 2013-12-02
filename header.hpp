#ifndef HEADER_ALREADYINCLUDED
#define HEADER_ALREADYINCLUDED

#include <string>
#include "libraries/autoinf.hpp"
#include <unordered_set>

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
int main (int argc, char *argv[]);
void printNode (autoinf::Multiverse::NodePath topNodePath, const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes, std::vector<autoinf::Multiverse::NodePath> &r_nodesByOrder);
void printNode (autoinf::Multiverse::ActionId actionId, const std::string *output, autoinf::Multiverse::Node *node, autoinf::Multiverse::NodePath &r_nodePath, const autoinf::Multiverse &multiverse, const std::unordered_set<autoinf::Multiverse::Node *> &selectedNodes, const std::unordered_set<autoinf::Multiverse::Node *> &verboseNodes, std::vector<autoinf::Multiverse::NodePath> &r_nodesByOrder, std::unordered_set<autoinf::Multiverse::Node *> &r_seenNodes, std::string &r_prefix);
void readTextLine(std::string &r_out);
is getNaturalNumber(const std::string &in);

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
#endif
