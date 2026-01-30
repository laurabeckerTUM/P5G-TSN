
/*
 * DQos.cc
 *
 *  Created on: Apr 23, 2023
 *      Author: devika
 */


#include "stack/mac/scheduling_modules/DQos.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"

using namespace omnetpp;

namespace simu5g {
void DQos::notifyActiveConnection(MacCid cid) {
    activeConnectionSet_->insert(cid);
}

void DQos::removeActiveConnection(MacCid cid) {
    activeConnectionSet_->erase(cid);
}

void DQos::prepareSchedule(bool static_scheduling) {
    EV << "DQos::prepareSchedule()" << endl;

    activeConnectionTempSet_ = *activeConnectionSet_;

    // Step 1: Retrieve QoS-based sorted connections
    std::map<double, std::vector<QosInfo>> sortedCids = mac_->getQosHandler()->getEqualPriorityMap(direction_);

    // Step 2: Build activeCids map for connections with QoS profile
    std::map<double, std::vector<QosInfo>> activeCids;
    for (auto &var : sortedCids) {
        for (auto &qosinfo : var.second) {
            for (auto &cid : activeConnectionTempSet_) {
                MacCid realCidQosInfo;
                if (direction_ == DL) {
                    realCidQosInfo = idToMacCid(qosinfo.destNodeId, qosinfo.lcid);
                } else {
                    realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid);
                }
                if (realCidQosInfo == cid) {
                    activeCids[var.first].push_back(qosinfo);
                }
            }
        }
    }

    // Step 3: Track which CIDs were scheduled (QoS-based)
    std::set<MacCid> scheduledQoSCids;

    // Step 4: Schedule QoS-based UEs
    for (auto &var : activeCids) {
        for (auto &qosinfos : var.second) {
            MacCid macCid = (direction_ == DL)
                ? idToMacCid(qosinfos.destNodeId, qosinfos.lcid)
                : idToMacCid(qosinfos.senderNodeId, qosinfos.lcid);

            if (tryScheduleCid(macCid) == -1){
                activeConnectionTempSet_.erase(macCid);
                continue;
            }

            scheduledQoSCids.insert(macCid);
        }
    }

    // Step 5: Identify UEs without QoS profile (residual scheduling)
    std::vector<MacCid> nonProfileCids;
    for (auto &cid : activeConnectionTempSet_) {
        if (scheduledQoSCids.find(cid) == scheduledQoSCids.end()) {
            nonProfileCids.push_back(cid);
        }
    }

    // Step 6: Schedule non-profile UEs with residual resources
    for (auto &macCid : nonProfileCids) {
        EV << "DQoS::prepareSchedule, schedule UEs without a profile" <<endl; 
        tryScheduleCid(macCid);
    }
}

// Helper function to avoid duplicating scheduling logic
int DQos::tryScheduleCid(const MacCid &macCid) {
    MacNodeId nodeId = MacCidToNodeId(macCid);
    OmnetId id = binder_->getOmnetId(nodeId);

    if (nodeId == NODEID_NONE || id == 0) {
        return false;
    }

    const UserTxParams &info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency_);

    const std::set<Band> &bands = info.readBands();
    auto cqiVec = info.readCqiVector();
    const std::set<Remote> &antennas = info.readAntennaSet();

    unsigned int codeword = info.getLayers().size();
    bool cqiNull = false;
    for (unsigned int i = 0; i < codeword && i < cqiVec.size(); ++i) {
        if (cqiVec[i] == 0) cqiNull = true;
    }
    if (cqiNull || eNbScheduler_->allocatedCws(nodeId) >= codeword) {
        return 0;
    }

    // Compute resources (optional, as before)
    unsigned int availableBlocks = 0;
    unsigned int availableBytes = 0;
    for (auto &ant : antennas) {
        for (auto &band : bands) {
            unsigned int rbs = eNbScheduler_->readAvailableRbs(nodeId, ant, band);
            unsigned int bytesOnRbs = eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, band, rbs, direction_, carrierFrequency_);
            availableBlocks += rbs;
            availableBytes += bytesOnRbs;
        }
    }

    if (availableBlocks == 0)
        return 0;

    bool terminate = false, active = true, eligible = true;
    unsigned int granted = requestGrant(macCid, 4294967295U, terminate, active, eligible);

    if (granted > 0 )
        return granted;
    else if (!active || !eligible)
        return -1; 
    return 0;
}

void DQos::commitSchedule(){
    if (!activeConnectionSet_) {
        EV_WARN << "MixedScheduler: activeConnectionSet_ is null, skipping scheduling" << endl;
        return;
    }
    *activeConnectionSet_ = activeConnectionTempSet_;
}

}
