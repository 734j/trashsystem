#!/bin/bash

mkdir -p testdir/testdir2/testdir3/{td1,td2,td3,td4,td5}

cd testdir
../gentestfiles.sh
cd testdir2/testdir3/td1
../../../../gentestfiles.sh
cd ../td2
../../../../gentestfiles.sh
