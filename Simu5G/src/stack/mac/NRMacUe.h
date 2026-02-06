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

#include "stack/mac/LteMacUeD2D.h"

namespace simu5g {

class NRMacUe : public LteMacUeD2D
{

  protected:
    // offset, Carrier frequency
    std::map<int, std::map<double, inet::IntrusivePtr<const LteSchedulingGrant> > >  schedulingGrantOffsetMap;
    std::map<double, std::vector<ConfiguredGrant>> configuredGrantMap;

    // Periodic grants are stored separately and never erased until expiration

    int slot_nr;
    SlotType cur_slot_type;
    bool tdd;
    bool bsr_already_sent;
    bool pdu_session_established = false; 
    int hp_cycle_max = -1;
    int hp_cycle_curr = 0;
    

    virtual SlotType defineSlotType();


    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void macHandleGrant(cPacket* pktAux) override;

    /**
     * Main loop
     */
    void handleSelfMessage() override;
    void handleSelfMessageFdd();
    void handleSelfMessageTdd();

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    int macSduRequestTdd();
    int macSduRequestFdd();

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    void macPduMake(MacCid cid = 0) override;
    void macPduMakeFdd(MacCid cid = 0);
    void macPduMakeTdd(MacCid cid = 0);


    virtual void checkRAC() override;
    virtual void updateUserTxParam(cPacket* pktAux) override;
    virtual void macHandleRac(cPacket* pktAux) override;

    void applyGrant(inet::IntrusivePtr<const LteSchedulingGrant> grant, double carrierFreq, int slotNr);

    bool compareCGs(const inet::IntrusivePtr<const LteSchedulingGrant>& a, const inet::IntrusivePtr<const LteSchedulingGrant>& b) const;

  public:
    virtual const LteSchedulingGrant* getSchedulingGrant(double carrierFrequency) override;
    virtual void flushHarqBuffers() override;
};

} //namespace

#endif

