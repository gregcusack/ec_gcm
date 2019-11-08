//
// Created by Greg Cusack on 10/23/19.
//

#include "mem_g.h"

ec::global::stats::mem::mem()
    : memory_limit(30000), current_usage(0), slice_size(5000) {
    memory_available = memory_limit;
}

ec::global::stats::mem::mem(uint64_t _mem_lim, uint64_t _curr_usage, uint64_t _slice_size)
    : memory_limit(_mem_lim), current_usage(_curr_usage),
    slice_size(_slice_size), memory_available(_mem_lim) {}

void ec::global::stats::mem::decr_memory_available(uint64_t _to_decr) {
    int64_t result = memory_available - _to_decr;
    memory_available = result < 0 ? 0 : result;

}

