#include "header.hpp"
#include <cstring>
#include <unordered_set>
#include <typeinfo>
#include <climits>

using autofrotz::Vm;
using autoinf::Multiverse;
using std::vector;
using core::u8string;
using Node = autoinf::Multiverse::Node;
using autoinf::ActionSet;
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
using autoinf::find;
using autoinf::contains;
using std::fill;
using autoinf::finally;
using std::thread;
using std::future_status;
using Value = NodeMetricsListener::Value;
using std::make_signed;
using core::hash;
using autoinf::StringSet;
using std::deque;
using std::swap;
using std::unordered_map;
using core::HashWrapper;
using core::string;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

void terminator () {
  core::dieHard();
}

int main (int argc, char *argv[]) {
  std::set_terminate(&terminator);
  try {
    DI(std::shared_ptr<core::debug::Stream> errs(new core::debug::Stream("LOG.TXT"));)
    DOPEN(, errs);
    autoinf::DOPEN(, errs);
    //autofrotz::DOPEN(, errs);
    //autofrotz::vmlink::DOPEN(, errs);

    if (argc < 3) {
      throw PlainException(u8("Syntax: autoinf <story> <mode> ..."));
    }

    const iu width = 70;
    const iu height = 64;
    const u8string saveActionInput(u8("save\n\1\n"));
    const u8string restoreActionInput(u8("restore\n\1\n"));
    const u8string noResurrectionSaveActionInput(u8("no\n") + saveActionInput);
    const u8string noResurrectionRestoreActionInput(u8("no\n") + restoreActionInput);
    iu c = 0;
    const ActionSet::Word::CategorySet direction = 1U << (c++);
    const ActionSet::Word::CategorySet noun = 1U << (c++);
    const ActionSet::Word::CategorySet  mobile = 1U << (c++);
    const ActionSet::Word::CategorySet  holdable = 1U << (c++);
    const ActionSet::Word::CategorySet  supporter = 1U << (c++);
    const ActionSet::Word::CategorySet  container = 1U << (c++);
    const ActionSet::Word::CategorySet  lockable = 1U << (c++);
    const ActionSet::Word::CategorySet  locker = 1U << (c++);
    const ActionSet::Word::CategorySet  unlocker = 1U << (c++);
    const ActionSet::Word::CategorySet  openable = 1U << (c++);
    const ActionSet::Word::CategorySet  animate = 1U << (c++);
    const ActionSet::Word::CategorySet  clothing = 1U << (c++);
    const ActionSet::Word::CategorySet  edible = 1U << (c++);
    const ActionSet::Word::CategorySet  switchable = 1U << (c++);
    const ActionSet::Word::CategorySet  readable = 1U << (c++);
    const ActionSet::Word::CategorySet  flammable = 1U << (c++);
    const ActionSet::Word::CategorySet  attachable = 1U << (c++);
    unordered_map<HashWrapper<string<char>>, Story> stories = {
      {HashWrapper<string<char>>("testgame"), {{
        "testgame/testgame.z5",
        width,
        height,
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
        u8("verbose\nfullscore\n"),
        vector<ActionSet::Word> {
          {u8("red sphere"), 0b011},
          {u8("blue sphere"), 0b011},
          {u8("green sphere"), 0b001},
          {u8("wheel"), 0b101},
          {u8("light"), 0b1001},
        },
        vector<ActionSet::Template> {
          {u8("examine "), 0b001U, u8("\n")},
        },
        [] (const Vm &vm, const u8string &output) -> bool {
          return output.find(u8("You can't see")) != std::string::npos;
        },
        vector<ActionSet::Template> {
          {u8("east\n")},
          {u8("west\n")},
          {u8("take "), 0b010U, u8("\n")},
          {u8("drop "), 0b010U, u8("\n")},
          {u8("open "), 0b010U, u8("\n")},
          {u8("turn "), 0b100U, u8(". pull "), 0b100U, u8("\n")},
          {u8("enter "), 0b1000U, u8("\n")}
        },
      }, 0x08C8}},
      {HashWrapper<string<char>>("advent"), {{
        "advent/advent.z5",
        width,
        height,
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
        u8("verbose\nfullscore\n"),
        vector<ActionSet::Word> {
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
        vector<ActionSet::Template> {
          {u8("examine "), 0U, u8("\n")},
        },
        [] (const Vm &vm, const u8string &output) -> bool {
          return
            output.find(u8("You can't see ")) != u8string::npos ||
            output.find(u8("I only understood you as far as ")) != u8string::npos ||
            output.find(u8("That's not something you need to refer to in the course of this game.")) != u8string::npos
          ;
        },
        vector<ActionSet::Template> {
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
      }, 0x3BEB /* or 0x3C0B or 0x3C0D */ }}
    };
    // TODO uargs
    auto e0 = stories.find(core::hashed(string<char>(argv[1])));
    if (e0 == stories.end()) {
      throw PlainException(u8string(u8("unknown story '")) + reinterpret_cast<char8_t *>(argv[1]) + u8("' specified"));
    }
    Story story(move(get<1>(*e0)));

    unordered_map<HashWrapper<string<char>>, void (*)(iu argsSize, char **args, Multiverse &multiverse, MultiverseView *view)> modes = {
      {HashWrapper<string<char>>("cmd"), runCmd},
      {HashWrapper<string<char>>("velocityrun"), runVelocityrun}
    };
    auto e1 = find(modes, core::hashed(string<char>(argv[2])));
    if (!e1) {
      throw PlainException(u8string(u8("unknown mode '")) + reinterpret_cast<char8_t *>(argv[2]) + u8("' specified"));
    }
    auto mode = *e1;

    u8string output;
    auto scoreAddr = story.scoreAddr;
    Multiverse multiverse(move(story._), output, {}, unique_ptr<Multiverse::Listener>(new MultiverseView(scoreAddr)));
    MultiverseView *view = static_cast<MultiverseView *>(multiverse.getListener());
    view->multiverseChanged(multiverse);
    (*mode)(static_cast<iu>(argc) - 3, argv + 3, multiverse, view);

    return 0;
  } catch (exception &e) {
    fprintf(stderr, "Error: %s\n", narrowise(core::createExceptionMessage(e, false)));
    return 1;
  }
}

void runCmd (iu argsSize, char **args, Multiverse &multiverse, MultiverseView *view) {
  const char *outPathName = nullptr;
  if (argsSize == 1) {
    outPathName = args[0];
  }

  u8string message;
  updateMultiverseDisplay(multiverse, outPathName, message);

  u8string in;
  while (true) {
    readLine(in);
    size_t comment = in.find(u8("#")[0]);
    if (comment != u8string::npos) {
      in.erase(comment);
    }

    bool done = !runCommandLine(multiverse, in, message);
    in.clear();
    if (done) {
      break;
    }

    updateMultiverseDisplay(multiverse, outPathName, message);
    message.clear();
  }
}

void runVelocityrun (iu argsSize, char **args, Multiverse &multiverse, MultiverseView *view) {
  if (argsSize != 6 && argsSize != 7) {
    throw PlainException(u8("command lines to be run initially, each round, on no change in node count, on max score change, on new words being used and on user exit must be specified"));
  }
  // TODO uargs
  u8string initialCommandLineTemplate(reinterpret_cast<char8_t *>(args[0]));
  u8string roundCommandLineTemplate(reinterpret_cast<char8_t *>(args[1]));
  u8string nullChangeCommandLineTemplate(reinterpret_cast<char8_t *>(args[2]));
  u8string maxScoreChangeCommandLineTemplate(reinterpret_cast<char8_t *>(args[3]));
  u8string wordsChangeCommandLineTemplate(reinterpret_cast<char8_t *>(args[4]));
  u8string stopCommandLineTemplate(reinterpret_cast<char8_t *>(args[5]));
  iu initialRoundCount = 0;
  double initialTTotal = 0;
  double initialTVm = 0;
  if (argsSize == 7) {
    int v0;
    float v1;
    float v2;
    if (sscanf(args[6], "%d,%f,%f", &v0, &v1, &v2) != 3 || v0 < 0) {
      throw PlainException(u8("invalid initial stats state specified"));
    }
    initialRoundCount = static_cast<unsigned int>(v0);
    initialTTotal = v1;
    initialTVm = v2;
  }

  u8string message;
  vector<tuple<u8string, iu>> history;

  volatile bool stopRequested = false;
  thread waiter([&] () {
    u8string in;
    readLine(in);
    stopRequested = true;
  });
  waiter.detach();

  if (!runCommandLineTemplate(multiverse, initialCommandLineTemplate, 0, message, history)) {
    return;
  }

  auto getUserTime = [] () -> double {
    return 0;
  };
  auto getVmTime = [] () -> double {
    return 0;
  };
  double tTotal0 = getUserTime() - initialTTotal;
  double tVm0 = getVmTime() - initialTVm;

  printf("\"Number of rounds\",\"total time\",\"VM time\",\"max score\",\"node count\"\n");
  auto printStats = [&] (iu roundCount, double tTotal1, double tVm1) {
    printf("%u,%f,%f,%d,%u\n", roundCount, tTotal1 - tTotal0, tVm1 - tVm0, view->getMaxScoreValue(multiverse), static_cast<iu>(view->nodesByIndex.size()));
    fflush(stdout);
  };
  printStats(initialRoundCount, getUserTime(), getVmTime());

  iu nullChangeCount = 0;
  for (iu round = initialRoundCount; true; ++round) {
    if (stopRequested) {
      printf("  stopping\n");
      fflush(stdout);
      runCommandLineTemplate(multiverse, stopCommandLineTemplate, round, message, history);
      break;
    }

    size_t nodesSize0 = view->nodesByIndex.size();
    Value maxScoreValue0 = view->getMaxScoreValue(multiverse);
    Bitset words0(view->getInterestingChildActionWords(multiverse));
    if (!runCommandLineTemplate(multiverse, roundCommandLineTemplate, round + 1, message, history)) {
      break;
    }

    double tTotal1 = getUserTime();
    double tVm1 = getVmTime();
    printStats(round + 1, tTotal1, tVm1);

    size_t nodesSize1 = view->nodesByIndex.size();
    if (nodesSize1 == nodesSize0) {
      ++nullChangeCount;
      printf("  node count unchanged (%u %s)\n", nullChangeCount, nullChangeCount == 1 ? "time" : "times");
      fflush(stdout);
      if (nullChangeCount >= 32) {
        break;
      }

      if (!runCommandLineTemplate(multiverse, nullChangeCommandLineTemplate, round + 1, message, history)) {
        break;
      }

      tTotal1 = getUserTime();
      tVm1 = getVmTime();
      printStats(round + 1, tTotal1, tVm1);
    } else {
      nullChangeCount = 0;
    }

    Value maxScoreValue1 = view->getMaxScoreValue(multiverse);
    bool maxScoreChanged = maxScoreValue1 != maxScoreValue0;
    Bitset words1(view->getInterestingChildActionWords(multiverse));
    words1.andNot(words0);
    bool wordsChanged = !words1.empty();
    if (maxScoreChanged) {
      printf("  max score changed (%d)\n", maxScoreValue1);
      fflush(stdout);
    }
    if (wordsChanged) {
      u8string wordList;
      appendWordList(wordList, words1, multiverse);
      printf("  words added (%s)\n", narrowise(wordList));
      fflush(stdout);
    }
    if (maxScoreChanged) {
      if (!runCommandLineTemplate(multiverse, maxScoreChangeCommandLineTemplate, round + 1, message, history)) {
        break;
      }
    }
    if (wordsChanged) {
      if (!runCommandLineTemplate(multiverse, wordsChangeCommandLineTemplate, round + 1, message, history)) {
        break;
      }
    }

    double tTotal2 = getUserTime();
    double tVm2 = getVmTime();
    tTotal0 -= (tTotal2 - tTotal1);
    tVm0 -= (tVm2 - tVm1);

    message.clear();
  }

  printf("\nCommands run:\n");
  for (const auto &e : history) {
    printf("x%u %s\n", get<1>(e), narrowise(get<0>(e)));
  }
  fflush(stdout);
}

bool runCommandLineTemplate (Multiverse &r_multiverse, const u8string &inTemplate, iu roundCount, u8string &r_message, vector<tuple<u8string, iu>> &r_history) {
  char8_t b[1024];
  sprintf(reinterpret_cast<char *>(b), "%u", roundCount);
  u8string sub(b);

  u8string in;

  size_t i = 0;
  while (true) {
    size_t i0 = i;
    i = inTemplate.find(u8("%")[0], i);
    if (i == u8string::npos) {
      in.append(inTemplate, i0, u8string::npos);
      break;
    }
    in.append(inTemplate, i0, i - i0);
    in.append(sub);
    ++i;
  }

  if (in.empty()) {
    return true;
  }

  tuple<u8string, iu> *e;
  if (!r_history.empty() && get<0>(*(e = &r_history.back())) == in) {
    ++get<1>(*e);
  } else {
    r_history.emplace_back(in, 1);
  }

  return runCommandLine(r_multiverse, in, r_message);
}

void appendWordList (u8string &r_o, const Bitset &words, const Multiverse &multiverse) {
  auto &actionSet = multiverse.getActionSet();
  bool first = true;
  for (size_t i = words.getNextSetBit(0); i != Bitset::NON_INDEX; i = words.getNextSetBit(i + 1)) {
    if (!first) {
      r_o.append(u8(", "));
    }
    auto word = actionSet.getWord(i);
    r_o.append(word.begin(), word.end());
    first = false;
  }
}

bool runCommandLine (Multiverse &multiverse, const u8string &in, u8string &message) {
  MultiverseView *view = static_cast<MultiverseView *>(multiverse.getListener());
  vector<Node *> &nodesByIndex = view->nodesByIndex;
  unordered_set<Node *> &selectedNodes = view->selectedNodes;
  unordered_set<Node *> &verboseNodes = view->verboseNodes;
  bool &elideDeadEndNodes = view->elideDeadEndNodes;
  bool &elideAntiselectedNodes = view->elideAntiselectedNodes;
  size_t &maxDepth = view->maxDepth;
  bool &combineSimilarSiblings = view->combineSimilarSiblings;

  const char8_t *inI = in.data();
  const char8_t *inEnd = inI + in.size();
  while (inI != inEnd) {
    Bitset seenWords0(view->getInterestingChildActionWords(multiverse));

    const char8_t *inPartBegin = inI = skipSpaces(inI, inEnd);
    const char8_t *inPartEnd = inI = skipNonSpaces(inI, inEnd);
    u8string line(inPartBegin, inPartEnd);

    if (line == u8("Q") || line == u8("q")) {
      return false;
    } else if (line.size() > 1 && (line[0] == u8("X")[0] || line[0] == u8("x")[0])) {
      is n = getNaturalNumber(line.data() + 1, line.data() + line.size());
      if (n >= 0) {
        iu times = static_cast<iu>(n);
        u8string subLine(inPartEnd, inEnd);
        for (iu i = 0; i != times; ++i) {
          bool notDone = runCommandLine(multiverse, subLine, message);
          if (!notDone) {
            return false;
          }
        }
        return true;
      }
    } else if (line == u8("D") || line == u8("d")) {
      elideDeadEndNodes = !elideDeadEndNodes;
    } else if (line == u8("N") || line == u8("n")) {
      elideAntiselectedNodes = !elideAntiselectedNodes;
    } else if (line == u8("B") || line == u8("b")) {
      maxDepth = numeric_limits<size_t>::max();
    } else if (line.size() > 2 && (line[0] == u8("B")[0] || line[0] == u8("b")[0]) && line[1] == u8("-")[0]) {
      is n = getNaturalNumber(line.data() + 2, line.data() + line.size());
      if (n >= 0) {
        maxDepth = static_cast<iu>(n);
      }
    } else if (line == u8("M") || line == u8("m")) {
      combineSimilarSiblings = !combineSimilarSiblings;
    } else if (line == u8("A") || line == u8("a")) {
      selectedNodes.clear();
      for (const auto &node : nodesByIndex) {
        selectedNodes.insert(node);
      }
      view->selectionChanged();
    } else if (line == u8("U") || line == u8("u")) {
      for (const auto &node : nodesByIndex) {
        if (node->getState()) {
          selectedNodes.insert(node);
        }
      }
      view->selectionChanged();
    } else if (line == u8("C") || line == u8("c")) {
      selectedNodes.clear();
      view->selectionChanged();
    } else if (line == u8("I") || line == u8("i")) {
      unordered_set<Node *> nextSelectedNodes(nodesByIndex.size() - selectedNodes.size());
      for (const auto &node : nodesByIndex) {
        if (!contains(selectedNodes, node)) {
          nextSelectedNodes.insert(node);
        }
      }
      selectedNodes = move(nextSelectedNodes);
      view->selectionChanged();
    } else if (line.size() > 3 && (line[0] == u8("V")[0] || line[0] == u8("v")[0]) && line[1] == u8("-")[0]) {
      char8_t valueName = line[2];
      size_t valueIndex;
      if (valueName >= u8("a")[0] && valueName <= u8("z")[0] && (valueIndex = static_cast<size_t>(valueName - u8("a")[0])) < NodeMetricsListener::VALUE_COUNT) {
        if (line.size() > 5 && line[3] == u8(">")[0] && line[4] == u8("=")[0]) {
          const char8_t *numBegin = line.data() + 5;
          const char8_t *numEnd = line.data() + line.size();
          is n = getNaturalNumber(numBegin, numEnd);
          if (n >= 0) {
            size_t prevSize = selectedNodes.size();
            size_t unprocessedCount = 0;
            for (auto i = selectedNodes.begin(), end = selectedNodes.end(); i != end;) {
              Node *node = *i;
              Value value = static_cast<NodeView *>(node->getListener())->getValue(valueIndex);
              if (value < n) {
                i = selectedNodes.erase(i);
                continue;
              }
              unprocessedCount += !!node->getState();
              ++i;
            }

            char8_t b[1024];
            sprintf(reinterpret_cast<char *>(b), "Selected %u (%u unprocessed) (from %u) nodes\n\n", static_cast<iu>(selectedNodes.size()), static_cast<iu>(unprocessedCount), static_cast<iu>(prevSize));
            message.append(b);
          }
        } else if (line.size() > 4) {
          const char8_t *numBegin = line.data() + 3;
          const char8_t *numEnd = line.data() + line.size() - 1;
          bool undershoot = false;
          bool overshootOnEmpty = false;
          switch (*numEnd) {
            case U'+':
              break;
            case U'-':
              undershoot = true;
              break;
            case U'|':
              undershoot = true;
              overshootOnEmpty = true;
              break;
            default:
              numBegin = nullptr;
              break;
          }
          if (numBegin) {
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
                for (auto i = nodes.begin(), end = i + count; i != end; ++i) {
                  Node *node = get<1>(*i);
                  unprocessedCount += !!node->getState();
                  selectedNodes.insert(node);
                }
                view->selectionChanged();

                char8_t b[1024];
                char *t = reinterpret_cast<char *>(b);
                t += sprintf(t, "Selected %u (%u unprocessed) (from %u) nodes", static_cast<iu>(count), static_cast<iu>(unprocessedCount), static_cast<iu>(nodes.size()));
                if (count != 0) {
                  t += sprintf(t, " (threshold metric value %d)", get<0>(nodes[count - 1]));
                }
                t += sprintf(t, "\n\n");
                message.append(b);
              } else {
                char8_t b[1024];
                sprintf(reinterpret_cast<char *>(b), "Selected %u nodes (selection unchanged)\n\n", static_cast<iu>(selectedNodes.size()));
                message.append(b);
              }
            }
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
        multiverse.processNodes(t.begin(), t.end());
      }
      view->multiverseChanged(multiverse);
    } else if (line == u8("L") || line == u8("l")) {
      if (!selectedNodes.empty()) {
        multiverse.collapseNodes(selectedNodes.cbegin(), selectedNodes.cend());
      }
      view->multiverseChanged(multiverse);
    } else if (line == u8("T") || line == u8("t")) {
      for (Node *node : selectedNodes) {
        node->clearState();
      }
      view->multiverseChanged(multiverse);
    } else if (line.size() > 2 && (line[0] == u8("E")[0] || line[0] == u8("e")[0]) && line[1] == u8("-")[0]) {
      u8string name(line.data() + 2, line.data() + line.size());
      multiverse.save(reinterpret_cast<const char *>(name.c_str()));
    } else if (line.size() > 2 && (line[0] == u8("O")[0] || line[0] == u8("o")[0]) && line[1] == u8("-")[0]) {
      u8string name(line.data() + 2, line.data() + line.size());
      multiverse.load(reinterpret_cast<const char *>(name.c_str()));
      view->multiverseChanged(multiverse);
    } else {
      const char8_t *numBegin = line.data();
      const char8_t *numEnd = numBegin + line.size();
      const char8_t *dash = find(numBegin, numEnd, u8("-")[0]);

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
        view->selectionChanged();
      }
    }

    Bitset newWords(view->getInterestingChildActionWords(multiverse));
    newWords.andNot(seenWords0);
    if (!newWords.empty()) {
      message.append(u8("New words: "));
      appendWordList(message, newWords, multiverse);
      message.append(u8("\n\n"));
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
    printf("%s", narrowise(message));
  }
  printf(
    "%s Hide _Dead End Nodes                   _Show Output\n"
    "%s Hide A_ntiselected Nodes               _Hide Output\n"
    "%s Hide Nodes _Beyond Depth...          ──────────────\n"
    "%s Co_mbine Similar Siblings              _Process    \n"
    "───────────────────────────────────      Co_llapse   \n"
    "  Select _All                            _Terminate  \n"
    "  Select _Unprocesseds                 ──────────────\n"
    "  _Clear Selection                       Sav_e As... \n"
    "  _Invert Selection                      _Open...    \n"
    "  Shrink Selection to Top_Valued...      _Quit        \n"
    ">",
    narrowise(getOptionIcon(view->elideDeadEndNodes)),
    narrowise(getOptionIcon(view->elideAntiselectedNodes)),
    narrowise(getOptionIcon(view->maxDepth != numeric_limits<size_t>::max())),
    narrowise(getOptionIcon(view->combineSimilarSiblings))
  );
  fflush(stdout);
}

const char8_t *getOptionIcon (bool enabled) {
  return enabled ? u8("\u2612") : u8("\u2610");
}

constexpr size_t NodeMetricsListener::VALUE_COUNT;
constexpr Value NodeMetricsListener::NON_VALUE;

NodeMetricsListener::NodeMetricsListener () :
  scoreValue(NON_VALUE), locationHash(0), visitageValue(NON_VALUE), localOutputtageValue(false), outputtageValue(NON_VALUE - 1)
{
}

template<typename _Walker> void NodeMetricsListener::beWalked (_Walker &w) {
  DS();
  w.process(scoreValue);
  w.process(locationHash);
  w.process(visitageValue);
  w.process(localOutputtageValue);
  w.process(outputtageValue);
}

Value NodeMetricsListener::getValue (size_t i) const {
  DPRE(i < VALUE_COUNT);
  static constexpr Value NodeMetricsListener::*const values[] = {
    &NodeMetricsListener::scoreValue,
    &NodeMetricsListener::scoreValue,
    &NodeMetricsListener::visitageValue,
    &NodeMetricsListener::localOutputtageValue,
    &NodeMetricsListener::outputtageValue
  };
  DA(sizeof(values) / sizeof(*values) == VALUE_COUNT);
  return this->*(values[i]);
}

const Value MultiverseMetricsListener::PER_TURN_VISITAGE_MODIFIER = -0;
const vector<Value> MultiverseMetricsListener::NEW_LOCATION_VISITAGE_MODIFIERS = {20, 13, 8, 5};
const Value MultiverseMetricsListener::OLD_LOCATION_VISITAGE_MODIFIER = -200;

const size_t MultiverseMetricsListener::OUTPUTTAGE_CHILD_OUTPUT_PRESKIP = 1;
const f64 MultiverseMetricsListener::PER_TURN_OUTPUTTAGE_SCALE = 1;
const Value MultiverseMetricsListener::NOVEL_OUTPUTTAGE_MODIFIER = 1;

MultiverseMetricsListener::MultiverseMetricsListener (zword scoreAddr) :
  scoreAddr(scoreAddr), interestingChildActionWordsIsDirty(false)
{
}

void MultiverseMetricsListener::nodeReached (const Multiverse &multiverse, Node::Listener *listener_, ActionSet::Size parentActionId, const u8string &output, const Signature &signature, const Vm &vm) {
  DW(, "DDDD created new node with sig of hash ", signature.hashSlow());
  NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(listener_);

  setScoreValue(listener, vm);
  setVisitageData(listener, output);
}

void MultiverseMetricsListener::setScoreValue (NodeMetricsListener *listener, const Vm &vm) {
  DPRE((static_cast<iu>(scoreAddr) + 1) < vm.getDynamicMemorySize() + 0);
  const zbyte *m = vm.getDynamicMemory();
  listener->scoreValue = static_cast<make_signed<zword>::type>(static_cast<zword>(static_cast<zword>(m[scoreAddr] << 8) | m[scoreAddr + 1]));
  DW(, "DDDD game score is ",listener->scoreValue);
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
    if (i == outEnd || *i == u8(" ")[0]) {
      break;
    }
    outI = i;
  }
  const char8_t *locationEnd = outI;
  DW(, "DDDD location string is ", u8string(locationBegin, locationEnd).c_str());

  listener->locationHash = u8string(locationBegin, locationEnd).hashSlow();
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
  visitageValue += PER_TURN_VISITAGE_MODIFIER;
  if (listener->locationHash == locationHash) {
    DW(, "DDDD   location didn't change");
    if (newLocationVisitageModifiersI != NEW_LOCATION_VISITAGE_MODIFIERS.end()) {
      visitageValue += *newLocationVisitageModifiersI++;
    }
  } else {
    locationHash = listener->locationHash;
    auto begin = visitedLocationHashes.begin();
    auto end = visitedLocationHashes.end();
    if (find(begin, end, locationHash) != end) {
      DW(, "DDDD   location changed to one we've visted");
      newLocationVisitageModifiersI = NEW_LOCATION_VISITAGE_MODIFIERS.end();
      visitageValue += OLD_LOCATION_VISITAGE_MODIFIER;
    } else {
      DW(, "DDDD   location changed to one we've not visted");
      visitedLocationHashes.emplace_back(locationHash);
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

  DW(, "DDDD   looking at node with sig of hash ",node->getSignature().hashFast());
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

size_t MultiverseMetricsListener::checkVisitageValueRecursively (const Node *node, NodeMetricsListener *listener, VisitageChain chain) {
  DA(!node->getPrimeParentNode() || static_cast<NodeMetricsListener *>(node->getPrimeParentNode()->getListener())->visitageValue == chain.getVisitageValue());
  chain.increment(listener);
  DA(listener->visitageValue == chain.getVisitageValue());
  size_t c = 1;

  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    if (childNode->getPrimeParentNode() == node && childNode->getPrimeParentArcChildIndex() == i) {
      NodeMetricsListener *childListener = static_cast<NodeMetricsListener *>(childNode->getListener());
      c += checkVisitageValueRecursively(childNode, childListener, chain);
    }
  }

  return c;
}

void MultiverseMetricsListener::nodeProcessed (const Multiverse &multiverse, const Node *node, size_t processedCount, size_t totalCount) {
  setWordData(node, multiverse.getActionSet());
}

void MultiverseMetricsListener::setWordData (const Node *node, const ActionSet &actionSet) {
  if (interestingChildActionWordsIsDirty) {
    return;
  }

  Bitset &words = interestingChildActionWords;
  words.ensureWidth(actionSet.getSize());
  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    auto action = actionSet.get(get<0>(node->getChild(i)));
    for (size_t i = 0, end = action.getWordCount(); i != end; ++i) {
      words.setExistingBit(action.getWord(i));
    }
  }
}

void MultiverseMetricsListener::nodesProcessed (const Multiverse &multiverse) {
  setOutputtageValues(multiverse);
}

void MultiverseMetricsListener::setOutputtageValues (const autoinf::Multiverse &multiverse) {
  const Node *rootNode = multiverse.getRoot();

  deque<const Node *> nodes;
  Bitset stringsToThisDepth, stringsToNextDepth;

  NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(rootNode->getListener());
  listener->outputtageValue = NodeMetricsListener::NON_VALUE;
  listener->localOutputtageValue = false;
  nodes.emplace_back(rootNode);

  do {
    swap(stringsToThisDepth, stringsToNextDepth);
    stringsToNextDepth = stringsToThisDepth;

    for (auto c = nodes.size(); c != 0; --c) {
      const Node *node = nodes.front();
      nodes.pop_front();

      for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
        const auto &child = node->getChild(i);
        const auto &childOutput = get<1>(child);
        const Node *childNode = get<2>(child);
        NodeMetricsListener *childListener = static_cast<NodeMetricsListener *>(childNode->getListener());
        DA((childNode->getPrimeParentNode() == node && childNode->getPrimeParentArcChildIndex() == i) == (childListener->outputtageValue != NodeMetricsListener::NON_VALUE));
        if (childListener->outputtageValue != NodeMetricsListener::NON_VALUE) {
          childListener->outputtageValue = NodeMetricsListener::NON_VALUE;
          childListener->localOutputtageValue = false;
          nodes.emplace_back(childNode);
        }
        if (childOutput.size() >= OUTPUTTAGE_CHILD_OUTPUT_PRESKIP) {
          for (auto i = childOutput.begin() + OUTPUTTAGE_CHILD_OUTPUT_PRESKIP, end = childOutput.end(); i != end; ++i) {
            size_t inpara = *i;
            if (!stringsToThisDepth.getBit(inpara)) {
              stringsToNextDepth.setBit(inpara);
              childListener->localOutputtageValue = true;
            }
          }
        }
      }
    }
  } while (!nodes.empty());

  setOutputtageValueRecursively(rootNode, listener, 0);
}

void MultiverseMetricsListener::setOutputtageValueRecursively (const Node *node, NodeMetricsListener *listener, Value parentValue) {
  setOutputtageValue(node, listener, parentValue);

  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    if (childNode->getPrimeParentNode() == node) {
      NodeMetricsListener *childListener = static_cast<NodeMetricsListener *>(childNode->getListener());
      DA((childNode->getPrimeParentArcChildIndex() == i) == (childListener->outputtageValue == NodeMetricsListener::NON_VALUE));
      if (childListener->outputtageValue == NodeMetricsListener::NON_VALUE) {
        setOutputtageValueRecursively(childNode, childListener, listener->outputtageValue);
        DA(childListener->outputtageValue != NodeMetricsListener::NON_VALUE);
      }
    }
  }
}

