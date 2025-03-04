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

DQos::DQos(Direction dir) {

    LteScheduler();
    this->dir = dir;
}

void DQos::notifyActiveConnection(MacCid cid) {
    activeConnectionSet_->insert(cid);
}

void DQos::removeActiveConnection(MacCid cid) {
    activeConnectionSet_->erase(cid);
}
void DQos::prepareSchedule(){

    if (binder_ == nullptr)
        binder_ = getBinder();

    activeConnectionTempSet_ = *activeConnectionSet_;

    //find out which cid has highest priority via qosHandler
    std::map<double, std::vector<QosInfo>> sortedCids = mac_->getQosHandler()->getEqualPriorityMap(dir); // sort cids based on priority or pdb
    std::map<double, std::vector<QosInfo>>::reverse_iterator sortedCidsIter = sortedCids.rbegin();

    //create a map with cids which are in the racStatusMap
    std::map<double, std::vector<QosInfo>> activeCids;
    for (auto &var : sortedCids) {  // checks which or the sorted cids is in the rac status
        for (auto &qosinfo : var.second) {
            for (auto &cid : activeConnectionTempSet_) {
                MacCid realCidQosInfo;
                if(dir == DL){
                    realCidQosInfo = idToMacCid(qosinfo.destNodeId, qosinfo.lcid);
                }else{
                    realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid) - 1; // LCID in UL is increased by one
                }
                if (realCidQosInfo == cid) {
                    //we have the QosInfos for the cid in the racStatus
                    activeCids[var.first].push_back(qosinfo);
                }
            }
        }
    }
    for (auto &var : activeCids) {

        for (auto &qosinfos : var.second) {
            MacCid cid;
            if (dir == DL) {
                cid = idToMacCid(qosinfos.destNodeId, qosinfos.lcid);
            } else {
                cid = idToMacCid(qosinfos.senderNodeId, qosinfos.lcid) - 1;
            }
            MacNodeId nodeId = MacCidToNodeId(cid);
            OmnetId id = binder_->getOmnetId(nodeId);
            if(nodeId == 0 || id == 0){
                // node has left the simulation - erase corresponding CIDs
                activeConnectionSet_->erase(cid);
                activeConnectionTempSet_.erase(cid);
                //carrierActiveConnectionSet_.erase(cid);
                continue;
            }

            // if we are allocating the UL subframe, this connection may be either UL or D2D
            /* Direction dir;
            if (direction_ == UL)
                dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
            else
                dir = DL;*/

            // compute available blocks for the current user
            const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId,direction_,carrierFrequency_); // changed
            const std::set<Band>& bands = info.readBands();
            std::set<Band>::const_iterator it = bands.begin(),et=bands.end();
            unsigned int codeword=info.getLayers().size();
            bool cqiNull=false;
            for (unsigned int i=0;i<codeword;i++)
            {
                if (info.readCqiVector()[i]==0)
                cqiNull=true;
            }
            if (cqiNull)
                continue;
            //no more free cw
            if (eNbScheduler_->allocatedCws(nodeId)==codeword)
                continue;

            std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt=info.readAntennaSet().end();

            // compute score based on total available bytes
            unsigned int availableBlocks=0;
            unsigned int availableBytes =0;
            // for each antenna
            for (;antennaIt!=antennaEt;++antennaIt)
            {
                // for each logical band
                for (;it!=et;++it)
                {
                    availableBlocks += eNbScheduler_->readAvailableRbs(nodeId,*antennaIt,*it);
                    availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId,*it, availableBlocks, direction_,carrierFrequency_);
                }
            }

            if (direction_ == UL || direction_ == DL)  // D2D background traffic not supported (yet?)
            {
                // query the BgTrafficManager to get the list of backlogged bg UEs to be added to the scorelist. This work
                // is done by this module itself, so that backgroundTrafficManager is transparent to the scheduling policy in use

                BackgroundTrafficManager* bgTrafficManager = eNbScheduler_->mac_->getBackgroundTrafficManager(carrierFrequency_);
                std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(direction_),
                                                    et = bgTrafficManager->getBackloggedUesEnd(direction_);

                int bgUeIndex;
                int bytesPerBlock;
                MacNodeId bgUeId;
                MacCid bgCid;
                for (; it != et; ++it)
                {
                    bgUeIndex = *it;
                    bgUeId = BGUE_MIN_ID + bgUeIndex;

                    // the cid for a background UE is a 32bit integer composed as:
                    // - the most significant 16 bits are set to the background UE id (BGUE_MIN_ID+index)
                    // - the least significant 16 bits are set to 0 (lcid=0)
                    bgCid = bgUeId << 16;
                    bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);
                }
            }

            bool terminate = false;
            bool active = true;
            bool eligible = true;
            unsigned int granted;
            granted = requestGrant (cid, 4294967295U, terminate, active, eligible);
            if ( terminate ) break;

            // Pop the descriptor from the score list if the active or eligible flag are clear.
            if ( ! active || ! eligible )
            {
                activeConnectionTempSet_.erase(cid);
            }
        }
    }

}

void DQos::commitSchedule(){
    *activeConnectionSet_ = activeConnectionTempSet_;
}
