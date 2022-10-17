#include <iostream>
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/Endpoint.h"
#include "cryptoTools/Network/IOService.h"

//using namespace std;
#include "cryptoTools/Common/Defines.h"
#include "libOTe/Tools/DefaultCurve.h"
//#include "cryptoTools/Common/Version.h"

//#if !defined(CRYPTO_TOOLS_VERSION_MAJOR) || CRYPTO_TOOLS_VERSION_MAJOR != 1 || CRYPTO_TOOLS_VERSION_MAJOR != 1
//#error "Wrong crypto tools version."
//#endif

using namespace osuCrypto;

#include <fstream>
#include <numeric>
#include <chrono>
#include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/BitVector.h"

#include "util.h"
#include "OtBinMain.h"

#include "cryptoTools/Common/CLP.h"

std::vector<std::string>
kkrtTag{ "kkrt" },
mkkrtTag{ "mkkrt" },
cm20Tag{ "cm20" },
helpTags{ "h", "help" },
numThreads{ "t", "threads" },
numItems{ "n","numItems" },
powNumItems{ "nn","powNumItems" },
verboseTags{ "v", "verbose" },
trialsTags{ "trials" },
roleTag{ "r", "role" },
hostNameTag{ "ip" },
pingTag{ "ping" },
bitSizeTag{ "b","bitSize" },
binScalerTag{ "s", "binScaler" },
statSecParamTag{ "ssp" },
numHashTag{ "nh" },
bigBlockTag{ "bigBlock" };

bool firstRun(true);

std::function<void(LaunchParams&)> NoOp;

void benchmark(
	std::vector<std::string> tag,
	CLP& cmd,
	std::function<void(LaunchParams&)> recvProtol,
	std::function<void(LaunchParams&)> sendProtol)
{
	if (cmd.isSet(tag))
	{
		LaunchParams params;

		params.mIP = cmd.get<std::string>(hostNameTag);
        params.mNumThreads = cmd.getMany<u64>(numThreads);
        params.mVerbose = cmd.get<u64>(verboseTags);
        params.mTrials = cmd.get<u64>(trialsTags);
        params.mHostName = cmd.get<std::string>(hostNameTag);
        params.mBitSize = cmd.get<u64>(bitSizeTag);
        params.mBinScaler = cmd.getMany<double>(binScalerTag);
        params.mStatSecParam = cmd.get<u64>(statSecParamTag);
        params.mCmd = &cmd;


		if (cmd.isSet(powNumItems))
		{
			params.mNumItems = cmd.getMany<u64>(powNumItems);
			std::transform(
				params.mNumItems.begin(),
				params.mNumItems.end(),
				params.mNumItems.begin(),
				[](u64 v) { return 1 << v; });
		}
		else
		{
			params.mNumItems = cmd.getMany<u64>(numItems);
		}

		IOService ios(0);

		auto go = [&](LaunchParams& params)
		{
			auto mode = params.mIdx ? EpMode::Server : EpMode::Client;
			Endpoint ep(ios, params.mIP, mode);
			params.mChls.resize(*std::max_element(params.mNumThreads.begin(), params.mNumThreads.end()));
			params.mChls2.resize(*std::max_element(params.mNumThreads.begin(), params.mNumThreads.end()));

			for (u64 i = 0; i < params.mChls.size(); ++i) {
				params.mChls[i] = ep.addChannel();
				params.mChls2[i] = ep.addChannel();
			}

            if (params.mIdx == 0)
            {
                if (firstRun) printHeader();
                firstRun = false;

				recvProtol(params);
			}
			else
			{
				sendProtol(params);
			}

			for (u64 i = 0; i < params.mChls.size(); ++i) {
				params.mChls[i].close();
				params.mChls2[i].close();
			}


			params.mChls.clear();
			params.mChls2.clear();
			ep.stop();
		};

        if (cmd.hasValue(roleTag))
        {
            params.mIdx = cmd.get<u32>(roleTag);
            go(params);
        }
        else
        {
			using namespace DefaultCurve;
			Curve curve;
            auto thrd = std::thread([&]()
            {
                auto params2 = params;
                params2.mIdx = 1;
                go(params2);
            });
            params.mIdx = 0;
            go(params);
            thrd.join();
        }

        ios.stop();
    }
}

