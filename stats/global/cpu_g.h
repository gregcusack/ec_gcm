//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_CPU_G_H
#define EC_GCM_CPU_G_H

#include <cstdint>
#include <iostream>

namespace ec {
    namespace global {
        namespace stats {
            class cpu {
            public:
                cpu();
                cpu(uint64_t _period, int64_t _quota, uint64_t _slice_size);

                /**
                 * GETTERS
                 */
                int64_t get_quota() { return quota; }
                uint64_t get_period() { return period; }
                uint64_t get_slice_size() { return slice_size; }
                uint64_t get_runtime_remaining() { return runtime_remaining; }

                /**
                 * SETTERS
                 */
                 void set_quota(int64_t _quota) { quota = _quota; }
                 void set_period(uint64_t _period) { period = _period; }
                 void set_slice_size(uint64_t _slice) { slice_size = _slice; }
                 void set_runtime_remaining(uint64_t _rt_remain) { runtime_remaining = _rt_remain; }
                 void decr_runtime_remaining(uint64_t _to_decr) { runtime_remaining -= _to_decr; }
                 uint64_t refill_runtime();


            private:
                int64_t quota;
                uint64_t period;
                uint64_t slice_size;
                uint64_t runtime_remaining;
//                uint32_t cpu_limit_in_cores;
//                uint32_t cpu_current_usage_in_cores;
//                uint32_t nr_need_cpu;
//                uint32_t nr_give_cpu;

            friend std::ostream& operator<<(std::ostream& os, const cpu& rhs) {
                os  << "period: " << rhs.period << ", "
                    << "quota: " << rhs.quota << ", "
                    << "slice_size: " << rhs.slice_size << ", "
                    << "runtime_remaining: " << rhs.runtime_remaining;
                return os;
            }


            };
        }
    }
}



#endif //EC_GCM_CPU_G_H
