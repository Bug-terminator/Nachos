#!/bin/sh
cd /vagrant/nachos/nachos-3.4/code/filesys

echo "Generate the large file for double indirect indexing"
dd if=/dev/urandom of=largeFile count=123 bs=1024 # 123KB

# use -Q to disable verbose machine messages
echo "=== format the DISK ==="
./nachos -Q -f
echo "=== copies file \"largeFile\" from UNIX to Nachos ==="
./nachos -Q -cp largeFile largeFile
echo "=== prints the contents of the entire file system ==="
./nachos -Q -D

echo "=== remove the file \"largeFile\" from Nachos ==="
./nachos -Q -r largeFile
echo "=== prints the contents of the entire file system again ==="
./nachos -Q -D