//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_MEM_G_H
#define EC_GCM_MEM_G_H

#include <cstdint>
#include <ostream>
#include <mutex>

namespace ec {
    namespace global {
        namespace stats {
            class mem {
            public:
                mem();
                mem(uint64_t _mem_lim, int64_t _alloc_pages, uint64_t _slice_size);

                /**
                 * GETTERS
                 */
                 uint64_t get_slice_size() { return slice_size; }

                 uint64_t get_mem_limit_in_pages();
                 int64_t get_unallocated_memory_in_pages();
                 int64_t get_allocated_memory_in_pages();

                /**
                 * SETTERS
                 */
                void set_slice_size(uint64_t _ss) { slice_size = _ss; }
                int64_t set_unallocated_memory_in_pages(int64_t _ma);
                void decr_unallocated_memory_in_pages(uint64_t _to_decr);
                void incr_unallocated_memory_in_pages(uint64_t _to_incr);

                void set_allocated_memory_in_pages(int64_t _ma);
                void incr_allocated_memory_in_pages(int64_t _incr);
                void decr_allocated_memory_in_pages(int64_t _decr);

                void set_memory_limit_in_pages(uint64_t _ml);
                void incr_memory_limit_in_pages(uint64_t _incr);
                void decr_memory_limit_in_pages(uint64_t _decr);


            private:
                uint64_t memory_limit_in_pages;
                uint64_t slice_size;
                int64_t unallocated_memory_in_pages;
                int64_t allocated_memory_in_pages;

                std::mutex totmem_limit_lock;
                std::mutex unalloc_mem_lock;
                std::mutex alloc_mem_lock;

            friend std::ostream& operator<<(std::ostream& os, const mem& rhs) {
                os << "memory_limit_in_pages: " << rhs.memory_limit_in_pages << ", "
                    << "allocated_memory_in_pages: " << rhs.allocated_memory_in_pages << ", "
                    << "slice_size: " << rhs.slice_size << ", "
                    << "unallocated_memory_in_pages: " << rhs.unallocated_memory_in_pages;
                return os;
            }

            };
        }
    }
}



#endif //EC_GCM_MEM_G_H