void MultiverseMetricsListener::setOutputtageValue (const Node *node, NodeMetricsListener *listener, Value parentValue) {
  bool novel = listener->localOutputtageValue;
  DA(parentValue != NodeMetricsListener::NON_VALUE);
  listener->outputtageValue = static_cast<Value>(parentValue * PER_TURN_OUTPUTTAGE_SCALE) + (novel ? NOVEL_OUTPUTTAGE_MODIFIER : 0);
}

void MultiverseMetricsListener::nodeCollapsed (const Multiverse &multiverse, const Node *node, bool childrenUpdated) {
}

void MultiverseMetricsListener::nodesCollapsed (const Multiverse &multiverse) {
  nodesProcessed(multiverse);

  interestingChildActionWordsIsDirty = true;
}

void MultiverseMetricsListener::loaded (const Multiverse &multiverse) {
  interestingChildActionWordsIsDirty = true;
}

Value MultiverseMetricsListener::getMaxScoreValue (const Multiverse &multiverse) const {
  Value maxScoreValue = numeric_limits<Value>::min();
  for (const Node *node : multiverse) {
    if (node->getState()) {
      NodeMetricsListener *listener = static_cast<NodeMetricsListener *>(node->getListener());
      if (listener->scoreValue > maxScoreValue) {
        maxScoreValue = listener->scoreValue;
      }
    }
  }
  return maxScoreValue;
}

