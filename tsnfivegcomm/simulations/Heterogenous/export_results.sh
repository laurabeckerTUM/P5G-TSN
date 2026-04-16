#!/bin/bash

SCENARIOS=()
SCENARIOS+=("Priority")
SCENARIOS+=("PF")
SCENARIOS+=("MAXCI")
SCENARIOS+=("DRR")
SCENARIOS+=("Priority_MDBV")
SCENARIOS+=("PF_MDBV")
SCENARIOS+=("MAXCI_MDBV")
SCENARIOS+=("DRR_MDBV")

cd ~/omnetpp-6.1/
source setenv

cd ~/omnetpp-6.1/p5g-tsn/tsnfivegcomm/simulations/Heterogenous/

INPUT_DIR="./results"
OUTPUT_DIR=~/omnetpp-6.1/p5g-tsn/post_processing/results/heterogenous

mkdir -p "$OUTPUT_DIR"

FILTER='module =~"NetworkHeterogenous*.Listener.app[*]" AND (name =~ "endToEndDelay:vector" OR name =~ "throughput:vector" OR name =~ "packetReceived:vector(packetBytes)" ) OR (module =~ "NetworkHeterogenous*.Talker*.app[*]" AND name =~ "packetSent:vector(packetBytes)") OR name=~"avgServedBlocksUl:vector"' 

for SCEN in "${SCENARIOS[@]}"; do
    echo "Processing scenario: $SCEN"

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
