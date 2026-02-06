//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "TSNScheduler.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "common/LteCommon.h"

namespace simu5g {

/*std::vector<std::pair<MacNodeId, TSCAISchedInfo>> TSNScheduler::getSortedSchedInfo(
    const std::map<MacNodeId, TSCAISchedInfo>& schedInfoMap) {

    std::vector<std::pair<MacNodeId, TSCAISchedInfo>> sortedList(schedInfoMap.begin(), schedInfoMap.end());

    std::sort(sortedList.begin(), sortedList.end(),
              [](const auto& a, const auto& b) {
                  return a.second.sched_window < b.second.sched_window;
              });

    return sortedList;
}*/

std::vector<std::pair<MacNodeId, TSCAISchedInfo>> 
TSNScheduler::getSortedSchedInfo(const std::map<MacNodeId, TSCAISchedInfo>& schedInfoMap) {

    std::vector<std::pair<MacNodeId, TSCAISchedInfo>> sortedList(
        schedInfoMap.begin(), schedInfoMap.end());

    std::sort(sortedList.begin(), sortedList.end(),
              [&](const auto& a, const auto& b) {
                  // 1. Sort by sched_window (ascending)
                  if (a.second.sched_window != b.second.sched_window)
                      return a.second.sched_window < b.second.sched_window;

                  // 2. Tie-breaker: number of usable bands (descending = prefer more bands)
                  const UserTxParams& infoA = eNbScheduler_->mac_->getAmc()
                      ->computeTxParams(a.first, dir, carrierFrequency_);
                  const UserTxParams& infoB = eNbScheduler_->mac_->getAmc()
                      ->computeTxParams(b.first, dir, carrierFrequency_);

                  size_t bandsA = infoA.readBands().size();
                  size_t bandsB = infoB.readBands().size();

                  return bandsA < bandsB; // more bands = earlier scheduling
              });

    return sortedList;
}


void TSNScheduler::prepareSchedule(bool static_scheduling)
{
    EV << "TSNScheduler::prepareSchedule() " << endl;
    if (slot_duration == -1 && tdd_duration == -1){
        // intialization of class variables
        slot_duration = binder_->getSlotDurationFromNumerologyIndex(binder_->getTddPattern().numerologyIndex) * 1000; // assumes ms
        tdd_duration = binder_->getTddPattern().Periodicity;
        globalData = check_and_cast<GlobalData*>(getSimulation()->getModuleByPath("globalData"));
        mappingTable = globalData->getTscaiMappingTable();
    }

    // todo: check whether static CG or adapted
    if (mac_->getTddCycleCntr() > mac_->getCGPreparationTime() + mac_->getTscaiHP()/(slot_duration*tdd_duration) && mac_->getStaticMcsIndex() != -1){
        EV << "Preparation time + one HP is passed, " << " preparation time " << preparation_time << " tdd cycle cntr " << mac_->getTddCycleCntr() << mac_->getTscaiHP()/(slot_duration*tdd_duration) << endl;
        applyCGs(mac_->getHpCycleCntr(), mac_->getCurrentSlotOffset());
    } else {
        PreallocationAlgorithm();
    }
}

void TSNScheduler::PreallocationAlgorithm(){
    EV << "TSNScheduler::PreallocationAlgorithm() " << endl;
    for (auto& [key, config] : mappingTable) {

       // MacNodeId id = MacNodeId(c);
        if (mac_->checkPDUStatus(carrierFrequency_, key)){

            auto iter = schedInfoMap.find(key);
            if (iter == schedInfoMap.end()){

                EV << "New PDU Session established, node ID" << key << "create new entry in schedInfoMap " << endl;

                TSCAISchedInfo info;
                info.nodeId = key;
                info.priority = config.priority;
                info.BAT_frame = std::floor((config.BAT) / (1000*tdd_duration *slot_duration));
                info.BAT_slot = std::ceil((config.BAT - info.BAT_frame * tdd_duration * slot_duration * 1000)/(slot_duration * 1000));
                info.BS_remaining = config.BS;

                info.sched_window = -1;
                EV << "slot_duration "<< slot_duration << " tdd_duration" << tdd_duration << " BAT " << config.BAT << " info.BAT_frame " << info.BAT_frame << " info.BAT_slot " << info.BAT_slot << endl;

                if (info.BAT_frame <= mac_->getTddCycleCntr() && info.BAT_slot < mac_->getCurrentSlotOffset()){
                    throw omnetpp::cRuntimeError("TSNScheduler::prepareSchedule UE connected after BAT of TSN Stream, cannot be scheduled appropriately");
                }
                schedInfoMap[key] = info;
            }
        }
        else{
            EV << "PDUSession of nodeId " << config.nodeId << " not established" << endl;
        }
    }


    for (auto& [key, info] : schedInfoMap) {
        if (info.sched_window == -1){
            if (info.BAT_slot <= (binder_->getTddPattern().numDlSlots + mac_->getCurrentSlotOffset()) && info.BAT_frame == mac_->getTddCycleCntr()){
                EV << "reset sched_window" << endl;
                if (info.priority == 6)
                    info.sched_window = 1;
                else
                    info.sched_window = binder_->getTddPattern().numUlSlots+1;

                std::map<MacNodeId, TSCAIConfiguration>::const_iterator it = mappingTable.find(key);
                if (it != mappingTable.end()) {
                    info.BS_remaining = it->second.BS +  MAC_HEADER + PDCP_HEADER_UM + RLC_HEADER_UM + PPP_TRAILER + PPP_HEADER;
                }
            }
        }
    }
    EV << "TSNScheduler: starting sorting" << endl;
    auto sortedSchedInfo = getSortedSchedInfo(schedInfoMap);
    EV << "TSNSCheduler: starting scheduling" << endl;
    for (auto& [key, sched_info] : sortedSchedInfo) {

        auto it_sched = schedInfoMap.find(key);
        if (it_sched != schedInfoMap.end()) {

            if (it_sched->second.BS_remaining > 0 && it_sched->second.sched_window != -1 ){

                EV << "schedule nodeId " << key << endl;

                const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(key, dir, carrierFrequency_);
                const std::set<Band>& bands = info.readBands();
                unsigned int codeword = info.getLayers().size();
                bool cqiNull = false;
                for (unsigned int i = 0; i < codeword; i++) {
                    if (info.readCqiVector()[i] == 0)
                        cqiNull = true;
                }
                if (cqiNull){
                    EV << "TSNScheduler skip due to cqiNull" << endl;
                    continue;
                }

                // No more free cw
                if (eNbScheduler_->allocatedCws(key) == codeword){
                    EV << "TSNScheduler skip due to codeword" << endl;
                    continue;
                }
                    
                unsigned int availableBlocks = 0;
                unsigned int availableBytes = 0;
                // For each antenna
                for (const auto& antenna : info.readAntennaSet()) {
                    // For each logical band
                    for (const auto& band : bands) {
                        unsigned int blocks = eNbScheduler_->readAvailableRbs(key, antenna, band);
                        availableBlocks += blocks;
                        availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(key, band, blocks, dir, carrierFrequency_, mac_->getMinMcsIndex(),  mac_->getStaticMcsIndex());
                    }
                }
                if (availableBytes < it_sched->second.BS_remaining){
                    if (it_sched->second.sched_window == 1){
                        EV << "TSNScheduler skip due to too less availble Bytes, nodeId " << key << endl;
                        continue;
                    }
                    else
                        EV << "Schedule UE partially " << endl;
                }
                    

                bool terminate = false;
                bool active = true;
                bool eligible = true;
                bool static_grant = true;
                LogicalCid mylcid = 0; // assuming only one logical UL channel per UE

               // LogicalCid mylcid = ht_.find_entry(mappingTable[key].SrcAddr, mappingTable[key].DstAddr, mappingTable[key].ToS, dir);

                auto it_sched = schedInfoMap.find(key);
                if (it_sched != schedInfoMap.end()) {
                    EV << "TSNScheduler request Grant for "<< key << endl;
                    it_sched->second.BS_remaining -= requestGrant(idToMacCid(key, mylcid), it_sched->second.BS_remaining, terminate, active, eligible, static_grant, mac_->getMinMcsIndex(), mac_->getStaticMcsIndex());
                    EV << "TSNScheduler remaining Bytes for "<< key << " " << it_sched->second.BS_remaining << endl;
                }

                if (it_sched->second.BS_remaining > 0){
                    it_sched->second.BS_remaining = it_sched->second.BS_remaining +  MAC_HEADER + RLC_HEADER_UM + PDCP_HEADER_UM + PPP_TRAILER + PPP_HEADER;
                    it_sched->second.sched_window--;
                    EV << "not all bytes served remaining " << it_sched->second.BS_remaining  << " remaining time " << it_sched->second.sched_window << endl;
                }

                if (it_sched->second.BS_remaining <= 0 || it_sched->second.sched_window < 1){
                    EV << "Reseting sched window, all Bytes are served " << it_sched->second.BS_remaining << " or sched window passed " << it_sched->second.sched_window << endl;
                    it_sched->second.sched_window = -1;

                    auto it_map = mappingTable.find(key);
                    if (it_map != mappingTable.end()) {
                        it_sched->second.BS_remaining = it_map->second.BS;
                        it_map->second.BAT += it_map->second.period;
                    }

                    if (it_sched != schedInfoMap.end()) {
                        it_sched->second.BAT_frame = std::floor(it_map->second.BAT/(tdd_duration *slot_duration*1000));
                        it_sched->second.BAT_slot = std::ceil((it_map->second.BAT - it_sched->second.BAT_frame * tdd_duration * slot_duration * 1000)/(slot_duration*1000));
                    }
                }
            }
        }
    }
}


} /* namespace simu5g */
