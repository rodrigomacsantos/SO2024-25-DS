#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Utilização: $ support_desk <nº students>"
    exit
fi

# Create a named pipe (FIFO)
#echo "Create pipe"
#mkdir tmp

if [[ ! -p /tmp/support_pipe ]]; then
    mkfifo /tmp/support_pipe
fi

# Simulate 3 students submitting requests
for ((i=1; i<=$1; i++))
do
    #echo "Student $i"
    ./output/main "Request from Student $i" &
done

# Simulate support agent handling the requests
./support_agent.sh &

#echo "sleep"
sleep 1

echo "quit" > /tmp/support_pipe

# Wait for the processes to finish
#echo "wait"
wait

# Clean up
rm /tmp/support_pipe