const Bitset &MultiverseMetricsListener::getInterestingChildActionWords (const Multiverse &multiverse) {
  if (interestingChildActionWordsIsDirty) {
    interestingChildActionWords.clear();
    interestingChildActionWordsIsDirty = false;
    for (const Node *node : multiverse) {
      setWordData(node, multiverse.getActionSet());
    }
  }

  return interestingChildActionWords;
}

constexpr size_t NodeView::NON_INDEX;

NodeView::NodeView () :
  index(NON_INDEX - 1), primeParentChildIndex(ActionSet::NON_ID), isDeadEnd(false), isAntiselected(false)
{
}

template<typename _Walker> void NodeView::beWalked (_Walker &w) {
  w.process(*static_cast<NodeMetricsListener *>(this));
}

MultiverseView::MultiverseView (zword scoreAddr) :
  MultiverseMetricsListener(scoreAddr), elideDeadEndNodes(false), elideAntiselectedNodes(false), maxDepth(numeric_limits<size_t>::max()), combineSimilarSiblings(false),
  deadEndnessIsDirty(true), antiselectednessIsDirty(true)
{
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

void MultiverseView::nodeProcessed (const Multiverse &multiverse, const Node *node, size_t processedCount, size_t totalCount) {
  MultiverseMetricsListener::nodeProcessed(multiverse, node, processedCount, totalCount);

  time_t now = time(NULL);
  if (processedCount == 1) {
    firstNodeProcessed = prevNodeProcessingProgressReport = now;
    prevNodeProcessingProgressReportSize = 0;
  } else {
    int reportSize = -1;

    if (processedCount == totalCount) {
      reportSize = 0;
    } else {
      DA(processedCount > 1);
      DA(processedCount <= totalCount);
      if (difftime(now, prevNodeProcessingProgressReport) > 2) {
        f64 d = difftime(now, firstNodeProcessed);
        auto remaining = (d / (processedCount - 1)) * (totalCount - processedCount);
        reportSize = printf("Processed %u (%u%%) of %u nodes (%d secs remaining)", static_cast<iu>(processedCount), static_cast<iu>((100 * processedCount) / totalCount), static_cast<iu>(totalCount), static_cast<is>(ceil(remaining / 10)) * 10);
      }
    }

    if (reportSize >= 0) {
      for (int i = reportSize; i < prevNodeProcessingProgressReportSize; ++i) {
        fputc(' ', stdout);
      }
      prevNodeProcessingProgressReport = now;
      prevNodeProcessingProgressReportSize = reportSize;
      fputc('\r', stdout);
      fflush(stdout);
    }
  }
}

unique_ptr<Node::Listener> MultiverseView::createNodeListener () {
  return unique_ptr<Node::Listener>(new NodeView());
}

void MultiverseView::multiverseChanged (const Multiverse &multiverse) {
  nodesByIndex.clear();
  selectedNodes.clear();
  verboseNodes.clear();
  deadEndnessIsDirty = true;
  antiselectednessIsDirty = true;

  studyNodes(multiverse);
}

void MultiverseView::selectionChanged () {
  antiselectednessIsDirty = true;
}

void MultiverseView::studyNodes (const Multiverse &multiverse) {
  DS();
  DA(nodesByIndex.empty());
  DA(selectedNodes.empty());
  DA(verboseNodes.empty());

  for (const Node *node : multiverse) {
    NodeView *nodeView = static_cast<NodeView *>(node->getListener());
    DA(nodeView->index != NodeView::NON_INDEX);
    nodeView->index = NodeView::NON_INDEX;
  }

  studyNode(multiverse.getRoot(), nullptr, ActionSet::NON_ID);
  size_t passBegin = 0;
  size_t passEnd = 1;
  DA(passEnd == nodesByIndex.size());
  iu depth = 0;
  do {
    DW(, "studying nodes at depth ",depth);
    for (size_t i = passBegin; i != passEnd; ++i) {
      Node *node = nodesByIndex[i];
      for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
        Node *childNode = get<2>(node->getChild(i));
        studyNode(childNode, node, i);
      }
    }
    DW(, "after studying nodes at depth ",depth,", we know about ",nodesByIndex.size()," nodes");
    ++depth;

    passBegin = passEnd;
    passEnd = nodesByIndex.size();
  } while (passBegin != passEnd);
}

