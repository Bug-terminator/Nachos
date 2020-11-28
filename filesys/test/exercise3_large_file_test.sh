#!/bin/sh
cd /vagrant/nachos/nachos-3.4/code/filesys

# echo "Generate the large file for single indirect indexing"
# dd if=/dev/zero of=largeFile count=3 bs=1024 # 3KB

echo "Generate the large file for double indirect indexing"
dd if=/dev/urandom of=largeFile count=20 bs=1024 # 20KB

# echo "Using 100,000 Decimal Digits of PI as large file"
# cp test/PI.100.000.TXT largeFile # 112KB


echo "=== format the DISK ==="
./nachos -f
echo "=== copies file \"largeFile\" from UNIX to Nachos ==="
./nachos -cp largeFile largeFile
echo "=== prints the contents of the entire file system ==="
./nachos -D

echo "=== remove the file \"largeFile\" from Nachos ==="
./nachos -r largeFile
echo "=== prints the contents of the entire file system again ==="
./nachos -D