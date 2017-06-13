#include <iostream>
#include "cryptoTools/Network/Channel.h"
#include "cryptoTools/Network/Endpoint.h"

//using namespace std;
#include "UnitTests.h" 
#include "cryptoTools/Common/Defines.h"
using namespace osuCrypto;

#include "bloomFilterMain.h"
#include "dcwMain.h"
#include "dktMain.h"
#include "OtBinMain.h"
#include "DrrnPSIMain.h"
#include "util.h"
#include "signalHandle.h"

#include "cryptoTools/Common/MatrixView.h"
#include "libOTe/TwoChooseOne/KosOtExtReceiver.h"
#include "libOTe/TwoChooseOne/KosOtExtSender.h"
#include <numeric>
#include "cryptoTools/Common/Log.h"
#include "libPSI/PIR/BgiPirClient.h"
#include "libPSI/PIR/BgiPirServer.h"

#include "cuckoo/cuckooTests.h"
#include "cryptoTools/Common/CLP.h"


std::vector<std::string>
unitTestTags{ "u", "unitTest" },
#ifdef ENABLE_DCW
DcwTags{ "dcw" },
DcwrTags{ "dcwr" },
#endif
rr16Tags{ "rr16" },
rr17aTags{ "rr17a" },
rr17aSMTags{ "rr17a-sm" },
rr17bTags{ "rr17b" },
rr17bSMTags{ "rr17b-sm" },
kkrtTag{ "kkrt" },
drrnTag{ "drrn" },
dktTags{ "dkt" },
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
binScalerTag{ "s", "binScaler" };

bool firstRun(true);

std::function<void(LaunchParams&)> NoOp;

void run(
    std::vector<std::string> tag,
    CLP& cmd,
    std::function<void(LaunchParams&)> recvProtol,
    std::function<void(LaunchParams&)> sendProtol)
{

    LaunchParams params;

    params.mNumThreads = cmd.getMany<u64>(numThreads);
    params.mVerbose = cmd.get<u64>(verboseTags);
    params.mTrials = cmd.get<u64>(trialsTags);
    params.mHostName = cmd.get<std::string>(hostNameTag);
    params.mBitSize = cmd.get<u64>(bitSizeTag);
    params.mBinScaler = cmd.getMany<double>(binScalerTag);


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

    if (cmd.isSet(tag))
    {
        IOService ios(0);


        if (cmd.hasValue(roleTag))
        {
            params.mIdx = cmd.get<u32>(roleTag);
            auto mode = params.mIdx ? EpMode::Server : EpMode::Client;
            Endpoint ep(ios, "localhost", 1213, mode, "none");
            params.mChls.resize(*std::max_element(params.mNumThreads.begin(), params.mNumThreads.end()));

            for (u64 i = 0; i < params.mChls.size(); ++i)
                params.mChls[i] = ep.addChannel("chl" + std::to_string(i), "chl" + std::to_string(i));

            if (cmd.get<bool>(roleTag))
            {
                if (firstRun) printHeader();

                recvProtol(params);
            }
            else
            {
                sendProtol(params);
            }

            for (u64 i = 0; i < params.mChls.size(); ++i)
                params.mChls[i].close();

            ep.stop();
        }
        else
        {
            auto params2 = params;
            if (firstRun) printHeader();

            auto thrd = std::thread([&]()
            {
                Endpoint ep(ios, "localhost", 1213, EpMode::Client, "none");
                params2.mChls.resize(*std::max_element(params2.mNumThreads.begin(), params2.mNumThreads.end()));

                for (u64 i = 0; i < params2.mChls.size(); ++i)
                    params2.mChls[i] = ep.addChannel("chl" + std::to_string(i), "chl" + std::to_string(i));

                recvProtol(params2);


                for (u64 i = 0; i < params.mChls.size(); ++i)
                    params2.mChls[i].close();
                ep.stop();

            });

            Endpoint ep(ios, "localhost", 1213, EpMode::Server, "none");
            params.mChls.resize(*std::max_element(params.mNumThreads.begin(), params.mNumThreads.end()));

            for (u64 i = 0; i < params.mChls.size(); ++i)
                params.mChls[i] = ep.addChannel("chl" + std::to_string(i), "chl" + std::to_string(i));

            sendProtol(params);


            for (u64 i = 0; i < params.mChls.size(); ++i)
                params.mChls[i].close();

            thrd.join();

            ep.stop();
        }

        firstRun = false;
        ios.stop();
    }
}

void pingTest(CLP& cmd)
{

    IOService ios(0);

    if (cmd.hasValue(roleTag))
    {
        if (cmd.get<bool>(roleTag))
        {
            Endpoint sendEP(ios, cmd.get<std::string>(hostNameTag), EpMode::Server, "pringTest");
            auto chl = sendEP.addChannel("test");
            senderGetLatency(chl);
            chl.close();
            sendEP.stop();
        }
        else
        {
            Endpoint recvEP(ios, cmd.get<std::string>(hostNameTag), EpMode::Client, "pringTest");
            auto chl = recvEP.addChannel("test");
            recverGetLatency(chl);
            chl.close();
            recvEP.stop();
        }
    }
    else
    {
        auto thrd = std::thread([&]()
        {
            Endpoint sendEP(ios, cmd.get<std::string>(hostNameTag), EpMode::Server, "pringTest");
            auto chl = sendEP.addChannel("test");
            senderGetLatency(chl);
            chl.close();
            sendEP.stop();
        });

        Endpoint recvEP(ios, cmd.get<std::string>(hostNameTag), EpMode::Client, "pringTest");
        auto chl = recvEP.addChannel("test");
        recverGetLatency(chl);
        chl.close();
        recvEP.stop();
        thrd.join();
    }

    ios.stop();
}

