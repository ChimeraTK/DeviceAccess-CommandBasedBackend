#!/usr/bin/env bash

cp ./devices.dmap.template ./devices.dmap

# Set hostname for TCP dummy
sed -i "/COMMAND_BASED_DUMMY_TCP/s/<hostname>/$HOSTNAME/g" ./devices.dmap

# TODO Configure Serial dummy
