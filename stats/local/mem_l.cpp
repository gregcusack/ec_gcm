//
// Created by Greg Cusack on 10/23/19.
//

#include "mem_l.h"

ec::local::stats::mem::mem()
    : mem_limit_in_pages(0), current_usage(0) {}

uint64_t ec::local::stats::mem::get_mem_limit_in_pages() {
    std::unique_lock<std::mutex> lk(mem_limit_lock);
    return mem_limit_in_pages;
}

void ec::local::stats::mem::set_mem_limit_in_pages(uint64_t _new_limit) {
    std::unique_lock<std::mutex> lk(mem_limit_lock);
    mem_limit_in_pages = _new_limit;

}

void ec::local::stats::mem::incr_mem_limit(uint64_t _incr) {
    std::unique_lock<std::mutex> lk(mem_limit_lock);
    mem_limit_in_pages += _incr;
}

void ec::local::stats::mem::decr_mem_limit(uint64_t _decr) {
    std::unique_lock<std::mutex> lk(mem_limit_lock);
    if(_decr > mem_limit_in_pages) {
        mem_limit_in_pages = 0;
    }
    else {
        mem_limit_in_pages -= _decr;
    }
}
