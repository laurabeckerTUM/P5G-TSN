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

#include "stack/mac/NRMacUe.h"

#include <inet/common/TimeTag_m.h>

#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/packet/LteSchedulingRequest_m.h"


namespace simu5g {

Define_Module(NRMacUe);


void NRMacUe::initialize(int stage){
    LteMacUeD2D::initialize(stage);
    if (stage == inet::INITSTAGE_LAST){
        if (binder_->getTddPattern().tdd == true){
            slot_nr = 0;
        }
    }
}


SlotType NRMacUe::defineSlotType(){
    if (slot_nr < binder_->getTddPattern().numDlSlots){
        EV_INFO << "Current Slot: DL, slot number " << slot_nr << endl;
        return DL_SLOT;
    }
    else if (slot_nr < (binder_->getTddPattern().Periodicity - binder_->getTddPattern().numUlSlots)){
        EV_INFO << "Current Slot: Shared, slot number " << slot_nr << endl;
        return SHARED_SLOT;
    }
    else{
        EV_INFO << "Current Slot: UL, slot number " << slot_nr << endl;
        return UL_SLOT;
    }
}


void NRMacUe::applyGrant(inet::IntrusivePtr<const LteSchedulingGrant> grant,
                         double carrierFreq, int slotNr)
{
    schedulingGrantOffsetMap[slotNr][carrierFreq] = grant;
}


void NRMacUe::handleMessage(cMessage* msg)
{
    EV << "NRMacUe::handleMessage"<< slot_nr << binder_->getTddPattern().numDlSlots+1 << endl;
    if (strcmp(msg->getName(), "ttiTick_") == 0 && binder_->getTddPattern().tdd == true){
        // Dynamic Grants
        if (slot_nr == ((binder_->getTddPattern().Periodicity) - 1)){
            slot_nr = 0;
            bsr_already_sent = false;
            for (int offset = binder_->getTddPattern().numDlSlots; offset < binder_->getTddPattern().numDlSlots + binder_->getTddPattern().numUlSlots + 1; offset++){
                auto git = schedulingGrantOffsetMap[offset].begin();
                for ( ; git != schedulingGrantOffsetMap[offset].end(); ++git){
                    if (git->second != NULL){
                        git->second = NULL;
                        git->second = nullptr;
                    }
                }
            }
            hp_cycle_curr++;
            if (hp_cycle_curr == hp_cycle_max)
                hp_cycle_curr = 0;
            EV << "new tdd cycle " << hp_cycle_curr << " hp_cycle_max " << hp_cycle_max << endl;
        }
        else{
            slot_nr++;
            // todo: get grant with expired cnt
        }
        
        // ConfiguredGrant
        for (auto &carrierEntry : configuredGrantMap) {
            double carrierFreq = carrierEntry.first;
            for (auto &cg : carrierEntry.second) {
                if (!cg.active || cg.HPCycle != hp_cycle_curr) continue;   // skip released CGs
                if (slot_nr == cg.slotOffset) {
                    int relSlot = slot_nr - cg.slotOffset;
                    if (relSlot % cg.period == 0) {
                        // Activate CG → reuse its stored grant
                        EV << NOW << " UE[" << nodeId_
                           << "] applying Configured Grant, Carrier=" << carrierFreq
                           << " Slot=" << slot_nr << endl;

                        applyGrant(cg.grant, carrierFreq, slot_nr);
                    }
                }
            }
        }
    }
    LteMacUeD2D::handleMessage(msg);
}


void NRMacUe::handleSelfMessage()
{
    EV << "----- NR UE MAIN LOOP -----" << endl;
    cur_slot_type = FDD;
    if (binder_->getTddPattern().tdd == true){
        cur_slot_type = defineSlotType();
    }

    if (cur_slot_type == FDD)
        handleSelfMessageFdd();
    else if (cur_slot_type == DL_SLOT){ // tdd pattern but dl slot, adapt timers
        if (racBackoffTimer_>0)
            racBackoffTimer_--;
        if(raRespTimer_>0)
            raRespTimer_--;
        if (bsrRtxTimer_>0)
            bsrRtxTimer_--;
    }
    else
        handleSelfMessageTdd();
}

void NRMacUe::handleSelfMessageFdd()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract PDUs from all HARQ RX buffers and pass them to unmaker
    for (auto& mit : harqRxBuffers_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit.first)) > 0)
            continue;

        std::list<Packet *> pduList;
        for (auto [macNodeId, rxBuf] : mit.second) {
            pduList = rxBuf->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << "NRMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no HARQ counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    for (auto& git : schedulingGrant_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git.first)) > 0)
            continue;

        if (git.second != nullptr)
            noSchedulingGrants = false;
    }

    if (noSchedulingGrants) {
        EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;
        checkRAC();
        // TODO ensure all operations done before return (i.e. move H-ARQ RX purge before this point)
    }

    scheduleList_.clear();
    requestedSdus_ = 0;
    if (!noSchedulingGrants) { // if a grant is configured
        EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        if (!firstTx) {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx = true;
            // the gNb will receive the first PDU in 2 TTI, thus initializing acid to 0
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }

        std::map<double, HarqTxBuffers>::iterator mtit;
        for (auto& mtit : harqTxBuffers_) {
            double carrierFrequency = mtit.first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                continue;

            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == nullptr)
                continue;

            for (auto [it2Key, currHarq] : mtit.second) {
                unsigned int numProcesses = currHarq->getNumProcesses();

                for (unsigned int proc = 0; proc < numProcesses; proc++) {
                    LteHarqProcessTx *currProc = currHarq->getProcess(proc);

                    // check if the current process has unit ready for retransmission
                    bool ready = currProc->hasReadyUnits();
                    CwList cwListRetx = currProc->readyUnitsIds();

                    EV << "\t [process=" << proc << "] , [retx=" << (ready ? "true" : "false") << "] , [n=" << cwListRetx.size() << "]" << endl;

                    // check if one 'ready' unit has the same direction of the grant
                    bool checkDir = false;
                    for (Codeword cw : cwListRetx) {
                        auto info = currProc->getPdu(cw)->getTag<UserControlInfo>();
                        if (info->getDirection() == schedulingGrant_[carrierFrequency]->getDirection()) {
                            checkDir = true;
                            break;
                        }
                    }

                    // if a retransmission is needed
                    if (ready && checkDir) {
                        UnitList signal;
                        signal.first = proc;
                        signal.second = cwListRetx;
                        currHarq->markSelected(signal, schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                        retx = true;
                        break;
                    }
                }
            }
        }
        // if no retransmission is needed, proceed with normal scheduling
        if (!retx) {
            emptyScheduleList_ = true;
            std::map<double, LteSchedulerUeUl *>::iterator sit;
            for (auto [carrierFrequency, carrierLcgScheduler] : lcgScheduler_) {
                // skip if this is not the turn of this carrier
                if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                    continue;

                EV << "NRMacUe::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList *carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "NRMacUe::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && emptyScheduleList_) {
                // no connection scheduled, but we can use this grant to send a BSR to the eNB
                macPduMake();
            }
            else {
                requestedSdus_ = macSduRequestFdd(); // returns an integer
            }
        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);
    }

    //============================ DEBUG ==========================
    if (debugHarq_) {
        for (auto& mtit : harqTxBuffers_) {
            EV << "\n carrier[ " << mtit.first << "] htxbuf.size " << mtit.second.size() << endl;
            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

            int cntOuter = 0;
            int cntInner = 0;
            for (auto [currId, currHarq] : mtit.second) {
                BufferStatus harqStatus = currHarq->getBufferStatus();
                EV << "\t cycleOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for (const auto& jt : harqStatus) {
                    EV << "\t\t cycleInner " << cntInner << " - jt->size=" << jt.size()
                       << " - statusCw(0/1)=" << jt.at(0).second << "/" << jt.at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

    // update current HARQ process id, if needed
    if (requestedSdus_ == 0) {
        EV << NOW << " NRMacUe::handleSelfMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_ + 1) % harqProcesses_ << endl;
        currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
    }

    decreaseNumerologyPeriodCounter();

    EV << "--- END UE MAIN LOOP ---" << endl;
}

void NRMacUe::handleSelfMessageTdd()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract PDUs from all HARQ RX buffers and pass them to unmaker
    for (auto& mit : harqRxBuffers_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit.first)) > 0)
            continue;

        std::list<Packet *> pduList;
        for (auto [macNodeId, rxBuf] : mit.second) {
            pduList = rxBuf->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << "NRMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no HARQ counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    bool noGrantInCurrentSlot = true;
    for (int offset = binder_->getTddPattern().numDlSlots; offset < binder_->getTddPattern().numDlSlots + binder_->getTddPattern().numUlSlots + 1; offset++){
        auto git = schedulingGrantOffsetMap[offset].begin();
        for ( ; git != schedulingGrantOffsetMap[offset].end(); ++git){
            if (git->second != NULL){
                if (offset == slot_nr){
                    noSchedulingGrants = false;
                    noGrantInCurrentSlot = false;
                    EV << "SchedulingGrants in current slot offset " << offset << " slot_nr "<< slot_nr << endl;
                }
                else if (offset > slot_nr){ // || offset < slot_nr) { Send SR also if a grant was available in the same cycle
                    noSchedulingGrants = false;
                    EV << "SchedulingGrants in other slot" << offset << slot_nr << endl;
                }
            }
        }
    }

    if (noSchedulingGrants) {
        if (pdu_session_established == false)
            EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " RA Procedure" << endl;
        else
            EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;
        checkRAC();
        // TODO ensure all operations done before return (i.e. move H-ARQ RX purge before this point)
    }
    else if (noSchedulingGrants == false && noGrantInCurrentSlot == true){
        EV << "There is a grant in another slot" << endl;
        return;
    }
    else{
        bool periodicGrant = false;
        bool checkRac = false;
        bool skip = false;
        for (auto& git : schedulingGrant_) {
            if (git.second != nullptr && git.second->getPeriodic()) {
                periodicGrant = true;
                double carrierFreq = git.first;
                if (--periodCounter_[carrierFreq] > 0) {
                    skip = true;
                }
                else {
                    // resetting grant period
                    periodCounter_[carrierFreq] = git.second->getPeriod();
                    // this is periodic grant TTI - continue with frame sending
                    checkRac = false;
                    skip = false;
                    break;
                }
            }
        }
        if (periodicGrant) {
            if (checkRac)
                checkRAC();
            else {
                if (skip)
                    return;
            }
        }
    }

    scheduleList_.clear();
    requestedSdus_ = 0;
    if (!noSchedulingGrants) { // if a grant is configured
        EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        if (!firstTx) {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx = true;
            // the gNb will receive the first PDU in 2 TTI, thus initializing acid to 0
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }

        std::map<double, HarqTxBuffers>::iterator mtit;
        for (auto& mtit : harqTxBuffers_) {
            double carrierFrequency = mtit.first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                continue;

            // skip if no grant is configured for this carrier
            if (schedulingGrantOffsetMap[slot_nr].find(carrierFrequency) == schedulingGrantOffsetMap[slot_nr].end() || schedulingGrantOffsetMap[slot_nr][carrierFrequency] == NULL)
                continue;

            for (auto [it2Key, currHarq] : mtit.second) {
                unsigned int numProcesses = currHarq->getNumProcesses();

                for (unsigned int proc = 0; proc < numProcesses; proc++) {
                    LteHarqProcessTx *currProc = currHarq->getProcess(proc);

                    // check if the current process has unit ready for retransmission
                    bool ready = currProc->hasReadyUnits();
                    CwList cwListRetx = currProc->readyUnitsIds();

                    EV << "\t [process=" << proc << "] , [retx=" << (ready ? "true" : "false") << "] , [n=" << cwListRetx.size() << "]" << endl;

                    // check if one 'ready' unit has the same direction of the grant
                    bool checkDir = false;
                    for (Codeword cw : cwListRetx) {
                        auto info = currProc->getPdu(cw)->getTag<UserControlInfo>();
                        if (info->getDirection() == schedulingGrantOffsetMap[slot_nr][carrierFrequency]->getDirection())
                        {
                            checkDir = true;
                            break;
                        }
                    }

                    // if a retransmission is needed
                    if (ready && checkDir) {
                        UnitList signal;
                        signal.first = proc;
                        signal.second = cwListRetx;
                        currHarq->markSelected(signal,schedulingGrantOffsetMap[slot_nr][carrierFrequency]->getUserTxParams()->getLayers().size());
                        retx = true;
                        break;
                    }
                }
            }
        }
        if(!retx)
        {
            emptyScheduleList_ = true;
            std::map<double, LteSchedulerUeUl*>::iterator sit;
            for (sit = lcgScheduler_.begin(); sit != lcgScheduler_.end(); ++sit)
            {
                double carrierFrequency = sit->first;

                // skip if this is not the turn of this carrier
                if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                    continue;

                LteSchedulerUeUl* carrierLcgScheduler = sit->second;

                EV << "NRMacUe::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList* carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "NRMacUe::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && emptyScheduleList_)
            {
                // no connection scheduled, but we can use this grant to send a BSR to the eNB
                macPduMake();
            }
            else
            {
                requestedSdus_ = macSduRequestTdd(); // returns an integer
            }

        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);
    }

    //============================ DEBUG ==========================
    if (debugHarq_) {
        for (auto& mtit : harqTxBuffers_) {
            EV << "\n carrier[ " << mtit.first << "] htxbuf.size " << mtit.second.size() << endl;
            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

            int cntOuter = 0;
            int cntInner = 0;
            for (auto [currId, currHarq] : mtit.second) {
                BufferStatus harqStatus = currHarq->getBufferStatus();
                EV << "\t cycleOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for (const auto& jt : harqStatus) {
                    EV << "\t\t cycleInner " << cntInner << " - jt->size=" << jt.size()
                       << " - statusCw(0/1)=" << jt.at(0).second << "/" << jt.at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

    // update current HARQ process id, if needed
    if (requestedSdus_ == 0) {
        EV << NOW << " NRMacUe::handleSelfMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_ + 1) % harqProcesses_ << endl;
        currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
    }

    decreaseNumerologyPeriodCounter();

    EV << "--- END UE MAIN LOOP ---" << endl;
}

int NRMacUe::macSduRequestFdd()
{
    EV << "----- START NRMacUe::macSduRequest -----\n";
    int numRequestedSdus = 0;

    // get the number of granted bytes for each codeword
    std::vector<unsigned int> allocatedBytes;

    for (auto& gitem : schedulingGrant_) {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(gitem.first)) > 0)
            continue;

        if (gitem.second == nullptr)
            continue;

        for (int cw = 0; cw < gitem.second->getGrantedCwBytesArraySize(); cw++)
            allocatedBytes.push_back(gitem.second->getGrantedCwBytes(cw));
    }

    // Ask for a MAC SDU for each scheduled user on each codeword
    for (auto [citFreq, citList] : scheduleList_) {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(citFreq)) > 0)
            continue;

        for (auto& item : *citList) {
            MacCid destCid = item.first.first;
            Codeword cw = item.first.second;
            MacNodeId destId = MacCidToNodeId(destCid);

            std::pair<MacCid, Codeword> key(destCid, cw);
            LteMacScheduleList *scheduledBytesList = lcgScheduler_[citFreq]->getScheduledBytesList();
            auto bit = scheduledBytesList->find(key);

            // consume bytes on this codeword
            if (bit == scheduledBytesList->end())
                throw cRuntimeError("NRMacUe::macSduRequest - cannot find entry in scheduledBytesList");
            else {
                allocatedBytes[cw] -= bit->second;

                EV << NOW << " NRMacUe::macSduRequest - cid[" << destCid << "] - SDU size[" << bit->second << "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

                // send the request message to the upper layer
                // TODO: Replace by tag
                auto pkt = new Packet("LteMacSduRequest");
                auto macSduRequest = makeShared<LteMacSduRequest>();
                macSduRequest->setChunkLength(b(1)); // TODO: should be 0
                macSduRequest->setUeId(destId);
                macSduRequest->setLcid(MacCidToLcid(destCid));
                macSduRequest->setSduSize(bit->second);
                pkt->insertAtFront(macSduRequest);
                *(pkt->addTag<FlowControlInfo>()) = connDesc_[destCid];
                sendUpperPackets(pkt);

                numRequestedSdus++;
            }
        }
    }

    EV << "------ END NRMacUe::macSduRequest ------\n";
    return numRequestedSdus;
}

