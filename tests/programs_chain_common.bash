#! /bin/bash

# WARNING: this tests rely on log messages of helloworld and
#          transparenthelloworld services, changing them may cause this tests to
#          fail

source "${BASH_SOURCE%/*}/helpers.bash"

LOG_SIZE=50

# Expected sequences of log messages for a packet flowing from ns1 to ns2 and
# from ns2 to ns1.
# Sequences are in reversed order.
seq_1to2=("[hw2] [EGRESS]"
          "[hw2] Forwarding"
          "[hw2] [INGRESS]"
          "[th3] [EGRESS]"
          "[hw1] Forwarding"
          "[hw1] [INGRESS]"
          "[th2] [INGRESS]"
          "[th1] [INGRESS]"
          )
seq_2to1=("[th1] [EGRESS]"
          "[hw1] [EGRESS]"
          "[th2] [EGRESS]"
          "[hw1] Forwarding"
          "[hw1] [INGRESS]"
          "[th3] [INGRESS]"
          "[hw2] Forwarding"
          "[hw2] [INGRESS]"
          )

# Expected sequence of log messages for a packet flowing out of the network
# stack.
# Sequence is in reversed order.
seq_stack_egress=("[th1] [EGRESS]"
                  "[hw1] [EGRESS]")

function cleanup {
  set +e

  polycubectl hw1 del
  polycubectl hw2 del
  polycubectl th1 del
  polycubectl th2 del
  polycubectl th3 del

  delete_veth 2

  echo "FAIL"
}

# Retrieves the last LOG_SIZE lines of polycubed log, output in string $1
function get_log {
  local -n output=$1

  # Check if polycubed is running into a container
  if [ -z "$container" ]; then
    # Retrieve log from tmp file
    output=$(tail -n $LOG_SIZE tmp)
  else
    # Retrieve the log from the stdout of the container
    output=$(docker logs --tail $LOG_SIZE $container)
  fi
}

# Reverses the log $1 and extracts only inportant parts, output in array $2
function process_log {
  local -n output=$2
  output=()
  IFS_BACKUP=$IFS
  while IFS= read -r line; do
    IFS=" "
    read -ra arr <<< "$line"
    log_line="${arr[3]} ${arr[5]}"
    output=("$log_line" "${output[@]}")
  done <<< "$1"
  IFS=$IFS_BACKUP
}

# Looks for sequence $1 in array $2, returns 0 if found, 1 otherwise
function find_sequence {
  local -n seq=$1
  local -n arr=$2
  local ret=1
  local i=0
  for line in "${arr[@]}"; do
    if [ "$line" = "${seq[i]}" ]; then
      ((i=i+1))
      if [ $i -eq ${#seq[@]} ]; then
        ret=0
        break
      fi

    elif [ $i -gt 0 ]; then
      break
    fi
  done

  return $ret
}


# Tests whether ingress and egress programs in a chain of cubes are executed
# correcly
# Cubes chain:
#       +-----+        +-----+-----+-----+        +-----+
# veth1-| th1 |--------| th2 | hw1 | th3 |--------| hw2 |-------- veth2
#       +-----+        +-----+-----+-----+        +-----+
# th = TransparentHelloworld
# hw = HelloWorld
# $1: xdp_skb | tc
function test_programs_chain {
  trap cleanup EXIT

  create_veth_no_ipv6 2

  set -e
  set -x

  polycubectl helloworld add hw1 loglevel=trace type=$1
  polycubectl hw1 set action=forward
  polycubectl hw1 ports add to_veth1 peer=veth1
  polycubectl hw1 ports add to_hw2

  polycubectl helloworld add hw2 loglevel=trace type=$1
  polycubectl hw2 set action=forward
  polycubectl hw2 ports add to_hw1
  polycubectl hw2 ports add to_veth2 peer=veth2

  polycubectl connect hw1:to_hw2 hw2:to_hw1

  polycubectl transparenthelloworld add th1 loglevel=trace type=$1
  polycubectl attach th1 veth1

  polycubectl transparenthelloworld add th2 loglevel=trace type=$1
  polycubectl attach th2 hw1:to_veth1

  polycubectl transparenthelloworld add th3 loglevel=trace type=$1
  polycubectl attach th3 hw1:to_hw2

  set +x

  sudo ip netns exec ns1 ping 10.0.0.2 -c 1 1>/dev/null 2>&1

  get_log log

  process_log "$log" plog

  find_sequence seq_1to2 plog
  find_sequence seq_2to1 plog

  # Cleanup
  set -x
  polycubectl hw1 del
  polycubectl hw2 del
  polycubectl th1 del
  polycubectl th2 del
  polycubectl th3 del
  set +x
  delete_veth 2
  trap - EXIT

  echo "SUCCESS"
}

# Tests whether egress programs are executed correcly with packets flowing out
# of the network stack of the host.
# Cubes chain:
#       +-----+        +-----+-----+
# veth1-| th1 |--------| th2 | hw1 |
#       +-----+        +-----+-----+
# th = TransparentHelloworld
# hw = HelloWorld
# $1: xdp_skb | tc
function test_stack_programs_chain {
  trap cleanup EXIT

  create_veth_no_ipv6 1

  set -e
  set -x

  sudo ip addr add 10.0.0.2/24 dev veth1

  polycubectl helloworld add hw1 loglevel=trace type=$1
  polycubectl hw1 ports add to_veth1 peer=veth1

  polycubectl transparenthelloworld add th1 loglevel=trace type=$1
  polycubectl attach th1 veth1

  polycubectl transparenthelloworld add th2 loglevel=trace type=$1
  polycubectl attach th2 hw1:to_veth1

  set +x

  # This command will fail since the echo reply is dropped by hw1 cube
  ping 10.0.0.1 -c 1 1>/dev/null 2>&1 || true

  get_log log

  process_log "$log" plog

  find_sequence seq_stack_egress plog

  # Cleanup
  set -x
  polycubectl hw1 del
  polycubectl th1 del
  polycubectl th2 del
  set +x
  delete_veth 1
  trap - EXIT

  echo "SUCCESS"
}