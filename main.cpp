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
using std::move;
using core::PlainException;
using std::copy;
using autoinf::Signature;
using std::unique_ptr;
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
using std::hash;
using autoinf::find;
using autoinf::contains;
using std::fill;
using autoinf::finally;
using std::packaged_task;
using std::thread;
using std::future_status;
using Value = NodeMetricsListener::Value;
using std::make_signed;

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
      unique_ptr<Multiverse::Listener>(new MultiverseView(0x08C8))
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
      unique_ptr<Multiverse::Listener>(new MultiverseView(one of 0x3BEB or 0x3C0B or 0x3C0D))
    );
    */
    MultiverseView *view = static_cast<MultiverseView *>(multiverse.getListener());
    view->multiverseChanged(multiverse);

    struct {
      const char *name;
      void (*impl)(int argc, char **argv, Vm &vm, Multiverse &multiverse, MultiverseView *view);
    } modes[] = {{"cmd", runCmd}, {"velocityrun", runVelocityrun}};
    if (argc < 2) {
      throw PlainException(u8("no mode specified"));
    } else {
      void (*impl)(int argc, char **argv, Vm &vm, Multiverse &multiverse, MultiverseView *view) = nullptr;
      for (auto &mode : modes) {
        if (strcmp(argv[1], mode.name) == 0) {
          impl = mode.impl;
          break;
        }
      }
      if (impl) {
        (*impl)(argc, argv, vm, multiverse, view);
      } else {
        throw PlainException(u8("no valid mode specified"));
      }
    }

    return 0;
  } catch (exception &e) {
    fprintf(stderr, "Error: %s\n", core::createExceptionMessage(e, false).c_str());
    return 1;
  }
}

void runCmd (int argc, char **argv, Vm &vm, Multiverse &multiverse, MultiverseView *view) {
  const char *outPathName = nullptr;
  if (argc == 3) {
    outPathName = argv[2];
  }

  u8string message;
  updateMultiverseDisplay(multiverse, outPathName, message);

  u8string in;
  while (true) {
    readLine(in);
    size_t comment = in.find(U'#');
    if (comment != u8string::npos) {
      in.erase(comment);
    }

    bool done = !runCommandLine(vm, multiverse, in, message);
    in.clear();
    if (done) {
      break;
    }

    updateMultiverseDisplay(multiverse, outPathName, message);
    message.clear();
  }
}

