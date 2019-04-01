#!/bin/sh
#usage sh run.sh <sim_time> <traffic_level>
xterm -e "./gateway 3542 $1 $2" & # & detaches to different terminal
BACK_PID=$! #Get most recent PID detach
echo $BACK_PID
sleep 0.1
for i in 0 1 2 3 4 5
do
    xterm -e "./client 3542 $i $1 $2" &
    sleep 0.1
done

#Wait for Simulation to complete before plotting
wait $BACK_PID

python3 plotter.py
