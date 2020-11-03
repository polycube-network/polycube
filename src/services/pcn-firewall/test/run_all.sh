#!/bin/bash

echo "" > log.txt
echo "" > results.txt

sudo polycubed &>/dev/null  2>&1 &
sleep 10
for f in $(find . -name '*.sh')
    do
    if [[ $f != *"run_all.sh"* ]];then
        echo "executing test $f ..."

        bash "$f" -H >> log.txt
        result=$(echo $?)
        echo "test $f -> $result"
        echo "test $f -> $result" >> results.txt
    fi
done

sudo pkill -SIGTERM polycubed
