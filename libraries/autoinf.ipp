#include "autoinf.using"

namespace autoinf {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */



template<typename _F> class Finally {
  prv _F functor;

  // XXXX needed?
  pub Finally (const _F &functor) : functor(functor) {
  }

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
  DS();
  DW(, "sizeof the functor is ", sizeof(functor));
  return Finally<_F>(move(functor));
}

template<typename _F> Finally<_F> finally (const _F &functor) {
  DS();
  DW(, "sizeof the functor is ", sizeof(functor));
  return Finally<_F>(functor);
}




template<typename _I> void Multiverse::processNodes (_I nodePathsBegin, _I nodePathsEnd, Vm &vm) {
  typedef tuple<Node *, ActionId, string, Signature, State> result;
  DS();

  deque<result> resultQueue;
  bool resultQueueDone = false;
  mutex resultQueueLock;
  condition_variable resultQueueCondVar;

  packaged_task<void ()> dispatcher([&] () {
    DS();
    auto resultQueueEnder = finally([&] () {
      DS();
      DW(, "adding done result to queue");
      unique_lock<mutex> l(resultQueueLock);
      resultQueue.emplace_back(nullptr, 0, string{}, Signature{}, State{});
      resultQueueCondVar.notify_one();
      DW(, "done adding done result to queue");
    });

    State postState;
    for (; nodePathsBegin != nodePathsEnd; ++nodePathsBegin) {
      DW(, "looking at the next node:");
      DS();
      const NodePath &nodePath = *nodePathsBegin;
      Node *parentNode = nodePath.resolve(rootNode);
      if (!parentNode) {
        throw new GeneralException("invalid node path specified");
      }
      DW(, "node has sig of hash ", parentNode->getSignature().hash(), " and ", parentNode->getChildCount(), " children");
      const State *parentState = parentNode->getState();
      if (!parentState) {
        continue;
      }
      DA(parentNode->getChildCount() == 0);

      size_t actionCount = actionInputBegins.size() - 1;
      for (ActionId id = 0; id != actionCount; ++id) {
        auto actionInputBegin = actionInputs.cbegin() + actionInputBegins[id];
        auto actionInputEnd = actionInputs.cbegin() + actionInputBegins[id + 1];
        DW(, "processing action **",string(actionInputBegin, actionInputEnd).c_str(),"** (id ",id,")");

        doRestoreAction(vm, parentState);
        doAction(vm, actionInputBegin, actionInputEnd, "VM was dead after doing action");
        Signature signature = createSignature(vm);
        if (signature == parentNode->getSignature()) {
          DW(, "the resultant VM state is the same as the parent's, so skipping");
          continue;
        }
        string output(vm.getOutput()); // XXXX also make vm output an externally-provided buffer? probably... and make save state + restore state + output buffer passed at doAction? probably best, too...
        DW(, "output from the action is **",output.c_str(),"**");
        doSaveAction(vm, &postState);
        {
          DW(, "adding the result to queue");
          unique_lock<mutex> l(resultQueueLock);
          resultQueue.emplace_back(parentNode, id, move(output), move(signature), postState);
          resultQueueCondVar.notify_one();
          DW(, "done adding the result to queue");
        }
      }
    }
  });
  thread dispatcherThread(dispatcher);

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
    if (!parentNode) {
      DW(, "M it's the end of the queue!");
      break;
    }
    DW(, "M the result is a child for the node with sig of hash ", parentNode->getSignature().hash());
    if (prevParentNode && parentNode != prevParentNode) {
      prevParentNode->allChildrenAdded();
    }
    prevParentNode = parentNode;
    ActionId parentActionId = get<1>(rs);
    string &resultOutput = get<2>(rs);
    Signature &resultSignature = get<3>(rs);
    State &resultState = get<4>(rs);
    DW(, "M the child is for the action of id ",parentActionId,"; the sig is of hash ", resultSignature.hash());
    DA(!(resultSignature == parentNode->getSignature()));

    Node *resultNode;
    auto e = nodesBySignature.find(resultSignature);
    if (e == nodesBySignature.end()) {
      DW(, "M this is a new node for this multiverse!");
      unique_ptr<Node> n(new Node(42 /* XXXX */, move(resultSignature), move(resultState)));
      resultNode = n.get();
      nodesBySignature.emplace(ref(resultNode->getSignature()), resultNode);
      n.release();
    } else {
      DW(, "M there already exists a node of this signature");
      resultNode = get<1>(*e);
      DA(resultSignature == resultNode->getSignature());
    }
    parentNode->addChild(parentActionId, move(resultOutput), resultNode);
  }

  dispatcher.get_future().get();
  dispatcherThread.join();
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}
