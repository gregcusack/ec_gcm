//
// Created by Greg Cusack on 10/23/19.
//

#include "cpu_l.h"

ec::local::stats::cpu::cpu()
    : quota(-1), period(100000000), alloc_extra_slices(false),
    num_local_slices_requested(0), num_local_slices_acquired(0),
    extra_runtime_to_give(0) {}
