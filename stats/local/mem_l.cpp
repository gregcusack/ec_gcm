//
// Created by Greg Cusack on 10/23/19.
//

#include "mem_l.h"

ec::local::stats::mem::mem()
    : mem_limit_in_pages(0), current_usage(0) {}

void ec::local::stats::mem::decr_mem_limit(uint64_t _decr) {
    if(_decr > mem_limit_in_pages) {
        mem_limit_in_pages = 0;
    }
    else {
        mem_limit_in_pages -= _decr;
    }
}