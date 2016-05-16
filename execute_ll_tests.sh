#!/bin/bash

echo "Running tests..."
./scripts/run_ll.sh

echo; echo"Data saved in /data folder"

echo "Generating plots..."
cd plots
gnuplot gp/ll_fixed2.gp
cd ..
echo "Plots generated in folder /plots/eps"