int main(int argc, char** argv)
{

    backtraceHook();


    CLP cmd;
    cmd.parse(argc, argv);

    //cmd.setDefault(rr17Tags, "");

    cmd.setDefault(numThreads, "1");
    cmd.setDefault(numItems, std::to_string(1 << 8));
    //cmd.setDefault(verboseTags, "0");
    cmd.setDefault(trialsTags, "1");
    cmd.setDefault(bitSizeTag, "-1");
    cmd.setDefault(binScalerTag, "1");
    cmd.setDefault(hostNameTag, "127.0.0.1:1212");

    cmd.setDefault(verboseTags, std::to_string(1 & (u8)cmd.isSet(verboseTags)));

    if (cmd.isSet(unitTestTags))
        run_all();

    if (cmd.isSet(pingTag))
        pingTest(cmd);


    //if ((cmd.isSet(roleTag) == false || cmd.hasValue(roleTag) && cmd.get<int>(roleTag)) &&
    //    (cmd.isSet(DcwTags) || cmd.isSet(DcwrTags) || cmd.isSet(rr16Tags) || cmd.isSet(rr17Tags) || cmd.isSet(dktTags)))
    //    printHeader();

#ifdef ENABLE_DCW
    run(DcwRecv, DcwSend, DcwTags, cmd);
    run(DcwRRecv, DcwRSend, DcwrTags, cmd);
#endif
    run(rr16Tags, cmd, bfRecv, bfSend);
    run(rr17aTags, cmd, rr17aRecv, rr17aSend);
    run(rr17aSMTags, cmd, rr17aRecv_StandardModel, rr17aSend_StandardModel);
    run(rr17bTags, cmd, rr17bRecv, rr17bSend);
    run(rr17bSMTags, cmd, rr17bRecv_StandardModel, rr17bSend_StandardModel);
    run(dktTags, cmd, DktRecv, DktSend);
    run(kkrtTag, cmd, kkrtRecv, kkrtSend);
    //run(drrnTag, cmd, Drrn17Recv, Drrn17Send, Drrn17Send);


    if ((cmd.isSet(unitTestTags) == false &&
#ifdef ENABLE_DCW
        cmd.isSet(DcwTags) == false &&
        cmd.isSet(DcwrTags) == false &&
#endif
        cmd.isSet(rr16Tags) == false &&
        cmd.isSet(rr17aTags) == false &&
        cmd.isSet(rr17aSMTags) == false &&
        cmd.isSet(rr17bTags) == false &&
        cmd.isSet(rr17bSMTags) == false &&
        cmd.isSet(kkrtTag) == false &&
        cmd.isSet(drrnTag) == false &&
        cmd.isSet(dktTags) == false &&
        cmd.isSet(pingTag) == false) ||
        cmd.isSet(helpTags))
    {
        std::cout
            << "#######################################################\n"
            << "#                      - libPSI -                     #\n"
            << "#               A library for performing              #\n"
            << "#               private set intersection              #\n"
            << "#                      Peter Rindal                   #\n"
            << "#######################################################\n" << std::endl;

        std::cout << "Protocols:\n"

#ifdef ENABLE_DCW
            << "   -" << DcwTags[0] << "  : DCW13  - Garbled Bloom Filter (semi-honest*)\n"
            << "   -" << DcwrTags[0] << " : PSZ14  - Random Garbled Bloom Filter (semi-honest*)\n"
#endif
            << "   -" << rr16Tags[0] << " : RR16   - Random Garbled Bloom Filter (malicious secure)\n"
            << "   -" << rr17aTags[0] << " : RR17   - Hash to bins & compare style (malicious secure)\n"
            << "   -" << rr17aSMTags[0] << ": RR17sm - Hash to bins & compare style (standard model malicious secure)\n"
            << "   -" << rr17bTags[0] << ": RR17b  - Hash to bins & commit compare style (malicious secure)\n"
            << "   -" << dktTags[0] << "  : DKT12  - Public key style (malicious secure)\n"
            << "   -" << kkrtTag[0] << "  : KKRT16  - Hash to Bin & compare style (semi-honest secure)\n"
            << "   -" << drrnTag[0] << "  : DRRN17  - Two server PIR style (semi-honest secure)\n" << std::endl;

        std::cout << "Parameters:\n"
            << "   -" << roleTag[0]
            << ": Two terminal mode. Value should be in { 0, 1 } where 0 means PSI sender and network server.\n"

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

            << "   -" << pingTag[0]
            << ": Perform a ping and bandwidth test (Default = " << cmd.isSet(pingTag) << ")\n"

            << "   -" << bitSizeTag[0]
            << ":  Bit size for protocols that depend on it.\n"

            << "   -" << binScalerTag[0]
            << ":  Have the Hash to bin type protocols use n / " << binScalerTag[0] << " number of bins (Default = 1)\n" << std::endl;


        std::cout << "Unit Tests:\n"
            << "   -" << unitTestTags[0] << ": Run all unit tests\n" << std::endl;

    }
    return 0;
}