//
// Created by Greg Cusack on 10/23/19.
//

#include "cpu_g.h"

ec::global::stats::cpu::cpu()
    : quota(100000000), period(100000000),
    slice_size(500000), runtime_remaining(100000000),
    unallocated_rt(0) {}

ec::global::stats::cpu::cpu(uint64_t _period, int64_t _quota, uint64_t _slice_size)
    : period(_period), quota(_quota),
    slice_size(_slice_size), runtime_remaining(_quota) {}

uint64_t ec::global::stats::cpu::refill_runtime() {
    std::cout << "refilling runtime_remaining" << std::endl;
    runtime_remaining = quota;
    return runtime_remaining;
}
