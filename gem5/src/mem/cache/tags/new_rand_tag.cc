/*
 * Copyright (c) 2012-2014 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Definitions of a conventional tag store.
 */

#include "mem/cache/tags/new_rand_tag.hh"

#include <string>

#include "base/intmath.hh"

NewRandTag::NewRandTag(const Params &p)
    :BaseTags(p), allocAssoc(p.assoc), blks(p.size / p.block_size),
     sequentialAccess(p.sequential_access),
     replacementPolicy(p.replacement_policy),
     nbits(p.nbits)
{
    //printf("GYGY debug NewRandTag::NewRandTag this: %p, &indexingPolicy: %p\n", this, &(this->indexingPolicy));
    indexMask = (1 << (floorLog2(p.size / p.block_size)+nbits))-1;
    std::cout<<"NewRandTag: indexMask is: "<<indexMask<<"\n";
    // There must be a indexing policy
    fatal_if(!p.indexing_policy, "An indexing policy is required");
    fatal_if(!(allocAssoc == p.size / p.block_size), "The associativity %u should "
        "be equal to the number of cache blocks %u", allocAssoc, p.size / p.block_size);

    // Check parameters
    if (blkSize < 4 || !isPowerOf2(blkSize)) {
        fatal("Block size must be at least 4 and a power of 2");
    }
}

CacheBlk*
NewRandTag::findBlock(Addr addr, bool is_secure) const
{
    panic("GYGY NewRandTag::findBlock should use domainID\n");
    /*
    // Extract block tag
    Addr tag = extractTag(addr);
    Addr index_bits = extractIndex(addr);

    // Find possible entries that may contain the given address
    const std::vector<ReplaceableEntry*> entries =
        indexingPolicy->getPossibleEntries(addr);

    // Search for block
    for (const auto& location : entries) {
        CacheBlk* blk = static_cast<CacheBlk*>(location);
        if (blk->matchTag(tag, is_secure) && blk->matchIndex(index_bits, is_secure)) {
            return blk;
        }
    }
    */

    // Did not find block
    return nullptr;
}

CacheBlk*
NewRandTag::findBlock(Addr addr, bool is_secure, uint64_t domainID) const
{
    // Extract block tag
    Addr tag = extractTag(addr);
    Addr index_bits = extractIndex(addr);

    // Find possible entries that may contain the given address
    const std::vector<ReplaceableEntry*> entries =
        indexingPolicy->getPossibleEntries(addr);

    // Search for block
    CacheBlk* hit_blk = nullptr;
    //int matchCnt = 0;
    for (const auto& location : entries) {
        CacheBlk* blk = static_cast<CacheBlk*>(location);
        if (blk->isValid() && blk->isSecure() == is_secure &&
            blk->getTag() == tag && blk->getIndexBits() == index_bits) {
            hit_blk = blk;
            //matchCnt++;
            return hit_blk;
        }
    }

    //assert(matchCnt == 0 || matchCnt == 1);

    return nullptr;
}

CacheBlk*
NewRandTag::findMappingHitBlock(PacketPtr pkt) const
{
    // Extract block tag
    Addr addr = pkt->getAddr();
    bool is_secure = pkt->isSecure();
    uint64_t domainID = pkt->getDomainID();
    Addr tag = extractTag(addr);
    Addr index_bits = extractIndex(addr);

    // Get possible entries
    const std::vector<ReplaceableEntry*> entries =
        indexingPolicy->getPossibleEntries(addr);

    // Search for block
    CacheBlk* hit_blk = nullptr;
    int matchCnt = 0;
    for (const auto& location : entries) {
        CacheBlk* blk_temp = static_cast<CacheBlk*>(location);
        if (blk_temp->matchIndex(index_bits, is_secure, domainID)) {
            hit_blk = blk_temp;
            matchCnt++;
        }
    }
    // to be compeleted
    if (matchCnt == 1) {
        assert(!(hit_blk->getTag() == tag));
    }
    else {
        assert(matchCnt == 0);
    }

    return hit_blk;
}

void
NewRandTag::tagsInit()
{
    // GYGY debug
    // printf("GYGY debug NewRandTag::tagsInit() this: %p, &indexingPolicy: %p\n", this, &(this->indexingPolicy));
    printf("NewRandTag::numBlocks: %u\n", numBlocks);
    // Initialize all blocks
    for (unsigned blk_index = 0; blk_index < numBlocks; blk_index++) {
        // Locate next cache block
        CacheBlk* blk = &blks[blk_index];

        // Link block to indexing policy
        indexingPolicy->setEntry(blk, blk_index);

        // Associate a data chunk to the block
        blk->data = &dataBlks[blkSize*blk_index];

        // Associate a replacement data entry to the block
        blk->replacementData = replacementPolicy->instantiateEntry();
    }
}

void
NewRandTag::invalidate(CacheBlk *blk)
{
    BaseTags::invalidate(blk);

    // Decrease the number of tags in use
    stats.tagsInUse--;

    // Invalidate replacement data
    replacementPolicy->invalidate(blk->replacementData);
}

void
NewRandTag::moveBlock(CacheBlk *src_blk, CacheBlk *dest_blk)
{
    BaseTags::moveBlock(src_blk, dest_blk);

    // Since the blocks were using different replacement data pointers,
    // we must touch the replacement data of the new entry, and invalidate
    // the one that is being moved.
    replacementPolicy->invalidate(src_blk->replacementData);
    replacementPolicy->reset(dest_blk->replacementData);
}
