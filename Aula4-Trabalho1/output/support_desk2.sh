#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Utilização: $ support_desk <nº students>"
    exit
fi

# Create a named pipe (FIFO)
echo "Create pipe"
#mkdir tmp
mkfifo /tmp/support_pipe

# Simulate 3 students submitting requests
for ((i=1; i<=$1; i++))
do
    echo "Req $i"
    ./student.sh "Request from Student $i" > /tmp/support_pipe &
done

# Simulate support agent handling the requests
./support_agent.sh < /tmp/support_pipe &

sleep 1
echo "quit" > /tmp/support_pipe

# Wait for the processes to finish
echo "wait"
wait

# Clean up
rm /tmp/support_pipe
