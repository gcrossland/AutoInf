#include "autoinf.using"

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */


// XXXX move this lot to somewhere more core!!

template<typename _F> class Finally {
  prv _F functor;

  pub Finally (_F &&functor) : functor(move(functor)) {
  }

  pub ~Finally () noexcept {
    try {
      functor();
    } catch (...) {
      // XXXX
    }
  }
};

template<typename _F> Finally<_F> finally (_F &&functor) {
  return Finally<_F>(move(functor));
}

template<typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator, typename Key_> const T *find (const unordered_map<Key, T, Hash, KeyEqual, Allocator> &map, const Key_ &key) {
  auto e = map.find(key);
  return e == map.end() ? nullptr : &get<1>(*e);
}



template<typename ..._Ts> Multiverse::ActionTemplate::ActionTemplate (_Ts &&...ts) {
  DS();
  segments.reserve((sizeof...(_Ts) + 1) / 2);
  words.reserve((sizeof...(_Ts) - 1) / 2);
  init(forward<_Ts>(ts)...);
  DW(,"created an action template with segments");
  for (auto &s : segments) {
    DW(,"  ", s.c_str());
  }
  DW(,"and words");
  for (auto &w : words) {
    DW(,"  ", w);
  }
}

template<typename ..._Ts> void Multiverse::ActionTemplate::init (u8string &&segment, ActionWord::CategorySet word, _Ts &&...ts)
{
  segments.push_back(move(segment));
  words.push_back(word);
  init(forward<_Ts>(ts)...);
}

template<typename _I> void Multiverse::processNodes (_I nodesBegin, _I nodesEnd, Vm &r_vm) {
  typedef tuple<Node *, ActionId, u8string, Signature, State> result;
  DS();

  deque<result> resultQueue;
  mutex resultQueueLock;
  condition_variable resultQueueCondVar;

  packaged_task<void ()> dispatcher([&] () {
    DS();
    auto _ = finally([&] () {
      DS();
      DW(, "adding done result to queue");
      unique_lock<mutex> l(resultQueueLock);
      resultQueue.emplace_back(nullptr, 0, u8string(), Signature(), State());
      resultQueueCondVar.notify_one();
      DW(, "done adding done result to queue");
    });

    u8string input;
    u8string output;
    State postState;
    for (; nodesBegin != nodesEnd; ++nodesBegin) {
      DW(, "looking at the next node:");
      DS();
      Node *parentNode = *nodesBegin;
      DW(, "node has sig of hash ", parentNode->getSignature().hash(), " and ", parentNode->getChildrenSize(), " children");
      const State *parentState = parentNode->getState();
      if (!parentState) {
        DW(, "  node has already been processed, so skipping");
        continue;
      }
      DA(parentNode->getChildrenSize() == 0);

      Bitset dewordedWords;
      for (ActionId id = 0, end = actionSet.getSize(); id != end; ++id) {
        ActionSet::Action action = actionSet.get(id);

        if (action.includesAnyWords(dewordedWords)) {
          DW(, "was about to process action of id ",id,", but at least one of the words used in the action is in the deworded set");
          continue;
        }

        input.clear();
        action.getInput(input);
        DW(, "processing action **",input.c_str(),"** (id ",id,")");
        output.clear();

        doRestoreAction(r_vm, *parentState);
        doAction(r_vm, input, output, u8("VM died while doing action"));
        Signature signature = createSignature(r_vm, ignoredByteRangeset);

        auto dewordingWord = action.getDewordingTarget();
        if (dewordingWord != NON_ID) {
          DW(, "this action is a dewording one (for word of id ",dewordingWord,")");
          if (deworder(r_vm, output)) {
            DW(, "word of id ",dewordingWord," is missing!");
            dewordedWords.setBit(dewordingWord);
          }
        }

        if (signature == parentNode->getSignature()) {
          DW(, "the resultant VM state is the same as the parent's, so skipping");
          continue;
        }

        DW(, "output from the action is **", output.c_str(), "**");
        try {
          // XXXX better response from doSaveAction on 'can't save'?
          doSaveAction(r_vm, postState);
        } catch (...) {
          postState.clear();
        }
        {
          DW(, "adding the result to queue");
          unique_lock<mutex> l(resultQueueLock);
          resultQueue.emplace_back(parentNode, id, output, move(signature), postState);
          resultQueueCondVar.notify_one();
          DW(, "done adding the result to queue");
        }
        // system("sleep 1"); // XXXX
      }
    }
  });
  auto dispatcherFuture = dispatcher.get_future();
  thread dispatcherThread(move(dispatcher));
  auto _ = finally([&dispatcherThread] () {
    dispatcherThread.join();
  });

  Node *prevParentNode = nullptr;
  while (true) {
    unique_lock<mutex> l(resultQueueLock);
    resultQueueCondVar.wait(l, [&] () {
      return !resultQueue.empty();
    });
    DW(, "M getting the next result:");
    result rs = move(resultQueue.front());
    resultQueue.pop_front();
    l.unlock();

    Node *parentNode = get<0>(rs);
    if (prevParentNode && parentNode != prevParentNode) {
      prevParentNode->batchOfChildChangesCompleted();
      prevParentNode->clearState();
      metric.get()->nodeProcessed(*this, *prevParentNode);
    }
    prevParentNode = parentNode;
    if (!parentNode) {
      DW(, "M it's the end of the queue!");
      break;
    }
    DW(, "M the result is a child for the node with sig of hash ", parentNode->getSignature().hash());
    ActionId parentActionId = get<1>(rs);
    u8string &resultOutput = get<2>(rs);
    Signature &resultSignature = get<3>(rs);
    State &resultState = get<4>(rs);
    DW(, "M the child is for the action of id ",parentActionId,"; the sig is of hash ", resultSignature.hash());
    DA(!(resultSignature == parentNode->getSignature()));

    Node *resultNode;
    auto v = find(nodes, resultSignature);
    if (!v) {
      DW(, "M this is a new node for this multiverse!");
      unique_ptr<Metric::State> metricState(metric.get()->nodeCreated(*this, parentActionId, resultOutput, resultSignature));
      unique_ptr<Node> n(new Node(move(resultSignature), move(resultState), move(metricState)));
      resultNode = n.get();
      nodes.emplace(ref(resultNode->getSignature()), resultNode);
      n.release();
    } else {
      DW(, "M there already exists a node of this signature");
      resultNode = *v;
      DA(resultSignature == resultNode->getSignature());
    }
    parentNode->addChild(parentActionId, move(resultOutput), resultNode);
  }

  dispatcherFuture.get();

  metric.get()->nodesProcessed(*this, *rootNode, nodes);
}

