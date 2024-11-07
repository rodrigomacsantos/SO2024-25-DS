#!/bin/bash

# Create a named pipe (FIFO)
mkfifo support_pipe

# Simulate 3 students submitting requests

./student.sh "Request from Student 1" > support_pipe &
./student.sh "Request from Student 2" > support_pipe &
./student.sh "Request from Student 3" > support_pipe &

# Simulate support agent handling the requests
./support_agent.sh < support_pipe &

# Wait for the processes to finish
wait

# Clean up
rm support_pipe
