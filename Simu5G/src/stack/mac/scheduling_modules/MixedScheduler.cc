#include "stack/mac/scheduling_modules/MixedScheduler.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include <omnetpp.h>

using namespace omnetpp;

namespace simu5g {

MixedScheduler::MixedScheduler(Binder *binder)
    : LteScheduler(binder),
      staticAllocation(nullptr),
      dynAllocation(nullptr)
{
    // Create sub-schedulers with the current direction_ (may be default/unset)
}

MixedScheduler::MixedScheduler(Binder *binder, Direction dir, SchedDiscipline discipline_, double PFAlpha)
    : LteScheduler(binder)
{
    this->direction_ = dir;
    staticAllocation = new TSNScheduler(binder, direction_);
    EV << "MixedScheduler " << discipline_ << endl;
   // dynAllocation = new DQos(binder_, direction_);

    switch (discipline_) {
        case DRR:
            dynAllocation = new LteDrr(binder_);
            break;
        case PF:
            dynAllocation = new LtePf(binder_, PFAlpha);
            break;
        case MAXCI:
            dynAllocation = new LteMaxCi(binder_);
            break;
        case MAXCI_MB:
            dynAllocation = new LteMaxCiMultiband(binder_);
            break;
        case MAXCI_OPT_MB:
            dynAllocation = new LteMaxCiOptMB(binder_);
            break;
        case MAXCI_COMP:
            dynAllocation = new LteMaxCiComp(binder_);
            break;
        case ALLOCATOR_BESTFIT:
            dynAllocation = new LteAllocatorBestFit(binder_);
            break;
        case DQOS:
            dynAllocation = new DQos(binder_, direction_);
            break;
        default:
            throw cRuntimeError("LteScheduler not recognized");
            break;
    }
}

MixedScheduler::~MixedScheduler() {
    delete staticAllocation;
    delete dynAllocation;
}

void MixedScheduler::injectCommonContext() {
    // Propagate pointers/values that sub-schedulers expect to be set.
    if (staticAllocation) {
        staticAllocation->mac_ = this->mac_;
        staticAllocation->eNbScheduler_ = this->eNbScheduler_;
        staticAllocation->direction_ = this->direction_;
        staticAllocation->carrierFrequency_ = this->carrierFrequency_;
        staticAllocation->bandLimit_ = this->bandLimit_;
        staticAllocation->slotReqGrantBandLimit_ = this->slotReqGrantBandLimit_;
    }
    if (dynAllocation) {
        dynAllocation->mac_ = this->mac_;
        dynAllocation->eNbScheduler_ = this->eNbScheduler_;
        dynAllocation->direction_ = this->direction_;
        dynAllocation->carrierFrequency_ = this->carrierFrequency_;
        dynAllocation->bandLimit_ = this->bandLimit_;
        dynAllocation->slotReqGrantBandLimit_ = this->slotReqGrantBandLimit_;
    }
}

void MixedScheduler::prepareSchedule(bool static_scheduling) {
    EV << "MixedScheduler::prepareSchedule()" << endl;
    injectCommonContext();
    // Run TSN first (preallocation)
    if (staticAllocation && static_scheduling) {
        EV << "MixedScheduler: running TSNScheduler::prepareSchedule()" << endl;
        staticAllocation->prepareSchedule();
    }
    else{
        // Make sure sub-schedulers have their required context to avoid garbage values.
        // DQoS should work only on the active dynamic flows that requested grants.
        // activeConnectionSet_ is owned by the base LteScheduler; pass it directly.
        dynAllocation->activeConnectionSet_ = this->activeConnectionSet_;
        dynAllocation->carrierActiveConnectionSet_ = this->carrierActiveConnectionSet_;
        dynAllocation->activeConnectionTempSet_ = this->activeConnectionTempSet_;


        // Run DQoS afterwards, only if there are active dynamic flows
        if (dynAllocation) {
            if (activeConnectionSet_ && !activeConnectionSet_->empty()) {
                EV << "MixedScheduler: running DQos::prepareSchedule()" << endl;
                dynAllocation->prepareSchedule();
            } else {
                EV << "MixedScheduler: no active dynamic connections for DQos" << endl;
            }
        }
    }
}

void MixedScheduler::commitSchedule() {
    EV << "MixedScheduler::commitSchedule() - committing TSN then DQos" << endl;

    // First commit TSN's preallocations (higher-priority / reserved)
    if (staticAllocation) {
        EV << "MixedScheduler: committing staticAllocation" << endl;
        staticAllocation->commitSchedule();
    }

    // Then commit DQoS allocations (they will operate on remaining resources).
    if (dynAllocation) {
        EV << "MixedScheduler: committing dynAllocation" << endl;
        dynAllocation->commitSchedule();
    }
}

void MixedScheduler::notifyActiveConnection(MacCid cid) {
    EV << "MixedScheduler::notifyActiveConnection"<< cid << endl;
    if (activeConnectionSet_) activeConnectionSet_->insert(cid);
    // No need to modify TSN internals — TSN has its own preallocations.
}

void MixedScheduler::removeActiveConnection(MacCid cid) {
    if (activeConnectionSet_) activeConnectionSet_->erase(cid);
    // TSN flows are independent; if you need to remove TSN flows too, handle that inside TSNScheduler config.
}

} // namespace simu5g
