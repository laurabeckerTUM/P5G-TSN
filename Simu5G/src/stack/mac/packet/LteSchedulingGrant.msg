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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

//
// TODO generated message class
//
class LteSchedulingGrant extends inet::FieldsChunk
{
    @customize(true);  // see the generated C++ header for more info
    chunkLength = inet::B(1); // TODO: size 0
    int SlotOffset;

    bool periodic;
    int period;
    int HPCycle;
    unsigned int expiration;
    bool active;
    int HPCycleMax;
    unsigned int mcsIndex = -1;

    // blocks granted on all Remotes, all Bands
    unsigned int totalGrantedBlocks;
    // number of codewords
    unsigned int codewords;
    // granted bytes per codeword
    abstract unsigned int grantedCwBytes[];

    
    //#  in LteSchedulingGrant.h
    //# const UserTxParams* userTxParams;
    //# RbMap grantedBlocks;
    //#
}