int NRMacUe::macSduRequestTdd()
{
    EV << "----- START NRMacUe::macSduRequest -----\n";
    int numRequestedSdus = 0;

    // get the number of granted bytes for each codeword
    std::vector<unsigned int> allocatedBytes;

    auto git = schedulingGrantOffsetMap[slot_nr].begin();
    for (; git != schedulingGrantOffsetMap[slot_nr].end(); ++git)
    {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
            continue;

        if (git->second == NULL)
            continue;

        for (int cw=0; cw<git->second->getGrantedCwBytesArraySize(); cw++)
            allocatedBytes.push_back(git->second->getGrantedCwBytes(cw));
    }

    // Ask for a MAC SDU for each scheduled user on each codeword
    for (auto [citFreq, citList] : scheduleList_) {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(citFreq)) > 0)
            continue;

        for (auto& item : *citList) {
            MacCid destCid = item.first.first;
            Codeword cw = item.first.second;
            MacNodeId destId = MacCidToNodeId(destCid);

            std::pair<MacCid, Codeword> key(destCid, cw);
            LteMacScheduleList *scheduledBytesList = lcgScheduler_[citFreq]->getScheduledBytesList();
            auto bit = scheduledBytesList->find(key);

            // consume bytes on this codeword
            if (bit == scheduledBytesList->end())
                throw cRuntimeError("NRMacUe::macSduRequest - cannot find entry in scheduledBytesList");
            else {
                allocatedBytes[cw] -= bit->second;

                EV << NOW << " NRMacUe::macSduRequest - cid[" << destCid << "] - SDU size[" << bit->second << "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

                // send the request message to the upper layer
                // TODO: Replace by tag
                auto pkt = new Packet("LteMacSduRequest");
                auto macSduRequest = makeShared<LteMacSduRequest>();
                macSduRequest->setChunkLength(b(1)); // TODO: should be 0
                macSduRequest->setUeId(destId);
                macSduRequest->setLcid(MacCidToLcid(destCid));
                macSduRequest->setSduSize(bit->second);
                pkt->insertAtFront(macSduRequest);
                *(pkt->addTag<FlowControlInfo>()) = connDesc_[destCid];
                sendUpperPackets(pkt);

                numRequestedSdus++;
            }
        }
    }

    EV << "------ END NRMacUe::macSduRequest ------\n";
    return numRequestedSdus;
}

