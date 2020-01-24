#!/bin/bash

echo "loading $1 into FPGA..."

CMD_PATH=/tmp/impact.cmd

echo "generating ${CMD_PATH}..."

echo "setmode -bscan" > ${CMD_PATH}
echo "setcable -p auto" >> ${CMD_PATH}
echo "addDevice -p 1 -file $1" >> ${CMD_PATH}
echo "program -p 1" >> ${CMD_PATH}
echo "quit" >> ${CMD_PATH}

impact -batch ${CMD_PATH}

echo "done!"
