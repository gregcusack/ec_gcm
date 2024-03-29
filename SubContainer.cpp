//
// Created by greg on 9/11/19.
//

#include "SubContainer.h"


ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, int _fd)
    : fd(_fd), cpu(local::stats::cpu()), mem(local::stats::mem()), inserted(false), quota_mismatch_counter(0),
      last_seen_ts(std::chrono::high_resolution_clock::now())  {
    c_id = ContainerId(cgroup_id, ip4_addr::from_net(ip));
    counter = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, int _fd, uint64_t _quota, uint32_t _nr_throttled)
    : fd(_fd), mem(local::stats::mem()), inserted(false), quota_mismatch_counter(0),
      last_seen_ts(std::chrono::high_resolution_clock::now())  {

    SPDLOG_TRACE("creating sc with cg_id, quota, throttle: {}, {}, {}", cgroup_id, _quota, _nr_throttled);
    c_id = ContainerId(cgroup_id, ip4_addr::from_net(ip));
    cpu = local::stats::cpu(_quota, _nr_throttled);
    counter = 0;
}

uint32_t ec::SubContainer::get_thr_incr_and_set_thr(uint32_t _throttled) {
    auto incr = cpu.get_throttle_increase(_throttled);
    if(incr != 0) {
        cpu.set_throttled(_throttled);
    }
    return incr;
}

uint64_t ec::SubContainer::get_quota() {
    std::unique_lock<std::mutex> lk(lockcpu);
    return cpu.get_quota();
}

void ec::SubContainer::set_quota(uint64_t _quota) {
    std::unique_lock<std::mutex> lk(lockcpu);
    cpu.set_quota(_quota);
}

void ec::SubContainer::set_quota_flag(bool val) {
    std::unique_lock<std::mutex> lk(lockcpu);
    cpu.set_set_quota_flag(val);
}

void ec::SubContainer::incr_cpustat_seq_num() {
    std::unique_lock<std::mutex> lk(lock_seqnum);
    cpu.incr_seq_num();
}

void ec::SubContainer::set_cpustat_seq_num(uint64_t val) {
    std::unique_lock<std::mutex> lk(lock_seqnum);
    cpu.set_seq_num(val);
}

uint64_t ec::SubContainer::get_seq_num() {
    std::unique_lock<std::mutex> lk(lock_seqnum);
    return cpu.get_seq_num();
}

int ec::SubContainer::incr_quota_mismatch_counter() {
    std::unique_lock<std::mutex> lk(lock_mismatch);
    return quota_mismatch_counter++;
}

int ec::SubContainer::get_quota_mismatch_counter() {
    std::unique_lock<std::mutex> lk(lock_mismatch);
    return quota_mismatch_counter;
}

void ec::SubContainer::reset_quota_mismatch_counter() {
    std::unique_lock<std::mutex> lk(lock_mismatch);
    quota_mismatch_counter = 0;
}

bool ec::SubContainer::check_if_idle(std::chrono::system_clock::time_point now) {
    auto time_diff = std::chrono::duration_cast<std::chrono::microseconds>(now - last_seen_ts).count();
    if (time_diff > 2000000) { //# 2 seconds
        return true;
    }
    return false;
}

void ec::SubContainer::update_last_seen_ts(std::chrono::system_clock::time_point now) {
    last_seen_ts = now;
}


bool ec::SubContainer::ContainerId::operator==(const ec::SubContainer::ContainerId &other_) const {
    return  cgroup_id   == other_.cgroup_id
            && server_ip == other_.server_ip;
}