void runVelocityrun (int argc, char **argv, Vm &vm, Multiverse &multiverse, MultiverseView *view) {
  if (argc != 7 && argc != 8) {
    throw PlainException(u8("command lines to be run initially, each round, on max score change, on no nodes being processed and on user exit must be specified"));
  }
  // TODO uargs
  u8string initialCommandLine(reinterpret_cast<char8_t *>(argv[2]));
  u8string roundCommandLine(reinterpret_cast<char8_t *>(argv[3]));
  u8string maxScoreChangeCommandLine(reinterpret_cast<char8_t *>(argv[4]));
  u8string nullChangeCommandLine(reinterpret_cast<char8_t *>(argv[5]));
  u8string stopCommandLine(reinterpret_cast<char8_t *>(argv[6]));
  iu initialRoundCount = 0;
  double initialTTotal = 0;
  double initialTVm = 0;
  if (argc == 8) {
    int v0;
    float v1;
    float v2;
    if (sscanf(argv[7], "%d,%f,%f", &v0, &v1, &v2) != 3 || v0 < 0) {
      throw PlainException(u8("invalid initial stats state specified"));
    }
    initialRoundCount = static_cast<unsigned int>(v0);
    initialTTotal = v1;
    initialTVm = v2;
  }

  u8string message;

  packaged_task<void ()> waiter([] () {
    u8string in;
    readLine(in);
  });
  auto waiterFuture = waiter.get_future();
  thread waiterThread(move(waiter));
  // XXXX isn't this supposed to not work?
  //auto _ = finally([&waiterThread] () {
  // waiterThread.join();
  //});

  if (!runCommandLine(vm, multiverse, initialCommandLine, message)) {
    return;
  }

  auto getUserTime = [] () -> double {
    return 0;
  };
  auto getVmTime = [&vm] () -> double {
    return 0;
  };
  double tTotal0 = getUserTime() - initialTTotal;
  double tVm0 = getVmTime() - initialTVm;

  printf("\"Number of rounds\",\"total time\",\"VM time\",\"max score\",\"node count\"\n");
  auto printStats = [&] (iu roundCount, double tTotal1, double tVm1) {
    printf("%d,%f,%f,%d,%d\n", static_cast<int>(roundCount), tTotal1 - tTotal0, tVm1 - tVm0, static_cast<int>(view->getMaxScoreValue()), static_cast<int>(view->nodesByIndex.size()));
    fflush(stdout);
  };
  printStats(initialRoundCount, getUserTime(), getVmTime());

  for (iu round = initialRoundCount; true; ++round) {
    if (waiterFuture.wait_for(std::chrono::nanoseconds::zero()) == future_status::ready) {
      printf("  stopping\n");
      fflush(stdout);
      waiterFuture.get();
      runCommandLine(vm, multiverse, stopCommandLine, message);
      return;
    }

    Value maxScoreValue0 = view->getMaxScoreValue();
    size_t nodesSize0 = view->nodesByIndex.size();
    if (!runCommandLine(vm, multiverse, roundCommandLine, message)) {
      return;
    }
    Value maxScoreValue1 = view->getMaxScoreValue();
    size_t nodesSize1 = view->nodesByIndex.size();

    double tTotal1 = getUserTime();
    double tVm1 = getVmTime();
    printStats(round + 1, tTotal1, tVm1);

    if (maxScoreValue1 != maxScoreValue0) {
      printf("  max score changed\n");
      fflush(stdout);
      if (!runCommandLine(vm, multiverse, maxScoreChangeCommandLine, message)) {
        return;
      }
    }

    if (nodesSize1 == nodesSize0) {
      printf("  node count unchanged\n");
      fflush(stdout);
      if (!runCommandLine(vm, multiverse, nullChangeCommandLine, message)) {
        return;
      }
    }

    double tTotal2 = getUserTime();
    double tVm2 = getVmTime();
    tTotal0 -= (tTotal2 - tTotal1);
    tVm0 -= (tVm2 - tVm1);

    message.clear();
  }
}

