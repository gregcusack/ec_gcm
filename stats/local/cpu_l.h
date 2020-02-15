//
// Created by Greg Cusack on 10/23/19.
//

#ifndef EC_GCM_CPU_L_H
#define EC_GCM_CPU_L_H

#include <cstdint>
#include "../window/Window.h"

#define __WINDOW_STAT_SIZE_ 10

namespace ec {
    namespace local {
        namespace stats {
            class cpu {
            public:
                cpu();
                cpu(uint64_t _quota, uint32_t _nr_throttled);

                uint64_t get_quota() { return quota; }
                uint32_t get_throttled() { return nr_throttled; }

                void set_quota(uint64_t _quota) { quota = _quota; }
                void set_throttled(uint32_t _throttled) { nr_throttled = _throttled; }

                uint32_t get_throttle_increase(uint32_t _throttled) { return _throttled - nr_throttled; }

                //TODO: insert pure rt_remain? or quota - rt_remain?
                uint64_t insert_rt_stats(uint64_t element);
                //TODO: insert curr_throt - last_throt
                double insert_th_stats(uint32_t element);

                bool get_set_quota_flag() { return set_quota_flag; }
                void set_set_quota_flag(bool val) { set_quota_flag = val; }

                uint64_t get_rt_mean() { return rt_winstats.get_mean(); }
                uint32_t get_thr_mean() { return th_winstats.get_mean(); }

            private:
                uint64_t quota;
                uint64_t period;
                bool alloc_extra_slices;
                uint32_t num_local_slices_requested;
                uint32_t num_local_slices_acquired;
                uint64_t extra_runtime_to_give;

                uint32_t nr_throttled;

                Window<int64_t, double> rt_winstats; //rt_remaining stats
                Window<uint32_t, double> th_winstats; //throttle stats
                bool set_quota_flag;

            };
        }
    }
}



#endif //EC_GCM_CPU_L_H
