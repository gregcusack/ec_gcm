//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_CPU_G_H
#define EC_GCM_CPU_G_H

#include <cstdint>
#include <iostream>
#include <mutex>
#include "spdlog/spdlog.h"

namespace ec {
    namespace global {
        namespace stats {
            class cpu {
            public:
                cpu();
                explicit cpu(uint64_t _slice_size);

                /**
                 * GETTERS
                 */
                [[nodiscard]] uint64_t get_slice() const { return slice_size; }
                uint64_t get_unallocated_rt();// { return unallocated_rt; }
                uint64_t get_total_cpu();
                uint64_t get_overrun();
                uint64_t get_alloc_rt();

                /**
                 * SETTERS
                 */
                 void set_slice_size(uint64_t _slice) { slice_size = _slice; }
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
                uint64_t slice_size;
                uint64_t unallocated_rt;
                uint64_t total_cpu;
                uint64_t overrun;
                uint64_t alloc_rt;

		        std::mutex unalloc_lock;
		        std::mutex overrun_lock;
		        std::mutex totcpu_lock;
                std::mutex alloc_lock;

            friend std::ostream& operator<<(std::ostream& os, const cpu& rhs) {
                os << "slice_size: " << rhs.slice_size;
                return os;
            }


            };
        }
    }
}



#endif //EC_GCM_CPU_G_H