bool runCommandLine (Vm &vm, Multiverse &multiverse, const u8string &in, u8string &message) {
  MultiverseView *view = static_cast<MultiverseView *>(multiverse.getListener());
  vector<Node *> &nodesByIndex = view->nodesByIndex;
  unordered_set<Node *> &selectedNodes = view->selectedNodes;
  unordered_set<Node *> &verboseNodes = view->verboseNodes;
  bool &elideDeadEndNodes = view->elideDeadEndNodes;
  size_t &maxDepth = view->maxDepth;

  const char8_t *inI = in.data();
  const char8_t *inEnd = inI + in.size();
  while (inI != inEnd) {
    const char8_t *inPartBegin = inI = skipSpaces(inI, inEnd);
    const char8_t *inPartEnd = inI = skipNonSpaces(inI, inEnd);
    u8string line(inPartBegin, inPartEnd);

    if (line == u8("quit")) {
      return false;
    } else if (line.size() > 1 && (line[0] == U'X' || line[0] == U'x')) {
      is n = getNaturalNumber(line.data() + 1, line.data() + line.size());
      if (n >= 0) {
        iu times = static_cast<iu>(n);
        u8string subLine(inPartEnd, inEnd);
        for (iu i = 0; i != times; ++i) {
          bool notDone = runCommandLine(vm, multiverse, subLine, message);
          if (!notDone) {
            return false;
          }
        }
        return true;
      }
    } else if (line == u8("N") || line == u8("n")) {
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
      for (const auto &node : nodesByIndex) {
        selectedNodes.insert(node);
      }
    } else if (line == u8("U") || line == u8("u")) {
      for (const auto &node : nodesByIndex) {
        if (node->getState()) {
          selectedNodes.insert(node);
        }
      }
    } else if (line == u8("C") || line == u8("c")) {
      selectedNodes.clear();
    } else if (line == u8("I") || line == u8("i")) {
      unordered_set<Node *> nextSelectedNodes(nodesByIndex.size() - selectedNodes.size());
      for (const auto &node : nodesByIndex) {
        if (!contains(selectedNodes, node)) {
          nextSelectedNodes.insert(node);
        }
      }
      selectedNodes = move(nextSelectedNodes);
    } else if (line.size() > 3 && (line[0] == U'V' || line[0] == U'v') && line[1] == U'-') {
      char8_t valueName = line[2];
      size_t valueIndex;
      if (valueName >= 'a' && valueName <= 'z' && (valueIndex = static_cast<size_t>(valueName - 'a')) < NodeMetricsListener::VALUE_COUNT) {
        const char8_t *numBegin = line.data() + 3;
        const char8_t *numEnd = line.data() + line.size();
        bool undershoot = false;
        bool overshootOnEmpty =  false;
        if (*(numEnd - 1) == U'-') {
          --numEnd;
          undershoot = true;
        } else if (*(numEnd - 1) == U'|') {
          --numEnd;
          undershoot = true;
          overshootOnEmpty = true;
        }
        is n = getNaturalNumber(numBegin, numEnd);
        if (n > 0 && !selectedNodes.empty()) {
          size_t count = static_cast<size_t>(n);
          if (count < selectedNodes.size()) {
            vector<tuple<Value, Node *>> nodes;
            nodes.reserve(selectedNodes.size());

            for (Node *node : selectedNodes) {
              Value value = static_cast<NodeView *>(node->getListener())->getValue(valueIndex);
              nodes.emplace_back(value, node);
            }
            sort(nodes.begin(), nodes.end(), [] (const tuple<Value, Node *> &o0, const tuple<Value, Node *> &o1) -> bool {
              return get<0>(o0) > get<0>(o1);
            });

            if (undershoot) {
              Value minValue = get<0>(nodes[count]);
              if (overshootOnEmpty && get<0>(nodes[0]) == minValue) {
                undershoot = false;
              } else {
                --count;
                for (; count != static_cast<size_t>(-1) && get<0>(nodes[count]) == minValue; --count);
                ++count;
              }
            }
            if (!undershoot) {
              Value minValue = get<0>(nodes[count - 1]);
              for (auto end = nodes.size(); count != end && get<0>(nodes[count]) == minValue; ++count);
            }

            selectedNodes.clear();
            size_t unprocessedCount = 0;
            for (auto i = nodes.begin(), end = i + static_cast<ptrdiff_t>(count); i != end; ++i) { // XXXX sort out size_t -> ptrdiff_t
              Node *node = get<1>(*i);
              unprocessedCount += !!node->getState();
              selectedNodes.insert(node);
            }

            char8_t b[1024];
            char *t = reinterpret_cast<char *>(b);
            t += sprintf(t, "Selected %d (%d unprocessed) (from %d) nodes", count, unprocessedCount, nodes.size());
            if (count != 0) {
              t += sprintf(t, " (threshold metric value %d)", get<0>(nodes[count - 1]));
            }
            t += sprintf(t, "\n\n");
            message.append(b);
          } else {
            char8_t b[1024];
            sprintf(reinterpret_cast<char *>(b), "Selected %d nodes (selection unchanged)\n\n", selectedNodes.size());
            message.append(b);
          }
        }
      }
    } else if (line == u8("S") || line == u8("s")) {
      verboseNodes.insert(selectedNodes.cbegin(), selectedNodes.cend());
    } else if (line == u8("H") || line == u8("h")) {
      for (Node *node : selectedNodes) {
        verboseNodes.erase(node);
      }
    } else if (line == u8("P") || line == u8("p")) {
      if (!selectedNodes.empty()) {
        vector<Node *> t;
        t.reserve(selectedNodes.size());
        for (const auto &n : nodesByIndex) {
          if (contains(selectedNodes, n)) {
            t.emplace_back(n);
          }
        }
        multiverse.processNodes(t.begin(), t.end(), vm);
      }
      view->multiverseChanged(multiverse);
    } else if (line == u8("L") || line == u8("l")) {
      if (!selectedNodes.empty()) {
        multiverse.collapseNodes(selectedNodes.cbegin(), selectedNodes.cend(), vm);
      }
      view->multiverseChanged(multiverse);
    } else if (line == u8("T") || line == u8("t")) {
      for (Node *node : selectedNodes) {
        node->clearState();
      }
      view->multiverseChanged(multiverse);
    } else if (line.size() > 2 && (line[0] == U'E' || line[0] == U'e') && line[1] == U'-') {
      u8string name(line.data() + 2, line.data() + line.size());
      multiverse.save(reinterpret_cast<const char *>(name.c_str()));
    } else if (line.size() > 2 && (line[0] == U'O' || line[0] == U'o') && line[1] == U'-') {
      u8string name(line.data() + 2, line.data() + line.size());
      multiverse.load(reinterpret_cast<const char *>(name.c_str()), vm);
      view->multiverseChanged(multiverse);
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
          Node *node = nodesByIndex[i];
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

  return true;
}

void updateMultiverseDisplay (Multiverse &multiverse, const char *outPathName, const u8string &message) {
  MultiverseView *view = static_cast<MultiverseView *>(multiverse.getListener());

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
    auto _ = finally([&] {
      if (outPathName) {
        fclose(out);
      }
    });

    view->printNodes(multiverse, out);
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
}

constexpr size_t NodeMetricsListener::VALUE_COUNT;
constexpr Value NodeMetricsListener::NON_VALUE;

NodeMetricsListener::NodeMetricsListener () :
  scoreValue(NON_VALUE), wordValue(NON_VALUE - 1), locationHash(0), visitageValue(NON_VALUE)
{
}

template<typename _Walker> void NodeMetricsListener::beWalked (_Walker &w) {
  DS();
  w.process(scoreValue);
  w.process(interestingChildActionWords);
  w.process(wordValue);
  w.process(locationHash);
  w.process(visitageValue);
}

Value NodeMetricsListener::getValue (size_t i) const {
  DPRE(i < VALUE_COUNT);
  return i == 0 ? scoreValue : i == 1 ? wordValue : visitageValue;
}

const vector<Value> MultiverseMetricsListener::NEW_LOCATION_VISITAGE_MODIFIERS = {20, 13, 8, 5};
const Value MultiverseMetricsListener::OLD_LOCATION_VISITAGE_MODIFIER = -200;

MultiverseMetricsListener::MultiverseMetricsListener (zword scoreAddr) : scoreAddr(scoreAddr), maxScoreValue(numeric_limits<Value>::min()) {
}

void MultiverseMetricsListener::nodeReached (const Multiverse &multiverse, Node::Listener *listener_, ActionId parentActionId, const u8string &output, const Signature &signature, const Vm &vm) {
  DW(, "DDDD created new node with sig of hash ", signature.hash());
  NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(listener_);

  setScoreValue(listener, vm);
  setVisitageData(listener, output);
}

void MultiverseMetricsListener::setScoreValue (NodeMetricsListener *listener, const Vm &vm) {
  DPRE((static_cast<iu>(scoreAddr) + 1) < vm.getDynamicMemorySize() + 0);
  const zbyte *m = vm.getDynamicMemory();
  listener->scoreValue = static_cast<make_signed<zword>::type>(static_cast<zword>(static_cast<zword>(m[scoreAddr] << 8) | m[scoreAddr + 1]));
  DW(, "DDDD game score is ",listener->scoreValue);
  if (listener->scoreValue > maxScoreValue) {
    maxScoreValue = listener->scoreValue;
  }
}

void MultiverseMetricsListener::setVisitageData (NodeMetricsListener *listener, const u8string &output) {
  const char8_t *outI = output.data();
  const char8_t *outEnd = outI + output.size();
  const char8_t *locationBegin = outI = skipSpaces(outI, outEnd);
  while (true) {
    outI = skipNonSpaces(outI, outEnd);
    if (outI == outEnd) {
      break;
    }
    const char8_t *i = outI + 1;
    if (i == outEnd || *i == ' ') {
      break;
    }
    outI = i;
  }
  const char8_t *locationEnd = outI;
  DW(, "DDDD location string is ", u8string(locationBegin, locationEnd).c_str());

  listener->locationHash = autoinf::hashImpl(locationBegin, locationEnd);
  DW(, "DDDD location hash is ", listener->locationHash);
  DA(listener->visitageValue == NodeMetricsListener::NON_VALUE);
}

void MultiverseMetricsListener::subtreePrimeAncestorsUpdated (const Multiverse &multiverse, const Node *node) {
  NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(node->getListener());

  setVisitageValueRecursively(node, listener, getVisitageChain(node->getPrimeParentNode()));
}

MultiverseMetricsListener::VisitageChain::VisitageChain (
  size_t locationHash, vector<Value>::const_iterator newLocationVisitageModifiersI, Value visitageValue
) :
  locationHash(locationHash), newLocationVisitageModifiersI(newLocationVisitageModifiersI), visitageValue(visitageValue)
{
}

void MultiverseMetricsListener::VisitageChain::increment (NodeMetricsListener *listener) {
  if (listener->locationHash == locationHash) {
    DW(, "DDDD   location didn't change");
    if (newLocationVisitageModifiersI != NEW_LOCATION_VISITAGE_MODIFIERS.end()) {
      visitageValue += *newLocationVisitageModifiersI++;
    }
  } else {
    locationHash = listener->locationHash;
    if (contains(visitedLocationHashes, locationHash)) {
      DW(, "DDDD   location changed to one we've visted");
      newLocationVisitageModifiersI = NEW_LOCATION_VISITAGE_MODIFIERS.end();
      visitageValue += OLD_LOCATION_VISITAGE_MODIFIER;
    } else {
      DW(, "DDDD   location changed to one we've not visted");
      visitedLocationHashes.emplace(locationHash);
      newLocationVisitageModifiersI = NEW_LOCATION_VISITAGE_MODIFIERS.begin();
      DA(!NEW_LOCATION_VISITAGE_MODIFIERS.empty());
      visitageValue += *newLocationVisitageModifiersI++;
    }
  }
}

Value MultiverseMetricsListener::VisitageChain::getVisitageValue () const {
  return visitageValue;
}

MultiverseMetricsListener::VisitageChain MultiverseMetricsListener::getVisitageChain (const Node *node) {
  if (!node) {
    DW(, "DDDD   reached the root; starting visitage value work");
    return VisitageChain(numeric_limits<size_t>::max(), NEW_LOCATION_VISITAGE_MODIFIERS.begin(), 0);
  }

  DW(, "DDDD   looking at node with sig of hash ",node->getSignature().hash());
  NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(node->getListener());

  VisitageChain r = getVisitageChain(node->getPrimeParentNode());
  r.increment(listener);
  DPRE(listener->visitageValue == r.getVisitageValue());

  return r;
}

void MultiverseMetricsListener::setVisitageValueRecursively (const Node *node, NodeMetricsListener *listener, VisitageChain chain) {
  setVisitageValue(node, listener, chain);

  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    if (childNode->getPrimeParentNode() == node) {
      NodeMetricsListener *childListener = static_cast<NodeMetricsListener *>(childNode->getListener());
      DA((childNode->getPrimeParentArcChildIndex() == i) != (childListener->visitageValue == NodeMetricsListener::NON_VALUE));
      childListener->visitageValue = NodeMetricsListener::NON_VALUE;
    }
  }
  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    if (childNode->getPrimeParentNode() == node) {
      NodeMetricsListener *childListener = static_cast<NodeMetricsListener *>(childNode->getListener());
      DA((childNode->getPrimeParentArcChildIndex() == i) == (childListener->visitageValue == NodeMetricsListener::NON_VALUE));
      if (childListener->visitageValue == NodeMetricsListener::NON_VALUE) {
        setVisitageValueRecursively(childNode, childListener, chain);
        DA(childListener->visitageValue != NodeMetricsListener::NON_VALUE);
      }
    }
  }
}

