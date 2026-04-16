# Joint Resource Allocation to Transparently Integrate 5G TDD Uplink with Time-Aware TSN

This README describes the extensions made to **P5G-TSN** to reproduce the results of:

> **Joint Resource Allocation to Transparently Integrate 5G TDD Uplink with Time-Aware TSN** [1]

The source code, simulation scenarios, installation scripts and evaluation scripts to reproduce the results of [1] are tagged with:

"JointScheduler_NOMS_Artifact"

## 1. Reproducing Results 

To install the framework, download the installation script ***installation_p5g_tsn.sh***, place it in your home directory, and execute:

```bash
chmod +x installation_p5g_tsn.sh
./installation_p5g_tsn.sh
```
Afterwards, the framework P5G-TSN including all required extension is installed in 
- **~/omnetpp6.1/p5g-tsn**

To reproduce the simulation results of [1], bash scripts are provided for both simulation scenarios. These scripts can be used to run the simulation (run_sim.sh) and export the results (export_results.sh). After installation they are located in:
- **~/omnetpp-6.1/p5g-tsn/tsnfivegcomm/simulations/Periodic/**
- **~/omnetpp-6.1/p5g-tsn/tsnfivegcomm/simulations/Heterogenous/**

After exporting, the results are stored in CSV format in the directory: 
**~/omnetpp-6.1/p5g-tsn/post_processing/results**. 

To generate the plots shown in Figures 4 and 5, execute the following Python scripts:
````bash
cd ~/omnetpp-6.1/
source .venv/bin/activate
cd p5g-tsn/post_processing/
python3 periodic_scenario.py
python3 heterogenous_scenario.py
````

The generated plots can then be found in:
- **~/omnetpp-6.1/p5g-tsn/post_processing/plots**.
For convenience, the repository already includes the corresponding plots in this directory.
Running the Python evaluation scripts will overwrite these files.

## 2. Context and Baseline

This repository builds upon the following simulation frameworks:

- [INET Framework (v4.5.x)](https://github.com/inet-framework/inet/tree/v4.5.x) [2]  
- [Simu5G (branch: topic/5gtq, commit: `9dc0b11`)](https://github.com/inet-framework/Simu5G/tree/9dc0b11f7cb8887c6ec396e8fc0366c645a99219) [3]  
- [5GTQ (commit: `2b92893`)](https://github.com/inet-framework/5GTQ/blob/2b9289388e071d5d2e2a0a67b04ef66703b9fee7) [4]  

The framework was firstly extended to support the configuration of arbitrary TDD patterns and published in:

> **P5G-TSN: A Private 5G TSN Simulation Framework** [5]

Please check for further details the `Readme_P5G_TSN.md`.

---

To ensure clarity and reproducibility for [1], we distinguish between different features and provide, for each feature, the relevant code changes.

---

## 3. Implementation of Heterogeneous RAN Scheduler

**Main implementation commit:** `d785d48`

In Chapter IV of [1], a heterogeneous RAN scheduler is presented combining:

- **Grant-free pre-allocation** for periodic TSN flows 
- **Grant-based dynamic scheduling** for non-deterministic flows

---

### 3.1 Implementation of Grant-Free Schedulers

In [1], a grant-free (GF) scheduler is proposed pre-allocating resources based on the TSN schedules serving streams with different latency and jitter requirements.
The pre-allocation scheme is defined in Algorithm 1 and 2 of [1], calculating for each slot the resources to be reserved. This functionality is implemented in:

- `Simu5G/src/stack/mac/scheduling_modules/TSNScheduler.*`

As described in Chapter IV of [1], the scheduler supports two configurations:

- GF with adaptive MCS selection  
- GF with static MCS selection  

---

#### GF Adaptive MCS Scheduling (Proposed Approach)

The proposed scheduler in [1] is based on a periodic pre-allocation of resources configuring the MCS based on the channel quality. Hence, in case of channel degradation, the MCS of the grants is adapted, and all affected grants are updated and resent to the UEs. The update mechanism is implemented in:

- `Simu5G/src/stack/mac/LteMacEnb*`
- `Simu5G/src/stack/mac/NRMacGnb.*`

To avoid overprovisioning, the MCS selection is bounded by a minimum MCS value, which is implemented in:

- `Simu5G/src/stack/phy/amc/NRAmc.cc`

---

#### GF Static MCS Scheduling (Baseline)

GF scheduling based on a static MCS is used as a baseline in [1], implementing the Configured Grant (CG) mechanism standardized in [6]. In this case, the MCS is predefined and not adapted to channel conditions. Hence, the MCS selection process is adapted to consider a fixed pre-configured MCS for CG allocations:

- `Simu5G/src/stack/phy/amc/NRAmc.cc`

---

#### Retransmission Probability Calculation

Due to the modified MCS selection, adjustments are required for the retransmission probability calculation. Since Simu5G uses CQI-based BLER curves, this work explicitly transmits the selected MCS and maps it back to a CQI value at the receiver to reuse the existing configured BLER curves. This functionality is implemented in:

- `Simu5G/src/common/PhyPisaData.*`
- `Simu5G/src/stack/phy/amc/NRMcs.*`
- `Simu5G/src/stack/phy/ChannelModel/LteRealisticChannelModel.cc`
- `Simu5G/src/stack/mac/packet/LteSchedulingGrant.msg`
- `Simu5G/src/stack/mac/packet/Airframe.msg`

---

### 3.2 Time Sensitive Communication Assistance Information (TSCAI)

**Implemented in commit:** `d785d48`

As described in Chapter III.B of [1], the 5G resource scheduler receives stream information in the form of TSCAI including stream information such as periodicity, burst size and burst arrival time. The TSCAI handling is implemented in:

- `Simu5G/src/common/GlobalData.*`

---

### 3.3 Combining Grant-Free and Grant-Based Scheduling

The proposed scheduler combines the pre-allocation mechanism (GF allocations) with state-of-the-art dynamic schedulers such as DRR, PF, Max/CI, or priority-based scheduling. This integration is implemented in:

- `Simu5G/src/stack/mac/scheduling_modules/MixedScheduler.*`

As disucssed in IV.C [1], resources for sporadic time-sensitive flows cannot be pre-allocated. To still meet their time-sensitive requirements, the Maximum Data Burst Volume (MDBV) field of the 5QI profile is used. The RAN scheduler uses this information to allocate resources already after receiving a Scheduling Request (SR), instead of waiting for a Buffer Status Report (BSR). This behavior is implemented in:

- `Simu5G/src/stack/mac/scheduler/LteSchedulerEnbUl.cc`

---

## 4. Additional Features

---
### 4.1 Indoor Factory Channel Model

As specified in Table III of [1], the simulations use an Indoor Factory Channel Model. Since Simu5G does not natively support this model, it is implemented in:

- `Simu5G/src/stack/phy/ChannelModel/NRChannelModel_3GPP38_901.*`

---

### 4.2 Priority-Based Dynamic Scheduling (UL Extension)

In the simulation scenarios described in Table III of [1], the scheduler ***Dyn. PDB** published in [4] is used as a baseline. Since [4] only considers downlink scheduling, several extensions were required for uplink support.

- `Simu5G/src/stack/mac/scheduler/DQoS.*`
- `Simu5G/src/stack/upperLayer/pdcp/LtePdcpRrc.cc`
- `Simu5G/src/stack/upperLayer/pdcp/NRPdcpRrcEnb.cc`

Additionally, for Delay Critical GBR (DC-GBR) flows, packets exceeding their Packet Delay Budget (PDB) shall not be retransmitted, requiring adjustments in the uplink processing.
- `Simu5G/src/stack/mac/buffer/UMTxEntity.cc`

---

### 4.3 TSN-Specific 5QI Profiles

As described in Table III of [1], TSN-specific 5QI profiles are used according to [7]. These profiles are defined in:

- `Simu5G/src/common/binder/Binder.ned`

---

## 5. Simulation Scenarios

The configuration of the simulation scenarios presented in Table III of [1] are provided in:

**Commits:**
- `6b80af3`
- `bc01975`

**Directories:**
- `tsnfivegcomm/simulations/Heterogenous/`
- `tsnfivegcomm/simulations/Periodic/`


## 6. Complete Implementation Overview

Due to the structure of Simu5G, implementing the proposed scheduler required additional modifications across multiple layers of the stack.  
This includes not only the scheduler itself but also supporting changes for:

- parameter propagation (e.g., MCS, TSCAI)
- control signaling (grants, airframes)
- QoS handling (5QI, MDBV)
- PHY and channel modeling
- HARQ and retransmission behavior

For completeness and reproducibility, a detailed mapping of all modified components is provided below.

## Detailed Implementation Changes (Complete Traceability)

The following table summarizes all modifications required to integrate the heterogeneous scheduler into Simu5G.  
It includes both core changes and supporting adaptations across the stack.

| Component | Files | Feature | Description |
|----------|------|--------|------------|
| Scheduler Core | `Simu5G/src/stack/mac/scheduling_modules/TSNScheduler.*` | E2E Scheduling | Implementation of pre-allocation (Algorithm 1 & 2) |
| Scheduler Integration | `Simu5G/src/stack/mac/scheduling_modules/MixedScheduler.*` | E2E Scheduling | Scheduler combining GF and Dyn Scheduling |
| Scheduler Pipeline | `Simu5G/src/stack/mac/scheduler/LteScheduler.*`, `LteSchedulerEnb.*` | E2E Scheduling | Modified scheduling workflow (static → SR → RTX → dynamic) |
| UL Scheduling | `Simu5G/src/stack/mac/scheduler/LteSchedulerEnbUl.cc` | QoS Mapping | MDBV-based scheduling for sporadic time-sensitive traffic |
| MAC (gNB/eNB) | `Simu5G/src/stack/mac/NRMacGnb.*`, `LteMacEnb.*` | E2E Scheduling | Sending and updating pre-allocated grants |
| MAC (UE) | `Simu5G/src/stack/mac/NRMacUe.*` | E2E Scheduling | Receiving, updating, and using CGs |
| MAC Base | `Simu5G/src/stack/mac/LteMacBase.*` | E2E Scheduling | Added configured grant structures |
| Control Signaling | `Simu5G/src/stack/mac/packet/LteSchedulingGrant.msg` | MCS Fix | Added CG and MCS information to grants |
| Air Interface | `Simu5G/src/stack/mac/packet/Airframe.msg` | MCS Fix | Added MCS to transmitted packets |
| QoS Mapping | `Simu5G/src/common/binder/Binder.ned` | TSN QoS | Added TSN-specific 5QI profiles |
| QoS Handling | `Simu5G/src/common/GlobalData.*` | TSCAI | Added TSCAI + MDBV information |
| QoS Propagation | `Simu5G/src/stack/upperLayer/pdcp/LtePdcpRrc.cc`, `NRPdcpRrcEnb.cc` | TSN QoS | Mapping TSN traffic classes to 5G QoS |
| Dynamic Scheduler Fix | `Simu5G/src/stack/mac/scheduler/DQoS.*`, `QosHandler.*` | E2E Scheduling | UL bug fixes for priority scheduler |
| MCS Handling | `Simu5G/src/stack/phy/amc/NRAmc.*` | MCS Fix | Static MCS + limited MCS selection |
| MCS Mapping | `Simu5G/src/stack/phy/amc/NRMcs.*` | MCS Fix | Mapping MCS → CQI for BLER curve reuse |
| PHY Data | `Simu5G/src/common/PhyPisaData.*` | MCS Fix | Mapping MCS → CQI for BLER curve reuse |
| Channel Model | `Simu5G/src/stack/phy/ChannelModel/LteRealisticChannelModel.*` | MCS Fix | RTX probability based on MCS |
| Channel Model | `Simu5G/src/stack/phy/ChannelModel/NRChannelModel_3GPP38_901.*` | Channel Model | Indoor factory model implementation |
| Channel Model Config | `Simu5G/src/stack/phy/ChannelModel/LteChannelModel.*` | Channel Model | Added indoor factory as available channel model |
| HARQ RX | `Simu5G/src/stack/mac/harq/LteHarqBufferRx.*` | Others | Forward packets independent of ACK process |
| HARQ RX | `Simu5G/src/stack/mac/harq/LteHarqProcessRx.cc` | E2E Scheduling | Disable retransmissions for frames using GF resources |
| HARQ TX | `Simu5G/src/stack/mac/harq/LteHarqUnitTx.*` | Others | Statistics adjustments |
| RLC | `Simu5G/src/stack/mac/buffer/UmTxEntity.*`, `UmRxEntity.*` | QoS | Drop DC-GBR packets exceeding PDB |
| Allocation | `Simu5G/src/stack/mac/scheduler/LteAllocationModule.*` | E2E Scheduling | Prevent re-sending unchanged pre-allocated resources |
| Common | `Simu5G/src/common/LteCommon.*` | E2E Scheduling | Added new scheduler types as available schedulers |
| Control Info | `Simu5G/src/common/LteControlInfo.*` | MCS Fix | Added MCS as header field |
| Network Mapping | `Simu5G/src/networklayer/Ip2Nic.cc` | Others | Mapping TSN device IP to UE |
| Misc | `Simu5G/src/common/NrCommon.h` | Others | Getter for resource type |
| PHY | `Simu5G/src/stack/phy/LtePhy*`, `NRPhyUe.cc` | Others | Added statistics |

## 7. References

[1] L. Becker, Y. Deshpande, and W. Kellerer, “Joint Resource Allocation to Transparently Integrate 5G TDD Uplink with Time-Aware TSN,” 2025. 

[2] “INET Framework,” https://inet.omnetpp.org/, accessed: 2025-10-23.

[3] G. Nardini, D. Sabella, G. Stea, P. Thakkar and A. Virdis, "Simu5G–An OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks," in IEEE Access, vol. 8, pp. 181176-181191, 2020, doi: 10.1109/ACCESS.2020.3028550.

[4] R. Debnath, M. S. Akinci, D. Ajith and S. Steinhorst, "5GTQ: QoS-Aware 5G-TSN Simulation Framework," 2023 IEEE 98th Vehicular Technology Conference (VTC2023-Fall), Hong Kong, Hong Kong, 2023, pp. 1-7, doi: 10.1109/VTC2023-Fall60731.2023.10333533. 

[5] L. Becker and W. Kellerer, “P5G-TSN: A Private 5G TSN Simulation Framework,” KuVS Fachgespräch - Würzburg Workshop on Modeling, Analysis and Simulation of Next-Generation Communication Networks (WueWoWAS), TUM School of Computation, Information and Technology, 2024.

[6] 3rd Generation Partnership Project (3GPP), “5G; NR; Medium Access Control (MAC) protocol specification (3GPP TS 38.321 version 18.2.0 Release 18),” 3GPP, Tech. Rep., 05 2024.

[7] Ambrosy, Niklas and Underberg, Lisa, “Traffic priority mapping for a joint 5G-TSN QoS model,” in Conference: Kommunikation in der
Automation: Beitr¨age des Jahreskolloquiums KommA, 2022