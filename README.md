# Readme

## P5G-TSN: A Private 5G TSN Simulation Framework
The OMNeT++ P5G-TSN framework simulates 5G networks integrated with TSN to analyze end-to-end scheduling algorithms considering the characteristics of a private 5G network. This framework extends 5GTQ [1] focussing on the implementation of configurable TDD patterns and the resource allocation procedure w.r.t. to these TDD patterns. 

[1] 5GTQ: QoS-Aware 5G-TSN Simulation Framework. IEEE 98th Vehicular Technology Conference: VTC2023-Fall. Hong Kong. 2023. p. 7.


To cite the P5G-TSN framework, please use the following BibTex code: 

````bibtex
@inproceedings{p5g-tsn,
	title        = {{P5G-TSN: A Private 5G TSN Simulation Framework}},
	author       = {Becker, Laura and Kellerer, Wolfgang},
	year         = 2024,
	booktitle    = {KuVS Fachgespräch - Würzburg Workshop on Modeling, Analysis and Simulation of Next-Generation Communication Networks (WueWoWAS)},
	institution  = {TUM School of Computation, Information and Technology }
}
````

## Installation of P5G-TSN
The framework is devoloped and tested only on Linux and MAC OS with the OMNeT++ version 6.0.3. 

### 1. Installation of 5GTQ
To ensure a clean build process, the 5GTQ framework is used as a base for P5G-TSN. For the installation follow the Readme of 5GTQ (https://github.com/inet-framework/5GTQ). 5GTQ depends on the frameworks Simu5G and INET. To avoid building conflicts the given versions in the 5GTQ frameworks should be used. 

After successful installation, your workspace directory should contain the following structure:
````
samples/ 
├── 📂 inet/ 
├── 📂 Simu5G/ 
├── 📂 tsnfivegcomm/
````

### 2. Integrating P5G-TSN
- The P5G-TSN implementation modifies only the Simu5G part of the framework.
- Replace the necessary files in the respective Simu5G directories.
- Rebuild the project.

### 3. Running an Example Simulation
- Copy the examples of this repository into the workspace
- To prevent path conflicts, store the examples in the directory tsnfivegcomm.simulations.p5g_tsn