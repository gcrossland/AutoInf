#include "header.hpp"
#include <cstring>
#include <unordered_set>
#include <typeinfo>

using autofrotz::Vm;
using autoinf::Multiverse;
using std::vector;
using core::u8string;
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
using autoinf::Signature;
using Metric = autoinf::Multiverse::Metric;
using std::unique_ptr;
using std::unordered_map;
using std::reference_wrapper;
using bitset::Bitset;
using std::tuple;
using std::sort;
using std::min;
using std::find;
using autoinf::Serialiser;
using autoinf::FileOutputIterator;
using std::type_info;
using autoinf::Deserialiser;
using autoinf::FileInputIterator;
using autofrotz::zword;
using autofrotz::zbyte;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

void terminator () {
  core::dieHard();
}

class WordUsageMetric : public virtual Metric {
  pub class State : public Metric::State {
    prv Bitset interestingChildActionWords;
    prv size_t wordValue;
    prv size_t score;
    pub static constexpr size_t NON_VALUE = static_cast<size_t>(-1);

    pub State () : wordValue(NON_VALUE), score(0) {
    }

    prt template<typename _Walker> void beWalked (_Walker &w) {
      DS();
      w.process(interestingChildActionWords);
      w.process(wordValue);
      w.process(score);
    }

    State (const State &) = delete;
    State &operator= (const State &) = delete;
    State (State &&) = delete;
    State &operator= (State &&) = delete;

    pub size_t getValue (size_t i) override {
      DPRE(i < 2);
      return i == 0 ? wordValue : score;
    }

    friend class WordUsageMetric;
  };

  pub const zword scoreAddr;

  pub WordUsageMetric (zword scoreAddr) : scoreAddr(scoreAddr) {
  }

  pub tuple<void *, size_t> deduceStateType (Metric::State *state) override {
    DPRE(!!dynamic_cast<State *>(state));
    return tuple<void *, size_t>(static_cast<State *>(state), sizeof(State));
  }

  pub tuple<Metric::State *, void *, size_t> constructState () override {
    State *state = new State();
    return tuple<Metric::State *, void *, size_t>(state, static_cast<void *>(state), sizeof(*state));
  }

  pub void walkState (Metric::State *state, Serialiser<FileOutputIterator> &s) override {
    static_cast<State *>(state)->beWalked(s);
  }

  pub void walkState (Metric::State *state, Deserialiser<FileInputIterator> &s) override {
    static_cast<State *>(state)->beWalked(s);
  }

  prv void updateInterestingChildActionWords (const Multiverse::ActionSet &actionSet, const Node &node, Bitset &r_words) {
    DPRE(!node.getState());

    r_words.clear();
    for (size_t i = 0, end = node.getChildrenSize(); i != end; ++i) {
      auto action = actionSet.get(get<0>(node.getChild(i)));
      for (size_t i = 0, end = action.getWordCount(); i != end; ++i) {
        r_words.setBit(action.getWord(i));
      }
    }
    r_words.compact();
  }

  pub unique_ptr<Metric::State> nodeCreated (const Multiverse &multiverse, ActionId parentActionId, const u8string &output, const Signature &signature, const Vm &vm) override {
    char b[10];
    sprintf(b, "&%08X", signature.hash());
    DW(, "DDDD created new node with sig of hash ",b);
    unique_ptr<State> state(new State());

    DPRE((static_cast<iu>(scoreAddr) + 1) < vm.getDynamicMemorySize() + 0);
    const zbyte *m = vm.getDynamicMemory();
    state->score = static_cast<zword>(static_cast<zword>(m[scoreAddr] << 8) | m[scoreAddr + 1]);

    return unique_ptr<Metric::State>(move(state));
  }

  pub void nodeProcessed (const Multiverse &multiverse, Node &r_node) override {
    const Multiverse::ActionSet &actionSet = multiverse.getActionSet();

    updateInterestingChildActionWords(actionSet, r_node, static_cast<State *>(r_node.getMetricState())->interestingChildActionWords);
  }

