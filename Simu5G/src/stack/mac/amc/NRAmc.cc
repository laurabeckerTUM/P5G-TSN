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

#include "stack/mac/amc/NRAmc.h"

namespace simu5g {

using namespace std;
using namespace omnetpp;

/********************
* PUBLIC FUNCTIONS
********************/

NRAmc::NRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas)
    : LteAmc(mac, binder, cellInfo, numAntennas)
{
}


unsigned int NRAmc::getSymbolsPerSlot(double carrierFrequency, Direction dir)
{
    unsigned totSymbols = 14;   // TODO get this parameter from CellInfo/Carrier

    SlotFormat sf = binder_->getSlotFormat(carrierFrequency);

    if (!sf.tdd)
        return totSymbols;
    else if (dir == DL){
        if (cellInfo_->getCurrentSlotType() == DL_SLOT)
            return totSymbols;
        else if (cellInfo_->getCurrentSlotType() == SHARED_SLOT)
            return sf.numDlSymbols;
    }
    else if (dir == UL){
        if (cellInfo_->getSlotTypeForULSchedule() == UL_SLOT){
            return totSymbols;
        }    
        else if (cellInfo_->getSlotTypeForULSchedule() == SHARED_SLOT){
            return sf.numUlSymbols;
        }
    }
    return 0;
}


unsigned int NRAmc::getResourceElementsPerBlock(unsigned int symbolsPerSlot)
{
    unsigned int numSubcarriers = 12;   // TODO get this parameter from CellInfo/Carrier
    unsigned int reSignal = 1;
    unsigned int nOverhead = 0;

    if (symbolsPerSlot == 0)
        return 0;
    return (numSubcarriers * symbolsPerSlot) - reSignal - nOverhead;
}

unsigned int NRAmc::getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot)
{
    unsigned int numRePerBlock = getResourceElementsPerBlock(symbolsPerSlot);

    if (numRePerBlock > 156)
        return 156 * blocks;

    return numRePerBlock * blocks;
}

unsigned int NRAmc::computeTbsFromNinfo(double nInfo, double coderate)
{
    unsigned int tbs = 0;
    unsigned int _nInfo = 0;
    unsigned int n = 0;
    if (nInfo == 0)
        return 0;

    if (nInfo <= 3824) {
        n = std::max((int)3, (int)(floor(log2(nInfo) - 6)));
        _nInfo = std::max((unsigned int)24, (unsigned int)((1 << n) * floor(nInfo / (1 << n))));

        // get tbs from table
        unsigned int j = 0;
        for (j = 0; j < TBSTABLESIZE - 1; j++) {
            if (nInfoToTbs[j] >= _nInfo)
                break;
        }

        tbs = nInfoToTbs[j];
    }
    else {
        unsigned int C;
        n = floor(log2(nInfo - 24) - 5);
        _nInfo = (1 << n) * round((nInfo - 24) / (1 << n));
        if (coderate <= 0.25) {
            C = ceil((_nInfo + 24) / 3816);
            tbs = 8 * C * ceil((_nInfo + 24) / (8 * C)) - 24;
        }
        else {
            if (_nInfo >= 8424) {
                C = ceil((_nInfo + 24) / 8424);
                tbs = 8 * C * ceil((_nInfo + 24) / (8 * C)) - 24;
            }
            else {
                tbs = 8 * ceil((_nInfo + 24) / 8) - 24;
            }
        }
    }
    return tbs;
}

unsigned int NRAmc::computeCodewordTbs(UserTxParams *info, Codeword cw, Direction dir, unsigned int numRe, int minMcsIndex, int staticMcsIndex)
{
    std::vector<unsigned char> layers = info->getLayers();
    NRMCSelem mcsElem = getMcsElemPerCqi(info->readCqiVector().at(cw), dir, minMcsIndex, staticMcsIndex);
    unsigned int modFactor;
    switch (mcsElem.mod_) {
        case _QPSK:   modFactor = 2;
            break;
        case _16QAM:  modFactor = 4;
            break;
        case _64QAM:  modFactor = 6;
            break;
        case _256QAM: modFactor = 8;
            break;
        default: throw cRuntimeError("NRAmc::computeCodewordTbs - unrecognized modulation.");
    }
    double coderate = mcsElem.coderate_ / 1024;
    double nInfo = numRe * coderate * modFactor * layers.at(cw);

    return computeTbsFromNinfo(floor(nInfo), coderate);
}

/*******************************************
*      Scheduler interface functions      *
*******************************************/

