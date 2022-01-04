#!/bin/bash

num_tenants=$1
base_gcm_port=8888
base_tcp_port=5000
base_udp_port=6000
base_grpc_port=4447

out="Running GCM with ${num_tenants} tenants......"
echo $out

for ((i=1; i <= num_tenants; i++)) do
  gcm_port=$((base_gcm_port + i - 1))
  tcp_port=$((base_tcp_port + i - 1))
  udp_port=$((base_udp_port + i - 1))
  grpc_port=$((base_grpc_port + i - 1))
  #echo `pwd`
  #`pwd`/cmake-build-debug/ec_gcm tests/lh_app_def_1_node.json $gcm_port $tcp_port $udp_port $grpc_port &
  `pwd`/ec_gcm tests/app_def.json $gcm_port $tcp_port $udp_port $grpc_port &
done
