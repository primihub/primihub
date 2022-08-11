// Copyright [2021] <primihub.com>
#pragma once
#include "gtest/gtest.h"
// #include "src/primihub/algorithm/logistic.h"
// #include "src/primihub/service/dataset/localkv/storage_default.h"

#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/protocol/aby3/runtime.h"
#include "src/primihub/protocol/aby3/encryptor.h"
#include "src/primihub/protocol/aby3/evaluator/evaluator.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/commpkg.h"
#include "src/primihub/operator/aby3_operator.h"
using namespace primihub;

TEST(add_operator, aby3_3pc_test)
{
	pid_t pid = fork();
	if (pid != 0)
	{
		// Child process as party 0.
		// integerOperations(0);
		// fixedPointOperations(0);
		MPCOperator mpc(0, "01", "02");
		mpc.setup("127.0.0.1", (u32)1313, (u32)1414);
		u64 rows = 2,
			cols = 2;
		// input data
		eMatrix<i64> plainMatrix(rows, cols);
		for (u64 i = 0; i < rows; ++i)
			for (u64 j = 0; j < cols; ++j)
				plainMatrix(i, j) = i + j;

		std::vector<si64Matrix> sharedMatrix;
		sharedMatrix.emplace_back(si64Matrix(rows, cols));
		sharedMatrix.emplace_back(si64Matrix(rows, cols));
		sharedMatrix.emplace_back(si64Matrix(rows, cols));
		// construct shares
		for (u64 i = 0; i < 3; i++)
		{
			if (i == mpc.partyIdx)
			{
				mpc.createShares(plainMatrix, sharedMatrix[i]);
			}
			else
			{
				mpc.createShares(sharedMatrix[i]);
			}
		}
		// Add
		//  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
		si64Matrix sum;
		mpc.MPC_Add(sharedMatrix, sum);
		LOG(INFO) << "starting reveal";

		i64Matrix sumVal = mpc.reveal(sum);
		LOG(INFO) << "Sum:" << sumVal;
		return;
	}

	pid = fork();
	if (pid != 0)
	{
		// Child process as party 1.
		sleep(1);
		// integerOperations(1);
		// fixedPointOperations(1);
		MPCOperator mpc(1, "12", "01");
		mpc.setup("127.0.0.1", (u32)1515, (u32)1313);
		u64 rows = 2,
			cols = 2;
		// input data
		eMatrix<i64> plainMatrix(rows, cols);
		for (u64 i = 0; i < rows; ++i)
			for (u64 j = 0; j < cols; ++j)
				plainMatrix(i, j) = i + j ;
		std::vector<si64Matrix> sharedMatrix;
		sharedMatrix.emplace_back(si64Matrix(rows, cols));
		sharedMatrix.emplace_back(si64Matrix(rows, cols));
		sharedMatrix.emplace_back(si64Matrix(rows, cols));
		// construct shares
		for (u64 i = 0; i < 3; i++)
		{
			if (i == mpc.partyIdx)
			{
				mpc.createShares(plainMatrix, sharedMatrix[i]);
			}
			else
			{
				mpc.createShares(sharedMatrix[i]);
			}
		}
		// Add
		//  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
		si64Matrix sum;
		mpc.MPC_Add(sharedMatrix, sum);
		mpc.reveal(sum, 0);

		return;
	}

	// Parent process as party 2.
	sleep(3);

	// integerOperations(2);
	// fixedPointOperations(2);
	MPCOperator mpc(2, "02", "12");
	mpc.setup("127.0.0.1", (u32)1414, (u32)1515);
	u64 rows = 2,
		cols = 2;
	// input data
	eMatrix<i64> plainMatrix(rows, cols);
	for (u64 i = 0; i < rows; ++i)
		for (u64 j = 0; j < cols; ++j)
			plainMatrix(i, j) = i + j + 1;

	std::vector<si64Matrix> sharedMatrix;
	sharedMatrix.emplace_back(si64Matrix(rows, cols));
	sharedMatrix.emplace_back(si64Matrix(rows, cols));
	sharedMatrix.emplace_back(si64Matrix(rows, cols));
	// construct shares
	for (u64 i = 0; i < 3; i++)
	{
		if (i == mpc.partyIdx)
		{
			mpc.createShares(plainMatrix, sharedMatrix[i]);
		}
		else
		{
			mpc.createShares(sharedMatrix[i]);
		}
	}
	// Add
	//  si64Matrix MPC_Add(std::vector<si64Matrix> sharedInt, si64Matrix &sum)
	si64Matrix sum;
	mpc.MPC_Add(sharedMatrix, sum);
	mpc.reveal(sum, 0);
	return;
}
