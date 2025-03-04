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

#ifndef _NRMACUED2D_H_
#define _NRMACUED2D_H_

#include "stack/mac/layer/LteMacUeD2D.h"

class NRMacUe : public LteMacUeD2D
{


  protected:

    bool periodic_transmission_started;
    /**
     * Main loop
     */
    virtual void handleSelfMessage() override;
    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    virtual void macPduMake(MacCid cid=0) override;
    virtual void macPduMakeFdd(MacCid cid=0);
    virtual void macPduMakeTdd(MacCid cid=0);


    virtual SlotType defineSlotType();

    virtual void initialize(int stage) override;

    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void macHandleGrant(cPacket* pktAux) override;

    virtual void handleSelfMessageFdd();
    virtual void handleSelfMessageTdd();
    virtual int macSduRequestFdd();
    virtual int macSduRequestTdd();

    virtual void updateUserTxParam(cPacket* pktAux) override;
    virtual void macHandleRac(cPacket* pktAux) override;

    /*
     * Checks RAC status
     */
    virtual void checkRAC() override;

    // current slot number
    int slot_nr;

    SlotType cur_slot_type;
    bool tdd;
    bool bsr_already_sent;
    bool rrc_connected = false; // should be fixed later --> set rrc_connected after RACH procedure, check when rrc_connected is left

    // definition of scheduling Grant map to consider the slot offset
    std::map<int, std::map<double, inet::IntrusivePtr<const LteSchedulingGrant> > > schedulingGrantOffsetMap;
    std::map<int, std::map<double, unsigned int>>  expirationCounterOffsetMap;


  public:
    NRMacUe();
    virtual ~NRMacUe();
    virtual const LteSchedulingGrant* getSchedulingGrant(double carrierFrequency) override;
    virtual void flushHarqBuffers() override;
};

#endif