void MultiverseMetricsListener::setVisitageValue (const Node *node, NodeMetricsListener *listener, VisitageChain &r_chain) {
  DA(!node->getPrimeParentNode() || static_cast<NodeMetricsListener *>(node->getPrimeParentNode()->getListener())->visitageValue == r_chain.getVisitageValue());
  r_chain.increment(listener);
  listener->visitageValue = r_chain.getVisitageValue();
}

void MultiverseMetricsListener::nodeChildrenUpdated (const Multiverse &multiverse, const Node *node) {
  NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(node->getListener());

  setWordData(node, listener, multiverse.getActionSet());
}

void MultiverseMetricsListener::setWordData (const Node *node, NodeMetricsListener *listener, const Multiverse::ActionSet &actionSet) {
  DA(!node->getState());
  Bitset &words = listener->interestingChildActionWords;
  words.clear();
  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    auto action = actionSet.get(get<0>(node->getChild(i)));
    for (size_t i = 0, end = action.getWordCount(); i != end; ++i) {
      words.setBit(action.getWord(i));
    }
  }
  words.compact();
}

void MultiverseMetricsListener::nodesProcessed (const Multiverse &multiverse) {
  const Node *rootNode = multiverse.getRoot();

  unique_ptr<size_t []> stats = getWordStats(multiverse, [] (const Node *node, NodeMetricsListener *listener) {
    DA(listener->wordValue != NodeMetricsListener::NON_VALUE);
    listener->wordValue = NodeMetricsListener::NON_VALUE;
  });
  setWordValueRecursively(rootNode, static_cast<NodeMetricsListener *>(rootNode->getListener()), multiverse.size(), stats.get(), 0);

  #ifndef NDEBUG
  for (const Node *node : multiverse) {
    getVisitageChain(node);
  }
  #endif
}

