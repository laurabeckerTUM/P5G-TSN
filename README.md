# P5G-TSN Simulation Framework
This repository contains P5G-TSN, an OMNeT++-based simulation framework for modeling private 5G networks integrated with Time-Sensitive Networking (TSN). The framework is designed to evaluate end-to-end scheduling and resource allocation mechanisms across 5G and TSN domains. P5G-TSN was originally introduced in [1] and extends the 5GTQ framework [2], which itself is based on INET [3] and Simu5G [4].

## Repository Versions and Paper Mapping
This repository contains multiple tagged versions corresponding to different publications:
- P5G-TSN_WueWoWas includes the framework itslef, which was presented in [1]
- Joint_Scheduler_NOMS_Artifact (current tag) extends P5G-TSN with a joint resource allocation and scheduling mechanism to support the simulation scenarios presented in [5]

To reproduce the results of a specific paper, check out the corresponding Readme to get further instructions.

To cite the framework P5G-TSN itself, please use the following BibTex code: 
````bibtex
@inproceedings{p5g-tsn,
	title        = {{P5G-TSN: A Private 5G TSN Simulation Framework}},
	author       = {Becker, Laura and Kellerer, Wolfgang},
	year         = 2024,
	booktitle    = {KuVS Fachgespräch - Würzburg Workshop on Modeling, Analysis and Simulation of Next-Generation Communication Networks (WueWoWAS)},
	institution  = {TUM School of Computation, Information and Technology }
}
````

To cite the joint scheduler explicitly, please cite the following:
````bibtex
@misc{becker2025JointResourceAllocation,
      title={Joint Resource Allocation to Transparently Integrate 5G TDD Uplink with Time-Aware TSN}, 
      author={Laura Becker and Yash Deshpande and Wolfgang Kellerer},
      year={2025}
}
````

##  References

[1] L. Becker and W. Kellerer, “P5G-TSN: A Private 5G TSN Simulation Framework,” WueWoWAS, 2024  

[2] INET Framework — https://inet.omnetpp.org/  

[3] G. Nardini et al., “Simu5G–An OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks,” IEEE Access, 2020  

[4] R. Debnath et al., “5GTQ: QoS-Aware 5G-TSN Simulation Framework,” IEEE VTC 2023  

[5] L. Becker, Y. Deshpande, and W. Kellerer, “Joint Resource Allocation to Transparently Integrate 5G TDD Uplink with Time-Aware TSN,” 2025. 