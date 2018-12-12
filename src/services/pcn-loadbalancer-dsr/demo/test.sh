#!/bin/bash

# for i in `seq 4 9`;
# do
#   for j in `seq 1 10`;
#   do
#     sudo ip netns exec ns$i wget 10.0.0.100:8000
#   done
# done

 sudo ip netns exec ns4 ab -n 10000 -c 20  http://10.0.0.100:8000/