unsigned int NRAmc::computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency, int minMcsIndex, int staticMcsIndex)
{
    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Node: " << id << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Band: " << b << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Direction: " << dirToA(dir) << "\n";

    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    unsigned int bits = 0;
    unsigned int codewords = info.getLayers().size();
    for (Codeword cw = 0; cw < codewords; ++cw) {
        // if CQI == 0 the UE is out of range, thus bits=0
        if (info.readCqiVector().at(cw) == 0) {
            EV << NOW << " NRAmc::computeBitsOnNRbs - CQI equal to zero on cw " << cw << ", return no blocks available" << endl;
            continue;
        }

        unsigned int tbs = computeCodewordTbs(&info, cw, dir, numRe, minMcsIndex, staticMcsIndex);
        bits += tbs;
    }

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Resource Blocks: " << blocks << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Available space: " << bits << "\n";

    return bits;
}

unsigned int NRAmc::computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency, int minMcsIndex, int staticMcsIndex)
{
    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Node: " << id << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Band: " << b << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Codeword: " << cw << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Direction: " << dirToA(dir) << "\n";

    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    // if CQI == 0 the UE is out of range, thus return 0
    if (info.readCqiVector().at(cw) == 0) {
        EV << NOW << " NRAmc::computeBitsOnNRbs - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }

    unsigned int tbs = computeCodewordTbs(&info, cw, dir, numRe, minMcsIndex, staticMcsIndex);

    // DEBUG
    EV << NOW << " NRAmc::computeBitsOnNRbs Resource Blocks: " << blocks << "\n";
    EV << NOW << " NRAmc::computeBitsOnNRbs Available space: " << tbs << "\n";

    return tbs;
}

unsigned int NRAmc::computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency)
{
    // DEBUG
    EV << NOW << " NRAmc::computeBitsPerRbBackground CQI: " << cqi << " Direction: " << dirToA(dir) << " carrierFrequency: " << carrierFrequency << endl;

    // if CQI == 0 the UE is out of range, thus return 0
    if (cqi == 0) {
        EV << NOW << " NRAmc::computeBitsPerRbBackground - CQI equal to zero, return no bytes available" << endl;
        return 0;
    }

    unsigned int blocks = 1;
    unsigned char layers = 1;

    // compute TBS

    NRMCSelem mcsElem = getMcsElemPerCqi(cqi, dir);
    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));
    unsigned int modFactor;
    switch (mcsElem.mod_) {
        case _QPSK:   modFactor = 2;
            break;
        case _16QAM:  modFactor = 4;
            break;
        case _64QAM:  modFactor = 6;
            break;
        case _256QAM: modFactor = 8;
            break;
        default: throw cRuntimeError("NRAmc::computeCodewordTbs - unrecognized modulation.");
    }
    double coderate = mcsElem.coderate_ / 1024;
    double nInfo = numRe * coderate * modFactor * layers;

    unsigned int tbs = computeTbsFromNinfo(floor(nInfo), coderate);

    EV << NOW << " NRAmc::computeBitsPerRbBackground Available space: " << tbs << "\n";

    return tbs;
}

NRMCSelem NRAmc::getMcsElemPerCqi(Cqi cqi, const Direction dir, int minMcsIndex, int staticMcsIndex)
{
    NRMCSelem ret;
    NRMcsTable *mcsTable;
    if (dir == DL)
        mcsTable = &dlNrMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulNrMcsTable_;
    else {
        throw cRuntimeError("NRAmc::getIMcsPerCqi(): Unrecognized direction");
    }

    if (staticMcsIndex != -1) {
        if (dir == DL){
            ret = dlNrMcsTable_.at(staticMcsIndex);
            ret.index_ = staticMcsIndex;
            return ret;
        }
        else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI)){
            mac_->emit(mac_->selectedMcsSignal_, staticMcsIndex);
            ret = ulNrMcsTable_.at(staticMcsIndex);
            ret.index_ = staticMcsIndex;
            return ret;
        }     
        else
            throw cRuntimeError("NRAmc::getMcsElemPerCqi(): Unrecognized direction");
    }


    CQIelem entry = mcsTable->getCqiElem(cqi);
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the McsTable (extended reporting supported)
    unsigned int min = mcsTable->getMinIndex(mod);
    unsigned int max = mcsTable->getMaxIndex(mod);

    // Initialize the working variables at the minimum value.
    ret = mcsTable->at(min);

    // Search in the McsTable from min to max until the rate exceeds
    // the coderate in an entry of the table.
    int selected_mcs=0;
    for (unsigned int i = min; i <= max; i++) {
        NRMCSelem elem = mcsTable->at(i);
        if (elem.coderate_ <= rate){
            ret = elem;
            selected_mcs = i;
        }
        else{
            // --- Enforce minimum MCS value ---
            if (i < minMcsIndex && minMcsIndex != -1) {
                ret = mcsTable->at(minMcsIndex);
                selected_mcs = minMcsIndex;
            }
            else
                break;
        }
    }
    mac_->emit(mac_->selectedMcsSignal_, selected_mcs);
    ret.index_ = selected_mcs;

    // Return the MCSElem found.
    return ret;
}

} //namespace

