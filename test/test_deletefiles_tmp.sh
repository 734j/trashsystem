#!/bin/sh

files="1000b.img 1000mb.img 1023b.img 1023kib.img 1024b.img 1024kib.img 1.1gib.img 1gib.img 1KB.img 1KiB.img 1mb.img 1mib.img 500kib.img thisfile.txt ts_file1.txt ts_file2.txt ts_file3.txt"
options="-v -t"

./tsr-TESTING $options $files
