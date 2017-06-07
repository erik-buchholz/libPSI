#pragma once
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/ArrayView.h>
#include <cryptoTools/Common/BitVector.h>

namespace osuCrypto
{

class BgiPirServer
{
public:
    u64 mKDepth, mGroupBlkSize;

    void init(u64 depth, u64 groupByteSize);

    void serve(Channel chan, span<block> data);

    static u8 evalOne(u64 idx, span<block> k, span<block> g, block* = nullptr, block* = nullptr, u8* tt = nullptr);
    static block traversePath(u64 depth, u64 idx, span<block> k);
    static block traverseOne(const block &s, const block&k, const osuCrypto::u8 &keep, bool print = false);
    static block fullDomain(span<block> data, span<block> k, span<block> g);
    //static BitVector BgiPirServer_bv;


};

}