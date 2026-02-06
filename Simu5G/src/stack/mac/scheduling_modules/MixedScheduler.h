#ifndef STACK_MAC_SCHEDULING_MODULES_MIXEDSCHEDULER_H_
#define STACK_MAC_SCHEDULING_MODULES_MIXEDSCHEDULER_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/scheduling_modules/DQos.h"
#include "stack/mac/scheduling_modules/TSNScheduler.h"
#include "stack/mac/scheduling_modules/LtePf.h"
#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduling_modules/LteDrr.h"
#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"
#include "stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "stack/mac/scheduling_modules/LteMaxCiOptMB.h"

#include <set>

namespace simu5g {

class MixedScheduler : public virtual LteScheduler {
public:
    MixedScheduler(Binder *binder);
    MixedScheduler(Binder *binder, Direction dir, SchedDiscipline discipline_, double PFAlpha);
    virtual ~MixedScheduler() override;

    // main scheduler hooks
    void prepareSchedule(bool static_scheduling = false) override;
    void commitSchedule() override;

    // active connection management (keeps main activeConnectionSet_ updated)
    virtual void notifyActiveConnection(MacCid cid) override;
    virtual void removeActiveConnection(MacCid cid);

protected:
    // Sub-schedulers — MixedScheduler owns them and will call them unchanged.
    TSNScheduler *staticAllocation;
    LteScheduler *dynAllocation;

    SchedDiscipline discipline_dyn;

    // helper: inject common context into sub-schedulers to avoid uninitialized fields
    void injectCommonContext();
};

} // namespace simu5g

#endif /* STACK_MAC_SCHEDULING_MODULES_MIXEDSCHEDULER_H_ */
