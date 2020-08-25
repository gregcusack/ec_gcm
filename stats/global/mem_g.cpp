//
// Created by Greg Cusack on 10/23/19.
//

#include "mem_g.h"

ec::global::stats::mem::mem()
    : memory_limit_in_pages(30000), current_usage(0), slice_size(5000) {
    memory_available_in_pages = memory_limit_in_pages;
}

ec::global::stats::mem::mem(uint64_t _mem_lim, uint64_t _curr_usage, uint64_t _slice_size)
    : memory_limit_in_pages(_mem_lim), current_usage(_curr_usage),
      slice_size(_slice_size), memory_available_in_pages(_mem_lim) {}

void ec::global::stats::mem::decr_memory_available_in_pages(uint64_t _to_decr) {
    int64_t result = memory_available_in_pages - _to_decr;
    memory_available_in_pages = result < 0 ? 0 : result;

}

void ec::global::stats::mem::incr_memory_available_in_pages(uint64_t _to_incr) {
    memory_available_in_pages += _to_incr;
}

void ec::global::stats::mem::decr_total_memory(uint64_t _decr) {
    if(memory_limit_in_pages < _decr) {
        memory_limit_in_pages = 0;
    } else {
        memory_limit_in_pages -= _decr;
    }

}

