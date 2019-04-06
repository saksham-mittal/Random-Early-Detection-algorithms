# !/bin/sh
# usage sh run.sh <sim_time> <traffic_level>

# & detaches to different terminal
for servIndex in 1 2 3 4
do
    xterm -e "bin/./server $servIndex" &
    sleep 0.1
done

sleep 1

# Get most recent PID detach
BACK_PID=$!

for gatewayIndex in 3 2 1
do
    xterm -e "bin/./gateway $gatewayIndex $1 $2" &
    sleep 0.1
done

sleep 1

for clientIndex in 1 2 3 4 5 6 7 8 9 10 11 12
do
    xterm -e "bin/./client $clientIndex $1 $2" &
done

# Wait for Simulation to complete before plotting
wait $BACK_PID

# Plotting the results in a graph 
for plotterIndex in 1 2 3
do
    python3 src/plotter.py $plotterIndex
done