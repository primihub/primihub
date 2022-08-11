
#include "src/primihub/operator/aby3_operator.h"

namespace primihub
{
	int MPCOperator::setup(std::string ip, u32 next_port, u32 prev_port)
	{
		/*
		  功能：初始化成员变量
		  参数1：IP
		  参数2：next端口
		  参数3：prev端口
		  返回值：0//失败
				 1//成功
		  */
		CommPkg comm = CommPkg();
		Session ep_next_;
		Session ep_prev_;

		switch (partyIdx)
		{
		case 0:
			ep_next_.start(ios, ip, next_port, SessionMode::Server, next_name);
			ep_prev_.start(ios, ip, prev_port, SessionMode::Server, prev_name);
			break;
		case 1:
			ep_next_.start(ios, ip, next_port, SessionMode::Server, next_name);
			ep_prev_.start(ios, ip, prev_port, SessionMode::Client, prev_name);
			break;
		default:
			ep_next_.start(ios, ip, next_port, SessionMode::Client, next_name);
			ep_prev_.start(ios, ip, prev_port, SessionMode::Client, prev_name);
			break;
		}
		comm.setNext(ep_next_.addChannel());
		comm.setPrev(ep_prev_.addChannel());
		comm.mNext().waitForConnection();
		comm.mPrev().waitForConnection();
		comm.mNext().send(partyIdx);
		comm.mPrev().send(partyIdx);

		u64 prev_party = 0;
		u64 next_party = 0;
		comm.mNext().recv(next_party);
		comm.mPrev().recv(prev_party);
		if (next_party != (partyIdx + 1) % 3)
		{
			LOG(ERROR) << "Party " << partyIdx << ", expect next party id "
					   << (partyIdx + 1) % 3 << ", but give " << next_party << ".";
			return -3;
		}

		if (prev_party != (partyIdx + 2) % 3)
		{
			LOG(ERROR) << "Party " << partyIdx << ", expect prev party id "
					   << (partyIdx + 2) % 3 << ", but give " << prev_party << ".";
			return -3;
		}

		// Establishes some shared randomness needed for the later protocols
		enc.init(partyIdx, comm, sysRandomSeed());

		// Establishes some shared randomness needed for the later protocols
		eval.init(partyIdx, comm, sysRandomSeed());

		// Copies the Channels and will use them for later protcols.
		auto commPtr = std::make_shared<CommPkg>(comm.mPrev(), comm.mNext());
		runtime.init(partyIdx, commPtr);
		return 1;
	}
} // namespace primihub
