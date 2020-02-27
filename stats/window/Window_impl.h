//
// Created by greg on 2/12/20.
//

#include "Window.h"

template<typename T, typename K>
ec::Window<T,K>::Window(uint32_t _winsize)
    : winsize(_winsize), winstat(_winsize, 0), mean(0),
    periods_elapsed(1), trend(0), offset(0), popped(0) {}

template<typename T, typename K>
K ec::Window<T,K>::insert(K element) {
    popped = winstat[periods_elapsed % winsize];
    winstat[periods_elapsed % winsize] = element;
    return update_stats(element);
}

template<typename T, typename K>
K ec::Window<T,K>::update_stats(K element) {
    if(unlikely(periods_elapsed <= winsize)) {
        mean = ((K)(periods_elapsed - 1) * (K) mean + element) / ((K) periods_elapsed);
//        K inc = ((K) element - (K) mean) / ((K) periods_elapsed);
//        mean += inc;
    }
    else {
//        std::cout << "prev mean: " << (T) mean << std::endl;
        K meanInc = ((K) element - (K) popped) / ((K) winsize);
        mean += meanInc;
//        std::cout << "p_elapsed, winsize, mean, element, popped: (" << periods_elapsed << "," << winsize << "," << (T)mean
//                  << "," << (T)element << "," << (T)popped << ")" << std::endl;
//        for(const auto &i : winstat) {
//            std::cout << (T)i << ", ";
//        }
//        std::cout << std::endl;
    }
//    if(periods_elapsed % winsize == 0) {
//        update_trend();
//    }
    periods_elapsed++;
    if(mean < 0.001) {
        mean = 0;
    }
    return mean;
}

//Calculate slope (trend) and offset (not sure offset needed)
template<typename T, typename K>
void ec::Window<T,K>::update_trend() {
    uint64_t sum_x = 0, sum_ind = 0, sum_x_ind = 0, sum_x2 = 0;
    for(uint32_t i=0; i < winsize; i++) {
        sum_x += winstat[i];
        sum_ind += 1; //assume time is static (aka every 100ms)
        sum_x_ind += winstat[i] * 1;
        sum_x2 += (winstat[i] * winstat[i]);
    }
    auto div = winsize * sum_x2 - (sum_x * sum_x);
    if(div == 0) {
        std::cout << "winsize, sum_x2, sum_x^2: " << winsize << ", " << sum_x2 << ", " << (sum_x * sum_x) << std::endl;
        trend = 0;
        offset = 1;
        return;
    }

    trend = (winsize * sum_x_ind - sum_x*sum_ind) / div;
    offset = (sum_ind - trend * sum_x) / winsize;
}

template<typename T, typename K>
void ec::Window<T, K>::flush() {
    std::fill(winstat.begin(), winstat.end(), 0);
    mean = 0;
    trend = 0;
    offset = 0;
    popped = 0;
}

