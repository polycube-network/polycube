#!/bin/bash

echo "" > log.txt
echo "" > results.txt

sleep 10
for f in $(find . -name '*.sh')
    do
    if [[ $f != *"run_all.sh"* ]];then
        echo "executing test $f ..."
        sudo polycubed &>/dev/null  2>&1 &
        bash "$f" -H >> log.txt
        result=$(echo $?)
        echo "test $f -> $result"
        echo "test $f -> $result" >> results.txt
        sudo pkill -SIGTERM polycubed
    fi
done