void NRMacUe::macPduMake(MacCid cid){
    if (cur_slot_type == FDD)
        macPduMakeFdd();
    else
        macPduMakeTdd();
}

void NRMacUe::macPduMakeFdd(MacCid cid)
{
    int64_t size = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = false;
    // UE is in D2D-mode but it received an UL grant (for BSR)
    for (auto& gitem : schedulingGrant_) {
        double carrierFreq = gitem.first;

        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (gitem.second != nullptr && gitem.second->getDirection() == UL && emptyScheduleList_) {
            if (bsrTriggered_ || bsrD2DMulticastTriggered_) {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                for (auto [cid, buffer] : macBuffers_) {
                    Direction connDir = (Direction)connDesc_[cid].getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (bsrTriggered_ && connDir != D2D)
                        continue;
                    if (bsrD2DMulticastTriggered_ && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += buffer->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0) {
                        if (connDesc_[cid].getRlcType() == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (connDesc_[cid].getRlcType() == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0) {
                    // Call the appropriate function for making a BSR for D2D communication
                    Packet *macPktBsr = makeBsr(sizeBsr);
                    auto info = macPktBsr->getTagForUpdate<UserControlInfo>();
                    if (info != nullptr) {
                        info->setCarrierFrequency(carrierFreq);
                        info->setUserTxParams(gitem.second->getUserTxParams()->dup());
                        if (bsrD2DMulticastTriggered_) {
                            info->setLcid(D2D_MULTI_SHORT_BSR);
                            bsrD2DMulticastTriggered_ = false;
                        }
                        else
                            info->setLcid(D2D_SHORT_BSR);
                    }

                    // Add the created BSR to the PDU List
                    if (macPktBsr != nullptr) {
                        LteChannelModel *channelModel = phy_->getChannelModel();
                        if (channelModel == nullptr)
                            throw cRuntimeError("NRMacUe::macPduMake - channel model is a null pointer. Abort.");
                        else
                            macPduList_[channelModel->getCarrierFrequency()][{getMacCellId(), 0}] = macPktBsr;
                        bsrAlreadyMade = true;
                        EV << "NRMacUe::macPduMake - BSR D2D created with size " << sizeBsr << " created" << endl;
                    }

                    bsrRtxTimer_ = bsrRtxTimerStart_;  // this prevents the UE from sending an unnecessary RAC request
                }
                else {
                    bsrD2DMulticastTriggered_ = false;
                    bsrTriggered_ = false;
                    bsrRtxTimer_ = 0;
                }
            }
            break;
        }
    }

    if (!bsrAlreadyMade) {
        // In a D2D communication if BSR was created above this part isn't executed
        // Build a MAC PDU for each scheduled user on each codeword
        for (auto [carrierFreq, schList] : scheduleList_) {
            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
                continue;

            LteMacScheduleList::const_iterator it;
            for (auto& item : *schList) {
                Packet *macPkt;

                MacCid destCid = item.first.first;
                Codeword cw = item.first.second;

                // get the direction (UL/D2D/D2D_MULTI) and the corresponding destination ID
                FlowControlInfo *lteInfo = &(connDesc_.at(destCid));
                MacNodeId destId = lteInfo->getDestId();
                Direction dir = (Direction)lteInfo->getDirection();

                std::pair<MacNodeId, Codeword> pktId = {destId, cw};
                unsigned int sduPerCid = item.second;

                if (sduPerCid == 0 && !bsrTriggered_ && !bsrD2DMulticastTriggered_)
                    continue;

                if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                MacPduList::iterator pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                if (pit == macPduList_[carrierFreq].end()) {
                    // Create a PDU
                    macPkt = new Packet("LteMacPdu");
                    auto header = makeShared<LteMacPdu>();
                    header->setHeaderLength(MAC_HEADER);
                    macPkt->insertAtFront(header);

                    macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(dir);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(SHORT_BSR));
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                    macPkt->addTagIfAbsent<UserControlInfo>()->setGrantId(schedulingGrant_[carrierFreq]->getGrantId());

                    if (usePreconfiguredTxParams_)
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(preconfiguredTxParams_->dup());
                    else
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else {
                    // Never goes here because of the macPduList_.clear() at the beginning
                    macPkt = pit->second;
                }

                while (sduPerCid > 0) {
                    // Add SDU to PDU
                    // Find Mac Pkt
                    if (mbuf_.find(destCid) == mbuf_.end())
                        throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);

                    if (mbuf_[destCid]->isEmpty())
                        throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);

                    auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());

                    // multicast support
                    // this trick gets the group ID from the MAC SDU and sets it in the MAC PDU
                    auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);
                    if (infoVec.empty())
                        throw cRuntimeError("No tag of type LteControlInfo found");

                    int32_t groupId = infoVec.front().getMulticastGroupId();
                    if (groupId >= 0) // for unicast, group id is -1
                        macPkt->getTagForUpdate<UserControlInfo>()->setMulticastGroupId(groupId);

                    drop(pkt);

                    auto header = macPkt->removeAtFront<LteMacPdu>();
                    header->pushSdu(pkt);
                    macPkt->insertAtFront(header);
                    sduPerCid--;
                }

                // consider virtual buffers to compute BSR size
                size += macBuffers_[destCid]->getQueueOccupancy();

                if (size > 0) {
                    // take into account the RLC header size
                    if (connDesc_[destCid].getRlcType() == UM)
                        size += RLC_HEADER_UM;
                    else if (connDesc_[destCid].getRlcType() == AM)
                        size += RLC_HEADER_AM;
                }
            }
        }
    }

    // Put MAC PDUs in H-ARQ buffers
    for (auto& lit : macPduList_) {
        double carrierFreq = lit.first;
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        for (auto& pit : lit.second) {
            MacNodeId destId = pit.first.first;
            Codeword cw = pit.first.second;
            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx *txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end()) {
                // The tx buffer already exists
                txBuf = hit->second;
            }
            else {
                // The tx buffer does not exist yet for this mac node id, create one
                LteHarqBufferTx *hb;
                // FIXME: hb is never deleted
                auto info = pit.second->getTag<UserControlInfo>();
                if (info->getDirection() == UL)
                    hb = new LteHarqBufferTx(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this, check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, destId)));
                else // D2D or D2D_MULTI
                    hb = new LteHarqBufferTxD2D(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this, check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, destId)));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the first available process
            UnitList txList = (pit.second->getTag<UserControlInfo>()->getDirection() == D2D_MULTI) ? txBuf->getEmptyUnits(currentHarq_) : txBuf->firstAvailable();
            EV << "NRMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "]" << endl;

            //Get a reference of the LteMacPdu from pit pointer (extract Pdu from the MAP)
            auto macPkt = pit.second;

            // BSR related operations

               // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
               // has to be sent even if there is no data in the user's queues. In few words, a BSR is always
               // triggered and has to be sent when there are enough resources

               // TODO implement differentiated BSR attach
               //
               //            // if there's enough space for a LONG BSR, send it
               //            if( (availableBytes >= LONG_BSR_SIZE) ) {
               //                // Create a PDU if data were not scheduled
               //                if (pdu==0)
               //                    pdu = new LteMacPdu();
               //
               //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
               //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, sending a Long BSR...\n",NOW,nodeId);
               //
               //                // create a full BSR
               //                pdu->ctrlPush(fullBufferStatusReport());
               //
               //                // do not reset BSR flag
               //                mac_->bsrTriggered() = true;
               //
               //                availableBytes -= LONG_BSR_SIZE;
               //
               //            }
               //
               //            // if there's space only for a SHORT BSR and there are scheduled flows, send it
               //            else if( (mac_->bsrTriggered() == true) && (availableBytes >= SHORT_BSR_SIZE) && (highestBackloggedFlow != -1) ) {
               //
               //                // Create a PDU if data were not scheduled
               //                if (pdu==0)
               //                    pdu = new LteMacPdu();
               //
               //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
               //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, sending a Short/Truncated BSR...\n",NOW,nodeId);
               //
               //                // create a short BSR
               //                pdu->ctrlPush(shortBufferStatusReport(highestBackloggedFlow));
               //
               //                // do not reset BSR flag
               //                mac_->bsrTriggered() = true;
               //
               //                availableBytes -= SHORT_BSR_SIZE;
               //
               //            }
               //            // if there's a BSR triggered but there's not enough space, collect the appropriate statistic
               //            else if(availableBytes < SHORT_BSR_SIZE && availableBytes < LONG_BSR_SIZE) {
               //                Stat::put(LTE_BSR_SUPPRESSED_NODE,nodeId,1.0);
               //                Stat::put(LTE_BSR_SUPPRESSED_CELL,mac_->cellId(),1.0);
               //            }
               //            Stat::put (LTE_GRANT_WASTED_BYTES_UL, nodeId, availableBytes);
               //        }
               //
               //        // 4) PDU creation
               //
               //        if (pdu!=0) {
               //
               //            pdu->cellId() = mac_->cellId();
               //            pdu->nodeId() = nodeId;
               //            pdu->direction() = mac::UL;
               //            pdu->error() = false;
               //
               //            if(LteDebug::trace("LteSchedulerUeUl::schedule"))
               //                fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, creating uplink PDU.\n", NOW, nodeId);
               //
               //        } */

            auto header = macPkt->removeAtFront<LteMacPdu>();
            // Attach BSR to PDU if RAC is won and wasn't already made
            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && !bsrAlreadyMade && size > 0) {
                MacBsr *bsr = new MacBsr();
                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);
                bsrTriggered_ = false;
                bsrD2DMulticastTriggered_ = false;
                bsrAlreadyMade = true;
                EV << "NRMacUe::macPduMake - BSR created with size " << size << endl;
                bsr_already_sent = true;
            }

            if (bsrAlreadyMade && size > 0) { // this prevents the UE from sending an unnecessary RAC request
                bsrRtxTimer_ = bsrRtxTimerStart_;
            }
            else
                bsrRtxTimer_ = 0;

            macPkt->insertAtFront(header);

            EV << "NRMacUe: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty()) {
                EV << "NRMacUe() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else {
                //Insert PDU in the Harq Tx Buffer
                //txList.first is the acid
                txBuf->insertPdu(txList.first, cw, macPkt);
            }
        }
    }
}

