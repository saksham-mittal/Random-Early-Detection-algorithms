# !/bin/sh
# usage sh run.sh <sim_time> <traffic_level>
# Compiling gateway.cpp file
g++ gateway.cpp -o gateway -std=c++11 -lpthread

# Compiling client.cpp file
g++ client.cpp -o client -std=c++11 -lpthread

# & detaches to different terminal
xterm -e "./gateway 3542 $1 $2" &

# Get most recent PID detach
BACK_PID=$!

sleep 0.1
for i in 1 2 3 4 5 6
do
    xterm -e "./client 3542 $i $1 $2" &
done

# Wait for Simulation to complete before plotting
wait $BACK_PID

# Plotting the results in a graph 
python3 plotter.py