  prv void calculateNodeWordValue (Node *node, size_t parentNodeValue, size_t nodesSize, const size_t *wordCounts) {
    State *state = static_cast<State *>(node->getMetricState());
    if (state->wordValue != State::NON_VALUE) {
      return;
    }
    char b[10];
    sprintf(b, "&%08X", node->getSignature().hash());
    DW(, "DDDD calculating node word value for node with sig of hash ",b);

    size_t value;
    if (node->getState()) {
      value = 0;
    } else {
      Bitset &words = state->interestingChildActionWords;
      value = 0;
      for (size_t i = words.getNextSetBit(0); i != Bitset::NON_INDEX; i = words.getNextSetBit(i + 1)) {
        DW(, "       action word of id ", i);
        value += nodesSize / wordCounts[i];
      }
    }
    DW(, "       final local word value is ", value);
    DW(, "       (parent word value is ", parentNodeValue,")");
    value += parentNodeValue;
    state->wordValue = value;

    for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
      calculateNodeWordValue(get<2>(node->getChild(i)), value, nodesSize, wordCounts);
    }
  }

  prv void nodesChanged (bool updateWords, const Multiverse &multiverse, Node &r_rootNode, unordered_map<reference_wrapper<const Signature>, Node *, autoinf::Hasher<Signature>> &r_nodes) {
    DS();
    DW(, "DDDD nodes have changed!");
    const Multiverse::ActionSet &actionSet = multiverse.getActionSet();
    size_t nodesSize = r_nodes.size();

    size_t wordCounts[actionSet.getWordsSize()];
    for (auto &c : wordCounts) {
      c = 0;
    }

    for (auto &entry : r_nodes) {
      Node *node = get<1>(entry);
      State *state = static_cast<State *>(node->getMetricState());
      state->wordValue = State::NON_VALUE;
      if (node->getState()) {
        continue;
      }
      Bitset &words = state->interestingChildActionWords;
      if (updateWords) {
        updateInterestingChildActionWords(actionSet, *node, words);
      }
      for (size_t i = words.getNextSetBit(0); i != Bitset::NON_INDEX; i = words.getNextSetBit(i + 1)) {
        ++wordCounts[i];
      }
    }
    DW(, "DDDD word counts:");
    for (size_t i = 0, end = actionSet.getWordsSize(); i != end; ++i) {
      DW(, "DDDD   ",actionSet.getWord(i).c_str()," - ",wordCounts[i]);
    }

    // XXXX walk in a bredth first fashion (like prime parent)?
    calculateNodeWordValue(&r_rootNode, 0, nodesSize, wordCounts);
  }

  pub void nodesProcessed (const Multiverse &multiverse, Node &r_rootNode, unordered_map<reference_wrapper<const Signature>, Node *, autoinf::Hasher<Signature>> &r_nodes) override {
    nodesChanged(false, multiverse, r_rootNode, r_nodes);
  }

  pub void nodesCollapsed (const Multiverse &multiverse, Node &r_rootNode, unordered_map<reference_wrapper<const Signature>, Node *, autoinf::Hasher<Signature>> &r_nodes) override {
    nodesChanged(true, multiverse, r_rootNode, r_nodes);
  }

  pub size_t getValueCount () const override {
    return 2;
  }
};

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
    const u8string saveActionInput(u8("save\n\1\n"));
    const u8string restoreActionInput(u8("restore\n\1\n"));
    u8string output;
    Vm vm("104/104.z5", 70, height, undoDepth, true, output);
    Multiverse multiverse(
      vm, u8("verbose\nfullscore\n"), output,
      [&saveActionInput] (Vm &r_vm) -> bool {
        u8string o;
        Multiverse::doAction(r_vm, saveActionInput, o, u8("VM died while saving a state"));
        return r_vm.getSaveCount() != 0;
      },
      [&restoreActionInput] (Vm &r_vm) -> bool {
        u8string o;
        Multiverse::doAction(r_vm, restoreActionInput, o, u8("VM died while restoring a state"));
        return r_vm.getRestoreCount() != 0;
      },
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
        {u8("wheel"), 0b101},
        {u8("light"), 0b1001},
      },
      vector<ActionTemplate> {
        {u8("examine "), 0b001U, u8("\n")},
      },
      vector<ActionTemplate> {
        {u8("east\n")},
        {u8("west\n")},
        {u8("take "), 0b010U, u8("\n")},
        {u8("drop "), 0b010U, u8("\n")},
        {u8("open "), 0b010U, u8("\n")},
        {u8("turn "), 0b100U, u8(". pull "), 0b100U, u8("\n")},
        {u8("enter "), 0b1000U, u8("\n")}
      },
      [] (const Vm &vm, const u8string &output) -> bool {
        return output.find(u8("You can't see")) != std::string::npos;
      },
      unique_ptr<Metric>(new WordUsageMetric(0x08C8))
    );
    /*
    Vm vm("advent/advent.z5", 70, height, undoDepth, true, output);
    const u8string noResurrectionSaveActionInput(u8("no\n") + saveActionInput);
    const u8string noResurrectionRestoreActionInput(u8("no\n") + restoreActionInput);
    iu c = 0;
    const ActionWord::CategorySet direction = 1U << (c++);
    const ActionWord::CategorySet noun = 1U << (c++);
    const ActionWord::CategorySet  mobile = 1U << (c++);
    const ActionWord::CategorySet  holdable = 1U << (c++);
    const ActionWord::CategorySet  supporter = 1U << (c++);
    const ActionWord::CategorySet  container = 1U << (c++);
    const ActionWord::CategorySet  lockable = 1U << (c++);
    const ActionWord::CategorySet  locker = 1U << (c++);
    const ActionWord::CategorySet  unlocker = 1U << (c++);
    const ActionWord::CategorySet  openable = 1U << (c++);
    const ActionWord::CategorySet  animate = 1U << (c++);
    const ActionWord::CategorySet  clothing = 1U << (c++);
    const ActionWord::CategorySet  edible = 1U << (c++);
    const ActionWord::CategorySet  switchable = 1U << (c++);
    const ActionWord::CategorySet  readable = 1U << (c++);
    const ActionWord::CategorySet  flammable = 1U << (c++);
    const ActionWord::CategorySet  attachable = 1U << (c++);
    Multiverse multiverse(
      vm, u8("verbose\nfullscore\n"), output,
      [&saveActionInput, &noResurrectionSaveActionInput] (Vm &r_vm) -> bool {
        u8string o;
        Multiverse::doAction(r_vm, saveActionInput, o, u8("VM died while saving a state"));

        if (r_vm.getSaveCount() == 0 && o.find(u8("Please answer yes or no.")) != u8string::npos) {
          Multiverse::doAction(r_vm, noResurrectionSaveActionInput, o, u8("VM died while declining resurrection and saving a state"));
        }

        return r_vm.getSaveCount() != 0;
      },
      [&restoreActionInput, &noResurrectionRestoreActionInput] (Vm &r_vm) -> bool {
        u8string o;
        Multiverse::doAction(r_vm, restoreActionInput, o, u8("VM died while restoring a state"));

        if (r_vm.getRestoreCount() == 0 && o.find(u8("Please answer yes or no.")) != u8string::npos) {
          Multiverse::doAction(r_vm, noResurrectionRestoreActionInput, o, u8("VM died while declining resurrection and restoring a state"));
        }

        return r_vm.getRestoreCount() != 0;
      },
      vector<vector<u8string>> {
      },
      vector<ActionWord> {
        {u8("north"), direction},
        {u8("south"), direction},
        {u8("east"), direction},
        {u8("west"), direction},
        {u8("northeast"), direction},
        {u8("northwest"), direction},
        {u8("southeast"), direction},
        {u8("southwest"), direction},
        {u8("up above"), direction},
        {u8("ground"), direction},
        {u8("inside"), direction},
        {u8("outside"), direction},
        {u8("hill"), noun | supporter},
        {u8("other side of hill"), noun | supporter},
        {u8("spring"), noun | container | edible},
        {u8("pipes"), noun | container},
        {u8("keys"), noun | mobile | holdable | locker | unlocker},
        {u8("tasty food"), noun | mobile | holdable | edible},
        {u8("brass lantern"), noun | mobile | holdable | container | openable | switchable},
        {u8("bottle"), noun | mobile | holdable | container | edible},
        {u8("streambed"), noun | container},
        {u8("slit"), noun | mobile},
        {u8("depression"), noun},
        {u8("grate"), noun | lockable | openable},
        {u8("cobbles"), noun},
        {u8("wicker cage"), noun | mobile | holdable | container | lockable | openable | flammable},
        {u8("debris"), noun | mobile | holdable},
        {u8("note"), noun | mobile | holdable | readable},
        {u8("rod"), noun | mobile | holdable | unlocker},
        {u8("bird"), noun | holdable | animate},
        {u8("pit"), noun | container},
        {u8("crack"), noun | container},
        {u8("wide stone staircase"), noun | supporter},
        {u8("rough stone steps"), noun | supporter},
        {u8("dome"), noun},
        {u8("large gold nugget"), noun | mobile | holdable},
        {u8("diamonds"), noun | mobile | holdable},
        {u8("bridge"), noun | supporter | flammable},
        {u8("fissure"), noun | container},
        {u8("crossover"), noun | supporter},
        {u8("snake"), noun | holdable | animate},
        {u8("bars of silver"), noun | mobile | holdable},
        {u8("precious jewelry"), noun | mobile | holdable | clothing},
        {u8("rare coins"), noun | mobile | holdable},
        {u8("Y2"), noun | supporter},
        {u8("window"), noun | lockable | openable},
        {u8("marks"), noun},
        {u8("shadowy figure"), noun | animate},
        {u8("rocks"), noun | mobile | holdable | supporter},
        {u8("orange column"), noun | supporter},
        {u8("bedrock block"), noun | supporter},
        {u8("beanstalk"), noun | mobile | supporter | flammable},
        {u8("hole above pit"), noun},
        {u8("plant"), noun | mobile | holdable | flammable},
        {u8("thin rock slabs"), noun | holdable | supporter},
        {u8("pool of oil"), noun | holdable | flammable},
        {u8("slab"), noun | mobile | supporter},
        {u8("boulders"), noun | mobile | supporter},
        {u8("stalactite"), noun | supporter},
        {u8("dragon"), noun | animate},
        {u8("rug"), noun | holdable | supporter | flammable},
        {u8("treasure chest"), noun | mobile | container | lockable | openable | flammable},
        {u8("leaves"), noun | mobile | holdable | flammable},
        {u8("scrawled inscription"), noun | readable},
        {u8("nest of golden eggs"), noun | mobile | holdable},
        {u8("rusty door"), noun | lockable | openable},
        {u8("waterfall"), noun | container | edible},
        {u8("trident"), noun | mobile | holdable | unlocker},
        {u8("carpet"), noun | supporter | flammable},
        {u8("curtains"), noun | holdable | openable | flammable},
        {u8("moss"), noun | holdable},
        {u8("pillow"), noun | holdable | supporter},
        {u8("drawings"), noun | holdable | readable},
        {u8("vase"), noun | mobile | holdable | container},
        {u8("shards"), noun | mobile | holdable | unlocker},
        {u8("emerald"), noun | mobile | holdable},
        {u8("tablet"), noun | mobile | holdable},
        {u8("platinum pyramid"), noun | mobile | holdable},
        {u8("clam"), noun | mobile | holdable | container | openable},
        {u8("pearl"), noun | mobile | holdable},
        {u8("sign"), noun | readable},
        {u8("Spelunker Today"), noun | holdable | readable | flammable},
        {u8("mirror"), noun | mobile | holdable},
        {u8("troll"), noun | animate},
        {u8("volcano"), noun | supporter | container},
        {u8("sparks of ash"), noun},
        {u8("jagged roof"), noun},
        {u8("gorge"), noun | container},
        {u8("river of fire"), noun},
        {u8("geyser"), noun},
        {u8("rare spices"), noun | mobile | holdable | edible},
        {u8("limestone formations"), noun | supporter},
        {u8("dust"), noun | mobile | holdable},
        {u8("bear"), noun | animate},
        {u8("golden chain"), noun | mobile | holdable | clothing},
        {u8("message"), noun | mobile | holdable},
        {u8("vending machine"), noun | mobile | container | lockable | openable | switchable},
        {u8("batteries"), noun | mobile | holdable | flammable},
        {u8("dwarf"), noun | animate},
        {u8("axe"), noun | mobile | holdable | unlocker},
        {u8("collection of adventure game materials"), noun | mobile | holdable | readable | flammable}
      },
      vector<ActionTemplate> {
        {u8("examine "), 0U, u8("\n")},
      },
      vector<ActionTemplate> {
        {u8(""), direction, u8("\n")},
        {u8("get in "), container, u8("\n")},
        {u8("exit "), container, u8("\n")},
        {u8("get on "), supporter, u8("\n")},
        {u8("get off "), supporter, u8("\n")},
        {u8("take "), holdable, u8("\n")},
        {u8("take "), holdable, u8(" from "), container, u8("\n")},
        {u8("take "), holdable, u8(" from "), supporter, u8("\n")},
        {u8("don "), clothing, u8("\n")},
        {u8("unlock "), lockable, u8(" with "), unlocker, u8("\n")},
        {u8("open "), openable, u8("\n")},
        {u8("close "), openable, u8("\n")},
        {u8("lock "), lockable, u8(" with "), locker, u8("\n")},
        {u8("say "), noun, u8(" to "), animate, u8("\n")},
        {u8("ask "), animate, u8(" about "), noun, u8("\n")},
        {u8("attack "), noun, u8("\n")},
        {u8("blow "), holdable, u8("\n")},
        {u8("burn "), flammable, u8("\n")},
        {u8("burn "), flammable, u8(" with "), holdable, u8("\n")},
        {u8("buy "), noun, u8("\n")},
        {u8("climb "), supporter, u8("\n")},
        {u8("consult "), readable, u8(" about "), 0U, u8("\n")},
        {u8("chop "), noun, u8("\n")},
        {u8("dig "), noun, u8("\n")},
        {u8("dig "), noun, u8(" with "), holdable, u8("\n")},
        {u8("doff "), clothing, u8("\n")},
        {u8("drink "), edible, u8("\n")},
        {u8("eat "), edible, u8("\n")},
        {u8("empty "), container, u8(" into "), container, u8("\n")},
        {u8("empty "), container, u8(" onto "), supporter, u8("\n")},
        {u8("empty "), supporter, u8(" into "), container, u8("\n")},
        {u8("empty "), supporter, u8(" onto "), supporter, u8("\n")},
        {u8("fill "), container, u8("\n")},
        {u8("feed "), holdable, u8(" to "), animate, u8("\n")},
        {u8("drop "), holdable, u8("\n")},
        {u8("drop "), holdable, u8(" into "), container, u8("\n")},
        {u8("drop "), holdable, u8(" onto "), supporter, u8("\n")},
        {u8("hop\n")},
        {u8("hop over "), noun, u8("\n")},
        {u8("attach "), attachable, u8("\n")},
        {u8("attach "), attachable, u8(" to "), noun, u8("\n")},
        {u8("embrace "), animate, u8("\n")},
        {u8("hear\n")},
        {u8("hear "), noun, u8("\n")},
        {u8("look under "), noun, u8("\n")},
        {u8("drag "), mobile, u8("\n")},
        {u8("push "), mobile, u8("\n")},
        {u8("rotate "), mobile, u8("\n")},
        {u8("push "), noun, u8(" "), direction, u8("\n")},
        {u8("clean "), noun, u8("\n")},
        {u8("look in "), container, u8("\n")},
        {u8("look on "), supporter, u8("\n")},
        {u8("adjust "), noun, u8("\n")},
        {u8("display "), holdable, u8(" to "), animate, u8("\n")},
        {u8("sing\n")},
        {u8("nap\n")},
        {u8("smell\n")},
        {u8("smell "), noun, u8("\n")},
        {u8("squash "), noun, u8("\n")},
        {u8("dive\n")},
        {u8("swing "), mobile, u8("\n")},
        {u8("switch on "), switchable, u8("\n")},
        {u8("switch off "), switchable, u8("\n")},
        {u8("taste "), noun, u8("\n")},
        {u8("tell "), animate, u8(" about "), 0U, u8("\n")},
        {u8("feel "), noun, u8("\n")},
        {u8("wake up\n")},
        {u8("wake up "), animate, u8("\n")},
        {u8("wave "), noun, u8("\n")},
        {u8("wave\n")},
        {u8("tell "), animate, u8(" to "), 0U, u8("\n")},
        {u8("ask "), animate, u8(" for "), noun, u8("\n")},
        {u8("push "), mobile, u8(" to "), noun, u8("\n")},
        {u8("say xyzzy\n")},
        {u8("say plugh\n")},
        {u8("count "), noun, u8("\n")},
        {u8("empty "), noun, u8("\n")},
        {u8("free "), animate, u8("\n")},
        {u8("capture "), animate, u8("\n")},
        {u8("capture "), animate, u8(" with "), holdable, u8("\n")},
        {u8("say plover\n")},
        {u8("douse water on "), noun, u8("\n")},
        {u8("douse oil on "), noun, u8("\n")},
        {u8("kick "), noun, u8("\n")},
        {u8("blast "), noun, u8(" with "), holdable, u8("\n")},
        {u8("say fee\n")},
        {u8("say fie\n")},
        {u8("say foe\n")},
        {u8("say foo\n")},
        {u8("say abracadabra\n")}
      },
      [] (const Vm &vm, const u8string &output) -> bool {
        return
          output.find(u8("You can't see ")) != u8string::npos ||
          output.find(u8("I only understood you as far as ")) != u8string::npos ||
          output.find(u8("That's not something you need to refer to in the course of this game.")) != u8string::npos
        ;
      },
      unique_ptr<Metric>(new WordUsageMetric(one of 0x3BEB or 0x3C0B or 0x3C0D))
    );
    */

    unordered_set<Node *> selectedNodes;
    unordered_set<Node *> verboseNodes;
    vector<NodeData> nodesByIndex;
    unordered_map<Node *, size_t> nodeIndices;
    bool elideDeadEndNodes = false;
    size_t maxDepth = numeric_limits<size_t>::max();
    u8string in(u8(" "));
    do {
      u8string message;

      const char8_t *inI = in.data();
      const char8_t *inEnd = inI + in.size();
      while (inI != inEnd) {
        const char8_t *inPartBegin = inI = skipSpaces(inI, inEnd);
        const char8_t *inPartEnd = inI = skipNonSpaces(inI, inEnd);
        u8string line(inPartBegin, inPartEnd);

        if (line == u8("N") || line == u8("n")) {
          elideDeadEndNodes = false;
        } else if (line == u8("D") || line == u8("d")) {
          elideDeadEndNodes = true;
        } else if (line == u8("W") || line == u8("w")) {
          maxDepth = numeric_limits<size_t>::max();
        } else if (line.size() > 2 && (line[0] == U'W' || line[0] == U'w') && line[1] == U'-') {
          is n = getNaturalNumber(line.data() + 2, line.data() + line.size());
          if (n >= 0) {
            maxDepth = static_cast<iu>(n);
          }
        } else if (line == u8("A") || line == u8("a")) {
          selectedNodes.clear();
          for (const auto &d : nodesByIndex) {
            selectedNodes.insert(d.node);
          }
        } else if (line == u8("U") || line == u8("u")) {
          for (const auto &d : nodesByIndex) {
            if (d.node->getState()) {
              selectedNodes.insert(d.node);
            }
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
        } else if (line.size() > 3 && (line[0] == U'V' || line[0] == U'v') && line[1] == U'-') {
          char8_t valueName = line[2];
          if (valueName >= 'a' && valueName <= 'z') {
            size_t valueIndex = static_cast<size_t>(valueName - 'a');
            is n = getNaturalNumber(line.data() + 3, line.data() + line.size());
            if (n > 0 && !selectedNodes.empty()) {
              vector<tuple<size_t, Node *>> nodes;
              nodes.reserve(selectedNodes.size());

              for (Node *node : selectedNodes) {
                size_t value = node->getMetricState()->getValue(valueIndex);
                nodes.emplace_back(value, node);
              }
              sort(nodes.begin(), nodes.end(), [] (const tuple<size_t, Node *> &o0, const tuple<size_t, Node *> &o1) -> bool {
                return get<0>(o0) > get<0>(o1);
              });

              auto nodesNetEnd = nodes.begin() + static_cast<ptrdiff_t>(min(static_cast<size_t>(n), nodes.size()) - 1); // XXXX sort out size_t -> ptrdiff_t
              size_t minValue = get<0>(*nodesNetEnd);
              ++nodesNetEnd;
              for (auto nodesEnd = nodes.end(); nodesNetEnd != nodesEnd && get<0>(*nodesNetEnd) == minValue; ++nodesNetEnd);
              size_t count = static_cast<size_t>(nodesNetEnd - nodes.begin());

              selectedNodes.clear();
              size_t unprocessedCount = 0;
              for (auto i = nodes.begin(); i != nodesNetEnd; ++i) {
                Node *node = get<1>(*i);
                unprocessedCount += !!node->getState();
                selectedNodes.insert(node);
              }

              char8_t b[1024];
              sprintf(reinterpret_cast<char *>(b), "Selected %d (%d unprocessed) (of %d) nodes (threshold metric value %d)\n\n", count, unprocessedCount, nodes.size(), minValue);
              message.append(b);
            }
          }
        } else if (line == u8("S") || line == u8("s")) {
          verboseNodes.insert(selectedNodes.cbegin(), selectedNodes.cend());
        } else if (line == u8("H") || line == u8("h")) {
          for (Node *node : selectedNodes) {
            verboseNodes.erase(node);
          }
        } else if (line == u8("P") || line == u8("p")) {
          vector<Node *> t;
          t.reserve(selectedNodes.size());
          for (const auto &d : nodesByIndex) {
            Node *n = d.node;
            if (selectedNodes.find(n) != selectedNodes.end()) {
              t.emplace_back(n);
            }
          }
          multiverse.processNodes(t.begin(), t.end(), vm);
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
        } else if (line.size() > 2 && (line[0] == U'E' || line[0] == U'e') && line[1] == U'-') {
          u8string name(line.data() + 2, line.data() + line.size());
          multiverse.save(reinterpret_cast<const char *>(name.c_str()));
        } else if (line.size() > 2 && (line[0] == U'O' || line[0] == U'o') && line[1] == U'-') {
          u8string name(line.data() + 2, line.data() + line.size());
          multiverse.load(reinterpret_cast<const char *>(name.c_str()), vm);
          nodesByIndex.clear();
        } else {
          const char8_t *numBegin = line.data();
          const char8_t *numEnd = numBegin + line.size();
          const char8_t *dash = find(numBegin, numEnd, U'-');

          const size_t rX = static_cast<size_t>(-1);
          size_t r0 = rX;
          size_t r1 = rX;

          is n = getNaturalNumber(numBegin, dash);
          size_t nn;
          if (n >= 0 && (nn = static_cast<size_t>(n)) < nodesByIndex.size()) {
            r0 = nn;
          }
          if (dash == numEnd) {
            r1 = r0 + 1;
          } else {
            is n = getNaturalNumber(dash + 1, numEnd);
            size_t nn;
            if (n >= 0 && (nn = static_cast<size_t>(n)) < nodesByIndex.size()) {
              r1 = nn + 1;
            }
          }

          if (r0 != rX && r1 != rX) {
            for (size_t i = r0; i != r1; ++i) {
              Node *node = nodesByIndex[i].node;
              auto pos = selectedNodes.find(node);
              if (pos == selectedNodes.end()) {
                selectedNodes.insert(node);
              } else {
                selectedNodes.erase(pos);
              }
            }
          }
        }

        if (nodesByIndex.empty()) {
          selectedNodes.clear();
          verboseNodes.clear();
          nodeIndices.clear();
          studyNodes(multiverse, nodesByIndex, nodeIndices);
        }
      }

      if (getenv("TERM")) {
        printf("\x1B[1J\x1B[;H");
      } else {
        system("cls");
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

        printNode(multiverse.getRootNode(), multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, maxDepth, out);
      }

      if (!message.empty()) {
        printf("%s", message.c_str());
      }
      printf(
        "Show All _Nodes         Hide _Dead End Nodes    Sho_w Nodes n Deep[-<n>]\n"
        "Select _All             Select _Unprocesseds\n"
        "_Clear Selection        _Invert Selection\n"
        "Shrink Selection to Highest _Valued-<n>\n"
        "_Show Output            _Hide Output\n"
        "_Process                Co_llapse               _Terminate\n"
        "Sav_e As-<name>         _Open File-<name>\n"
        ">"
      );
      fflush(stdout);

      in.clear();
      readLine(in);
      size_t comment = in.find(U'#');
      if (comment != u8string::npos) {
        in.erase(comment);
      }
    } while (in != u8("quit"));

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
  Node *rootNode = multiverse.getRootNode();
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
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, size_t maxDepth,
  FILE *out
) {
  u8string prefix;
  printNodeAsNonleaf(0, nullptr, rootNode, nullptr, Multiverse::NON_ID, multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, maxDepth, prefix, out);
}