template<typename _I> Bitset Multiverse::createExtraIgnoredBytes (const Signature &firstSignature, _I otherSignatureIsBegin, _I otherSignatureIsEnd, const Vm &vm) {
  Bitset extraIgnoredBytes;

  iu16 addr = 0;
  const Bitset &vmWordSet = *vm.getWordSet();
  for (auto byte : firstSignature) {
    bool same = true;
    for (auto otherSignatureIsI = otherSignatureIsBegin; otherSignatureIsI != otherSignatureIsEnd; ++otherSignatureIsI) {
      Signature::Iterator &i = *otherSignatureIsI;
      if (*(i++) != byte) {
        same = false;
      }
    }

    if (!same) {
      DW(, "the signatures don't all match at dynmem addr ", addr);
      extraIgnoredBytes.setBit(addr);

      if (vmWordSet.getExistingBit(addr)) {
        DW(, "  (it's the first byte of a zword)");
        extraIgnoredBytes.setBit(addr + 1);
      }
      if (addr > 0 && vmWordSet.getExistingBit(addr - 1)) {
        DW(, "  (it's the second byte of a zword)");
        extraIgnoredBytes.setBit(addr - 1);
      }
    }

    ++addr;
  }
  DA(addr == vm.getDynamicMemorySize());

  return extraIgnoredBytes;
}

template<typename _I> void Multiverse::collapseNodes (_I nodesBegin, _I nodesEnd, const Vm &vm) {
  DS();
  DPRE(nodesBegin != nodesEnd);
  DW(, "Collapsing nodes:");

  // Build the set of extra ignored bytes (those which aren't the same across all
  // of the given nodes).

  Node *firstNode = *(nodesBegin++);
  DW(, "first node has sig of hash", firstNode->getSignature().hash());
  const Signature &firstSignature = firstNode->getSignature();
  vector<Signature::Iterator> otherSignatureIs;
  for (; nodesBegin != nodesEnd; ++nodesBegin) {
    Node *node = *nodesBegin;
    DW(, "other node has sig of hash", node->getSignature().hash());
    otherSignatureIs.emplace_back(node->getSignature().begin());
  }
  Bitset extraIgnoredBytes = createExtraIgnoredBytes(firstSignature, otherSignatureIs.begin(), otherSignatureIs.end(), vm);

  // Walk through the node tree, processing each node once (unless all its parents
  // are collapsed away, in which case it'll never be reached), and rebuild the
  // node map with only those nodes which have survived (and with their new
  // signatures).

  Rangeset extraIgnoredByteRangeset(extraIgnoredBytes, vm.getDynamicMemorySize());
  decltype(nodes) survivingNodes(nodes.bucket_count());
  unordered_map<Node *, Node *> nodeCollapseTargets(nodes.bucket_count());
  unordered_map<Node *, Signature> survivingNodePrevSignatures(nodes.bucket_count());
  // XXXX should have an obj per surviving node to hold this + the child undo data i.e. 'surviving node undo object'

  Node *t = collapseNode(rootNode, extraIgnoredByteRangeset, survivingNodes, nodeCollapseTargets, survivingNodePrevSignatures);
  DW(, "after collapsing all ",nodes.size()," nodes, there are ",survivingNodes.size()," surviving nodes");

  ignoredBytes |= move(extraIgnoredBytes);
  ignoredByteRangeset = Rangeset(ignoredBytes, vm.getDynamicMemorySize());
  DA(t == rootNode);
  size_t dc = 0;
  for (const auto &i : nodes) {
    Node *node = get<1>(i);
    auto s = find(survivingNodePrevSignatures, node);
    if (!s) {
      delete node;
      ++dc;
    }
  }
  DW(, dc, " old nodes were deleted");
  nodes = move(survivingNodes);

  metric.get()->nodesCollapsed(*this, *rootNode, nodes);
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
