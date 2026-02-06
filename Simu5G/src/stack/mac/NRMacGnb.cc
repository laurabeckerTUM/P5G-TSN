//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
#include "stack/mac/NRMacGnb.h"
#include "stack/mac/scheduler/NRSchedulerGnbUl.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/amc/NRMcs.h"
#include "stack/mac/amc/NRAmc.h"
#include "stack/mac/LteMacEnbD2D.h"
#include "stack/mac/LteMacUeD2D.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/amc/AmcPilotD2D.h"
#include "stack/mac/conflict_graph/DistanceBasedConflictGraph.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"


namespace simu5g {

Define_Module(NRMacGnb);

NRMacGnb::NRMacGnb() : LteMacEnbD2D()
{
    nodeType_ = GNODEB;
}


void NRMacGnb::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LINK_LAYER)
    {
        qosHandler = check_and_cast<QosHandlerGNB*>(getParentModule()->getSubmodule("qosHandlerGnb"));
        /* Create and initialize NR MAC Uplink scheduler */
        if (enbSchedulerUl_ == nullptr)
        {
            enbSchedulerUl_ = new NRSchedulerGnbUl();
            enbSchedulerUl_->resourceBlocks() = cellInfo_->getNumBands();
            enbSchedulerUl_->initialize(UL, this, binder_);
        }
        PDCCH_slot_ = par("PDCCH_slot").intValue();
    }
    LteMacEnbD2D::initialize(stage);
}

void NRMacGnb::handleMessage(cMessage *msg) {

    //std::cout << "NRMacGnbRealistic::handleMessage start at " << simTime().dbl() << std::endl;

    if (strcmp(msg->getName(), "RRC") == 0) {
        cGate *incoming = msg->getArrivalGate();
        if (incoming == upInGate_) {
            //from pdcp to mac
            send(msg, gate("lowerLayer$o"));
        } else if (incoming == downInGate_) {
            //from mac to pdcp
            send(msg, gate("upperLayer$o"));
        }
    }

    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "flushHarqMsg") == 0) {
            flushHarqBuffers();
            delete msg;
            return;
        }
        else if (strcmp(msg->getName(), "ttiTick_") == 0 && cellInfo_->tddUsed() == true){
            if (slot_nr == ((cellInfo_->getTddPattern().Periodicity) - 1)){
                slot_nr = 0;
                tdd_cycle_cntr++;
                hp_cycle_cntr++;
            }
            else{
                slot_nr++;
            }
            if (hp_cycle_cntr * binder_->getTddPattern().Periodicity * binder_->getSlotDurationFromNumerologyIndex(binder_->getTddPattern().numerologyIndex) * 1000 >= tscaiHP)
                hp_cycle_cntr = 0;
        }
    }


    LteMacBase::handleMessage(msg);

    //std::cout << "NRMacGnbRealistic::handleMessage end at " << simTime().dbl() << std::endl;
}

