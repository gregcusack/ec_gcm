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
        std::cout << "unalloc - decr < 0. decr: " << _decr << std::endl;
        unallocated_rt = 0;
    }
    else {
        unallocated_rt -= _decr;
    }
    std::cout << "decr unalloc post update: " << unallocated_rt << std::endl;
}


void ec::global::stats::cpu::incr_unalloacted_rt(uint64_t _incr) {
    std::unique_lock<std::mutex> lk(unalloc_lock);
    unallocated_rt += _incr;
}

void ec::global::stats::cpu::set_unallocated_rt(uint64_t _val) {
    std::unique_lock<std::mutex> lk(unalloc_lock);
    unallocated_rt = _val;
}

uint64_t ec::global::stats::cpu::get_unallocated_rt() {
    std::unique_lock<std::mutex> lk(unalloc_lock);
    return unallocated_rt;
}

uint64_t ec::global::stats::cpu::get_total_cpu() {
    std::unique_lock<std::mutex> lk(totcpu_lock);
    return total_cpu;
}

uint64_t ec::global::stats::cpu::get_overrun() {
    std::unique_lock<std::mutex> lk(overrun_lock);
    return overrun;
}

void ec::global::stats::cpu::set_total_cpu(uint64_t _tot_cpu) {
    std::unique_lock<std::mutex> lk(totcpu_lock);
    total_cpu = _tot_cpu;
}

void ec::global::stats::cpu::incr_total_cpu(uint64_t _incr) {
    std::unique_lock<std::mutex> lk(totcpu_lock);
    total_cpu += _incr;
}

void ec::global::stats::cpu::decr_total_cpu(uint64_t _decr) {
    std::unique_lock<std::mutex> lk(totcpu_lock);
    if( ((int64_t) total_cpu - (int64_t) _decr) < 0) {
        total_cpu = 0;
    }
    else {
        total_cpu -= _decr;
    }
}

void ec::global::stats::cpu::incr_overrun(uint64_t _incr) {
    std::unique_lock<std::mutex> lk(overrun_lock);
    overrun += _incr;
}

void ec::global::stats::cpu::decr_overrun(uint64_t _decr) {
    std::unique_lock<std::mutex> lk(overrun_lock);
    if( ((int64_t) overrun - (int64_t) _decr) < 0) {
        overrun = 0;
    }
    else {
        overrun -= _decr;
    }
}

void ec::global::stats::cpu::set_overrun(uint64_t _val) {
    std::unique_lock<std::mutex> lk(overrun_lock);
    overrun = _val;
}