template<typename F> unique_ptr<size_t []> MultiverseMetricsListener::getWordStats (const Multiverse &multiverse, const F &nodeFunctor) {
  DS();
  DW(, "DDDD nodes have changed!");
  auto &actionSet = multiverse.getActionSet();
  unique_ptr<size_t []> wordCounts(new size_t[actionSet.getWordsSize()]);
  fill(wordCounts.get(), wordCounts.get() + actionSet.getWordsSize(), 0);

  for (const Node *node : multiverse) {
    NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(node->getListener());

    nodeFunctor(node, listener);

    Bitset &words = listener->interestingChildActionWords;
    for (size_t i = words.getNextSetBit(0); i != Bitset::NON_INDEX; i = words.getNextSetBit(i + 1)) {
      ++wordCounts[i];
    }
  }
  DW(, "DDDD word counts:");
  for (size_t i = 0, end = actionSet.getWordsSize(); i != end; ++i) {
    DW(, "DDDD   ",actionSet.getWord(i).c_str()," - ",wordCounts[i]);
  }

  return wordCounts;
}

void MultiverseMetricsListener::setWordValueRecursively (const Node *node, NodeMetricsListener *listener, size_t nodesSize, const size_t *stats, Value wordValue) {
  setWordValue(node, listener, nodesSize, stats, wordValue);

  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    if (childNode->getPrimeParentNode() == node) {
      NodeMetricsListener *childListener = static_cast<NodeMetricsListener *>(childNode->getListener());
      DA((childNode->getPrimeParentArcChildIndex() == i) == (childListener->wordValue == NodeMetricsListener::NON_VALUE));
      if (childListener->wordValue == NodeMetricsListener::NON_VALUE) {
        setWordValueRecursively(childNode, childListener, nodesSize, stats, wordValue);
      }
    }
  }
}

