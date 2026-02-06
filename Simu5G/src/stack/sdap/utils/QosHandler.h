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

#ifndef __SIMU5G_QOSHANDLER_H_
#define __SIMU5G_QOSHANDLER_H_

#include <omnetpp.h>
#include "common/LteCommonEnum_m.h"
#include "common/LteCommon.h"
#include "common/NrCommon.h"
#include "common/LteControlInfo_m.h"
#include "common/binder/GlobalData.h"

using namespace omnetpp;

namespace simu5g {

/**
 * TODO - Generated class
 */
struct QosInfo {
    QosInfo(){};
    QosInfo(Direction dir) {
        this->dir = dir;
    };
    MacNodeId destNodeId;
    MacNodeId senderNodeId;
    inet::Ipv4Address senderAddress;
    inet::Ipv4Address destAddress;
    ApplicationType appType;
    LteTrafficClass trafficClass;
    unsigned short rlcType;
    unsigned short lcid = 0;
    unsigned short qfi = 0;
    //unsigned short _5Qi = 0;
    unsigned short radioBearerId = 0;
    unsigned int cid = 0;
    bool containsSeveralCids = false;
    Direction dir;
};
class QosHandler : public cSimpleModule
{
public:
	QosHandler() {
	}

	virtual ~QosHandler() {
	}

	virtual RanNodeType getNodeType() {
		Enter_Method_Silent("getNodeType");
		return nodeType;
	}


	virtual std::map<MacCid, QosInfo>& getQosInfo() {
		Enter_Method_Silent("getQosInfo");
		return QosInfos;
	}

	virtual void insertQosInfo(MacCid cid, QosInfo info){
		Enter_Method_Silent("insertQosInfo");
		QosInfos[cid] = info;
	}

    virtual unsigned short getLcid(MacNodeId nodeId, unsigned short msgCat) {
    	Enter_Method_Silent("getLcid");
        for (auto const & var : QosInfos) {
            if (var.second.destNodeId == nodeId && var.second.appType == msgCat) {
                return var.second.lcid;
            }
        }
        return 0;
    }

    virtual void deleteNode(MacNodeId nodeId) {
    	Enter_Method_Silent("deleteNode");
        std::vector<unsigned int> tmp;
        for (auto const & var : QosInfos) {
            if (var.second.destNodeId == nodeId
                    || var.second.senderNodeId == nodeId) {
                tmp.push_back(var.first);
            }
        }

        for (auto const & var : tmp) {
            QosInfos.erase(var);
        }
    }

    virtual std::vector<QosInfo> getAllQosInfos(MacNodeId nodeId) {
		Enter_Method_Silent("getAllQosInfos");
		std::vector<QosInfo> tmp;
		for (auto const &var : QosInfos) {
			if (var.second.destNodeId == nodeId || var.second.senderNodeId == nodeId) {
				if (var.second.lcid != 0)
					tmp.push_back(var.second);
			}
		}
		return tmp;
	}

    virtual int numInitStages() const override {
        return 2;
    }


	//sorts the qosInfos by its lcid value (from smallest to highest)
	virtual std::vector<std::pair<MacCid, QosInfo>> getSortedQosInfos(Direction dir) {
		Enter_Method_Silent("getSortedQosInfos");
		std::vector<std::pair<unsigned int, QosInfo>> tmp;
		for (auto var : QosInfos) {
			if (var.second.lcid != 0 && var.second.lcid != 10 && var.second.dir == dir)
				tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return a.second.lcid > b.second.lcid;
		});
		return tmp;
	}

