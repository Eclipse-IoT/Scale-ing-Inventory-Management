#!/bin/sh

echo "starting Mosquitto"
mosquitto -d
echo "starting node server"
nodemon .