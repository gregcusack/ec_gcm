//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_MEM_G_H
#define EC_GCM_MEM_G_H

#include <cstdint>
#include <ostream>

namespace ec {
    namespace global {
        namespace stats {
            class mem {
            public:
                mem();
                mem(uint64_t _mem_lim, uint64_t _curr_usage, uint64_t _slice_size);

                /**
                 * GETTERS
                 */
                 uint64_t get_mem_limit_in_pages() { return memory_limit_in_pages; }
                 uint64_t get_current_usage() { return current_usage; }
                 uint64_t get_slice_size() { return slice_size; }
                 [[nodiscard]] int64_t get_mem_available_in_pages() const { return memory_available_in_pages; }

                /**
                 * SETTERS
                 */

                void set_mem_limit_in_pages(uint64_t _ml) { memory_limit_in_pages = _ml; }
                void set_current_usage(uint64_t _cu) { current_usage = _cu; }
                void set_slice_size(uint64_t _ss) { slice_size = _ss; }
                int64_t set_memory_available_in_pages(int64_t _ma) { memory_available_in_pages = _ma; return memory_available_in_pages; }
                void decr_memory_available_in_pages(uint64_t _to_decr);
                void incr_memory_available_in_pages(uint64_t _to_incr);

                void incr_total_memory(uint64_t _incr) { memory_limit_in_pages += _incr; }
                void decr_total_memory(uint64_t _decr);


            private:
                uint64_t memory_limit_in_pages;
                uint64_t current_usage;
                uint64_t slice_size;
                int64_t memory_available_in_pages;

            friend std::ostream& operator<<(std::ostream& os, const mem& rhs) {
                os << "memory_limit_in_pages: " << rhs.memory_limit_in_pages << ", "
                    << "current_usage: " << rhs.current_usage << ", "
                    << "slice_size: " << rhs.slice_size << ", "
                    << "memory_available_in_pages: " << rhs.memory_available_in_pages;
                return os;
            }

            };
        }
    }
}



#endif //EC_GCM_MEM_G_H