u8string renderActionInput (ActionId actionId, const Multiverse &multiverse) {
  u8string actionInput;
  if (actionId != Multiverse::NON_ID) {
    actionInput.push_back(U'"');
    multiverse.getActionSet().get(actionId).getInput(actionInput);
    actionInput.erase(actionInput.size() - 1);
    actionInput.append(u8("\" ->"));
  }
  return actionInput;
}

void printNodeHeader (
  char8_t nodeIndexRenderingPrefix, char8_t nodeIndexRenderingSuffix, Node *node, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes,
  const unordered_map<Node *, size_t> &nodeIndices, FILE *out
) {
  // XXXX 'map contains' fn as well? also 'find an entry known to be present'? just subclass unordered_*<> (as we will soon do for basic_string) ???
  bool selected = selectedNodes.find(node) != selectedNodes.end();
  if (selected) {
    fprintf(out, "* ");
  }

  fprintf(out, "%c%u%c ", nodeIndexRenderingPrefix, *find(nodeIndices, node), nodeIndexRenderingSuffix);

  fprintf(out, "%s", renderActionInput(actionId, multiverse).c_str());
  fprintf(out, " [sig of hash &%08X]", node->getSignature().hash());
  fprintf(out, " metric values {");
  Metric::State *state = node->getMetricState();
  for (size_t i = 0, end = multiverse.getMetric()->getValueCount(); i != end; ++i) {
    fprintf(out, "%s%d", i == 0 ? "" : ", ", state->getValue(i));
  }
  fprintf(out, "}");
}

