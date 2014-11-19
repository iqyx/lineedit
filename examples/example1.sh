#!/bin/bash

stty -icanon -echo
./example1
stty icanon echo
echo

