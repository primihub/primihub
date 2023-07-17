#include <cstdlib>
#include <glog/logging.h>
#include <iostream>
#include <random>

#include "src/primihub/operator/aby3_operator.h"
#include "gtest/gtest.h"

using namespace primihub;

static void run_mpc(uint64_t party_id, std::string ip, uint16_t next_port,
                    uint16_t prev_port, std::vector<double> &vals,
                    std::vector<bool> &cmp_res) {
  std::string next_name, prev_name;
  if (party_id == 0) {
    next_name = "01";
    prev_name = "02";
  } else if (party_id == 1) {
    next_name = "12";
    prev_name = "01";
  } else if (party_id == 2) {
    next_name = "02";
    prev_name = "12";
  }

  MPCOperator *mpc_exec = new MPCOperator(party_id, next_name, prev_name);
  mpc_exec->setup(ip, ip, next_port, prev_port);

  try {
    sbMatrix sh_res;
    if (vals.size() != 0) {
      f64Matrix<D16> m(1, vals.size());
      for (size_t i = 0; i < vals.size(); i++)
        m(i) = vals[i];
      mpc_exec->MPC_Compare(m, sh_res);
    } else {
      mpc_exec->MPC_Compare(sh_res);
    }

    if (party_id == 0) {
      i64Matrix tmp;
      tmp = mpc_exec->reveal(sh_res);
      for (i64 i = 0; i < tmp.rows(); i++)
        cmp_res.emplace_back(static_cast<bool>(tmp(i, 0)));
    } else {
      mpc_exec->reveal(sh_res, 0);
    }
  } catch (std::exception &e) {
    LOG(ERROR) << "In party " << party_id << ":\n" << e.what() << ".";
  }

  mpc_exec->fini();
  delete mpc_exec;
}

TEST(cmp_op, mpc_cmp_op) {
  std::vector<double> party_0_val;
  std::vector<double> party_1_val;
  std::vector<double> party_2_val;

  for (int i = 0; i < 100; i++)
    party_0_val.push_back(1.23456);

  for (int i = 0; i < 100; i++)
    party_1_val.push_back(-10.3656);

  bool standalone_mode = true;
  if (standalone_mode) {
    pid_t pid = fork();
    if (pid != 0) {
      // Party 2.
      std::vector<bool> mpc_res;
      run_mpc(2, "127.0.0.1", 10020, 10030, party_1_val, mpc_res);
      return;
    }

    pid = fork();
    if (pid != 0) {
      // Party 1.
      std::vector<bool> mpc_res;
      run_mpc(1, "127.0.0.1", 10030, 10010, party_2_val, mpc_res);
      return;
    }

    // Party 0.
    std::vector<bool> mpc_res;
    run_mpc(0, "127.0.0.1", 10010, 10020, party_0_val, mpc_res);

    std::vector<bool> local_res;
    for (int i = 0; i < 100; i++)
      local_res.push_back(party_0_val[i] < party_1_val[i]);

    for (int i = 0; i < 100; i++) {
      if (local_res[i] != mpc_res[i]) {
        std::stringstream ss;
        ss << "Find result mismatch between MPC and local, index is " << i
           << ".";
        throw std::runtime_error(ss.str());
      }
    }
  } else {
    std::vector<bool> mpc_res;
    if (!strncmp(std::getenv("MPC_PARTY"), "PARTY_0", strlen("PARTY_0")))
      run_mpc(0, "127.0.0.1", 10010, 10020, party_0_val, mpc_res);
    else if (!strncmp(std::getenv("MPC_PARTY"), "PARTY_1", strlen("PARTY_1")))
      run_mpc(1, "127.0.0.1", 10030, 10010, party_2_val, mpc_res);
    else if (!strncmp(std::getenv("MPC_PARTY"), "PARTY_2", strlen("PARTY_2")))
      run_mpc(2, "127.0.0.1", 10020, 10030, party_1_val, mpc_res);
    else
      LOG(ERROR) << "Invalid MPC_PARTY value " << std::getenv("MPC_PARTY")
                 << ".";
  }
}