void NRMacGnb::sendGrants(std::map<double, LteMacScheduleList> *scheduleList, int slot_offset)
{
    EV << NOW << "NRMacGnb::sendGrants " << endl;

    // reset hp cntr in first tdd cycle and first to be scheduled slot
    if (hp_cycle_cntr == 0 && slot_offset == 0) {
        EV << "reset CGs check marks if new HP has started" << endl;
        for (auto &uePair : historyCGsHP) {
            for (auto &cg : uePair.second) {
                cg.scheduledThisRound = false;
            }
        }
    }

    for (auto& [carrierFreq, carrierScheduleList] : *scheduleList) {
        while (!carrierScheduleList.empty()) {
            LteMacScheduleList::iterator it, ot;
            it = carrierScheduleList.begin();

            Codeword cw = it->first.second;
            Codeword otherCw = MAX_CODEWORDS - cw;
            MacCid cid = it->first.first;
            LogicalCid lcid = MacCidToLcid(cid);
            MacNodeId nodeId = MacCidToNodeId(cid);
            unsigned int granted = it->second;
            unsigned int codewords = 0;

            // removing visited element from scheduleList.
            carrierScheduleList.erase(it);

            if (granted > 0) {
                // increment number of allocated Cw
                ++codewords;
            }
            else {
                // active cw becomes the "other one"
                cw = otherCw;
            }

            std::pair<MacCid, Codeword> otherPair(num(nodeId), otherCw);  // note: MacNodeId used as MacCid

            if ((ot = (carrierScheduleList.find(otherPair))) != (carrierScheduleList.end())) {
                // increment number of allocated Cw
                ++codewords;

                // removing visited element from scheduleList.
                carrierScheduleList.erase(ot);
            }

            if (granted == 0)
                continue; // avoiding transmission of 0 grant (0 grant should not be created)

            EV << NOW << " LteMacEnbD2D::sendGrants Node[" << getMacNodeId() << "] - "
               << granted << " blocks to grant for user " << nodeId << " on "
               << codewords << " codewords. CW[" << cw << "\\" << otherCw << "] carrier[" << carrierFreq << "]" << endl;

            // get the direction of the grant, depending on which connection has been scheduled by the eNB
            Direction dir = (lcid == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : ((lcid == D2D_SHORT_BSR) ? D2D : UL);

            // TODO Grant is set aperiodic as default
            // TODO: change to tag instead of header
            auto pkt = new Packet("LteGrant");
            auto grant = makeShared<LteSchedulingGrant>();
            grant->setDirection(dir);
            grant->setCodewords(codewords);

            // set total granted blocks
            grant->setTotalGrantedBlocks(granted);
            
            // set slot offset
            grant->setSlotOffset(slot_offset);
            grant->setActive(true);
            grant->setChunkLength(b(1));

            pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
            pkt->addTagIfAbsent<UserControlInfo>()->setDestId(nodeId);
            pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(GRANTPKT);
            pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

            const UserTxParams& ui = getAmc()->computeTxParams(nodeId, dir, carrierFreq);
            UserTxParams *txPara = new UserTxParams(ui);
            // FIXME: possible memory leak
            grant->setUserTxParams(txPara);

            // acquiring remote antennas set from user info
            const std::set<Remote>& antennas = ui.readAntennaSet();

            // get bands for this carrier
            const unsigned int firstBand = cellInfo_->getCarrierStartingBand(carrierFreq);
            const unsigned int lastBand = cellInfo_->getCarrierLastBand(carrierFreq);

            //  HANDLE MULTICW
            for ( ; cw < codewords; ++cw) {
                unsigned int grantedBytes = 0;

                for (Band b = firstBand; b <= lastBand; ++b) {
                    unsigned int bandAllocatedBlocks = 0;
                    for (const auto& antenna : antennas) {
                        bandAllocatedBlocks += enbSchedulerUl_->readPerUeAllocatedBlocks(nodeId, antenna, b);
                    }
                    auto iter = tcsaiMappingTable.find(nodeId);
                    if (iter != tcsaiMappingTable.end()){
                        grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw, bandAllocatedBlocks, dir, carrierFreq, minMcsIndex, staticMcsIndex);
                    }
                    else{
                        grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw, bandAllocatedBlocks, dir, carrierFreq);
                    }                    
                }
                NRMCSelem mcsElem = amc_->getMcsElemPerCqi(txPara->readCqiVector().at(cw), dir, minMcsIndex, staticMcsIndex);
                EV << "set mcs index for grant "<< mcsElem.index_<< endl;
                if (staticMcsIndex != -1)
                    grant->setMcsIndex(mcsElem.index_);

                grant->setGrantedCwBytes(cw, grantedBytes);
                EV << NOW << " LteMacEnbD2D::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
            }
            RbMap map;

            enbSchedulerUl_->readRbOccupation(nodeId, carrierFreq, map);

            grant->setGrantedBlocks(map);
            bool foundMatch = false;

            // check whether updated Grant is required
            double tdd_duration = binder_->getTddPattern().Periodicity *  binder_->getSlotDurationFromNumerologyIndex(binder_->getTddPattern().numerologyIndex) * 1000;
            auto iter = tcsaiMappingTable.find(nodeId);
            if (iter != tcsaiMappingTable.end()){
                grant->setPeriodic(true);
                grant->setActive(true);
                grant->setPeriod(tscaiHP);
                grant->setHPCycle(hp_cycle_cntr);
                grant->setHPCycleMax(tscaiHP/tdd_duration);

                auto &prevCgs = historyCGsHP[nodeId];

                for (auto &itOld : prevCgs) {
                    if (itOld.HPCycle == hp_cycle_cntr && itOld.slotOffset == slot_offset) {
                        if (compareCGs(itOld.grant, grant)) {
                            // Case 1: identical grant → mark scheduled
                            EV << "Mark as used " << endl;
                            itOld.scheduledThisRound = true;
                            foundMatch = true; // we will drop sending
                        }
                        // else: content changed → do nothing here
                        break;
                    }
                }

                if (!foundMatch) {
                    // Case 2: new or updated grant → add to list (will be sent)
                    auto grantCopy = inet::IntrusivePtr<LteSchedulingGrant>(
                        check_and_cast<LteSchedulingGrant *>(grant->dup())
                    );
                    prevCgs.push_back(ConfiguredGrant{slot_offset, tscaiHP, hp_cycle_cntr, grantCopy, true, true, carrierFreq});
                }
            }
            /*
             * @author Alessandro Noferi
             * Notify the packet flow manager about the successful arrival of a TB from a UE.
             * From ETSI TS 138314 V16.0.0 (2020-07)
             *   tSched: the point in time when the UL MAC SDU i is scheduled as
             *   per the scheduling grant provided
             */
            if (packetFlowManager_ != nullptr)
                packetFlowManager_->grantSent(nodeId, grant->getGrantId());

            // send grant to PHY layer
            if (!foundMatch) {
               pkt->insertAtFront(grant);
               sendLowerPackets(pkt);
            } else {
               delete pkt; // drop packet for still-valid CG
            }
        }
    }

    double slot_duration = binder_->getSlotDurationFromNumerologyIndex(binder_->getTddPattern().numerologyIndex) * 1000; // assumes ms
    double tdd_duration = binder_->getTddPattern().Periodicity;

    EV << "tdd_cycle_cntr " << tdd_cycle_cntr << " cg prep " << (cg_preparation_time + tscaiHP/(slot_duration*tdd_duration)) << endl;
    if (slot_offset == binder_->getTddPattern().numUlSlots && (staticMcsIndex == -1 ||
        (staticMcsIndex != -1 && tdd_cycle_cntr <= cg_preparation_time + tscaiHP/(slot_duration*tdd_duration)))) {
        EV << "check for outdated grants" << endl;
        for (auto &uePair : historyCGsHP) {
            auto &prevCgs = uePair.second;
            for (auto it = prevCgs.begin(); it != prevCgs.end();) {
                // Only consider CGs that belong to this TDD cycle
                if (it->HPCycle == hp_cycle_cntr && !it->scheduledThisRound && it->active) {
                    auto pkt = new Packet("LteGrant");
                    auto inactiveGrant = makeShared<LteSchedulingGrant>(*it->grant);
                    inactiveGrant->setActive(false);
                    pkt->insertAtFront(inactiveGrant);
                    pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    pkt->addTagIfAbsent<UserControlInfo>()->setDestId(uePair.first);
                    pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(GRANTPKT);
                    pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(it->carrierFrequency);
                    sendLowerPackets(pkt);

                    it->active = false;
                    it = prevCgs.erase(it); 

                    emit(macCGRescheduledSignal_, 1);

                } else {
                    ++it;
                }
            }
        }
    }
}


} //namespace

