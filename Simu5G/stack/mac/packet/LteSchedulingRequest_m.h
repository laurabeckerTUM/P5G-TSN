//
// Generated file, do not edit! Created by opp_msgtool 6.0 from stack/mac/packet/LteSchedulingRequest.msg.
//

#ifndef __LTESCHEDULINGREQUEST_M_H
#define __LTESCHEDULINGREQUEST_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// opp_msgtool version check
#define MSGC_VERSION 0x0600
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgtool: 'make clean' should help.
#endif

class LteSchedulingRequest;
#include "LteRac_m.h" // import LteRac

/**
 * Class generated from <tt>stack/mac/packet/LteSchedulingRequest.msg:20</tt> by opp_msgtool.
 * <pre>
 * class LteSchedulingRequest extends LteRac
 * {
 * }
 * </pre>
 */
class LteSchedulingRequest : public ::LteRac
{
  protected:

  private:
    void copy(const LteSchedulingRequest& other);

  protected:
    bool operator==(const LteSchedulingRequest&) = delete;

  public:
    LteSchedulingRequest();
    LteSchedulingRequest(const LteSchedulingRequest& other);
    virtual ~LteSchedulingRequest();
    LteSchedulingRequest& operator=(const LteSchedulingRequest& other);
    virtual LteSchedulingRequest *dup() const override {return new LteSchedulingRequest(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const LteSchedulingRequest& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, LteSchedulingRequest& obj) {obj.parsimUnpack(b);}


namespace omnetpp {

template<> inline LteSchedulingRequest *fromAnyPtr(any_ptr ptr) { return check_and_cast<LteSchedulingRequest*>(ptr.get<cObject>()); }

}  // namespace omnetpp

#endif // ifndef __LTESCHEDULINGREQUEST_M_H

