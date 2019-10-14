#!/usr/bin/env bash

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

# Check if services are loaded
function services_are_loaded {
  echo "load_services:" > load_services
  count=0
  services_show=$(polycubectl services show)
  for i in "${!services[@]}"
  do
    lines=$(echo $services_show | grep $i | wc -l)
    if [ $lines -ne 0 ]
    then
      echo "$i YES" >> load_services
    else
      count=$((count + 1))
      echo "$i NO" >> load_services
    fi
  done
  echo $count
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
  if [ -z "$KILL_COMMAND" ]
  then
      sudo pkill polycubed >> $test_tmp
  else
      `$KILL_COMMAND` >> $test_tmp
  fi

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

  done=0
  i=0
  while : ; do
    sleep 1
    loaded=$(services_are_loaded)
    if [[ $loaded -eq 0 ]]; then
      done=1
    fi

    i=$((i+1))
    if [ "$i" -eq $SERVICES_LOAD_TIMEOUT ]
    then
        echo "+ERROR+ timeout in checking services loaded $i seconds. try to run test anyway"
        cat load_services | grep NO
        break
    fi
    if [ "$done" -ne 0 ]; then
        echo "checking services loaded in $i seconds"
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