void NRMacUe::macPduMakeTdd(MacCid cid)
{
    int64_t size = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = false;
    // UE is in D2D-mode but it received an UL grant (for BSR)
    auto git = schedulingGrantOffsetMap[slot_nr].begin();
    for (; git != schedulingGrantOffsetMap[slot_nr].end(); ++git)
    {
        double carrierFreq = git->first;

        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (git->second != nullptr && git->second->getDirection() == UL && emptyScheduleList_) {
            if (bsrTriggered_ || bsrD2DMulticastTriggered_) {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                for (auto [cid, buffer] : macBuffers_) {
                    Direction connDir = (Direction)connDesc_[cid].getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (bsrTriggered_ && connDir != D2D)
                        continue;
                    if (bsrD2DMulticastTriggered_ && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += buffer->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0) {
                        if (connDesc_[cid].getRlcType() == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (connDesc_[cid].getRlcType() == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0) {
                    // Call the appropriate function for making a BSR for D2D communication
                    Packet *macPktBsr = makeBsr(sizeBsr);
                    auto info = macPktBsr->getTagForUpdate<UserControlInfo>();
                    if (info != nullptr) {
                        info->setCarrierFrequency(carrierFreq);
                        info->setUserTxParams(git->second->getUserTxParams()->dup());
                        if (bsrD2DMulticastTriggered_) {
                            info->setLcid(D2D_MULTI_SHORT_BSR);
                            bsrD2DMulticastTriggered_ = false;
                        }
                        else
                            info->setLcid(D2D_SHORT_BSR);
                    }

                    // Add the created BSR to the PDU List
                    if (macPktBsr != nullptr) {
                        LteChannelModel *channelModel = phy_->getChannelModel();
                        if (channelModel == nullptr)
                            throw cRuntimeError("NRMacUe::macPduMake - channel model is a null pointer. Abort.");
                        else
                            macPduList_[channelModel->getCarrierFrequency()][{getMacCellId(), 0}] = macPktBsr;
                        bsrAlreadyMade = true;
                        EV << "NRMacUe::macPduMake - BSR D2D created with size " << sizeBsr << " created" << endl;
                    }

                    bsrRtxTimer_ = bsrRtxTimerStart_;  // this prevents the UE from sending an unnecessary RAC request
                }
                else {
                    bsrD2DMulticastTriggered_ = false;
                    bsrTriggered_ = false;
                    bsrRtxTimer_ = 0;
                }
            }
            break;
        }
    }

    if (!bsrAlreadyMade) {
        // In a D2D communication if BSR was created above this part isn't executed
        // Build a MAC PDU for each scheduled user on each codeword
        for (auto [carrierFreq, schList] : scheduleList_) {
            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
                continue;

            LteMacScheduleList::const_iterator it;
            for (auto& item : *schList) {
                Packet *macPkt;

                MacCid destCid = item.first.first;
                Codeword cw = item.first.second;

                // get the direction (UL/D2D/D2D_MULTI) and the corresponding destination ID
                FlowControlInfo *lteInfo = &(connDesc_.at(destCid));
                MacNodeId destId = lteInfo->getDestId();
                Direction dir = (Direction)lteInfo->getDirection();

                std::pair<MacNodeId, Codeword> pktId = {destId, cw};
                unsigned int sduPerCid = item.second;

                if (sduPerCid == 0 && !bsrTriggered_ && !bsrD2DMulticastTriggered_)
                    continue;

                if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                MacPduList::iterator pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                if (pit == macPduList_[carrierFreq].end()) {
                    // Create a PDU
                    macPkt = new Packet("LteMacPdu");
                    auto header = makeShared<LteMacPdu>();
                    header->setHeaderLength(MAC_HEADER);
                    macPkt->insertAtFront(header);

                    macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(dir);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(SHORT_BSR));
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                    macPkt->addTagIfAbsent<UserControlInfo>()->setGrantId(schedulingGrantOffsetMap[slot_nr][carrierFreq]->getGrantId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setMcsIndex(schedulingGrantOffsetMap[slot_nr][carrierFreq]->getMcsIndex());
                    if (usePreconfiguredTxParams_)
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(preconfiguredTxParams_->dup());
                    else
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrantOffsetMap[slot_nr][carrierFreq]->getUserTxParams()->dup());

                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else {
                    // Never goes here because of the macPduList_.clear() at the beginning
                    macPkt = pit->second;
                }

                while (sduPerCid > 0) {
                    // Add SDU to PDU
                    // Find Mac Pkt
                    if (mbuf_.find(destCid) == mbuf_.end())
                        throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);

                    if (mbuf_[destCid]->isEmpty()){
                        if (getSimulation()->getSystemModule()->par("packetDropEnabled").boolValue()) {
                             EV << NOW << " UmTxEntity::rlcPduMake - all SDUs dropped due to PDB, no PDU sent" << endl;
                            return; // skip sending
                        }
                        throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);
                    }

                    auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());

                    // multicast support
                    // this trick gets the group ID from the MAC SDU and sets it in the MAC PDU
                    auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);
                    if (infoVec.empty())
                        throw cRuntimeError("No tag of type LteControlInfo found");

                    int32_t groupId = infoVec.front().getMulticastGroupId();
                    if (groupId >= 0) // for unicast, group id is -1
                        macPkt->getTagForUpdate<UserControlInfo>()->setMulticastGroupId(groupId);

                    drop(pkt);

                    auto header = macPkt->removeAtFront<LteMacPdu>();
                    header->pushSdu(pkt);
                    macPkt->insertAtFront(header);
                    sduPerCid--;
                }

                // consider virtual buffers to compute BSR size
                size += macBuffers_[destCid]->getQueueOccupancy();

                if (size > 0) {
                    // take into account the RLC header size
                    if (connDesc_[destCid].getRlcType() == UM)
                        size += RLC_HEADER_UM;
                    else if (connDesc_[destCid].getRlcType() == AM)
                        size += RLC_HEADER_AM;
                }
            }
        }
    }

    // Put MAC PDUs in H-ARQ buffers
    for (auto& lit : macPduList_) {
        double carrierFreq = lit.first;
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        for (auto& pit : lit.second) {
            MacNodeId destId = pit.first.first;
            Codeword cw = pit.first.second;
            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx *txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end()) {
                // The tx buffer already exists
                txBuf = hit->second;
            }
            else {
                // The tx buffer does not exist yet for this mac node id, create one
                LteHarqBufferTx *hb;
                // FIXME: hb is never deleted
                auto info = pit.second->getTag<UserControlInfo>();
                if (info->getDirection() == UL)
                    hb = new LteHarqBufferTx(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this, check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, destId)));
                else // D2D or D2D_MULTI
                    hb = new LteHarqBufferTxD2D(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this, check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, destId)));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the first available process
            UnitList txList = (pit.second->getTag<UserControlInfo>()->getDirection() == D2D_MULTI) ? txBuf->getEmptyUnits(currentHarq_) : txBuf->firstAvailable();
            EV << "NRMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "]" << endl;

            //Get a reference of the LteMacPdu from pit pointer (extract Pdu from the MAP)
            auto macPkt = pit.second;

            // BSR related operations

               // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
               // has to be sent even if there is no data in the user's queues. In few words, a BSR is always
               // triggered and has to be sent when there are enough resources

               // TODO implement differentiated BSR attach
               //
               //            // if there's enough space for a LONG BSR, send it
               //            if( (availableBytes >= LONG_BSR_SIZE) ) {
               //                // Create a PDU if data were not scheduled
               //                if (pdu==0)
               //                    pdu = new LteMacPdu();
               //
               //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
               //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, sending a Long BSR...\n",NOW,nodeId);
               //
               //                // create a full BSR
               //                pdu->ctrlPush(fullBufferStatusReport());
               //
               //                // do not reset BSR flag
               //                mac_->bsrTriggered() = true;
               //
               //                availableBytes -= LONG_BSR_SIZE;
               //
               //            }
               //
               //            // if there's space only for a SHORT BSR and there are scheduled flows, send it
               //            else if( (mac_->bsrTriggered() == true) && (availableBytes >= SHORT_BSR_SIZE) && (highestBackloggedFlow != -1) ) {
               //
               //                // Create a PDU if data were not scheduled
               //                if (pdu==0)
               //                    pdu = new LteMacPdu();
               //
               //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
               //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, sending a Short/Truncated BSR...\n",NOW,nodeId);
               //
               //                // create a short BSR
               //                pdu->ctrlPush(shortBufferStatusReport(highestBackloggedFlow));
               //
               //                // do not reset BSR flag
               //                mac_->bsrTriggered() = true;
               //
               //                availableBytes -= SHORT_BSR_SIZE;
               //
               //            }
               //            // if there's a BSR triggered but there's not enough space, collect the appropriate statistic
               //            else if(availableBytes < SHORT_BSR_SIZE && availableBytes < LONG_BSR_SIZE) {
               //                Stat::put(LTE_BSR_SUPPRESSED_NODE,nodeId,1.0);
               //                Stat::put(LTE_BSR_SUPPRESSED_CELL,mac_->cellId(),1.0);
               //            }
               //            Stat::put (LTE_GRANT_WASTED_BYTES_UL, nodeId, availableBytes);
               //        }
               //
               //        // 4) PDU creation
               //
               //        if (pdu!=0) {
               //
               //            pdu->cellId() = mac_->cellId();
               //            pdu->nodeId() = nodeId;
               //            pdu->direction() = mac::UL;
               //            pdu->error() = false;
               //
               //            if(LteDebug::trace("LteSchedulerUeUl::schedule"))
               //                fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, creating uplink PDU.\n", NOW, nodeId);
               //
               //        } */

            auto header = macPkt->removeAtFront<LteMacPdu>();
            // Attach BSR to PDU if RAC is won and wasn't already made
            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && !bsrAlreadyMade && size > 0) {
                MacBsr *bsr = new MacBsr();
                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);
                bsrTriggered_ = false;
                bsrD2DMulticastTriggered_ = false;
                bsrAlreadyMade = true;
                EV << "NRMacUe::macPduMake - BSR created with size " << size << endl;
                bsr_already_sent = true;
            }

            if (bsrAlreadyMade && size > 0) { // this prevents the UE from sending an unnecessary RAC request
                bsrRtxTimer_ = bsrRtxTimerStart_;
            }
            else
                bsrRtxTimer_ = 0;

            macPkt->insertAtFront(header);

            EV << "NRMacUe: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty()) {
                EV << "NRMacUe() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else {
                //Insert PDU in the Harq Tx Buffer
                //txList.first is the acid
                txBuf->insertPdu(txList.first, cw, macPkt);
            }
        }
    }
}

