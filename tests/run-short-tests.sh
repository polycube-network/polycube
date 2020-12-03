#!/bin/bash

source "${BASH_SOURCE%/*}/helpers_tests.bash"

# use a clean instance of polycubed to run each test
RELAUNCH_POLYCUBED=true

SERVICES_LOAD_TIMEOUT=10

# launch polycubed in DEBUG mode
DEBUG=false

# test bridge stp
TEST_BRIDGE_STP=false

# result.json path
RESULT_JSON="result.json"

declare -A servicester
services[ddosmitigator]=pcn-ddosmitigator
services[firewall]=pcn-firewall
services[helloworld]=pcn-helloworld
services[lbdsr]=pcn-loadbalancer-dsr
services[lbrp]=pcn-loadbalancer-rp
services[nat]=pcn-nat
services[pbforwarder]=pcn-pbforwarder
services[router]=pcn-router
services[simplebridge]=pcn-simplebridge
services[transparenthelloworld]=pcn-transparent-helloworld

trap cleanup EXIT

# $1 setup RELAUNCH_POLYCUBED
if [ $# -ne 0 ]
  then
    RELAUNCH_POLYCUBED=$1
fi

export PATH=$PATH:$GOPATH/bin

test_passed=0
test_total=0

failed=false
polycubed_crash=false
last_test_result=0


if [ -z "$polycubed" ]
then
      polycubed="sudo polycubed"
      if $DEBUG ; then
         polycubed=$polycubed"-d"
      fi
else
      echo "Polycube restart command is set to: $polycubed"
fi

test_results="test_results_"$(date +%Y%m%d_%H%M%S)
test_log="test_log_"$(date +%Y%m%d_%H%M%S)
test_tmp="tmp"

echo "Tests Scripts" > $test_results
date >> $test_results
echo >> $test_results
echo "RELAUNCH_POLYCUBED: "$RELAUNCH_POLYCUBED >> $test_results
echo "DEBUG: "$DEBUG >> $test_results
echo >> $test_results

echo "Tests Scripts - LOG FILE" > $test_log
date >> $test_log
echo >> $test_log

function run_tests_from_dir {
  for test in $1test*.sh; do
    if [[ $test == *"test*.sh"* ]]; then
      echo ""
    else
      run_test $test
    fi
  done
}

# Main script here
echo "*scanning current dir for tests*"
# ./test*.sh
run_tests_from_dir ./short-tests/

cat $test_log
