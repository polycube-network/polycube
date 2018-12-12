#!/bin/bash

# use a clean instance of polycubed to run each test
RELAUNCH_POLYCUBED=true

# launch polycubed in DEBUG mode
DEBUG=false

# result.json path
RESULT_JSON="result.json"

# cleanup environment before exit
function cleanup {
  set +e
  echo "killing polycubed ..."
  sudo pkill polycubed >> $test_tmp
  echo "{ \"passed\":\"$test_passed\", \"total\":\"$test_total\", \"test_log\":\"$test_log\" }" > $RESULT_JSON

  cat $test_results

  echo ""
  echo "FAILED TESTS:"
  echo ""
  cat $test_results | grep FAILED -A 1

  if $failed ; then
    exit 1
  fi
}
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

polycubed="sudo polycubed "
if $DEBUG ; then
  polycubed=$polycubed"-d"
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

# execute and log test ($1:path)
function log_test {
  "$@" >> $test_tmp 2>&1
  local status=$?
  last_test_result=$status

  # check if polycubed is still alive (no crash in the meanwhile)
  polycubed_alive=$(ps -el | grep polycubed)
  if [ -z "$polycubed_alive" ]; then
    echo "polycubed not running ..."
    polycubed_crash=true
    status=1
  fi
  test_total=$(($test_total+1))
  if [ $status -ne 0 ]; then
    echo "++++TEST $1 FAILED++++"
    echo "++++TEST $1 FAILED++++" >> $test_results
    echo "++++TEST $1 FAILED++++" >> $test_tmp
    failed=true
    cat $test_tmp >> $test_log
  else
    test_passed=$(($test_passed+1))
    echo "++++TEST $1 PASSED++++"
    echo "++++TEST $1 PASSED++++" >> $test_results
    echo "++++TEST $1 PASSED++++" >> $test_tmp
  fi
  return $status
}

# Check if polycubed rest server is responding
function polycubed_is_responding {
  ret=$(polycubectl ? > /dev/null)
  ret=$(echo $?)
  echo $ret
}

# Kill polycubed, and wait all services to be unloaded and process to be completely killed
function polycubed_kill_and_wait {
  echo "killing polycubed ..."
  sudo pkill polycubed >> $test_tmp

  done=0
  i=0
  while : ; do
    sleep 1
    alive=$(ps -el | grep polycubed)
    if [ -z "$alive" ]; then
      done=1
    fi

    i=$((i+1))

    if [ "$done" -ne 0 ]; then
        echo "killing polycubed in $i seconds"
        break
    fi
  done
}

# Relaunch polycubed, if deamon is not running
function polycubed_relaunch_if_not_running {
  alive=$(ps -el | grep polycubed)
  if [ -z "$alive" ]; then
    echo "polycubed not running ..."
    echo "relaunching polycubed ..."
    $polycubed >> $test_tmp 2>&1 &
  fi
}

# Launch polycubed, and wait until it becomes responsive
function launch_and_wait_polycubed_is_responding {
  if $RELAUNCH_POLYCUBED; then
    echo "starting polycubed ..."
    $polycubed >> $test_tmp 2>&1 &
  else
    polycubed_alive=$(ps -el | grep polycubed)
    if [ -z "$polycubed_alive" ]; then
      echo "polycubed not running ..."
      echo "relaunching polycubed ..."
      $polycubed >> $test_tmp 2>&1 &
    fi
  fi

  done=0
  i=0
  while : ; do
    sleep 1
    responding=$(polycubed_is_responding)
    if [[ $responding -eq 0 ]]; then
      done=1
    else
      polycubed_relaunch_if_not_running
    fi
    i=$((i+1))
    if [ "$done" -ne 0 ]; then
      if $RELAUNCH_POLYCUBED; then
        echo "starting polycubed in $i seconds"
      else
        if [ -z "$polycubed_alive" ]; then
          echo "relaunching polycubed in $i seconds"
        fi
      fi
        break
    fi
  done
}

# run test ($1:path)
function run_test {
  polycubed_crash=false
  STARTTIME=$(date +%s)
  echo
  echo "Starting test $1 ..."
  echo "++++Starting test $1++++" > $test_tmp
  launch_and_wait_polycubed_is_responding
  echo "executing test ..."
  log_test $1
  if $RELAUNCH_POLYCUBED; then
   polycubed_kill_and_wait
  fi
  echo "Finished test $1"
  ENDTIME=$(date +%s)
  echo "Time elapsed: $(($ENDTIME - $STARTTIME))s"
  echo "Time elapsed: $(($ENDTIME - $STARTTIME))s" >> $test_results
  echo "Time elapsed: $(($ENDTIME - $STARTTIME))s" >> $test_tmp
  echo "++++Finished test $1++++" >> $test_tmp
}

function run_tests_from_dir {
  for test in $1local_test*.sh; do
    if [[ $test == *"local_test*.sh"* ]]; then
      echo ""
    else
      run_test $test
    fi
  done
}

# Main script here

echo "*scanning current dir for tests*"
run_tests_from_dir ./../src/services/pcn-iptables/test/

cat $test_log
