//
// Created by Greg Cusack on 10/23/19.
//

#include "mem_g.h"

ec::global::stats::mem::mem()
    : memory_limit_in_pages(0), allocated_memory_in_pages(0), slice_size(5000) {
    unallocated_memory_in_pages = memory_limit_in_pages;
}

ec::global::stats::mem::mem(uint64_t _mem_lim, int64_t _alloc_pages, uint64_t _slice_size)
    : memory_limit_in_pages(_mem_lim), allocated_memory_in_pages(_alloc_pages),
      slice_size(_slice_size), unallocated_memory_in_pages(_mem_lim) {}

uint64_t ec::global::stats::mem::get_mem_limit_in_pages() {
    std::unique_lock<std::mutex> lk(totmem_limit_lock);
    return memory_limit_in_pages;
}

void ec::global::stats::mem::set_memory_limit_in_pages(uint64_t _ml) {
    std::unique_lock<std::mutex> lk(totmem_limit_lock);
    memory_limit_in_pages = _ml;
}

void ec::global::stats::mem::incr_memory_limit_in_pages(uint64_t _incr) {
    std::unique_lock<std::mutex> lk(totmem_limit_lock);
    memory_limit_in_pages += _incr;
}

void ec::global::stats::mem::decr_memory_limit_in_pages(uint64_t _decr) {
    std::unique_lock<std::mutex> lk(totmem_limit_lock);
    if(memory_limit_in_pages < _decr) {
        memory_limit_in_pages = 0;
    } else {
        memory_limit_in_pages -= _decr;
    }
}

int64_t ec::global::stats::mem::get_unallocated_memory_in_pages() {
    std::unique_lock<std::mutex> lk(unalloc_mem_lock);
    return unallocated_memory_in_pages;
}

int64_t ec::global::stats::mem::set_unallocated_memory_in_pages(int64_t _ma) {
    std::unique_lock<std::mutex> lk(unalloc_mem_lock);
    unallocated_memory_in_pages = _ma;
    return unallocated_memory_in_pages;
}

void ec::global::stats::mem::incr_unallocated_memory_in_pages(uint64_t _to_incr) {
    std::unique_lock<std::mutex> lk(unalloc_mem_lock);
    unallocated_memory_in_pages += _to_incr;
}

void ec::global::stats::mem::decr_unallocated_memory_in_pages(uint64_t _to_decr) {
    std::unique_lock<std::mutex> lk(unalloc_mem_lock);
    int64_t result = unallocated_memory_in_pages - _to_decr;
    unallocated_memory_in_pages = result < 0 ? 0 : result;
}

int64_t ec::global::stats::mem::get_allocated_memory_in_pages() {
    std::unique_lock<std::mutex> lk(alloc_mem_lock);
    return allocated_memory_in_pages;
}

void ec::global::stats::mem::set_allocated_memory_in_pages(int64_t _ma) {
    std::unique_lock<std::mutex> lk(alloc_mem_lock);
    allocated_memory_in_pages = _ma;
}

void ec::global::stats::mem::incr_allocated_memory_in_pages(int64_t _incr) {
    std::unique_lock<std::mutex> lk(alloc_mem_lock);
    allocated_memory_in_pages += _incr;
}

void ec::global::stats::mem::decr_allocated_memory_in_pages(int64_t _decr) {
    std::unique_lock<std::mutex> lk(alloc_mem_lock);
    int64_t result = allocated_memory_in_pages - _decr;
    allocated_memory_in_pages = result < 0 ? 0 : result;
}