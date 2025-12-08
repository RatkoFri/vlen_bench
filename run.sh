#!/bin/bash

# Array of parameters
params=("4" "8" "16" "32" "64" "128" "256" "512" "1024")

# Loop through parameters and run script
for param in "${params[@]}"; do
    ./vec_add "$param"
done