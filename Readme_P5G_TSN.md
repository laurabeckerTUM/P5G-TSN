# P5G-TSN: A Private 5G TSN Simulation Framework

This README describes the extensions made to the 5GTQ framework to realize the P5G-TSN framework as presented in:

> **P5G-TSN: A Private 5G TSN Simulation Framework** [1]

The code corresponding to this publication is available under the tag:

P5G-TSN_WueWoWas
---

## 1. Context and Baseline

This implementation builds upon:

- INET Framework [2]  
- Simu5G [3]  
- 5GTQ [4]  

The goal of P5G-TSN is to enable **end-to-end evaluation of TSN traffic over private 5G networks**, focusing on:

- TDD-aware scheduling  
- integration of TSN traffic characteristics  
- realistic channel modeling  
- uplink signaling behavior  

---

## 2. Installation of P5G-TSN
The framework is devoloped and tested only on Linux and MAC OS with the OMNeT++ version 6.0.3. 

### Installing 5GTQ
To ensure a clean build process, the 5GTQ framework is used as a base for P5G-TSN. For the installation follow the Readme of 5GTQ (https://github.com/inet-framework/5GTQ). 5GTQ depends on the frameworks Simu5G and INET. To avoid building conflicts the given versions in the 5GTQ frameworks should be used. 

After successful installation, your workspace directory should contain the following structure:
````
samples/ 
├── 📂 inet/ 
├── 📂 Simu5G/ 
├── 📂 tsnfivegcomm/
````

### Integrating P5G-TSN
- The P5G-TSN implementation modifies only the Simu5G part of the framework.
- Replace the necessary files in the respective Simu5G directories.
- Rebuild the project.

### Running an Example Simulation
- Copy the examples of this repository into the workspace
- To prevent path conflicts, store the examples in the directory tsnfivegcomm.simulations.p5g_tsn

---

## 2. Overview of Contributions

The main extensions introduced in P5G-TSN are:

- Support for **configurable TDD patterns**
- Integration of **Scheduling Request (SR) signaling in uplink**
- Implementation of an **optimal channel model**

---

## 3. TDD Pattern Integration

### Description

A central contribution of P5G-TSN is the introduction of **TDD-aware transmission and scheduling**.  
This includes:

- definition of UL/DL frame structure  
- configuration of arbitrary TDD patterns  
- enforcement of slot-based transmission constraints  

The scheduler and MAC layer are extended to ensure that:

- control signaling is only sent in valid slots  
- data transmissions respect the TDD pattern  
- scheduling decisions are aligned with slot availability  

---

### Implementation

#### TDD Pattern Definition and Configuration

The structure and configuration of TDD patterns are implemented in:

- `Simu5G/src/common/LteCommon.h`
- `Simu5G/src/common/LteCommonEnum.msg`
- `Simu5G/src/common/binder/Binder.*`
- `Simu5G/src/common/componentcarrier/ComponentCarrier.*`
- `Simu5G/src/common/cell/CellInfo.*`
- `Simu5G/src/stack/phy/ChannelModel/LteChannelModel.cc`

These components define the TDD pattern and propagate it throughout the system.

---

#### TDD-Aware MAC Scheduling

The MAC layer is extended to enforce TDD constraints during scheduling:

- `Simu5G/src/stack/mac/LteMacEnb*`
- `Simu5G/src/stack/mac/NRMacGnb.*`
- `Simu5G/src/stack/mac/LteMacBase.ned`

This includes:

- restricting communication of control and data signals to valid slots  
- performing TDD-aware grant scheduling  

---

#### UE-Side Slot Handling

The UE side is extended to follow the assigned slot structure:

- `Simu5G/src/stack/mac/LteMacUe.*`
- `Simu5G/src/stack/mac/NRMacUe.*`

This includes:

- receiving grants  
- executing transmissions in the assigned slot  

---

## 4. Scheduling Request (SR) Integration

### Description

P5G-TSN introduces explicit support for **Scheduling Requests (SR)** in uplink scheduling.

This includes:

- definition of SR as a control message  
- transmission of SR in valid uplink slots  
- processing of SR at the base station  
- granting minimum resources after SR  

---

### Implementation

#### SR Definition and Packet Type

The SR is introduced as a dedicated control message and packet type:

- `Simu5G/src/stack/mac/packet/LteSchedulingRequest.msg`
- `Simu5G/src/common/LteCommon.h`
- `Simu5G/src/common/LteCommonEnum.msg`

---

#### SR Handling in MAC Layer

The handling of SR messages is implemented in:

- `Simu5G/src/stack/mac/LteMacBase.*`
- `Simu5G/src/stack/mac/LteMacUe.*`
- `Simu5G/src/stack/mac/NRMacUe.*`
- `Simu5G/src/stack/mac/LteMacEnb.*`
- `Simu5G/src/stack/mac/NRMacGnb.*`

This includes:

- reception and processing of SR  
- triggering scheduling decisions based on SR reception
- SR is sent in valid UL slots  
---


#### Minimum Resource Allocation after SR

After a SR, the gNB grants a minimum amount of resource blocks which is configured and granted in:

- `Simu5G/src/stack/mac/LteMacBase.ned`
- `Simu5G/src/stack/mac/scheduler/LteSchedulerEnbUl.cc`

---

#### PHY Support for SR

Support for SR at the PHY layer is implemented in:

- `Simu5G/src/stack/phy/LtePhyBase.cc`
- `Simu5G/src/stack/phy/LtePhyEnb.*`
- `Simu5G/src/stack/phy/LtePhyUe.*`
- `Simu5G/src/stack/phy/NRPhyUe.cc`

This enables SR to be used as a valid sending and receiving frame type.

---

## 5. Optimal Channel Model

### Description

To enable performance evaluation under idealized conditions, P5G-TSN introduces an **optimal channel model**.

This model allows:

- evaluation under perfect channel conditions  
- a controlled baseline for TSN scheduling  

---

### Implementation

#### Channel Model Integration

The optimal channel model is added as a configurable option:

- `Simu5G/src/stack/phy/ChannelModel/LteChannelModel.h`
- `Simu5G/src/stack/phy/ChannelModel/LteChannelModel.ned`

---

#### Channel Model Implementation

The implementation of the optimal channel model is provided in:

- `Simu5G/src/stack/phy/ChannelModel/LteRealisticChannelModel.*`
- `Simu5G/src/stack/phy/ChannelModel/NRChannelModel_3GPP38_901.*`

---

## 6. Simulation Scenarios
The configuration of the simulation scenarios which are introduced in Table II in [1] are located in:
- `Simulation_Exmpamples/single_stream/`

## 7. References

[1] L. Becker and W. Kellerer, “P5G-TSN: A Private 5G TSN Simulation Framework,” WueWoWAS, 2024  

[2] INET Framework — https://inet.omnetpp.org/  

[3] G. Nardini et al., “Simu5G–An OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks,” IEEE Access, 2020  

[4] R. Debnath et al., “5GTQ: QoS-Aware 5G-TSN Simulation Framework,” IEEE VTC 2023  