	//sort by priority from 23.501
	virtual std::vector<std::pair<MacCid, QosInfo>> getPrioritySortedQosInfos(Direction dir) {
		Enter_Method_Silent("getPrioritySortedQosInfos");
		std::vector<std::pair<MacCid, QosInfo>> tmp;
		for (auto &var : QosInfos) {
			if (dir == UL && MacCidToLcid(var.first) != 10 && var.second.dir == dir)
				tmp.push_back(var);
			else if (MacCidToLcid(var.first) != 0 && MacCidToLcid(var.first) != 10 && var.second.dir == dir)
				tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return (getPriority(get5Qi(a.second.qfi)) > getPriority(get5Qi(b.second.qfi)));
		});
		return tmp;
	}

	//sort by pdb from 23.501
	virtual std::vector<std::pair<MacCid, QosInfo>> getPdbSortedQosInfos(Direction dir) {
		Enter_Method_Silent("getPdbSortedQosInfos");
		std::vector<std::pair<MacCid, QosInfo>> tmp;
		for (auto &var : QosInfos) {
			if (dir == UL && MacCidToLcid(var.first) != 10 && var.second.dir == dir)
				tmp.push_back(var);
			else if (MacCidToLcid(var.first) != 0 && MacCidToLcid(var.first) != 10 && var.second.dir == dir)
				tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return (getPdb(get5Qi(a.second.qfi)) > getPdb(get5Qi(b.second.qfi)));
		});
		return tmp;
	}


	//prio --> priority, pdb, weight
	//
	virtual std::map<double, std::vector<QosInfo>> getEqualPriorityMap(Direction dir) {
		Enter_Method_Silent("getEqualPriorityMap");

		double lambdaPriority = getSimulation()->getSystemModule()->par("lambdaPriority").doubleValue();
		double lambdaRemainDelayBudget = getSimulation()->getSystemModule()->par("lambdaRemainDelayBudget").doubleValue();
		double lambdaCqi = getSimulation()->getSystemModule()->par("lambdaCqi").doubleValue();
		double lambdaByteSize = getSimulation()->getSystemModule()->par("lambdaByteSize").doubleValue();
		double lambdaRtx = getSimulation()->getSystemModule()->par("lambdaRtx").doubleValue();

		std::string prio = "default";
		if (getSimulation()->getSystemModule()->hasPar("qosModelPriority")) {
			prio = getSimulation()->getSystemModule()->par("qosModelPriority").stdstringValue();

			/*if(lambdaPriority == 1 && lambdaByteSize == 0 && lambdaCqi == 0 && lambdaRemainDelayBudget == 0 && lambdaRtx == 0){
				prio = "priority";
			}else if(lambdaPriority == 0 && lambdaByteSize == 0 && lambdaCqi == 0 && lambdaRemainDelayBudget == 1 && lambdaRtx == 0){
				prio = "pdb";
			}else{
				prio = "default";
			}*/
		}

		//first item is the (calculated prio), second item is the cid
		std::map<double, std::vector<QosInfo>> retValue;

		if (prio == "priority") {
			//use priority from table in 23.501, qos characteristics

			//get the priorityMap
			std::vector<std::pair<MacCid, QosInfo>> tmp = getPrioritySortedQosInfos(dir);

			for (auto &var : tmp) {
				double prio = getPriority(get5Qi(getQfi(var.first)));
				retValue[prio].push_back(var.second);
			}

			return retValue;

		} else if (prio == "pdb") {
			//use packet delay budget from table in 23.501, a shorter pdb gets a higher priority

			//get the priorityMap
			std::vector<std::pair<MacCid, QosInfo>> tmp = getPdbSortedQosInfos(dir);

			//find out the pdb

			for (auto &var : tmp) {
				double prio = getPdb(get5Qi(getQfi(var.first)));
				retValue[prio].push_back(var.second);
			}

			return retValue;

		} else if (prio == "weight") {
			//calculate a weight by priority * pdb * per (values from 23.501)

			//get the priorityMap
			//consider the pdb value as above --> ensure that the most urgent one gets highest prio
			std::vector<std::pair<MacCid, QosInfo>> tmp = getPrioritySortedQosInfos(dir);

			for (auto &var : tmp) {
				double prio = getPriority(get5Qi(getQfi(var.first)));
				double pdb = getPdb(get5Qi(getQfi(var.first)));
				//double per = getPer(getQfi(var.first));
				double weight = prio * pdb;

				retValue[weight].push_back(var.second);
			}

			return retValue;

		} else if (prio == "default") {
			//by lcid
			std::vector<std::pair<MacCid, QosInfo>> tmp = getSortedQosInfos(dir);

			for (auto &var : tmp) {
				double lcid = var.second.lcid;
				retValue[lcid].push_back(var.second);
			}
			return retValue;

		} else {
			throw cRuntimeError("Error - QosHandler::getEqualPriorityMap - unknown priority");
		}

	}

	virtual void clearQosInfos(){
        QosInfos.clear();
    }

    virtual void modifyControlInfo(LteControlInfo * info)
    {

    	Enter_Method_Silent("modifyControlInfo");
        info->setApplication(0);
        MacCid newMacCid = idToMacCid(MacCidToNodeId(info->getCid()),0);
        info->setCid(newMacCid);
        info->setLcid(0);
        info->setQfi(0);
        info->setTraffic(0);
        info->setContainsSeveralCids(true);
        QosInfo tmp((Direction)info->getDirection());
        tmp.appType = (ApplicationType)0;
        tmp.cid = newMacCid;
        tmp.containsSeveralCids = true;
        tmp.destNodeId = info->getDestId();
        tmp.senderNodeId = info->getSourceId();
        tmp.lcid = 0;
        tmp.trafficClass = (LteTrafficClass)0;
        QosInfos[newMacCid] = tmp;

    }

    virtual ApplicationType getApplicationType(MacCid cid) {
    	Enter_Method_Silent("getApplicationType");
        for (auto & var : QosInfos) {
            if (var.first == cid)
                return (ApplicationType) var.second.appType;
        }
        return (ApplicationType)0;
    }


    virtual unsigned short getQfi(MacCid cid) {
    	Enter_Method_Silent("getQfi");
        for (auto & var : QosInfos) {
            if (var.first == cid)
                return var.second.qfi;
        }
        return 0;
    }


    //type is the application type V2X, VOD, VOIP, DATA_FLOW
    //returns the QFI which is pre-configured in QosHandler.ned file
    virtual unsigned short getQfi(ApplicationType type){
    	Enter_Method_Silent("getQfi");
        switch(type){
        case VOD:
            return videoQfi;
        case VOIP:
            return voipQfi;
        case CBR:
            return dataQfi;
        case NETWORK_CONTROL:
            return networkControlQfi;
		case TSN0:
			return tsn0Qfi;
		case TSN1:
			return tsn1Qfi;
		case TSN2:
			return tsn2Qfi;
		case TSN3:
			return tsn3Qfi;
		case TSN4:
			return tsn4Qfi;
		case TSN5:
			return tsn5Qfi;
		case TSN6:
			return tsn6Qfi;
		case TSN7:
			return tsn7Qfi;
        default:
        	return dataQfi;
        }
    }
    virtual unsigned short getFlowQfi(std::string pktName){
        Enter_Method_Silent("getQfi value");
        auto qfi = 1;
		if (pktName.find("tsn0")==0)
			return tsn0Qfi;
		else if (pktName.find("tsn1")==0)
			return tsn1Qfi;
		else if (pktName.find("tsn2")==0)
			return tsn2Qfi;
		else if (pktName.find("tsn3")==0)
			return tsn3Qfi;
		else if (pktName.find("tsn4")==0)
			return tsn4Qfi;
		else if (pktName.find("tsn5")==0)
			return tsn5Qfi;
		else if (pktName.find("tsn6")==0)
			return tsn6Qfi;
		else if (pktName.find("tsn7")==0)
			return tsn7Qfi;
        else if ((pktName.find("VoIP") == 0) || (pktName.find("audio") == 0)){
            return voipQfi;
        }
        else if ((pktName.find("video") == 0) || (pktName.find("VOD") == 0)){
            return videoQfi;
        }
        else if (pktName.find("gaming") == 0){
            return dataQfi;
        }
        else if ((pktName.find("network control") == 0) || (pktName.find("networkControl") == 0)){
            return networkControlQfi;
        }
        else return qfi;

    }

	virtual unsigned short get5Qi(unsigned short qfi) {

		Enter_Method_Silent("get5Qi");
		 // TSN extensions
        if (qfi == tsn0Qfi) {
            return tsn0_5Qi;
        } else if (qfi == tsn1Qfi) {
            return tsn1_5Qi;
        } else if (qfi == tsn2Qfi) {
            return tsn2_5Qi;
        } else if (qfi == tsn3Qfi) {
            return tsn3_5Qi;
        } else if (qfi == tsn4Qfi) {
            return tsn4_5Qi;
        } else if (qfi == tsn5Qfi) {
            return tsn5_5Qi;
        } else if (qfi == tsn6Qfi) {
            return tsn6_5Qi;
        } else if (qfi == tsn7Qfi) {
            return tsn7_5Qi;
        } else if (qfi == v2xQfi) {
			return v2x5Qi;
		} else if (qfi == videoQfi) {
			return video5Qi;
		} else if (qfi == voipQfi) {
			return voip5Qi;
		} else if (qfi == dataQfi) {
			return data5Qi;
		}
		else if (qfi == networkControlQfi){
		    return networkControl5Qi;
		}
		else {
			return data5Qi;
		}
	}

    virtual unsigned short getRadioBearerId(ApplicationType type){
    	Enter_Method_Silent("getRadioBearerId");
    	switch(type){
    	        case VOD:
    	            return videoQfiToRadioBearer;
    	        case VOIP:
    	            return voipQfiToRadioBearer;
    	        case CBR:
    	            return dataQfiToRadioBearer;
    	        case NETWORK_CONTROL:
    	            return networkControlQfiToRadioBearer;
    	        case GAMING:
    	            return gamingQfiToRadioBearer;
				// TSN extensions
				case TSN0:
					return tsn0QfiToRadioBearer;
				case TSN1:
					return tsn1QfiToRadioBearer;
				case TSN2:
					return tsn2QfiToRadioBearer;
				case TSN3:
					return tsn3QfiToRadioBearer;
				case TSN4:
					return tsn4QfiToRadioBearer;
				case TSN5:
					return tsn5QfiToRadioBearer;
				case TSN6:
					return tsn6QfiToRadioBearer;
				case TSN7:
					return tsn7QfiToRadioBearer;
    	        default:
    	            return dataQfiToRadioBearer;
    	        }

    }

	virtual void initQfiParams() {
		Enter_Method_Silent("initQfiParams");
		this->v2xQfi = par("v2xQfi");
		this->videoQfi = par("videoQfi");
		this->voipQfi = par("voipQfi");
		this->dataQfi = par("dataQfi");
		this->networkControlQfi = par("networkControlQfi");

		this->v2x5Qi = par("v2x5Qi");
		this->video5Qi = par("video5Qi");
		this->voip5Qi = par("voip5Qi");
		this->data5Qi = par("data5Qi");
		this->networkControl5Qi = par("networkControl5Qi");

		this->gamingQfiToRadioBearer = par("gamingQfiToRadioBearer");
		this->videoQfiToRadioBearer = par("videoQfiToRadioBearer");
		this->voipQfiToRadioBearer = par("voipQfiToRadioBearer");
		this->dataQfiToRadioBearer = par("dataQfiToRadioBearer");
		this->networkControlQfiToRadioBearer = par("networkControlQfiToRadioBearer");

		 // TSN extensions
        this->tsn0Qfi = par("tsn0Qfi");
        this->tsn1Qfi = par("tsn1Qfi");
        this->tsn2Qfi = par("tsn2Qfi");
        this->tsn3Qfi = par("tsn3Qfi");
        this->tsn4Qfi = par("tsn4Qfi");
        this->tsn5Qfi = par("tsn5Qfi");
        this->tsn6Qfi = par("tsn6Qfi");
        this->tsn7Qfi = par("tsn7Qfi");

        this->tsn0_5Qi = par("tsn0_5Qi");
        this->tsn1_5Qi = par("tsn1_5Qi");
        this->tsn2_5Qi = par("tsn2_5Qi");
        this->tsn3_5Qi = par("tsn3_5Qi");
        this->tsn4_5Qi = par("tsn4_5Qi");
        this->tsn5_5Qi = par("tsn5_5Qi");
        this->tsn6_5Qi = par("tsn6_5Qi");
        this->tsn7_5Qi = par("tsn7_5Qi");

        this->tsn0QfiToRadioBearer = par("tsn0QfiToRadioBearer");
        this->tsn1QfiToRadioBearer = par("tsn1QfiToRadioBearer");
        this->tsn2QfiToRadioBearer = par("tsn2QfiToRadioBearer");
        this->tsn3QfiToRadioBearer = par("tsn3QfiToRadioBearer");
        this->tsn4QfiToRadioBearer = par("tsn4QfiToRadioBearer");
        this->tsn5QfiToRadioBearer = par("tsn5QfiToRadioBearer");
        this->tsn6QfiToRadioBearer = par("tsn6QfiToRadioBearer");
        this->tsn7QfiToRadioBearer = par("tsn7QfiToRadioBearer");
	}


    virtual double getCharacteristic(std::string characteristic, unsigned short _5Qi) {
    	Enter_Method_Silent("getCharacteristic");
		QosCharacteristic qosCharacteristics = NRQosCharacteristics::getNRQosCharacteristics()->getQosCharacteristic(_5Qi);

		if (characteristic == "PDB") {
			return qosCharacteristics.getPdb();
		} else if (characteristic == "PER") {
			return qosCharacteristics.getPer();
		} else if (characteristic == "PRIO") {
			return qosCharacteristics.getPriorityLevel();
		} else {
			throw cRuntimeError("Unknown QoS characteristic");
		}
	}


    virtual unsigned short getPriority(unsigned short _5Qi){
    	Enter_Method_Silent("getPriority");
    	return getCharacteristic("PRIO", _5Qi);
    }

    virtual double getPdb(unsigned short _5Qi){
    	Enter_Method_Silent("getPdb");
    	return getCharacteristic("PDB", _5Qi);
    }

    virtual double getPer(unsigned short _5Qi){
    	Enter_Method_Silent("getPer");
    	return getCharacteristic("PER", _5Qi);
    }

    virtual ResourceType getResType(unsigned short _5Qi) {
        QosCharacteristic qosCharacteristics = NRQosCharacteristics::getNRQosCharacteristics()->getQosCharacteristic(_5Qi);
        Enter_Method_Silent("getResType");
        return qosCharacteristics.getResType();
    }

    virtual int getQfiFromPcp(std::string pktName){
        auto mappingTable = globalData->getQosMappingTable();
        auto found = -1;
        for (auto& item: mappingTable){
            found = pktName.find(item.first);
            if (found != std::string::npos){
                return std::stoi(item.second.pcp);
              }
        }
        EV<<"Packet not configured!!";
        return -1;
    }