void MultiverseView::studyNode (Node *node, Node *primeParentNode, ActionSet::Size childIndex) {
  DS();
  DW(, "looking at node with sig of hash ",node->getSignature().hashFast());
  NodeView *nodeView = static_cast<NodeView *>(node->getListener());

  DA(!primeParentNode || (node->getPrimeParentNode() == primeParentNode && node->getPrimeParentArcChildIndex() == childIndex) == (nodeView->index == NodeView::NON_INDEX));
  if (nodeView->index == NodeView::NON_INDEX) {
    nodeView->index = nodesByIndex.size();
    nodeView->primeParentChildIndex = childIndex;
    nodesByIndex.emplace_back(node);
  }
}

void MultiverseView::markDeadEndAndAntiselectedNodes () {
  bool doDeadEndness = deadEndnessIsDirty && elideDeadEndNodes;
  bool doAntiselectedness = antiselectednessIsDirty && elideAntiselectedNodes;
  if (!doDeadEndness && !doAntiselectedness) {
    return;
  }

  for (auto i = nodesByIndex.rbegin(), end = nodesByIndex.rend(); i != end; ++i) {
    Node *node = *i;
    NodeView *nodeView = static_cast<NodeView *>(node->getListener());

    if (doDeadEndness) {
      markDeadEndNode(node, nodeView);
    }
    if (doAntiselectedness) {
      markAntiselectedNode(node, nodeView);
    }
  }

  if (doDeadEndness) {
    deadEndnessIsDirty = false;
  }
  if (doAntiselectedness) {
    antiselectednessIsDirty = false;
  }
}