void NRMacUe::checkRAC()
{
    EV << NOW << " LteMacUeD2D::checkRAC , Ue  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_
       << ", raRespTimer:" << raRespTimer_ << endl;

    if (racBackoffTimer_ > 0) {
        racBackoffTimer_--;
        return;
    }

    if (raRespTimer_ > 0) {
        // decrease RAC response timer
        raRespTimer_--;
        EV << NOW << " LteMacUeD2D::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
        return;
    }

    if (bsrRtxTimer_ > 0) {
        // decrease BSR timer
        bsrRtxTimer_--;
        EV << NOW << " LteMacUe::checkRAC - waiting for a grant, BSR rtx timer has not expired yet (timer=" << bsrRtxTimer_ << ")" << endl;

        return;
    }

    // Avoids double requests within the same TTI window
    if (racRequested_) {
        EV << NOW << " LteMacUeD2D::checkRAC - double RAC request" << endl;
        racRequested_ = false;
        return;
    }
    if (racD2DMulticastRequested_) {
        EV << NOW << " LteMacUeD2D::checkRAC - double RAC request" << endl;
        racD2DMulticastRequested_ = false;
        return;
    }

    auto tcsaiMappingTable = binder_->getGlobalDataModule()->getTscaiMappingTable();
    auto iter = tcsaiMappingTable.find(getMacNodeId());
    if (iter != tcsaiMappingTable.end() && pdu_session_established == true) {
        EV << " LteMacUeD2D::checkRAC -  UE Grants are pre-allocated, don't request dyn. Grants" << endl; 
        return;
    }

    bool trigger = false;
    bool triggerD2DMulticast = false;

    LteMacBufferMap::const_iterator it;

    for (auto [cid, macBuffer] : macBuffers_) {
        if (!(macBuffer->isEmpty())) {
            if (connDesc_.at(cid).getDirection() == D2D_MULTI)
                triggerD2DMulticast = true;
            else
                trigger = true;
            break;
        }
    }

    if (!trigger && !triggerD2DMulticast && pdu_session_established) {
        EV << NOW << " LteMacUeD2D::checkRAC , Ue " << nodeId_ << ", RAC aborted, no data in queues " << pdu_session_established << endl;
    }

    if (pdu_session_established == false){
        auto pkt = new Packet("RacRequest");
        double carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        auto racReq = makeShared<LteRac>();

        pkt->insertAtFront(racReq);
        sendLowerPackets(pkt);

        EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << ", RAC request sent to PHY " << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        raRespTimer_ = raRespWinStart_;
        pdu_session_established = true;
    }
    else if ((racRequested_ = trigger) || (racD2DMulticastRequested_ = triggerD2DMulticast)) {
        auto pkt = new Packet("SchedulingRequest");
        double carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(SCHEDULINGREQPKT);

        auto schedulingReq = makeShared<LteSchedulingRequest>();

        pkt->insertAtFront(schedulingReq);
        sendLowerPackets(pkt);

        // wait at least  "raRespWinStart_" TTIs before another request
        raRespTimer_ = raRespWinStart_;
        //BSR has to be always sent if grant is scheduled
        bsrTriggered_ = true;
	}
}

