//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_MEM_L_H
#define EC_GCM_MEM_L_H

#include <cstdint>

namespace ec {
    namespace local {
        namespace stats {
            class mem {
            public:
                mem();

            private:
                uint32_t mem_limit;
                uint32_t current_usage;


            };
        }
    }
}



#endif //EC_GCM_MEM_L_H
