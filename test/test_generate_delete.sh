#!/bin/bash

for i in {1..10}
do
  ./gentestfiles.sh
  ./test_deletefiles.sh
done
