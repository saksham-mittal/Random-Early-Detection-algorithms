# !/bin/sh
# usage sh run.sh <sim_time> <traffic_level>

# Compiling gateway.cpp file
g++ src/gateway.cpp -o bin/gateway -std=c++11 -lpthread

# Compiling server.cpp file
g++ src/server.cpp -o bin/server -std=c++11 -lpthread

# Compiling client.cpp file
g++ src/client.cpp -o bin/client -std=c++11 -lpthread

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