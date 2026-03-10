#!/bin/bash

# Array of node counts to test (Matching the paper's scalability tests)
nodes=(10 50 100 150 200)

echo "Starting EACO-DE FANET Experiments..."
echo "-------------------------------------"

for n in "${nodes[@]}"
do
    echo "Running simulation for $n nodes..."
    # Run the new code and save only the final results block to a specific file
    ./ns3 run "aco-test --nNodes=$n" 2>&1 | tee full_log_$n.txt | grep -A 10 "ACO FANET PERFORMANCE RESULTS" > nresults_$n.txt
done

echo "Experiments complete. Results saved to results_XX.txt files."
