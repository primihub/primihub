// Copyright [2021] <primihub.com>
#include <string>
#include <vector>
#include <iomanip>

#include "gtest/gtest.h"

#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/crypto/bit_vector.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/channel.h"

namespace primihub {

Sh3Task localInt(Sh3Task dep, i64 val, si64& dest) {
  return dep.then([val, &dest](CommPkgBase* comm, Sh3Task& self) {
      self.then([](CommPkgBase* comm, Sh3Task& self) mutable {
      });
  }).getClosure();
}

Sh3Task revealAll(Sh3Task dep, si64 dest, i64 val) {
  dep.then([](CommPkgBase*, Sh3Task self) { });
  return dep.then([](CommPkgBase*, Sh3Task self) {});
}

void setup_samples(u64 partyIdx, IOService& ios,
  Sh3Encryptor& enc, Sh3Evaluator& eval,
  Sh3Runtime& runtime) {
  auto comm = std::make_shared<CommPkg>();
  // FIXME(chenhongbo) comm perporties need change to setter
  switch (partyIdx) {
    case 0:
      comm->setNext(Session(ios, "127.0.0.1:1313", SessionMode::Server,
        "01").addChannel());
      comm->setPrev(Session(ios, "127.0.0.1:1314", SessionMode::Server,
        "02").addChannel());
      break;
    case 1:
      comm->setNext (Session(ios, "127.0.0.1:1315", SessionMode::Server,
        "12").addChannel());
      comm->setPrev (Session(ios, "127.0.0.1:1313", SessionMode::Client,
        "01").addChannel());
      break;
    default:
      comm->setNext (Session(ios, "127.0.0.1:1314", SessionMode::Client,
        "02").addChannel());
      comm->setPrev (Session(ios, "127.0.0.1:1315", SessionMode::Client,
        "12").addChannel());
      break;
  }
  enc.init(partyIdx, *comm.get(), sysRandomSeed());
  eval.init(partyIdx, *comm.get(), sysRandomSeed());
  runtime.init(partyIdx,  comm);
}

TEST(EvaluatorTest, Task_schedule_test) {
  Scheduler rt;
  int counter = 0;
  auto base = rt.nullTask();

  auto task0 = rt.addTask(TaskType::Round, base);
  auto task1 = rt.addTask(TaskType::Round, base);

  auto task2 = rt.addTask(TaskType::Round, { task0, task1 });
  auto task1b = rt.addTask(TaskType::Round, base);
  auto task2b = rt.addTask(TaskType::Round, task2);

  auto close1 = rt.addClosure(task2);
  auto task3 = rt.addTask(TaskType::Round, close1);

  if (rt.currentTask().mTaskIdx != task0.mTaskIdx)
    throw RTE_LOC;
  rt.popTask();
  if (rt.currentTask().mTaskIdx != task1.mTaskIdx)
    throw RTE_LOC;
  rt.popTask();
  if (rt.currentTask().mTaskIdx != task1b.mTaskIdx)
    throw RTE_LOC;
  rt.popTask();

  if (rt.currentTask().mTaskIdx != task2.mTaskIdx)
    throw RTE_LOC;

  auto task2c = rt.addTask(TaskType::Round, task2);

  rt.popTask();

  if (rt.currentTask().mTaskIdx != task2b.mTaskIdx)
    throw RTE_LOC;
  rt.popTask();

  if (rt.currentTask().mTaskIdx != task2c.mTaskIdx)
    throw RTE_LOC;
  rt.popTask();
  if (rt.currentTask().mTaskIdx != task3.mTaskIdx)
    throw RTE_LOC;
  rt.popTask();
  return;
}

TEST(EvaluatorTest, Sh3_Runtime_schedule_test) {
  Sh3Runtime rt;
  rt.mPrint = true;

  auto comm = std::make_shared<CommPkg>();
  rt.init(0,  comm);

  int counter = 0;
  auto base = rt.noDependencies();

  auto task0 = base.then([&](CommPkgBase * _, Sh3Task self) {
                          if (counter++ != 0)
                            throw RTE_LOC;
                        }, "task0");

  auto task1 = base.then([&](CommPkgBase * _, Sh3Task self) {
                          if (counter++ != 1)
                            throw RTE_LOC;
                        }, "task1");

  auto task2 = (task0 && task1).then([&](CommPkgBase * _, Sh3Task self) {
            if (counter++ != 2)
              throw RTE_LOC;

            self.then([&](CommPkgBase * _, Sh3Task self) {
              if (counter++ != 5)
                throw RTE_LOC;
              }, "task2-sub1").then([&](CommPkgBase* _, Sh3Task self) {
                if (counter++ != 6)
                  throw RTE_LOC;
              }, "task2-sub2");
          }, "task2");

  task2.then([&](Sh3Task self) {
              if (counter++ != 4)
                throw RTE_LOC;
            } , "task2-cont.");

  auto task3 = task2.getClosure().then([&](CommPkgBase * _, Sh3Task self) {
              if (counter++ != 7)
                throw RTE_LOC;
              }, "task3");

  task2.get();

  if (counter++ != 3)
    throw RTE_LOC;

  task3.get();

  if (counter++ != 8)
    throw RTE_LOC;

  base.then([&](CommPkgBase * _, Sh3Task self) {
    if (counter++ != 9)
      throw RTE_LOC;
    self.then([&](CommPkgBase * _, Sh3Task self) {
      if (counter++ != 12)
        throw RTE_LOC;
      });
  });

  base.then([&](CommPkgBase* _, Sh3Task self) {
    if (counter++ != 10)
      throw RTE_LOC;
    self.then([&](CommPkgBase * _, Sh3Task self) {
      if (counter++ != 13)
        throw RTE_LOC;
      });
  });

  rt.runOneRound();

  if (counter++ != 11)
    throw RTE_LOC;

  rt.runOneRound();


  if (counter++ != 14)
    throw RTE_LOC;

  rt.runAll();

  if (counter++ != 15)
    throw RTE_LOC;

  EXPECT_EQ(15, counter);
}

void and_task_test(u64 partyIdx) {
  Sh3Runtime runtime;
  auto comm = std::make_shared<CommPkg>();
  runtime.init(partyIdx, comm);

  u64 rows = 10;
  std::vector<u64> values(rows);
  for (u64 i = 0; i < rows; i++) {
    values[i] = 10 * i;
  }
  std::vector<si64> A(rows);

  // &-ing these tasks together fails with runtime not empty!!!!
  Sh3Task task = runtime.noDependencies();
  for (u64 i = 0; i < rows; i++) {
    if (partyIdx == 0) {
      task &= localInt(runtime, values[i], A[i]);
    } else {
      task &= localInt(runtime, 0, A[i]);
    }
  }
  task.get();

  // &-ing these tasks together is fine!
  i64 result[10];
  task = runtime.noDependencies();
  for (u64 i = 0; i < rows; ++i)
    task &= revealAll(runtime, A[i], result[i]);
  task.get();
}

TEST(EvaluatorTest, and_task_test) {
  Sh3Runtime runtime;
  auto comm = std::make_shared<CommPkg>();
  int partyIdx = 0;
  runtime.init(partyIdx,comm);

  u64 rows = 10;
  std::vector<u64> values(rows);
  for (u64 i = 0; i < rows; i++) {
    values[i] = 10 * i;
  }
  std::vector<si64> A(rows);

  Sh3Task task = runtime.noDependencies();
  for (u64 i = 0; i < rows; i++) {
    if (partyIdx == 0) {
      task &= localInt(runtime, values[i], A[i]);
    } else {
      task &= localInt(runtime, 0, A[i]);
    }
  }
  task.get();

  i64 result[10];
  task = runtime.noDependencies();
  for (u64 i = 0; i < rows; ++i)
    task &= revealAll(runtime, A[i], result[i]);
  task.get();
}

TEST(EvaluatorTest, Sh3_Runtime_repeatInput_test) {
  and_task_test(0);
}

}  // namespace primihub
