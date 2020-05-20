//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_CPU_G_H
#define EC_GCM_CPU_G_H

#include <cstdint>
#include <iostream>
#include <mutex>

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
                uint64_t get_quota() { return quota; }
                uint64_t get_period() { return period; }
                uint64_t get_slice() { return slice_size; }
                uint64_t get_runtime_remaining() { return runtime_remaining; }
                uint64_t get_unallocated_rt();// { return unallocated_rt; }
                uint64_t get_total_cpu();
                uint64_t get_overrun();
                uint64_t get_alloc_rt();

                /**
                 * SETTERS
                 */
                 void set_quota(int64_t _quota) { quota = _quota; }
                 void set_period(uint64_t _period) { period = _period; }
                 void set_slice_size(uint64_t _slice) { slice_size = _slice; }
                 void set_runtime_remaining(uint64_t _rt_remain) { runtime_remaining = _rt_remain; }
                 void decr_runtime_remaining(uint64_t _to_decr) { runtime_remaining -= _to_decr; }
                 uint64_t refill_runtime();
                 void incr_unalloacted_rt(uint64_t _incr);// { unallocated_rt += _incr; }
                 void decr_unallocated_rt(uint64_t _decr);// { unallocated_rt -= _decr; }
                 void set_unallocated_rt(uint64_t _val);// { unallocated_rt = _val; }
                 void set_total_cpu (uint64_t _tot_cpu);// { total_cpu = _tot_cpu; }
                 void incr_total_cpu (uint64_t _incr);// { total_cpu += _incr; }
                 void decr_total_cpu (uint64_t _decr);// { total_cpu -= _decr; }
                 void incr_overrun(uint64_t _incr);// { overrun += _incr; }
                 void decr_overrun(uint64_t _decr);// { overrun -= _decr; }
                 void set_overrun(uint64_t _val);// { overrun = _val; }

                 void incr_alloc_rt(uint64_t _incr);
                 void decr_alloc_rt(uint64_t _decr);




            private:
                uint64_t quota;
                uint64_t period;
                uint64_t slice_size;
                uint64_t runtime_remaining;
                uint64_t unallocated_rt;
                uint64_t total_cpu;
                uint64_t overrun;
                uint64_t alloc_rt;

		        std::mutex unalloc_lock;
		        std::mutex overrun_lock;
		        std::mutex totcpu_lock;
                std::mutex alloc_lock;
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
