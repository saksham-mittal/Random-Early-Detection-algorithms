# !/bin/sh
# usage sh run.sh <algo-type> <sim_time> <traffic_level>

cd build/$1
cmake ../../src/$1
make
cd ../..

# & detaches to different terminal
for servIndex in 1 2 3 4
do
    xterm -e "bin/$1/./server $servIndex" &
    sleep 0.1
done

sleep 1

# Get most recent PID detach
BACK_PID=$!

for gatewayIndex in 3 2 1
do
    xterm -e "bin/$1/./gateway $gatewayIndex $2 $3" &
    sleep 0.1
done

sleep 1

for clientIndex in 1 2 3 4 5 6 7 8 9 10 11 12
do
    xterm -e "bin/$1/./client $clientIndex $2 $3" &
done

# Wait for Simulation to complete before plotting
wait $BACK_PID

# Plotting the results in a graph

if [ "$1" = "RED" ]; then
    for plotterIndex in 1 2 3
    do
        python3 src/$1/plotter.py $plotterIndex
    done
else
    python3 src/$1/plotter.py
fi
