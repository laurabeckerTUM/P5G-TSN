#!/bin/bash

SCENARIOS=()
SCENARIOS+=("Preallocation_adaptiveMCS")
SCENARIOS+=("Priority")
SCENARIOS+=("Preallocation_CG5")
SCENARIOS+=("Preallocation_CG11")
SCENARIOS+=("Preallocation_CG20")

cd ~/omnetpp-6.1/
source setenv

cd ~/omnetpp-6.1/p5g-tsn/tsnfivegcomm/simulations/Periodic/

INPUT_DIR="./results"
OUTPUT_DIR=~/omnetpp-6.1/p5g-tsn/post_processing/results/periodic

mkdir -p "$OUTPUT_DIR"

FILTER='module =~"NetworkPeriodic*.Listener.app[*]" AND (name =~ "endToEndDelay:vector" OR name =~ "packetReceived:vector(packetBytes)" ) OR (module =~ "NetworkPeriodic*.Talker*.app[*]" AND name =~ "packetSent:vector(packetBytes)")  OR name=~"avgServedBlocksUl:vector"' 

for SCEN in "${SCENARIOS[@]}"; do
    echo "Processing scenario: $SCEN"

    # Loop over all runs of the scenario
    for VECFILE in "$INPUT_DIR"/${SCEN}-*.vec; do
        if [[ -f "$VECFILE" ]]; then
            BASENAME=$(basename "$VECFILE" .vec)
            OUTFILE="$OUTPUT_DIR/${BASENAME}.csv"

            echo "  Exporting $VECFILE -> $OUTFILE"
            opp_scavetool export -f "$FILTER" -F CSV-S -o "$OUTFILE" "$VECFILE"
        fi
    done
done

echo "All exports finished. CSV files are in $OUTPUT_DIR"
