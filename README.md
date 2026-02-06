# P5G-TSN Simulation Framework
This repository contains P5G-TSN, an OMNeT++-based simulation framework for modeling private 5G networks integrated with Time-Sensitive Networking (TSN). The framework is designed to evaluate end-to-end scheduling and resource allocation mechanisms across 5G and TSN domains. P5G-TSN was originally introduced in [1] and extends the 5GTQ framework [2], which itself is based on INET [3] and Simu5G [4].

## Repository Versions and Paper Mapping
This repository contains multiple tagged versions corresponding to different publications:
- P5G-TSN_WueWoWas includes the framework itslef, which was presented in [1]
- Joint_Scheduler_NOMS (current tag) extends P5G-TSN with a joint resource allocation and scheduling mechanism to support the simulation scenarios presented in [5]

To reproduce the results of a specific paper, check out the corresponding tag and follow the README included with that version.


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
      year={2025},
      eprint={2511.23373},
      archivePrefix={arXiv},
      primaryClass={cs.NI},
      url={https://arxiv.org/abs/2511.23373}, 
}
````


## Installation of P5G-TSN
P5G-TSN is devoloped and tested with the OMNeT++ version 6.1 which can be downloaded from https://omnetpp.org/download/old.html. 

Clone the repository:
````
git clone --recurse-submodules git@github.com:laurabeckerTUM/P5G-TSN.git
````
Afterwards, the project can be imported to the OMNeT++ IDE work space and build. 

The simulation scenarios used in [2] are located in: "tsnfivegcomm/simulations/". 

Each scenario includes:
- run_sim.sh (executes all simulation configurations)
- export_results.sh (extracts and processes simulation results)

Run these scripts to reproduce the figures and results presented in the paper.

[1] L. Becker and W. Kellerer, “P5G-TSN: A Private 5G TSN Simulation Framework,” KuVS Fachgespräch - Würzburg Workshop on Modeling, Analysis and Simulation of Next-Generation Communication Networks (WueWoWAS), TUM School of Computation, Information and Technology, 2024.

[2] R. Debnath, M. S. Akinci, D. Ajith and S. Steinhorst, "5GTQ: QoS-Aware 5G-TSN Simulation Framework," 2023 IEEE 98th Vehicular Technology Conference (VTC2023-Fall), Hong Kong, Hong Kong, 2023, pp. 1-7, doi: 10.1109/VTC2023-Fall60731.2023.10333533. 

[3] “INET Framework,” https://inet.omnetpp.org/, accessed: 2025-10-23.

[4] G. Nardini, D. Sabella, G. Stea, P. Thakkar and A. Virdis, "Simu5G–An OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks," in IEEE Access, vol. 8, pp. 181176-181191, 2020, doi: 10.1109/ACCESS.2020.3028550.

[5] L. Becker, Y. Deshpande, and W. Kellerer, “Joint Resource Allocation to Transparently Integrate 5G TDD Uplink with Time-Aware TSN,” arXiv preprint arXiv:2511.23373, 2025. [Online]. Available: https://arxiv.org/abs/2511.23373
