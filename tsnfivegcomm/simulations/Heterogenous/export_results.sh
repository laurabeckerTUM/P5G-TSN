#!/bin/bash

# Scenarios to process
SCENARIOS=()
SCENARIOS+=("Priority")
SCENARIOS+=("PF")
SCENARIOS+=("MAXCI")
SCENARIOS+=("DRR")
SCENARIOS+=("Priority_MDBV")
SCENARIOS+=("PF_MDBV")
SCENARIOS+=("MAXCI_MDBV")
SCENARIOS+=("DRR_MDBV")

# Directories
INPUT_DIR="./results"
OUTPUT_DIR="./results/csv"

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

# Filter expression (same as you wrote)
FILTER='module =~"NetworkHeterogenous*.Listener.app[*]" AND (name =~ "endToEndDelay:vector" OR name =~ "throughput:vector" OR name =~ "packetReceived:vector(packetBytes)" ) OR (module =~ "NetworkHeterogenous*.Talker*.app[*]" AND name =~ "packetSent:vector(packetBytes)") OR name=~"macCGRescheduled:vector" OR name=~"avgServedBlocksUl:vector"' 

# Loop over scenarios
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