protected:
    RanNodeType nodeType;
    std::map<MacCid, QosInfo> QosInfos;

    unsigned short v2xQfi;
    unsigned short videoQfi;
    unsigned short voipQfi;
    unsigned short dataQfi;
    unsigned short networkControlQfi;

    unsigned short v2x5Qi;
    unsigned short video5Qi;
    unsigned short voip5Qi;
    unsigned short data5Qi;
    unsigned short networkControl5Qi;

    unsigned short gamingQfiToRadioBearer;
    unsigned short videoQfiToRadioBearer;
    unsigned short voipQfiToRadioBearer;
    unsigned short dataQfiToRadioBearer;
    unsigned short networkControlQfiToRadioBearer;

	 // TSN extensions
    unsigned short tsn0Qfi;
    unsigned short tsn1Qfi;
    unsigned short tsn2Qfi;
    unsigned short tsn3Qfi;
    unsigned short tsn4Qfi;
    unsigned short tsn5Qfi;
    unsigned short tsn6Qfi;
    unsigned short tsn7Qfi;

    unsigned short tsn0_5Qi;
    unsigned short tsn1_5Qi;
    unsigned short tsn2_5Qi;
    unsigned short tsn3_5Qi;
    unsigned short tsn4_5Qi;
    unsigned short tsn5_5Qi;
    unsigned short tsn6_5Qi;
    unsigned short tsn7_5Qi;

    unsigned short tsn0QfiToRadioBearer;
    unsigned short tsn1QfiToRadioBearer;
    unsigned short tsn2QfiToRadioBearer;
    unsigned short tsn3QfiToRadioBearer;
    unsigned short tsn4QfiToRadioBearer;
    unsigned short tsn5QfiToRadioBearer;
    unsigned short tsn6QfiToRadioBearer;
    unsigned short tsn7QfiToRadioBearer;

    GlobalData *globalData;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
class QosHandlerUE: public QosHandler {

protected:
    using QosHandler::initialize;
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};

class QosHandlerGNB: public QosHandler {

protected:
    using QosHandler::initialize;
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};

class QosHandlerUPF: public QosHandler {

protected:
    using QosHandler::initialize;
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};

} // namespace

#endif
