//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_MEM_L_H
#define EC_GCM_MEM_L_H

#include <cstdint>
#include <mutex>

namespace ec {
    namespace local {
        namespace stats {
            class mem {
            public:
                mem();

                [[nodiscard]] uint64_t get_mem_limit_in_pages() const; //{ return mem_limit_in_pages; }
                void set_mem_limit_in_pages(uint64_t _new_limit);// { mem_limit_in_pages = _new_limit; }
                void incr_mem_limit(uint64_t _incr);// { mem_limit_in_pages += _incr; }
                void decr_mem_limit(uint64_t _decr);

            private:
                uint64_t mem_limit_in_pages;
                uint64_t current_usage;
            };
        }
    }
}



#endif //EC_GCM_MEM_L_H