void
NRMacUe::macHandleGrant(cPacket* pktAux)
{
    EV << NOW << " NRMacUe::macHandleGrant - UE [" << nodeId_ << "] - Grant received " << endl;
    if (binder_->getTddPattern().tdd == false)
        LteMacUeD2D::macHandleGrant(pktAux);
    else{
        // extract grant
        auto pkt = check_and_cast<inet::Packet*> (pktAux);
        auto grant = pkt->popAtFront<LteSchedulingGrant>();


        auto userInfo = pkt->getTag<UserControlInfo>();
        double carrierFrequency = userInfo->getCarrierFrequency();
        int slot_offset = binder_->getTddPattern().numDlSlots + grant->getSlotOffset();

        EV << NOW << " NRMacUe::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << " Carrier: "  << carrierFrequency << " Slot Offset " << slot_offset << " periodic " << grant->getPeriod() << endl;

        // store received grant
        if (!grant->getPeriodic())
            schedulingGrantOffsetMap[slot_offset][carrierFrequency] = grant;
        else {
            auto &vec = configuredGrantMap[carrierFrequency];

            if (grant->getActive()) {
                inet::IntrusivePtr<LteSchedulingGrant> grantCopy(check_and_cast<LteSchedulingGrant *>(grant->dup()));
                vec.push_back(ConfiguredGrant{slot_offset, grant->getPeriod(), grant->getHPCycle(), grantCopy, true, false, carrierFrequency});
                schedulingGrantOffsetMap[slot_offset][carrierFrequency] = grant;

                EV << NOW << " NRMacUe::macHandleGrant - Added new CG at carrier "
                   << carrierFrequency << " slotOffset " << slot_offset << endl;
                hp_cycle_max = grant->getHPCycleMax();
            }
            else {
                // Remove inactive grants with same slotOffset
                vec.erase(
                    std::remove_if(
                        vec.begin(),
                        vec.end(),
                        [&](ConfiguredGrant &cg) {
                            return compareCGs(cg.grant, grant);
                        }
                    ),
                    vec.end()
                );
                EV << NOW << " NRMacUe::macHandleGrant - Removed CG at carrier "
                   << carrierFrequency << " slotOffset " << slot_offset << endl;
            }
        }

        EV << NOW << "Node " << nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
           << ", bytes " << grant->getGrantedCwBytes(0) <<" Direction: "<<dirToA(grant->getDirection()) << endl;

        // clearing pending RAC requests
        racRequested_=false;
        racD2DMulticastRequested_=false;
        if (!grant->getPeriodic())
            bsrTriggered_ = true;

        delete pkt;
    }
}

