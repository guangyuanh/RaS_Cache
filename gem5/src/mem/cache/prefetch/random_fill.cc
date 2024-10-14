#include "mem/cache/prefetch/random_fill.hh"

#include <cassert>

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/random.hh"
#include "base/trace.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"

namespace Prefetcher {

RandomFill::RandomFill(const RandomFillPrefetcherParams &p)
  : Queued(p),
  	lowRange(p.low_range),
    highRange(p.high_range),
    degree(p.degree)
{
	printf("Creating RandomFill prefetcher lowRange: %i, "
		"highRange: %i, degree: %i\n", lowRange, highRange, degree);
	panic_if(lowRange < -32 || lowRange > 32,
		"To prevent cross-page prefetching, "
		"abs of lowRange cannot exceed 32\n");
	panic_if(highRange < -32 || highRange > 32,
		"To prevent cross-page prefetching, "
		"abs of lowRange cannot exceed 32\n");
}

void
RandomFill::insert(const PacketPtr &pkt, PrefetchInfo &new_pfi,
                         int32_t priority)
{
    if (queueFilter) {
        if (alreadyInQueue(pfq, new_pfi, priority)) {
            return;
        }
        if (alreadyInQueue(pfqMissingTranslation, new_pfi, priority)) {
            return;
        }
    }

    /*
     * Physical address computation
     * if the prefetch is within the same page
     *   using VA: add the computed stride to the original PA
     *   using PA: no actions needed
     * if we are page crossing
     *   using VA: Create a translaion request and enqueue the corresponding
     *       deferred packet to the queue of pending translations
     *   using PA: use the provided VA to obtain the target VA, then attempt to
     *     translate the resulting address
     */

    panic_if(useVirtualAddresses, "Current RandomFill does not expect to use vaddr\n");
    Addr orig_addr = useVirtualAddresses ?
        pkt->req->getVaddr() : pkt->req->getPaddr();

    Addr target_paddr = new_pfi.getAddr();
    RequestPtr translation_req = nullptr;
    panic_if(!samePage(orig_addr, target_paddr),
    	"Current RandomFill does not expect to have cross-page fetches\n");
    
    if (cacheSnoop &&
            (inCache(target_paddr, new_pfi.isSecure(),
                new_pfi.getDomainID()) ||
            inMissQueue(target_paddr, new_pfi.isSecure(),
                new_pfi.getDomainID()))
        ) {
        statsQueued.pfInCache++;
        DPRINTF(HWPrefetch, "Dropping redundant in "
                "cache/MSHR RFilladdr:%#x\n", target_paddr);
        return;
    }

    /* Create the packet and find the spot to insert it */
    DeferredPacket dpp(this, new_pfi, 0, priority);
    Tick pf_time = curTick() + clockPeriod() * latency;
    dpp.createPkt(target_paddr, blkSize, requestorId, tagPrefetch,
                  pf_time);
    DPRINTF(HWPrefetch, "Prefetch queued. "
            "RFilladdr:%#x priority: %3d tick:%lld.\n",
            new_pfi.getAddr(), priority, pf_time);
    addToQueue(pfq, dpp);
}

void
RandomFill::calculatePrefetch(const PrefetchInfo &pfi,
	                       std::vector<AddrPriority> &addresses)
{
	Addr blk_addr = blockAddress(pfi.getAddr());
	for (int d = 0; d < degree; d++) {
		int random_offset = random_mt.random<int>(lowRange, highRange);
		Addr new_addr = blk_addr + (random_offset << lBlkSize);
		if(!samePage(blk_addr, new_addr)) {
			random_offset = -random_offset;
			new_addr = blk_addr + (random_offset << lBlkSize);
			panic_if(!samePage(blk_addr, new_addr),
				"new_addr: %#x and blk_addr: %#x not on the same page\n",
				blk_addr, new_addr);
            panic_if(new_addr != blockAddress(new_addr),
                "new_addr: %#x unaligned to blockSize\n",
                new_addr);
		}
		addresses.push_back(AddrPriority(new_addr, 0));
		DPRINTF(HWPrefetch, "InsertRFill new_addr: %#x and blk_addr: %#x "
			"offset: %d\n", blk_addr, new_addr, random_offset);
	}
}

} // namespace Prefetcher