void printNodeOutput (const u8string *output, const u8string &prefix, FILE *out) {
  if (!output) {
    return;
  }

  u8string o(*output);
  char8_t c;
  while ((c = o.back()) == U'\n' || c == U'>') {
    o.pop_back();
  }
  o.push_back(U'\n');

  auto lineStart = o.cbegin();
  for (auto i = o.cbegin(), end = o.cend(); i != end; ++i) {
    if (*i == U'\n') {
      fprintf(out, "%s  %.*s\n", prefix.c_str(), static_cast<int>(i - lineStart), &*lineStart);
      lineStart = i + 1;
    }
  }
}

void printNodeAsLeaf (
  size_t depth, const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, size_t maxDepth,
  u8string &r_prefix, FILE *out
) {
  printNodeHeader(U'{', U'}', node, actionId, multiverse, selectedNodes, nodeIndices, out);
  fprintf(out, " / (elsewhere)\n");
  printNodeOutput(output, r_prefix, out);
}

void printNodeAsNonleaf (
  size_t depth, const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse &multiverse, const unordered_set<Node *> &selectedNodes, const unordered_set<Node *> &verboseNodes,
  const vector<NodeData> &nodesByIndex, const unordered_map<Node *, size_t> &nodeIndices, bool elideDeadEndNodes, size_t maxDepth,
  u8string &r_prefix, FILE *out
) {
  printNodeHeader(U'(', U')', node, actionId, multiverse, selectedNodes, nodeIndices, out);
  if (node->getState()) {
    fprintf(out, " / unprocessed\n");
  } else {
    size_t c = node->getChildrenSize();
    fprintf(out, " / %u %s\n", c, c == 1 ? "child" : "children");
  }
  printNodeOutput(output, r_prefix, out);

  DA(depth <= maxDepth);
  if (depth == maxDepth) {
    return;
  }
  ++depth;

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
    (fmts[i] == LEAF ? printNodeAsLeaf : printNodeAsNonleaf)(depth, verboseNodes.find(childNode) != verboseNodes.end() ? &childOuput : nullptr, childNode, node, childActionId, multiverse, selectedNodes, verboseNodes, nodesByIndex, nodeIndices, elideDeadEndNodes, maxDepth, r_prefix, out);
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
    --size;
  }
  return size;
}

void readLine (u8string &r_out) {
  char8_t b[1024];
  size_t size = readLine(b, sizeof(b));
  r_out.append(b, size);
}

const char8_t *skipSpaces (const char8_t *i, const char8_t *end) {
  for (; i != end && *i == ' '; ++i);
  return i;
}

const char8_t *skipNonSpaces (const char8_t *i, const char8_t *end) {
  for (; i != end && *i != ' '; ++i);
  return i;
}

is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd) {
  if (iBegin == iEnd || std::isspace(*iBegin)) {
    return -1;
  }

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
