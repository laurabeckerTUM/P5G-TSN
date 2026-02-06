#!/usr/bin/env bash

# --- Environment setup ---
OMNETPP_ROOT=<path_to_omnetpp-6.1>
TSN5G_ROOT=$OMNETPP_ROOT/P5G-TSN/tsnfivegcomm
TSN5G_SRC=$TSN5G_ROOT/src
SIMU5G_ROOT=$OMNETPP_ROOT/P5G-TSN/Simu5G
SIMU5G_SRC=$SIMU5G_ROOT/src
INET_ROOT=$OMNETPP_ROOT/P5G-TSN/inet4.5
INET_SRC=$INET_ROOT/src

export PATH="$TSN5G_SRC:$SIMU5G_SRC:$INET_ROOT/out/clang-release/src:$OMNETPP_ROOT/bin:$PATH"

# --- Simulation parameters ---
SCENARIOS=()
SCENARIOS+=("Preallocation_adaptiveMCS")
SCENARIOS+=("Priority")
SCENARIOS+=("Preallocation_CG5")
SCENARIOS+=("Preallocation_CG11")
SCENARIOS+=("Preallocation_CG20")

REPEATS=10
PARALLEL=3
SIM_FOLDER="."

# --- Function to run one simulation instance ---
run_sim() {
    SCEN=$1
    RUN=$2
    LOG_FILE="logs/${SCEN}_run${RUN}.log"
    mkdir -p logs
    echo "Starting scenario $SCEN, run $RUN..."
    opp_run -m -u Cmdenv -c $SCEN -n ..:../../src:../../../inet4.5/examples:../../../inet4.5/showcases:../../../inet4.5/src:../../../inet4.5/tests/validation:../../../inet4.5/tests/networks:../../../inet4.5/tutorials:../../../Simu5G/emulation:../../../Simu5G/simulations:../../../Simu5G/src -x "inet.applications.voipstream;inet.common.selfdoc;inet.emulation;inet.examples.emulation;inet.examples.voipstream;inet.linklayer.configurator.gatescheduling.z3;inet.showcases.emulation;inet.showcases.visualizer.osg;inet.transportlayer.tcp_lwip;inet.visualizer.osg;simu5g.simulations.LTE.cars;simu5g.simulations.NR.cars;simu5g.nodes.cars" --image-path=../../../inet4.5/images:../../../Simu5G/images -l ../../src/tsnfivegcomm -l ../../../inet4.5/src/INET  \
    -l ../../../Simu5G/src/simu5g periodic_scenario.ini \
    -r $RUN > "$LOG_FILE" 2>&1
    echo "Scenario $SCEN, run $RUN finished. Log: $LOG_FILE"
}

# --- Launch runs ---
for SCEN in "${SCENARIOS[@]}"; do
    echo "=== Starting scenario: $SCEN ==="
    COUNTER=0
    for RUN in $(seq 0 $((REPEATS-1))); do
        run_sim $SCEN $RUN &
        ((COUNTER++))
        if (( COUNTER % PARALLEL == 0 )); then
            wait
        fi
    done
    wait
    echo "=== Finished scenario: $SCEN ==="
done

echo "All scenarios and runs completed."
