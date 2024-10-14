/**
 * @file
 * Describes a random fill prefetcher.
 */

#ifndef __MEM_CACHE_PREFETCH_RANDOMFILL_HH__
#define __MEM_CACHE_PREFETCH_RANDOMFILL_HH__

#include <string>
#include <unordered_map>
#include <vector>

#include "base/sat_counter.hh"
#include "base/types.hh"
#include "mem/cache/prefetch/associative_set.hh"
#include "mem/cache/prefetch/queued.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"
#include "mem/cache/tags/indexing_policies/set_associative.hh"
#include "mem/packet.hh"

#include "params/RandomFillPrefetcher.hh"

struct RandomFillPrefetcherParams;

namespace Prefetcher {

class RandomFill : public Queued
{
  protected:
  	int lowRange;
		int highRange;
		const int degree;

  public:
		RandomFill(const RandomFillPrefetcherParams &p);
		void insert(const PacketPtr &pkt, PrefetchInfo &new_pfi, int32_t priority) override;

		void calculatePrefetch(const PrefetchInfo &pfi,
														std::vector<AddrPriority> &addresses) override;
};

} // namespace Prefetcher

#endif // __MEM_CACHE_PREFETCH_RANDOMFILL_HH__