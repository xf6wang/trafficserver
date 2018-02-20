#!/bin/bash
for ((i=0; i<1000; i++)); do 
    sleep 5
    /usr/local/bin/traffic_ctl plugin msg TAG $(($(date +%s%N)/1000000))
done
