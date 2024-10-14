/*
 * Copyright (c) 2014-2015 ARM Limited
 * All rights reserved
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

#ifndef __MEM_CACHE_PREFETCH_SHB_HH__
#define __MEM_CACHE_PREFETCH_SHB_HH__

#include <cstdint>
#include <list>
#include <utility>

#include "base/statistics.hh"
#include "base/types.hh"
#include "base/random.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/base.hh"
#include "mem/packet.hh"
#include "mem/request.hh"

struct SHBPrefetcherParams;
//struct BaseCacheParams;

namespace Prefetcher {

class SHB : public Base
{
  protected:
    struct shbReqInfo {
        Addr addr;
        bool secure;
        uint64_t domainID;
        RequestorID requestorID;

        shbReqInfo(Addr iaddr, bool isecure, uint64_t idomainID,
            RequestorID irequestorID):
            addr(iaddr), secure(isecure), domainID(idomainID),
            requestorID(irequestorID)
            {}
    };
    std::vector<shbReqInfo> shb;

    // PARAMETERS

    /** Cycles after generation when a prefetch can first be issued */
    const Cycles latency;

    /** Snoop the cache before generating prefetch (cheating basically) */
    const bool cacheSnoop;

    // SHB: Maximum number of SHB entries
    const unsigned shbSize;

    // SHB: Issue rate Mode
    // 1: constant rate
    // 2: random interval
    const unsigned shbIssue;

    // SHB: Number of cycles between sending SHBfetch
    const Cycles issueInterval;

    // SHB: Maximum number of cycles between sending SHBfetch
    // when the random interval mode is enabled
    const Cycles maxIssueInterval;

    // SHB: Number of blocks in the window
    const unsigned blkNum;

    // SHB: Policy for selecting SHB entry
    // 1: random selection
    unsigned shbSelect;

    // SHB: Ready time to issue
    Tick readyTime;

    struct SHBStats : public Stats::Group
    {
        SHBStats(SHB *parent);

        SHB *shb_obj;

        void regStats() override;

        // STATS
        //Stats::Scalar pfIdentified;
        Stats::Vector offsetCount;
    } statsSHB;
  public:

    SHB(const SHBPrefetcherParams &p);
    virtual ~SHB();

    void notify(const PacketPtr &pkt, const PrefetchInfo &pfi) override
    {
        panic("SHB prefetch does not support notify!\n");
    }

    void notifySHB(const PacketPtr &pkt) override;

    PacketPtr getPacket() override;

    Tick nextPrefetchReadyTime() override
    {
        if (shbIssue == 1) {
            Tick issue_tick = issueInterval * clockPeriod();
            return curTick() - (curTick() % issue_tick) + issue_tick;
        }
        else {
            assert(shbIssue == 2);
            unsigned add_time = 0;
            Tick prev_time = readyTime;
            while (readyTime <= curTick()) {
                unsigned randCycle = random_mt.random<unsigned>
                    (issueInterval, maxIssueInterval);
                readyTime += randCycle * clockPeriod();
                add_time += randCycle;
            }
            DPRINTF(HWPrefetch, "SHBrandIssue addDelay %u, "
                "prev: %llu, new: %llu\n",
                add_time, prev_time, readyTime);
            return readyTime;
        }
    }

  private:

    RequestPtr createPrefetchRequest(Addr addr, PrefetchInfo const &pfi,
                                        PacketPtr pkt);
};

} // namespace Prefetcher

#endif //__MEM_CACHE_PREFETCH_SHB_HH__

