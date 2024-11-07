#!/bin/bash
# Simulate the support agent reading and processing requests

if [[ $# -eq 1 ]]; then
    pipe=$1
else
    pipe="/tmp/support_pipe"
fi

while read request; do
    #echo "Support Agent: request= $request"
    if [[ $request == "quit" ]]; then
        echo "Support Agent: quit"
        exit
    fi
    echo "Support Agent processing: $request"

    sleep $((1 + $RANDOM % 5))  # Simulate time spent handling the request
done < $pipe

echo "Support Agent: read request terminated"