void MultiverseMetricsListener::setWordValue (const Node *node, NodeMetricsListener *listener, size_t nodesSize, const size_t *stats, Value &r_wordValue) {
  DA(listener->wordValue == NodeMetricsListener::NON_VALUE);
  DW(, "DDDD calculating node word value for node with sig of hash ", node->getSignature().hash());
  DA((!node->getPrimeParentNode() && r_wordValue == 0) || r_wordValue == static_cast<NodeMetricsListener *>(node->getPrimeParentNode()->getListener())->wordValue);

  Value value = r_wordValue;
  Bitset &words = listener->interestingChildActionWords;
  for (size_t i = words.getNextSetBit(0); i != Bitset::NON_INDEX; i = words.getNextSetBit(i + 1)) {
    DW(, "       action word of id ", i);
    value += static_cast<Value>(nodesSize / stats[i]);
  }
  DW(, "       final local word value is ", value - r_wordValue);
  DW(, "       (parent word value is ", r_wordValue,")");
  listener->wordValue = r_wordValue = value;
}

void MultiverseMetricsListener::nodesCollapsed (const Multiverse &multiverse) {
  nodesProcessed(multiverse);
}

void MultiverseMetricsListener::loaded (const Multiverse &multiverse) {
  Value maxScoreValue = numeric_limits<Value>::min();
  for (const Node *node : multiverse) {
    NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(node->getListener());
    if (listener->scoreValue > maxScoreValue) {
      maxScoreValue = listener->scoreValue;
    }
  }
  this->maxScoreValue = maxScoreValue;
}

Value MultiverseMetricsListener::getMaxScoreValue () const {
  DPRE(maxScoreValue != numeric_limits<Value>::min());
  return maxScoreValue;
}

constexpr size_t NodeView::NON_INDEX;

NodeView::NodeView () :
  index(NON_INDEX - 1), primeParentChildIndex(Multiverse::NON_ID), allChildrenAreNonPrime(false)
{
}

