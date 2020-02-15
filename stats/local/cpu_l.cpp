//
// Created by Greg Cusack on 10/23/19.
//

#include "cpu_l.h"

ec::local::stats::cpu::cpu()
    : quota(-1), period(100000000), alloc_extra_slices(false),
    num_local_slices_requested(0), num_local_slices_acquired(0),
    extra_runtime_to_give(0), nr_throttled(0),
    rt_winstats(__WINDOW_STAT_SIZE_), th_winstats(__WINDOW_STAT_SIZE_),
    set_quota_flag(false) {}

ec::local::stats::cpu::cpu(uint64_t _quota, uint32_t _nr_throttled)
    : quota(_quota), period(100000000), alloc_extra_slices(false),
    num_local_slices_requested(0), num_local_slices_acquired(0),
    extra_runtime_to_give(0), nr_throttled(_nr_throttled),
      rt_winstats(__WINDOW_STAT_SIZE_), th_winstats(__WINDOW_STAT_SIZE_),
      set_quota_flag(false) {}

uint64_t ec::local::stats::cpu::insert_rt_stats(uint64_t element) {
    return rt_winstats.insert(element);
}

//Insert increase in throttle
double ec::local::stats::cpu::insert_th_stats(uint32_t element) {
    auto insert = th_winstats.insert(get_throttle_increase(element));
    set_throttled(element);
    return insert;
}


