#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Common/CLP.h"
using namespace osuCrypto;

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}



struct LaunchParams
{
    LaunchParams()
        :mVerbose(0),
        mTrials(1),
        mStatSecParam(40)
    {
    }

    std::vector<Channel> getChannels(u64 n) {
        return  std::vector<Channel>( mChls.begin(), mChls.begin() + n);
    }

    std::vector<Channel> getChannels2(u64 n) {
        return  std::vector<Channel>(mChls2.begin(), mChls2.begin() + n);
    }

    std::string mHostName;
    std::vector<Channel> mChls, mChls2;
    std::vector<u64> mNumItems, mNumItems2;
    std::vector<u64> mNumThreads;
    std::vector<double> mBinScaler;

    u64 mBitSize;
    u64 mVerbose;
    u64 mTrials;
    u64 mStatSecParam;
    u64 mIdx;
	u64 mNumHash;

    std::string mIP;
	CLP* mCmd;
};


#include "cryptoTools/Network/Channel.h"
void senderGetLatency(osuCrypto::Channel& chl);

void recverGetLatency(osuCrypto::Channel& chl);




void printTimings(
    std::string tag,
    std::vector<osuCrypto::Channel> chls,
    long long offlineTime, long long onlineTime,
    LaunchParams & params,
    const osuCrypto::u64 &setSize,
    const osuCrypto::u64 &numThreads,
    double s = 1,
    std::vector<osuCrypto::Channel>* chls2 = nullptr,
    u64 n2  = -1);

void printHeader();