const LteSchedulingGrant* NRMacUe::getSchedulingGrant(double carrierFrequency)
{
    EV << "NRMacUe::getSchedulingGrant" << slot_nr << endl;
    if (cur_slot_type == FDD)
        LteMacUe::getSchedulingGrant(carrierFrequency);
    else{
        if(schedulingGrantOffsetMap.find(slot_nr) == schedulingGrantOffsetMap.end()){
            return NULL;
        }
        else{
            std::map<double, inet::IntrusivePtr<const LteSchedulingGrant> > scheduling_grant = schedulingGrantOffsetMap[slot_nr];
            if(scheduling_grant.find(carrierFrequency) == scheduling_grant.end())
                return NULL;
            else
                return scheduling_grant.at(carrierFrequency).get();
        }
    }
}

void NRMacUe::updateUserTxParam(cPacket* pktAux)
{
    if (cur_slot_type == FDD)
        LteMacUe::updateUserTxParam(pktAux);
    else{
        auto pkt = check_and_cast<inet::Packet *>(pktAux);

        auto lteInfo = pkt->getTagForUpdate<UserControlInfo> ();

        if (lteInfo->getFrameType() != DATAPKT)
            return;

        double carrierFrequency = lteInfo->getCarrierFrequency();

        lteInfo->setUserTxParams(schedulingGrantOffsetMap[slot_nr][carrierFrequency]->getUserTxParams()->dup());

        lteInfo->setTxMode(schedulingGrantOffsetMap[slot_nr][carrierFrequency]->getUserTxParams()->readTxMode());

        int grantedBlocks = schedulingGrantOffsetMap[slot_nr][carrierFrequency]->getTotalGrantedBlocks();

        lteInfo->setGrantedBlocks(schedulingGrantOffsetMap[slot_nr][carrierFrequency]->getGrantedBlocks());
        lteInfo->setTotalGrantedBlocks(grantedBlocks);
    }
}