template<typename _Walker> void NodeView::beWalked (_Walker &w) {
  DS();
  w.process(*static_cast<NodeMetricsListener *>(this));
  w.process(index);
  w.process(primeParentChildIndex);
  w.process(allChildrenAreNonPrime);
}

MultiverseView::MultiverseView (zword scoreAddr) : MultiverseMetricsListener(scoreAddr), elideDeadEndNodes(false), maxDepth(numeric_limits<size_t>::max()) {
}

tuple<void *, size_t> MultiverseView::deduceNodeListenerType (Node::Listener *listener) {
  DPRE(!!dynamic_cast<NodeView *>(listener));
  return tuple<void *, size_t>(static_cast<NodeView *>(listener), sizeof(NodeView));
}

tuple<Node::Listener *, void *, size_t> MultiverseView::constructNodeListener () {
  NodeView *listener = new NodeView();
  return tuple<Node::Listener *, void *, size_t>(listener, static_cast<void *>(listener), sizeof(*listener));
}

void MultiverseView::walkNodeListener (Node::Listener *listener, Serialiser<FileOutputIterator> &s) {
  static_cast<NodeView *>(listener)->beWalked(s);
}

void MultiverseView::walkNodeListener (Node::Listener *listener, Deserialiser<FileInputIterator> &s) {
  static_cast<NodeView *>(listener)->beWalked(s);
}

unique_ptr<Node::Listener> MultiverseView::createNodeListener () {
  return unique_ptr<Node::Listener>(new NodeView());
}

void MultiverseView::multiverseChanged (const Multiverse &multiverse) {
  nodesByIndex.clear();
  selectedNodes.clear();
  verboseNodes.clear();

  studyNodes(multiverse);
}

void MultiverseView::studyNodes (const Multiverse &multiverse) {
  DS();
  DA(nodesByIndex.empty());
  DA(selectedNodes.empty());
  DA(verboseNodes.empty());

  Node *rootNode = multiverse.getRoot();

  for (const Node *node : multiverse) {
    NodeView *nodeView = static_cast<NodeView *>(node->getListener());
    DA(nodeView->index != NodeView::NON_INDEX);
    nodeView->index = NodeView::NON_INDEX;
  }

  iu depth = 0;
  size_t s0;
  size_t s1 = 0;
  do {
    DW(, "studying nodes at depth ",depth);
    s0 = s1;
    studyNode(0, depth, rootNode, nullptr, Multiverse::NON_ID);
    s1 = nodesByIndex.size();
    DW(, "after studying nodes at depth ",depth,", we know about ",s1," nodes");
    ++depth;
  } while (s0 != s1);
  DA(s1 != 0);

  markDeadEndNodes();
}

void MultiverseView::studyNode (iu depth, iu targetDepth, Node *node, Node *parentNode, ActionId childIndex) {
  DS();
  DW(, "looking at node with sig of hash ",node->getSignature().hash());
  DA(node->getPrimeParentNode() == parentNode);
  DA(!node->getPrimeParentNode() || node->getPrimeParentArcChildIndex() == childIndex);
  NodeView *nodeView = static_cast<NodeView *>(node->getListener());

  DPRE(targetDepth >= depth, "");
  if (depth != targetDepth) {
    DW(, " not yet at the right depth");
    DPRE(nodeView->index != NodeView::NON_INDEX);
    for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
      DW(, " looking at child ",i);
      Node *childNode = get<2>(node->getChild(i));
      NodeView *childNodeView = static_cast<NodeView *>(childNode->getListener());

      bool skip = true;
      if (childNodeView->index == NodeView::NON_INDEX) {
        DA(depth == targetDepth - 1);
        DA(childNode->getPrimeParentNode() == node);
        skip = false;
      } else {
        if (childNode->getPrimeParentNode() == node && childNodeView->primeParentChildIndex == i) {
          skip = false;
        }
      }
      if (!skip) {
        studyNode(depth + 1, targetDepth, childNode, node, i);
      }
      DA(childNodeView->index != NodeView::NON_INDEX);
    }

    return;
  }

  DW(, " this node is at the right depth");
  DPRE(nodeView->index == NodeView::NON_INDEX);
  nodeView->index = nodesByIndex.size();
  nodeView->primeParentChildIndex = childIndex;
  nodesByIndex.emplace_back(node);
  return;
}

