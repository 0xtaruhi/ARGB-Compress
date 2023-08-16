#!/bin/bash

for file in `ls res`; do
    echo $file
    ./check.sh res/$file > gen/$file.stat 2>&1 &
done