void MultiverseView::markDeadEndNode (Node *node, NodeView *nodeView) {
  if (node->getState()) {
    DA(!nodeView->isDeadEnd);
    return;
  }

  bool isDeadEnd = true;
  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    NodeView *childNodeView = static_cast<NodeView *>(childNode->getListener());

    if (childNode->getPrimeParentNode() == node && !childNodeView->isDeadEnd) {
      isDeadEnd = false;
      break;
    }
  }
  nodeView->isDeadEnd = isDeadEnd;
}

void MultiverseView::markAntiselectedNode (Node *node, NodeView *nodeView) {
  if (selectedNodes.empty()) {
    nodeView->isAntiselected = true;
    return;
  }

  if (contains(selectedNodes, node)) {
    nodeView->isAntiselected = false;
    return;
  }

  bool isAntiselected = true;
  for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
    Node *childNode = get<2>(node->getChild(i));
    NodeView *childNodeView = static_cast<NodeView *>(childNode->getListener());

    if (childNode->getPrimeParentNode() == node && !childNodeView->isAntiselected) {
      isAntiselected = false;
      break;
    }
  }
  nodeView->isAntiselected = isAntiselected;
}

void MultiverseView::printNodes (const Multiverse &multiverse, FILE *out) {
  markDeadEndAndAntiselectedNodes();

  u8string prefix;
  printNodeAsNonleaf(0, nullptr, multiverse.getRoot(), nullptr, nullptr, nullptr, multiverse, prefix, out);
}

