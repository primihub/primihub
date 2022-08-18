#include "util.h"

using namespace osuCrypto;
#include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/Timer.h"
#include <chrono>
#include <iomanip>

#define tryCount 4
#define trials (1 << 8)

void senderGetLatency(Channel& chl)
{
    u8 dummy[1];
    Timer timer;

    chl.resetStats();

    u64 s(0), r(0);


    s++;
    chl.asyncSend(dummy, 1);

    r++;
    chl.recv(dummy, 1);

    auto start = timer.setTimePoint("");
    s++;
    chl.asyncSend(dummy, 1);
    r++;
    chl.recv(dummy, 1);

    auto mid = timer.setTimePoint("");

    auto recvStart = mid;
    auto recvEnd = mid;

    auto rrt = mid - start;

    std::cout << "latency:   " << std::chrono::duration_cast<std::chrono::milliseconds>(rrt).count() << " ms" << std::endl;



    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////



    std::vector<u8> oneMbit((1 << 20) / 8);
    for (u64 i = 0; i < tryCount; ++i)
    {
        r++;
        chl.recv(dummy, 1);


        for (u64 j = 0; j < trials; ++j)
        {
            s+= oneMbit.size();
            chl.asyncSend(oneMbit.data(), oneMbit.size());
        }
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////



    for (u64 i = 0; i < tryCount; ++i)
    {
        auto recvStart = timer.setTimePoint("");
        s++;
        chl.asyncSend(dummy, 1);

        for (u64 j = 0; j < trials; ++j)
        {
            r+= oneMbit.size();
            chl.recv(oneMbit);
        }

        auto recvEnd = timer.setTimePoint("");

        // nanoseconds per GegaBit
        auto uspGb = std::chrono::duration_cast<std::chrono::nanoseconds>(recvEnd - recvStart - rrt / 2).count();

        // nanoseconds per second
        double usps = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count());

        // MegaBits per second
        auto Mbps = usps / uspGb *  trials;

        std::cout << "B -> A bandwidth: " << Mbps << " Mbps" << std::endl;
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////

    for (u64 i = 0; i < tryCount; ++i)
    {
        recvStart = timer.setTimePoint("");

        for (u64 j = 0; j < trials; ++j)
        {
            s+= oneMbit.size();
            chl.asyncSend(oneMbit.data(), oneMbit.size());
            r+= oneMbit.size();
            chl.recv(oneMbit);
        }

        recvEnd = timer.setTimePoint("");

        // nanoseconds per GegaBit
        auto uspGb = std::chrono::duration_cast<std::chrono::nanoseconds>(recvEnd - recvStart - rrt / 2).count();

        // nanoseconds per second
        double usps = 1.0 * std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count();

        // MegaBits per second
        auto Mbps = usps / uspGb *  trials;

        std::cout << "A <-> B bandwidth: " << Mbps << " Mbps" << std::endl;
    }
    std::cout << "r " << r << "  " << chl.getTotalDataRecv() << std::endl;
    std::cout << "s " << s << "  " << chl.getTotalDataSent() << std::endl;
}

void recverGetLatency(Channel& chl)
{
    u8 dummy[1];
    chl.resetStats();
    u64 s(0), r(0);

    r++;
    chl.recv(dummy, 1);
    Timer timer;
    auto start = timer.setTimePoint("");

    s++;
    chl.asyncSend(dummy, 1);

    r++;
    chl.recv(dummy, 1);

    auto mid = timer.setTimePoint("");

    s++;
    chl.asyncSend(dummy, 1);

    auto recvStart = mid;
    auto recvEnd = mid;

    auto rrt = mid - start;
    std::cout << "latency:   " << std::chrono::duration_cast<std::chrono::milliseconds>(rrt).count() << " ms" << std::endl;





    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////




    std::vector<u8> oneMbit((1 << 20) / 8);
    for (u64 i = 0; i < tryCount; ++i)
    {
        recvStart = timer.setTimePoint("");
        s++;
        chl.asyncSend(dummy, 1);

        for (u64 j = 0; j < trials; ++j)
        {
            r+= oneMbit.size();
            chl.recv(oneMbit);
        }

        recvEnd = timer.setTimePoint("");

        // nanoseconds per GegaBit
        auto uspGb = std::chrono::duration_cast<std::chrono::nanoseconds>(recvEnd - recvStart - rrt / 2).count();

        // nanoseconds per second
        double usps = 1.0 * std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count();

        // MegaBits per second
        auto Mbps = usps / uspGb *  trials;

        std::cout << "A -> B bandwidth: " << Mbps << " Mbps" << std::endl;
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////


    for (u64 i = 0; i < tryCount; ++i)
    {
        r++;
        chl.recv(dummy, 1);


        for (u64 j = 0; j < trials; ++j)
        {
            s+= oneMbit.size();
            chl.asyncSend(oneMbit.data(), oneMbit.size());
        }
    }


    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////


    for (u64 i = 0; i < tryCount; ++i)
    {
        recvStart = timer.setTimePoint("");

        for (u64 j = 0; j < trials; ++j)
        {
            s+= oneMbit.size();
            chl.asyncSend(oneMbit.data(), oneMbit.size());
            r+= oneMbit.size();
            chl.recv(oneMbit);
        }

        recvEnd = timer.setTimePoint("");

        // nanoseconds per GegaBit
        auto uspGb = std::chrono::duration_cast<std::chrono::nanoseconds>(recvEnd - recvStart - rrt / 2).count();

        // nanoseconds per second
        double usps = 1.0 * std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count();

        // MegaBits per second
        auto Mbps = usps / uspGb *  trials;

        std::cout << "A <-> B bandwidth: " << Mbps << " Mbps" << std::endl;
    }

    std::cout << "r " << r << "  " << chl.getTotalDataRecv() << std::endl;
    std::cout << "s " << s << "  " << chl.getTotalDataSent() << std::endl;

}

void printTimings(
    std::string tag,
    std::vector<osuCrypto::Channel> chls,
    long long offlineTime, long long onlineTime,
    LaunchParams & params,
    const osuCrypto::u64 &setSize,
    const osuCrypto::u64 &numThreads,
    double s,
    std::vector<osuCrypto::Channel>* chls2, 
    u64 n2)
{
    u64 dataSent = 0, dataRecv(0);
    for (u64 g = 0; g < chls.size(); ++g)
    {
        dataSent += chls[g].getTotalDataSent();
        dataRecv += chls[g].getTotalDataRecv();
        chls[g].resetStats();

        if (chls2)
        {
            dataSent += (*chls2)[g].getTotalDataSent();
            dataRecv += (*chls2)[g].getTotalDataRecv();
            (*chls2)[g].resetStats();
        }

    }

    // mico seconds
    double time = 1.0 * offlineTime + onlineTime;

    // milliseconds
    time /= 1000;
    auto Mbps = dataSent * 8 / time / (1 << 20);
    auto MbpsRecv = dataRecv * 8 / time / (1 << 20);

    if (params.mVerbose)
    {
        std::cout << std::setw(6) << tag << " n = " << setSize << "  threads = " << numThreads << "\n"
            << "      Total Time = " << time << " ms\n"
            << "         Total = " << offlineTime << " ms\n"
            << "          Online = " << onlineTime << " ms\n"
            << "      Total Comm = " << string_format("%5.2f", (dataRecv + dataSent) / std::pow(2.0, 20)) << " MB\n"
            //<< "      Total Comm = " << string_format("%4.2f", dataSent / std::pow(2.0, 20)) << ", " << string_format("%4.2f", dataRecv / std::pow(2.0, 20)) << " MB\n"
            << "       Bandwidth = " << string_format("%4.2f", Mbps) << ", " << string_format("%4.2f", MbpsRecv) << " Mbps\n" << std::endl;


        if (params.mVerbose > 1)
            std::cout << gTimer << std::endl;
    }
    else
    {
        std::cout << std::dec << std::setw(6) << tag
            << std::setw(8) << (std::to_string(setSize) + (n2 == u64(-1)? "" : "vs"+std::to_string(n2)))
            << std::setw(10) << (std::to_string(numThreads) + " " + std::to_string(s))
            << std::setw(14) << (offlineTime + onlineTime)
            << std::setw(14) << onlineTime
            << std::setw(18) << (string_format("%4.2f", (dataRecv + dataSent) / std::pow(2.0, 20)))
            //<< std::setw(18) << (string_format("%4.2f", dataSent / std::pow(2.0, 20)) + ", " + string_format("%4.2f", dataRecv / std::pow(2.0, 20)))
            << std::setw(18) << (string_format("%4.2f", Mbps) + ", " + string_format("%4.2f", MbpsRecv)) << std::endl;
    }
}


void printHeader()
{
    std::cout
        << "protocol     n      threads      total(ms)    online(ms)     comm (MB)        bandwidth (Mbps)\n"
        << "------------------------------------------------------------------------------------------------" << std::endl;
}