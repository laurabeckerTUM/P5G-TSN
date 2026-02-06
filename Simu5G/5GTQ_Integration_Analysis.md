# Analysis of the Latest Commit in Simu5G
The latest commit (c616507a3d48c4f198d21c142cead7e926ae821d) integrates functionality from the 5GTQ (5G Time-sensitive networking Quality of service) project developed by the Technical University of Munich (TUM-ESI).

## Summary of Changes

This commit adds comprehensive Quality of Service (QoS) support to the Simu5G framework, implementing the 5G QoS model as defined in 3GPP specifications (particularly TS 23.501). The changes span 59 files with 2,660 insertions and 46 deletions, indicating a significant feature addition rather than a modification of existing functionality.

## Key Components Added:
1. QoS Class Identifier (5QI) Framework:

Implementation of the 5G QoS model with standardized QoS characteristics
Support for different resource types (GBR, NGBR, DCGBR)
QoS parameters including priority levels, packet delay budgets (PDB), and packet error rates (PER)

2. QoS Flow Identifier (QFI) Management:

Mapping between application types and QFIs
QFI to 5QI conversion for QoS enforcement

3. Time-Sensitive Networking (TSN) Integration:

PCP (Priority Code Point) checker for Ethernet traffic
TSN to 5G QoS translation mechanisms
XML-based configuration for QoS mapping

4. QoS-aware Scheduling:

New DQos scheduler implementation
Priority-based, delay-based, and weighted scheduling options
Support for different scheduling strategies based on QoS requirements
5.SDAP (Service Data Adaptation Protocol) Layer:

QoS handling in the SDAP layer
QoS checking and enforcement mechanisms
Purpose of the Changes:
The primary purpose of these changes is to enhance Simu5G with the ability to simulate time-sensitive networking (TSN) scenarios over 5G networks with proper QoS guarantees. This is particularly important for industrial applications, autonomous vehicles, and other use cases requiring deterministic communication with strict latency and reliability requirements.

The implementation allows:

Differentiated handling of traffic based on QoS requirements
Mapping between Ethernet PCP values and 5G QoS parameters
Prioritization of traffic based on standardized 5G QoS characteristics
Simulation of time-sensitive applications with strict delay requirements
Code Quality Assessment
Strengths:
Standards Compliance: The implementation closely follows 3GPP specifications (TS 23.501), using standardized QoS parameters and mechanisms.

Modularity: The QoS functionality is implemented in a modular way with clear separation of concerns:

QosHandler classes for different network elements (UE, gNB, UPF)
Separate modules for PCP checking, QoS mapping, and scheduling
Configurability: The implementation uses XML configuration files and NED parameters for QoS mapping, making it highly configurable without code changes.

Documentation: Most classes include comments explaining their purpose and the standards they implement.

Areas for Improvement:
Code Formatting: Some inconsistencies in formatting, particularly in parameter lists and function declarations (missing commas and spaces in some class definitions).

Error Handling: Some error conditions use throw statements directly rather than more graceful error handling mechanisms.

Hardcoded Values: Some QoS parameters are hardcoded in the NED files rather than being fully configurable through XML.

Limited Comments: While the code has some documentation, more detailed comments explaining the complex QoS mechanisms would improve maintainability.

Testing: The commit doesn't include comprehensive test cases for the new QoS functionality, which would be beneficial for such a complex feature.

Conclusion
This commit represents a significant enhancement to Simu5G, adding comprehensive QoS support for time-sensitive networking over 5G. The implementation follows 3GPP standards and provides a flexible framework for simulating various QoS scenarios. While there are some minor code quality issues, the overall architecture is sound and modular.

The integration of 5GTQ functionality enables researchers and developers to simulate and evaluate time-sensitive applications over 5G networks with realistic QoS constraints, which is particularly valuable for industrial IoT, autonomous vehicles, and other emerging 5G use cases requiring deterministic communication.







# Detailed Analysis of the 5GTQ Integration in Simu5G

The latest commit (c616507a3d48c4f198d21c142cead7e926ae821d) integrates
functionality from the 5GTQ (5G Time-sensitive networking Quality of service)
project developed by the Technical University of Munich (TUM-ESI). This analysis
provides detailed pointers to the implementation of each functionality and
identifies oversimplifications and potential issues in the code.

## 1. QoS Framework Implementation

### 1.1 QoS Class Identifier (5QI) Framework

**Implementation Location:**
- `src/common/NrCommon.h`: Defines the core QoS classes and structures (lines
  20-146)
  - `QosCharacteristic` class (lines 22-52): Implements the QoS characteristics
    as defined in 3GPP TS 23.501
  - `NRQosCharacteristics` class (lines 62-90): Singleton class that maintains a
    mapping of 5QI values to QoS characteristics
  - `NRQosParameters` class (lines 92-125): Encapsulates QoS parameters
    including bitrates, ARP, and packet loss rates
  - `NRQosProfile` class (lines 128-146): Wrapper for QoS parameters