int main(int argc, char** argv)
{
    CLP cmd;
    cmd.parse(argc, argv);

	// default parameters for various things
	cmd.setDefault(numThreads, "1");
	cmd.setDefault(numItems, std::to_string(1 << 8));
	cmd.setDefault(trialsTags, "1");
	cmd.setDefault(bitSizeTag, "-1");
	cmd.setDefault(binScalerTag, "1");
	cmd.setDefault(hostNameTag, "127.0.0.1:1212");
	cmd.setDefault(numHashTag, "3");
	cmd.setDefault(bigBlockTag, "16");
    cmd.setDefault(statSecParamTag, 40);
    cmd.setDefault("eps", "0.1");
	cmd.setDefault(verboseTags, std::to_string(1 & (u8)cmd.isSet(verboseTags)));

	benchmark(kkrtTag, cmd, kkrtRecv, kkrtSend);
	benchmark(cm20Tag, cmd, cm20Recv, cm20Send);
	benchmark(mkkrtTag, cmd, mkkrtRecv, mkkrtSend);

	if (cmd.isSet(kkrtTag) == false &&
		cmd.isSet(cm20Tag) == false &&
		cmd.isSet(mkkrtTag) == false &&
		cmd.isSet(helpTags))
	{
		std::cout << Color::Red 
			<< "#######################################################\n"
			<< "#                      - libPSI -                     #\n"
			<< "#               A library for performing              #\n"
			<< "#               private set intersection              #\n"
			<< "#                      Peter Rindal                   #\n"
			<< "#######################################################\n" << std::endl;

		std::cout << Color::Green << "Protocols:\n" << Color::Default
			<< "   -" << kkrtTag[0] << "    : KKRT16  - Hash to Bin & compare style (semi-honest secure, fastest)\n"
			<< "   -" << mkkrtTag[0] << "   : KKRT16  - Hash to Bin & compare style (semi-honest secure, fastest, multithread)\n"
			<< "   -" << cm20Tag[0] << "    : CM16  - Multi-Point OPRF & compare style (semi-honest secure, fastest)\n"
			<< std::endl;

		std::cout << Color::Green << "Benchmark Parameters:\n" << Color::Default
			<< "   -" << roleTag[0]
			<< ": Two terminal mode. Value should be in { 0, 1 } where 0 means PSI sender and network server.\n"

			<< "   -ip: IP address and port of the server = PSI receiver. (Default = localhost:1212)\n"

			<< "   -" << numItems[0]
			<< ": Number of items each party has, white space delimited. (Default = " << cmd.get<std::string>(numItems) << ")\n"

            << "   -" << powNumItems[0]
            << ": 2^n number of items each party has, white space delimited.\n"

			<< "   -" << numThreads[0]
			<< ": Number of theads each party has, white space delimited. (Default = " << cmd.get<std::string>(numThreads) << ")\n"

			<< "   -" << trialsTags[0]
			<< ": Number of trials performed. (Default = " << cmd.get<std::string>(trialsTags) << ")\n"

			<< "   -" << verboseTags[0]
			<< ": print extra information. (Default = " << cmd.get<std::string>(verboseTags) << ")\n"

			<< "   -" << hostNameTag[0]
			<< ": The server's address (Default = " << cmd.get<std::string>(hostNameTag) << ")\n"

			<< "   -" << bitSizeTag[0]
			<< ":  Bit size for protocols that depend on it.\n"

            << "   -" << binScalerTag[0]
            << ":  Have the Hash to bin type protocols use n / " << binScalerTag[0] << " number of bins (Default = 1)\n"

            << "   -bigBlock"
            << ":  The number of entries from the PSI-PSI server database each PIR contributes to the PSI. Larger=smaller PIR queries but larger PSI (Default = 16)\n"

            << std::endl;
	}
	return 0;
}