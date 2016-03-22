#!/bin/bash

rmmod kernetix
make clean
make
insmod kernetix.ko
chmod 777 /dev/KernetixDriver0

echo "All done..."