- `src/common/binder/Binder.cc`: Initializes the QoS characteristics (lines
  89-143)
  - Loads QoS parameters from NED file parameters (lines 89-143)
  - Registers the QoS values in the `NRQosCharacteristics` singleton

**Oversimplifications/Issues:**
1. **Hardcoded Resource Types**: The resource types (GBR, NGBR, DCGBR) are
   defined as an enum without proper string conversion handling (line 17-20 in
   NrCommon.h), which could lead to parsing errors.

2. **Incomplete Parameter Validation**: While there are some ASSERT statements
   for parameter validation (lines 27-29 in NrCommon.h), many QoS parameters
   lack proper range checking.

3. **Memory Management Issues**: The `NRQosCharacteristics` singleton (lines
   62-90 in NrCommon.h) doesn't follow proper singleton pattern implementation
   and could lead to memory leaks.

4. **Incomplete XML Configuration**: The `readTsnFiveGTrafficXml()` method in
   Binder.cc (lines 143-163) contains commented-out code and doesn't actually
   parse the XML configuration, suggesting incomplete implementation.

### 1.2 QoS Flow Identifier (QFI) Management

**Implementation Location:**
- `src/stack/sdap/utils/QosHandler.h`: Manages QFI mappings and QoS enforcement
  (lines 29-490)
  - `QosInfo` struct (lines 30-49): Stores QoS information for connections
  - `getQfi()` methods (lines 280-323): Map application types to QFIs
  - `get5Qi()` method (lines 325-345): Maps QFIs to 5QI values

- `src/stack/sdap/utils/QosHandler.ned`: Defines default QFI and 5QI mappings
  (lines 27-78)
  - Default mappings for different traffic types (lines 27-50)
  - Radio bearer mappings (lines 52-60)

**Oversimplifications/Issues:**
1. **Hardcoded QFI Mappings**: The QFI to application type mappings are
   hardcoded in the `getQfi()` method (lines 280-323 in QosHandler.h) rather
   than being fully configurable.

2. **Inconsistent QFI Handling**: The `getFlowQfi()` method (lines 304-323) uses
   string matching for packet names, which is fragile and could lead to
   misclassification.

3. **Limited Traffic Types**: The QoS handler only supports a limited set of
   predefined traffic types (VOD, VOIP, CBR, NETWORK_CONTROL, GAMING) and
   doesn't provide a mechanism for custom traffic types.

4. **Conflicting QFI Values**: In QosHandler.ned, there are conflicting comments
   about 5QI values (line 30-31), suggesting uncertainty in the implementation.

## 2. Time-Sensitive Networking (TSN) Integration

**Implementation Location:**
- `src/corenetwork/trafficFlowFilter/pcpChecker.h` and `.cc`: Implements PCP
  (Priority Code Point) checking for Ethernet frames (lines 1-33)

- `src/corenetwork/trafficFlowFilter/tsnQosMapper.xml`: Maps PCP values to QoS
  parameters (lines 1-43)
  - Defines traffic types with different PCP values (lines 2-42)
  - Maps each PCP to packet size and delay requirements

- `src/corenetwork/gtp/TsnQoSMapper.xml`: Maps PCP values to 5G QoS parameters
  (lines 1-15)
  - Maps PCP 1 to 5QI 83 with 1ms delay requirement (lines 3-7)
  - Maps PCP 2 to 5QI 84 with 3ms delay requirement (lines 8-12)

- `src/stack/sdap/QosChecker.h`: Implements QoS checking for packets (lines
  1-81)
  - `QoSInfo` struct (lines 20-28): Stores QoS information
  - Hardcoded mappings between PCP and QFI values (lines 34-54)

**Oversimplifications/Issues:**
1. **Incomplete PCP Checker Implementation**: The `pcpChecker.h` file defines a
   class but the implementation in `.cc` is empty, suggesting incomplete
   functionality.

2. **Inconsistent XML Formats**: The two XML mapping files (`tsnQosMapper.xml`
   and `TsnQoSMapper.xml`) use different formats and tag names for similar
   concepts, which could lead to confusion.

3. **Hardcoded PCP-QFI Mappings**: The mappings in `QosChecker.h` (lines 34-54)
   are hardcoded rather than being loaded from configuration files.

4. **Limited PCP Values**: Only a subset of the possible PCP values (0-7) are
   properly mapped to 5QI values.

5. **Incomplete TSN-5G Translation**: The `TsnFiveGTranslator` files are
   mentioned but their implementation is minimal, suggesting incomplete TSN to
   5G translation functionality.

## 3. QoS-aware Scheduling

**Implementation Location:**
- `src/stack/mac/scheduling_modules/DQos.h` and `.cc`: Implements QoS-aware
  scheduling (lines 1-42 in .h, lines 1-399 in .cc)
  - Extends the `LteScheduler` class to implement QoS-aware scheduling
  - `prepareSchedule()` method (lines 34-326 in .cc): Implements the scheduling
    algorithm
  - Uses QoS information to prioritize connections

