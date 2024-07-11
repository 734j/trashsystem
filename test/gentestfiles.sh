#!/bin/sh

touch ts_file1.txt
touch ts_file2.txt
touch ts_file3.txt
touch thisfile.txt
 
dd if=/dev/zero of=./1gib.img bs=1GiB count=1 # 1024 exactly, should show 1GiB on tsr -l
dd if=/dev/zero of=./1000mb.img bs=1000MB count=1 # 1000Mib should show 1000MiB on tsr -l
dd if=/dev/zero of=./1.1gib.img bs=1100MiB count=1 # 1100MiB should show 1.1 Gib on tsr -l

dd if=/dev/zero of=./1mb.img bs=1MB count=1 # MB =/= MiB
dd if=/dev/zero of=./1mib.img bs=1MiB count=1 # 1 Mib
dd if=/dev/zero of=./500kib.img bs=500KiB count=1 # 500 Kib
dd if=/dev/zero of=./1023kib.img bs=1023KiB count=1 # 1023 Kib
dd if=/dev/zero of=./1024kib.img bs=1024KiB count=1 # 1 Mib

dd if=/dev/zero of=./1000b.img bs=1000B count=1
dd if=/dev/zero of=./1KiB.img bs=1KiB count=1
dd if=/dev/zero of=./1023b.img bs=1023B count=1
dd if=/dev/zero of=./1024b.img bs=1024B count=1
dd if=/dev/zero of=./1KB.img bs=1KB count=1 # KB =/= KiB
