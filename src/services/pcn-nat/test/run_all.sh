#!/bin/bash

touch log.txt
touch results.txt

for f in $(find . -name 'test_*.sh')
    do
    if [[ $f != *"run_all.sh"* ]];then
        echo "executing test $f ..."

        bash "$f" -H >> log.txt
        result=$(echo $?)
        echo "test $f -> $result"
        echo "test $f -> $result" >> results.txt
    fi
done
