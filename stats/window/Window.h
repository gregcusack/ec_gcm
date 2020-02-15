//
// Created by greg on 2/12/20.
//

#ifndef EC_GCM_WINDOW_H
#define EC_GCM_WINDOW_H

#include <vector>
#include <cstdint>
#include <iostream>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace ec {
    template <typename T, typename K>
    class Window {
    public:
        explicit Window(uint32_t _winsize);

        K insert(K element);
        K get_mean() { return mean; }

    private:
        K update_stats(K element);
        void update_trend();

        uint32_t winsize;
        K mean;
        uint32_t periods_elapsed;
        T trend;
        T offset;
        K popped;

        std::vector<K> winstat;
    };
}
#include "Window_impl.h"

#endif //EC_GCM_WINDOW_H