void NRMacUe::flushHarqBuffers()
{
    if (cur_slot_type == FDD){
        LteMacUe::flushHarqBuffers();
    }
    else{
        // send the selected units to lower layers
        std::map<double, HarqTxBuffers>::iterator mtit;
        for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        {
            HarqTxBuffers::iterator it2;
            for(it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++)
                it2->second->sendSelectedDown();
        }

        for (const auto& pair : schedulingGrantOffsetMap){
            int offset = pair.first;
            for (const auto& innerpair: schedulingGrantOffsetMap[offset]){
                int frequency = innerpair.first;
                if (schedulingGrantOffsetMap[offset][frequency] != nullptr && !(schedulingGrantOffsetMap[offset][frequency]->getPeriodic())){
                    schedulingGrantOffsetMap[offset][frequency] = nullptr;
                }
            }
        }
    }
}


void NRMacUe::macHandleRac(cPacket* pktAux)
{
    EV << "NrMacUE::macHandleRac"<< endl;
    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto racPkt = pkt->peekAtFront<LteRac>();

    if (racPkt->getSuccess())
    {
        EV << "NrMacUE::macHandleRac - Ue " << nodeId_ << " won RAC" << endl;
        // is RAC is won, BSR has to be sent
        bsrTriggered_=true;
        // reset RAC counter
        currentRacTry_=0;
        //reset RAC backoff timer
        racBackoffTimer_=0;
    }
    else
    {
        // RAC has failed
        if (++currentRacTry_ >= maxRacTryouts_)
        {
            EV << NOW << " Ue " << nodeId_ << ", RAC reached max attempts : " << currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            //reset RAC counter
            currentRacTry_=0;
            //reset RAC backoff timer
            racBackoffTimer_=0;
        }
        else
        {
            // recompute backoff timer
            racBackoffTimer_= uniform(minRacBackoff_,maxRacBackoff_);
            EV << NOW << " Ue " << nodeId_ << " RAC attempt failed, backoff extracted : " << racBackoffTimer_ << endl;
        }
    }
    delete pkt;
}

bool NRMacUe::compareCGs(const inet::IntrusivePtr<const LteSchedulingGrant>& a, const inet::IntrusivePtr<const LteSchedulingGrant>& b) const
{
    if (!b)
        return false;

    // Compare slot offset and TDD cycle offset
    if (a->getSlotOffset() != b->getSlotOffset())
        return false;

    if (a->getDirection() != b->getDirection())
        return false;

    if (a->getPeriodic() && a->getPeriod() != b->getPeriod())
        return false;

    if (a->getHPCycle() != b->getHPCycle())
        return false;

    if (a->getCodewords() != b->getCodewords())
        return false;

    for (int cw = 0; cw < a->getCodewords(); ++cw) {
        if (a->getGrantedCwBytes(cw) != b->getGrantedCwBytes(cw))
            return false;
    }

    const RbMap &rbMap1 = a->getGrantedBlocks();
    const RbMap &rbMap2 = b->getGrantedBlocks();
    if (rbMap1 != rbMap2)
        return false;

    if (a->getTotalGrantedBlocks() != b->getTotalGrantedBlocks())
        return false;

    return true;
}


} //namespace

