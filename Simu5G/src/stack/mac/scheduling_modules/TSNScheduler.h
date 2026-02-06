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

#ifndef STACK_MAC_SCHEDULING_MODULES_TSNSCHEDULER_H_
#define STACK_MAC_SCHEDULING_MODULES_TSNSCHEDULER_H_

#include "stack/mac/scheduler/LteScheduler.h"
namespace simu5g {

struct TSCAISchedInfo{
    MacNodeId nodeId;
    int priority;
    int BAT_frame;
    int BAT_slot;
    int BS_remaining;
    int sched_window;
};


class  TSNScheduler : public virtual LteScheduler{
public:
    TSNScheduler(Binder *binder) : LteScheduler(binder){
    };
    TSNScheduler(Binder *binder, Direction dir): LteScheduler(binder), dir(dir){
    };
    virtual void prepareSchedule(bool static_scheduling = false);

protected:
    GlobalData *globalData;
    Direction dir;
    std::map<MacNodeId, TSCAISchedInfo> schedInfoMap;
    std::map<MacNodeId, TSCAIConfiguration> mappingTable;
    double slot_duration = -1;
    double tdd_duration = -1;
    double preparation_time = -1;

    std::vector<std::pair<MacNodeId, TSCAISchedInfo>> getSortedSchedInfo(const std::map<MacNodeId, TSCAISchedInfo>& schedInfoMap);
    void PreallocationAlgorithm();
};

} /* namespace simu5g */

#endif /* STACK_MAC_SCHEDULING_MODULES_TSNSCHEDULER_H_ */