- `src/stack/sdap/utils/QosHandler.h`: Provides methods for QoS-based scheduling
  decisions (lines 150-250)
  - `getEqualPriorityMap()` method (lines 150-250): Implements different
    scheduling strategies based on QoS parameters
  - Supports priority-based, PDB-based, and weighted scheduling

**Oversimplifications/Issues:**
1. **Commented-Out Code**: The DQos.cc file contains a large block of
   commented-out code (lines 327-399), suggesting incomplete refactoring or
   testing.

2. **Inefficient Priority Handling**: The priority handling in
   `getEqualPriorityMap()` (lines 150-250 in QosHandler.h) uses string
   comparisons for scheduling strategy selection, which is inefficient.

3. **Limited Scheduling Strategies**: Only a few scheduling strategies are
   implemented (priority, pdb, weight, default), and there's no mechanism for
   custom strategies.

4. **Missing Error Handling**: The scheduling code lacks proper error handling
   for edge cases like no available resources or conflicting priorities.

5. **Incomplete Integration**: The DQos scheduler is not fully integrated with
   the existing scheduling framework, as evidenced by the commented-out code and
   duplicated functionality.

## 4. SDAP (Service Data Adaptation Protocol) Layer

**Implementation Location:**
- `src/stack/sdap/entity/NRSdapEntity.h` and `.cc`: Implements the SDAP entity
  (lines 1-53 in .h, lines 1-41 in .cc)
  - Basic SDAP entity with sequence number handling
  - Minimal functionality compared to the 3GPP specification

- `src/stack/sdap/packet/SdapPdu.msg`: Defines the SDAP PDU structure (lines
  1-22)
  - Empty packet definition, lacking the fields specified in 3GPP

- `src/stack/sdap/sdap.h` and `.cc`: Main SDAP module implementation
  - Connects the SDAP layer to other protocol layers

**Oversimplifications/Issues:**
1. **Minimal SDAP Implementation**: The SDAP entity implementation is extremely
   minimal, only handling sequence numbers without the full functionality
   specified in 3GPP TS 37.324.

2. **Empty PDU Definition**: The SdapPdu.msg file defines an empty packet
   without any of the required fields like QFI, RDI, or data.

3. **Missing QoS Mapping Functionality**: The SDAP layer should map QoS flows to
   radio bearers, but this functionality is not fully implemented.

4. **Incomplete Header Handling**: The SDAP header processing (adding/removing
   headers) is not implemented in the code.

## 5. Global Data Management

**Implementation Location:**
- `src/common/binder/GlobalData.h` and `.cc`: Manages global QoS data (lines
  1-59 in .h)
  - `InterfaceTable` struct (lines 21-29): Stores interface information
  - `QosConfiguration` struct (lines 31-35): Stores QoS configuration
  - Methods for reading XML configurations (lines 43-53)

- `src/common/binder/Binder.cc`: Extensions to the Binder class for QoS support
  - `registerUeConnectedEthernetDevices()` method (lines 12-25): Registers
    Ethernet devices connected to UEs
  - QoS parameter initialization (lines 89-143)

**Oversimplifications/Issues:**
1. **Incomplete XML Parsing**: The XML parsing functionality in
   `readQosXmlConfiguration()` and `readIpXmlConfig()` methods is referenced but
   not fully implemented.

2. **Hardcoded IP Addresses**: The IP address handling in
   `registerUeConnectedEthernetDevices()` uses string conversions without proper
   validation.

3. **Missing Thread Safety**: The global data structures are not thread-safe,
   which could lead to race conditions in multi-threaded simulations.

4. **Incomplete PCP-5QI Conversion**: The methods `convertPcpToFiveqi()` and
   `convertFiveqiToPcp()` are declared but their implementation is minimal.

## Conclusion

The 5GTQ integration adds significant QoS capabilities to Simu5G, but the
implementation has several issues and oversimplifications:

1. **Incomplete Implementation**: Many components (like the PCP checker and TSN
   translator) are only partially implemented, with empty methods or
   commented-out code.

2. **Hardcoded Values**: Many QoS parameters and mappings are hardcoded rather
   than being configurable, limiting flexibility.

3. **Limited Error Handling**: The code lacks comprehensive error handling for
   edge cases and invalid inputs.

4. **Minimal SDAP Implementation**: The SDAP layer implementation is very basic
   compared to the 3GPP specification.

5. **Inconsistent XML Configurations**: The XML configuration files use
   different formats and tag names for similar concepts.

6. **Code Quality Issues**: There are formatting inconsistencies, commented-out
   code blocks, and potential memory management issues.

Despite these issues, the integration provides a foundation for simulating QoS
in 5G networks, particularly for time-sensitive applications. With further
refinement and completion of the missing functionality, it could become a
valuable tool for researching QoS in 5G networks.
