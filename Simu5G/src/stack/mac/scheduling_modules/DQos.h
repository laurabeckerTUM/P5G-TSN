/*
 * DQos.h
 *
 *  Created on: Apr 23, 2023
 *      Author: devika
 */




#ifndef _LTE_DQOS_H_
#define _LTE_DQOS_H_

#include "stack/mac/scheduler/LteScheduler.h"

namespace simu5g {

class DQos : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    std::map<MacCid, unsigned int> grantedBytes_;
    Direction dir;

  public:
    DQos(Binder *binder) : LteScheduler(binder) {}
    DQos(Binder *binder, Direction dir) : LteScheduler(binder), dir(dir) {}

    virtual void prepareSchedule();

    virtual void commitSchedule();

    virtual void notifyActiveConnection(MacCid cid);

    virtual void removeActiveConnection(MacCid cid);

};

} // namespace

#endif // _LTE_DQOS_H_
