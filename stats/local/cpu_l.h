//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_CPU_L_H
#define EC_GCM_CPU_L_H

#include <cstdint>

namespace ec {
    namespace local {
        namespace stats {
            class cpu {
            public:
                cpu();
                cpu(uint64_t _quota, uint32_t _nr_throttled);

                int64_t get_quota() { return quota; }
                uint32_t get_throttled() { return nr_throttled; }

                void set_quota(int64_t _quota) { quota = _quota; }
                void set_throttled(uint32_t _throttled) { nr_throttled = _throttled; }

                uint32_t get_throttle_increase(uint32_t _throttled) { return _throttled - nr_throttled; }

            private:
                int64_t quota;
                uint64_t period;
                bool alloc_extra_slices;
                uint32_t num_local_slices_requested;
                uint32_t num_local_slices_acquired;
                uint64_t extra_runtime_to_give;
                uint32_t nr_throttled;

            };
        }
    }
}



#endif //EC_GCM_CPU_L_H