void MultiverseView::printNodeHeader (
  char8_t nodeIndexRenderingPrefix, char8_t nodeIndexRenderingSuffix, Node *node,
  ActionSet::Size *actionIdsI, ActionSet::Size *actionIdsEnd,
  const Multiverse &multiverse, FILE *out
) {
  NodeView *nodeView = static_cast<NodeView *>(node->getListener());

  bool selected = contains(selectedNodes, node);
  fprintf(out, "%c%s %u%c", narrowise(nodeIndexRenderingPrefix), narrowise(getOptionIcon(selected)), static_cast<iu>(nodeView->index), narrowise(nodeIndexRenderingSuffix));

  fprintf(out, "%s", narrowise(renderActionInput(actionIdsI, actionIdsEnd, multiverse.getActionSet())));
  fprintf(out, " -> [&%08X]", static_cast<iu>(node->getSignature().hashFast()));
  fprintf(out, " {");
  for (size_t i = 0; i != NodeView::VALUE_COUNT; ++i) {
    fprintf(out, "%s%d", i == 0 ? "" : ", ", nodeView->getValue(i));
  }
  fprintf(out, "}");
}

u8string MultiverseView::renderActionInput (ActionSet::Size *actionIdsI, ActionSet::Size *actionIdsEnd, const ActionSet &actionSet) {
  u8string actionInput;
  for (; actionIdsI != actionIdsEnd; ++actionIdsI) {
    auto actionId = *actionIdsI;
    DA(actionId != ActionSet::NON_ID);
    actionInput.append(u8(" \""));
    actionSet.get(actionId).getInput(actionInput);
    actionInput.data()[actionInput.size() - 1] = u8("\"")[0];
  }
  return actionInput;
}

