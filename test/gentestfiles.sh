#!/bin/sh

touch ts_file1.txt
touch ts_file2.txt
touch ts_file3.txt
touch thisfile.txt
dd if=/dev/zero of=./1gb.img bs=100M count=10
