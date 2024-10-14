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

#include "mem/cache/prefetch/shb.hh"

#include <cassert>

#include "arch/generic/tlb.hh"
#include "base/logging.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/base.hh"
#include "mem/request.hh"
#include "params/SHBPrefetcher.hh"

namespace Prefetcher {

SHB::SHB(const SHBPrefetcherParams &p)
    : Base(p),
      latency(p.latency),
      cacheSnoop(p.cache_snoop),
      shbSize(p.shb_size),
      shbIssue(p.shb_issue),
      issueInterval(p.issue_interval),
      maxIssueInterval((shbIssue == 1)?p.issue_interval:p.max_issue_interval),
      blkNum(p.blk_num),
      shbSelect(p.shb_select),
      readyTime(0),
      statsSHB(this)
{
    printf("Creating SHB prefetcher shbSize: %u, windowSize: %u, clockPeriod:%lu,\n"
        "IssueMode: %s, issueInterval: %lu, maxInterval: %lu, shbSelect: %s\n",
        shbSize, blkNum*blkSize, clockPeriod(),
        (shbIssue == 1)? "Constant": "Random",
        uint64_t(issueInterval), uint64_t(maxIssueInterval),
        (shbSelect == 1)? "Random": "Error");
    panic_if(shbIssue == 2 && uint64_t(issueInterval) >= uint64_t(maxIssueInterval),
        "In Random Issue Mode, the maximum issue interval should be larger than "
        "the base issue interval!\n");
    printf("SHB init size: %lu\n", shb.size());
}

SHB::~SHB()
{
}

PacketPtr
SHB::getPacket() {
    if (shb.size() == 0) {
        DPRINTF(HWPrefetch, "EmptySHB\n");
        return nullptr;
    }

    assert(shb.size() <= shbSize);
    unsigned idx;
    if (shbSelect == 1)
        idx = random_mt.random<unsigned>(0, shb.size()-1);
    else
        panic("Undefined SHB selection policy!\n");

    Addr pkt_addr = shb[idx].addr;
    // Round the lower bound to a multiple of window size
    pkt_addr -= pkt_addr%(blkNum * blkSize);
    unsigned blk_offset = random_mt.random<unsigned>(0, blkNum-1);
    pkt_addr += blk_offset * blkSize;

    /* Create a prefetch memory request */
    RequestPtr req = std::make_shared<Request>(pkt_addr, blkSize,
                                                0, shb[idx].requestorID);

    if (shb[idx].secure) {
        req->setFlags(Request::SECURE);
    }
    req->setDomainID(shb[idx].domainID);
    req->taskId(ContextSwitchTaskId::Prefetcher);
    req->clearFlags(Request::DEMAND_FETCH);
    PacketPtr pkt = new Packet(req, MemCmd::HardPFReq);
    pkt->allocate();

    prefetchStats.pfIssued++;
    statsSHB.offsetCount[blk_offset]++;
    DPRINTF(HWPrefetch, "SHBpkt size:%llu, idx:%llu, origAddr:%#x, SHBoffset:%#x, "
        "SHBAddr:%#x\n", shb.size(), idx, shb[idx].addr, blk_offset * blkSize,
        pkt->getAddr());

    return pkt;
}

void
SHB::notifySHB(const PacketPtr &pkt)
{
    Addr align_addr = pkt->getAddr() - (pkt->getAddr() % blkSize);
    DPRINTF(HWPrefetch, "SHBinsert size:%llu, addr:%#x, Domain:%#x\n",
        shb.size(), align_addr, pkt->getDomainID());
    assert(shb.size() <= shbSize);
    if (shb.size() == shbSize)
        shb.pop_back();
    shb.emplace(shb.begin(), align_addr, pkt->isSecure(),
        pkt->getDomainID(), pkt->requestorId());
}

SHB::SHBStats::SHBStats(SHB *parent)
    : shb_obj(parent),
    Stats::Group(parent),
    // ADD_STAT(pfIdentified, UNIT_COUNT,
    //         "number of prefetch candidates identified"),
    ADD_STAT(offsetCount, UNIT_COUNT,
        "number of different random offsets generated")
{
}

void
SHB::SHBStats::regStats()
{
    using namespace Stats;
    Stats::Group::regStats();
    offsetCount.init(shb_obj->blkNum);
}

/*
void
Queued::insert(const PacketPtr &pkt, PrefetchInfo &new_pfi,
                         int32_t priority)
{
     * Physical address computation
     * if the prefetch is within the same page
     *   using VA: add the computed stride to the original PA
     *   using PA: no actions needed
     * if we are page crossing
     *   using VA: Create a translaion request and enqueue the corresponding
     *       deferred packet to the queue of pending translations
     *   using PA: use the provided VA to obtain the target VA, then attempt to
     *     translate the resulting address

    Addr orig_addr = useVirtualAddresses ?
        pkt->req->getVaddr() : pkt->req->getPaddr();
    bool positive_stride = new_pfi.getAddr() >= orig_addr;
    Addr stride = positive_stride ?
        (new_pfi.getAddr() - orig_addr) : (orig_addr - new_pfi.getAddr());

    Addr target_paddr;
    bool has_target_pa = false;
    RequestPtr translation_req = nullptr;
    if (samePage(orig_addr, new_pfi.getAddr())) {
        if (useVirtualAddresses) {
            // if we trained with virtual addresses,
            // compute the target PA using the original PA and adding the
            // prefetch stride (difference between target VA and original VA)
            target_paddr = positive_stride ? (pkt->req->getPaddr() + stride) :
                (pkt->req->getPaddr() - stride);
        } else {
            target_paddr = new_pfi.getAddr();
        }
        has_target_pa = true;
    } else {
        // Page crossing reference

        // ContextID is needed for translation
        if (!pkt->req->hasContextId()) {
            return;
        }
        if (useVirtualAddresses) {
            has_target_pa = false;
            translation_req = createPrefetchRequest(new_pfi.getAddr(), new_pfi,
                                                    pkt);
        } else if (pkt->req->hasVaddr()) {
            has_target_pa = false;
            // Compute the target VA using req->getVaddr + stride
            Addr target_vaddr = positive_stride ?
                (pkt->req->getVaddr() + stride) :
                (pkt->req->getVaddr() - stride);
            translation_req = createPrefetchRequest(target_vaddr, new_pfi,
                                                    pkt);
        } else {
            // Using PA for training but the request does not have a VA,
            // unable to process this page crossing prefetch.
            return;
        }
    }
    if (has_target_pa && cacheSnoop &&
            (inCache(target_paddr, new_pfi.isSecure()) ||
            inMissQueue(target_paddr, new_pfi.isSecure()))) {
        statsQueued.pfInCache++;
        DPRINTF(HWPrefetch, "Dropping redundant in "
                "cache/MSHR prefetch addr:%#x\n", target_paddr);
        return;
    }

    // Create the packet and find the spot to insert it
    DeferredPacket dpp(this, new_pfi, 0, priority);
    if (has_target_pa) {
        Tick pf_time = curTick() + clockPeriod() * latency;
        dpp.createPkt(target_paddr, blkSize, requestorId, tagPrefetch,
                      pf_time);
        DPRINTF(HWPrefetch, "Prefetch queued. "
                "addr:%#x priority: %3d tick:%lld.\n",
                new_pfi.getAddr(), priority, pf_time);
        addToQueue(pfq, dpp);
    } else {
        // Add the translation request and try to resolve it later
        dpp.setTranslationRequest(translation_req);
        dpp.tc = cache->system->threads[translation_req->contextId()];
        DPRINTF(HWPrefetch, "Prefetch queued with no translation. "
                "addr:%#x priority: %3d\n", new_pfi.getAddr(), priority);
        addToQueue(pfqMissingTranslation, dpp);
    }
}
*/


} // namespace Prefetcher