void MultiverseView::printNodeOutput (const StringSet<char8_t>::String *output, const Multiverse &multiverse, const u8string &prefix, FILE *out) {
  u8string o;
  DPRE(output);
  multiverse.getOutputStringSet().rebuildString(*output, o);
  char8_t c;
  while (!o.empty() && ((c = o.back()) == u8("\n")[0] || c == u8(">")[0])) {
    o.pop_back();
  }
  o.push_back(u8("\n")[0]);

  auto lineStart = o.cbegin();
  for (auto i = o.cbegin(), end = o.cend(); i != end; ++i) {
    if (*i == u8("\n")[0]) {
      fprintf(out, "%s%.*s\n", narrowise(prefix), static_cast<int>(i - lineStart), narrowise(&*lineStart));
      lineStart = i + 1;
    }
  }
}

void MultiverseView::printNodeAsLeaf (
  size_t depth, const StringSet<char8_t>::String *output, Node *node, Node *parentNode,
  ActionSet::Size *actionIdsI, ActionSet::Size *actionIdsEnd,
  const Multiverse &multiverse, u8string &r_prefix, FILE *out
) {
  printNodeHeader(u8("{")[0], u8("}")[0], node, actionIdsI, actionIdsEnd, multiverse, out);
  fprintf(out, "\n");

  if (output) {
    size_t prefixSize = r_prefix.size();
    r_prefix.append(u8("  "));
    printNodeOutput(output, multiverse, r_prefix, out);
    r_prefix.resize(prefixSize);
  }
}

