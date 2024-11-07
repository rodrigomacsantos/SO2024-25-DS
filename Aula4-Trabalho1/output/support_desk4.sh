#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Utilização: $ support_desk <nº students> [nome pipe] [-d (debug)]"
    exit
fi

if [[ $# -ge 2 ]]; then
    pipe=$2
else
    pipe="/tmp/support_pipe"
fi
if [[ $# -ge 3 ]] && [[ $3 == "-d" ]]; then
    debug="TRUE"
else
    debug="FALSE"
fi

if [[ $debug = "TRUE" ]]; then
    echo "debug=$debug"
    echo "pipe=$pipe"
fi

# Create a named pipe (FIFO)
#echo "Create pipe"
#mkdir tmp

if [[ ! -p $pipe ]]; then
    if [[ $debug = "TRUE" ]]; then
    echo "Create pipe=$pipe"
    fi
    mkfifo $pipe
fi

# Simulate 3 students submitting requests
for ((i=1; i<=$1; i++))
do
    if [[ $debug = "TRUE" ]]; then
        echo "Student $i"
    fi
    ./output/main "Request from Student $i" $pipe &
done

# Simulate support agent handling the requests
./support_agent.sh $pipe &

if [[ $debug = "TRUE" ]]; then
        echo "sleep"
fi
sleep 1

echo "quit" > $pipe

# Wait for the processes to finish
if [[ $debug = "TRUE" ]]; then
        echo "wait"
fi
wait

# Clean up
rm $pipe
