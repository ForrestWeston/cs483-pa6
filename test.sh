#!/bin/bash
echo "1 Proc"
./run 1 1024 .2
./run 1 2048 .2
./run 1 4096 .2

echo "2 Proc"
./run 2 1024 .2
./run 2 2048 .2
./run 2 4096 .2

echo "4 Proc"
./run 4 1024 .2
./run 4 2048 .2
./run 4 4096 .2