void MultiverseView::printNodeAsNonleaf (
  size_t depth, const StringSet<char8_t>::String *output, Node *node, Node *parentNode,
  ActionSet::Size *actionIdsI, ActionSet::Size *actionIdsEnd,
  const Multiverse &multiverse, u8string &r_prefix, FILE *out
) {
  printNodeHeader(u8("(")[0], u8(")")[0], node, actionIdsI, actionIdsEnd, multiverse, out);
  if (node->getState()) {
    fprintf(out, " / unprocessed\n");
  } else {
    size_t c = node->getChildrenSize();
    fprintf(out, " / %u %s\n", static_cast<iu>(c), c == 1 ? "child" : "children");
  }

  enum {
    NONE,
    LEAF,
    NONLEAF
  } fmts[node->getChildrenSize()];
  size_t fmtsEnd;
  unique_ptr<unordered_map<Node *, vector<ActionSet::Size>>> childGroups;

  DA(depth <= maxDepth);
  if (depth == maxDepth) {
    fmtsEnd = 0;
  } else {
    fmtsEnd = 0;
    if (combineSimilarSiblings) {
      childGroups.reset(new unordered_map<Node *, vector<ActionSet::Size>>(node->getChildrenSize()));
    }

    for (size_t i = 0, end = node->getChildrenSize(); i != end; ++i) {
      auto &child = node->getChild(i);
      ActionSet::Size childActionId = get<0>(child);
      Node *childNode = get<2>(child);
      DA(&node->getChild(node->getChildIndex(childActionId)) == &child);
      NodeView *childNodeView = static_cast<NodeView *>(childNode->getListener());

      bool elided = (elideDeadEndNodes && childNodeView->isDeadEnd) || (elideAntiselectedNodes && childNodeView->isAntiselected);
      bool elision = elideDeadEndNodes || elideAntiselectedNodes;
      auto fmt = (childNode->getPrimeParentNode() == node && childNodeView->primeParentChildIndex == i && !elided) ? NONLEAF : !elision ? LEAF : NONE;
      fmts[i] = fmt;
      if (fmt != NONE) {
        fmtsEnd = i + 1;

        if (childGroups) {
          auto childGroupsI = childGroups->find(childNode);
          if (childGroupsI == childGroups->end()) {
            vector<ActionSet::Size> t;
            t.reserve(8);
            childGroupsI = get<0>(childGroups->emplace(childNode, move(t)));
          } else {
            DA(fmt == LEAF);
          }
          get<1>(*childGroupsI).emplace_back(childActionId);
        }
      }
    }
  }

  ++depth;

  size_t prefixSize = r_prefix.size();
  if (output) {
    bool last = fmtsEnd == 0;
    r_prefix.append(last ? u8("  ") : u8("│ "));
    printNodeOutput(output, multiverse, r_prefix, out);
    r_prefix.resize(prefixSize);
  }
  for (size_t i = 0; i != fmtsEnd; ++i) {
    if (fmts[i] == NONE) {
      continue;
    }
    bool last = i == fmtsEnd - 1;

    auto &child = node->getChild(i);
    ActionSet::Size childActionId = get<0>(child);
    const StringSet<char8_t>::String &childOuput = get<1>(child);
    Node *childNode = get<2>(child);

    ActionSet::Size *childActionIdsBegin;
    ActionSet::Size *childActionIdsEnd;
    vector<ActionSet::Size> t;
    if (childGroups) {
      auto childGroupsI = childGroups->find(childNode);
      if (childGroupsI == childGroups->end()) {
        continue;
      }
      t = move(get<1>(*childGroupsI));
      childGroups->erase(childGroupsI);

      childActionIdsBegin = t.data();
      childActionIdsEnd = childActionIdsBegin + t.size();
    } else {
      childActionIdsBegin = &childActionId;
      childActionIdsEnd = childActionIdsBegin + 1;
    }

    fprintf(out, "%s%s", narrowise(r_prefix), narrowise(last ? u8("└─") : u8("├─")));
    r_prefix.append(last ? u8("  ") : u8("│ "));
    (this->*(fmts[i] == LEAF ? &MultiverseView::printNodeAsLeaf : &MultiverseView::printNodeAsNonleaf))(depth, contains(verboseNodes, childNode) ? &childOuput : nullptr, childNode, node, childActionIdsBegin, childActionIdsEnd, multiverse, r_prefix, out);
    r_prefix.resize(prefixSize);
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
      b[size] = u8("?")[0];
    }
  }
  if (size > 0 && b[size - 1] == u8("\n")[0]) {
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
  for (; i != end && *i == u8(" ")[0]; ++i);
  return i;
}

const char8_t *skipNonSpaces (const char8_t *i, const char8_t *end) {
  for (; i != end && *i != u8(" ")[0]; ++i);
  return i;
}

is getNaturalNumber (const char8_t *iBegin, const char8_t *iEnd) {
  if (iBegin == iEnd || std::isspace(*iBegin)) {
    return -1;
  }

  char8_t in[iEnd - iBegin + 1];
  copy(iBegin, iEnd, in);
  in[iEnd - iBegin] = u8("\0")[0];

  const char8_t *inBegin = in;
  const char8_t *inEnd = inBegin + (iEnd - iBegin);
  char8_t *numberEnd;
  long number = strtol(reinterpret_cast<const char *>(inBegin), reinterpret_cast<char **>(&numberEnd), 10);

  if (numberEnd != inEnd || number < 0 || number == LONG_MAX || number > numeric_limits<is>::max()) {
    return -1;
  }
  return static_cast<is>(number);
}

char narrowise (char8_t c) {
  return static_cast<char>(c);
}

const char *narrowise (const char8_t *s) {
  return reinterpret_cast<const char *>(s);
}

const char *narrowise (const u8string &s) {
  return narrowise(s.c_str());
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
