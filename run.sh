#!/bin/bash

# Array of parameters
params=("2" "4" "8" "10" "12" "16" "32")

# Loop through parameters and run script
for param in "${params[@]}"; do
    ./vec_add "$param"
done