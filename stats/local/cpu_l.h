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

            private:
                int64_t quota;
                uint64_t period;
                bool alloc_extra_slices;
                uint32_t num_local_slices_requested;
                uint32_t num_local_slices_acquired;
                uint64_t extra_runtime_to_give;

            };
        }
    }
}



#endif //EC_GCM_CPU_L_H
