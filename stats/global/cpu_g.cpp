//
// Created by Greg Cusack on 10/23/19.
//

#include "cpu_g.h"

ec::global::stats::cpu::cpu()
    : quota(0), period(100000000),
    slice_size(30000000), runtime_remaining(100000000),
    unallocated_rt(0), total_cpu(0), overrun(0) {}

ec::global::stats::cpu::cpu(uint64_t _period, int64_t _quota, uint64_t _slice_size)
    : period(_period), quota(_quota),
    slice_size(_slice_size), runtime_remaining(_quota),
    unallocated_rt(0), total_cpu(0), overrun(0) {}

uint64_t ec::global::stats::cpu::refill_runtime() {
    std::cout << "refilling runtime_remaining" << std::endl;
    runtime_remaining = quota;
    return runtime_remaining;
}

void ec::global::stats::cpu::decr_unallocated_rt(uint64_t _decr) {
    std::unique_lock<std::mutex> lk(unalloc_lock);
    std::cout << "Decr unalloc: " << unallocated_rt << std::endl;
    if( ((int64_t) unallocated_rt - (int64_t) _decr) < 0) {
        unallocated_rt = 0;
	std::cout << "unalloc - decr < 0. decr: " << _decr << std::endl;
    }
    else {
        unallocated_rt -= _decr;
    }
    std::cout << "decr unalloc post update: " << unallocated_rt << std::endl;
}