void MultiverseView::markDeadEndNodes () {
  for (auto i = nodesByIndex.rbegin(), end = nodesByIndex.rend(); i != end; ++i) {
    Node *node = *i;
    NodeView *nodeView = static_cast<NodeView *>(node->getListener());

    if (node->getState()) {
      DA(!nodeView->allChildrenAreNonPrime);
      continue;
    }

    bool interesting = false;
    for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
      Node *childNode = get<2>(node->getChild(i));
      NodeView *childNodeView = static_cast<NodeView *>(childNode->getListener());

      if (childNode->getPrimeParentNode() == node && !childNodeView->allChildrenAreNonPrime) {
        interesting = true;
        break;
      }
    }
    if (interesting) {
      continue;
    }

    nodeView->allChildrenAreNonPrime = true;
  }
}

void MultiverseView::printNodes (const Multiverse &multiverse, FILE *out) {
  u8string prefix;
  printNodeAsNonleaf(0, nullptr, multiverse.getRoot(), nullptr, Multiverse::NON_ID, multiverse.getActionSet(), prefix, out);
}

void MultiverseView::printNodeHeader (
  char8_t nodeIndexRenderingPrefix, char8_t nodeIndexRenderingSuffix, Node *node, ActionId actionId,
  const Multiverse::ActionSet &actionSet, FILE *out
) {
  NodeView *nodeView = static_cast<NodeView *>(node->getListener());

  bool selected = contains(selectedNodes, node);
  if (selected) {
    fprintf(out, "* ");
  }

  fprintf(out, "%c%u%c ", nodeIndexRenderingPrefix, nodeView->index, nodeIndexRenderingSuffix);

  fprintf(out, "%s", renderActionInput(actionId, actionSet).c_str());
  fprintf(out, " [sig of hash &%08X]", node->getSignature().hash());
  fprintf(out, " metric values {");
  for (size_t i = 0; i != NodeView::VALUE_COUNT; ++i) {
    fprintf(out, "%s%d", i == 0 ? "" : ", ", nodeView->getValue(i));
  }
  fprintf(out, "}");
}

u8string MultiverseView::renderActionInput (ActionId actionId, const Multiverse::ActionSet &actionSet) {
  u8string actionInput;
  if (actionId != Multiverse::NON_ID) {
    actionInput.push_back(U'"');
    actionSet.get(actionId).getInput(actionInput);
    actionInput.erase(actionInput.size() - 1);
    actionInput.append(u8("\" ->"));
  }
  return actionInput;
}

void MultiverseView::printNodeOutput (const u8string *output, const u8string &prefix, FILE *out) {
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

void MultiverseView::printNodeAsLeaf (
  size_t depth, const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse::ActionSet &actionSet, u8string &r_prefix, FILE *out
) {
  printNodeHeader(U'{', U'}', node, actionId, actionSet, out);
  fprintf(out, " / (elsewhere)\n");
  printNodeOutput(output, r_prefix, out);
}

void MultiverseView::printNodeAsNonleaf (
  size_t depth, const u8string *output, Node *node, Node *parentNode, ActionId actionId,
  const Multiverse::ActionSet &actionSet, u8string &r_prefix, FILE *out
) {
  printNodeHeader(U'(', U')', node, actionId, actionSet, out);
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
    DA(&node->getChild(node->getChildIndex(childActionId)) == &child);
    NodeView *childNodeView = static_cast<NodeView *>(childNode->getListener());

    auto fmt = NONE;
    if (elideDeadEndNodes) {
      if (!(childNode->getPrimeParentNode() == node && childNodeView->primeParentChildIndex == i && !childNodeView->allChildrenAreNonPrime)) {
        fmt = NONE;
      } else {
        fmt = NONLEAF;
      }
    } else {
      if (!(childNode->getPrimeParentNode() == node && childNodeView->primeParentChildIndex == i)) {
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
    (this->*(fmts[i] == LEAF ? &MultiverseView::printNodeAsLeaf : &MultiverseView::printNodeAsNonleaf))(depth, contains(verboseNodes, childNode) ? &childOuput : nullptr, childNode, node, childActionId, actionSet, r_prefix, out);
    r_prefix.resize(r_prefix.size() - 4);
  }
}

size_t readLine (char8_t *b, size_t bSize) {
  // TODO convert input from native
  if (!fgets(reinterpret_cast<char *>(b), static_cast<int>(bSize), stdin)) {
    throw PlainException(u8("failed to read input"));
  }
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
