#!/bin/bash
echo "Starting FANET Data Collection..."
for nodes in 10 50 100 150 200
do
echo "Running with $nodes vehicles..."
./ns3 run "aco-test --nNodes=$nodes" > results_$nodes.txt 2>&1
done
echo "All experiments complete!"
