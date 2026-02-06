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

package simu5g.stack.sdap.utils;

//
// TODO auto-generated module
//
moduleinterface IQosHandler
{
}

simple QosHandler like IQosHandler
{
    parameters:
        int v2xQfi = default(1);
        //int v2x5Qi = default(84);//GBR 84
        int v2x5Qi = default(86);//GBR 86

        int voipQfi = default(2);
        int voip5Qi = default(2);//GBR

		int videoQfi = default(3);
        int video5Qi = default(74);//GBR

        int dataQfi = default(4);
        int data5Qi = default(9);//NGBR
   
        
        int networkControlQfi = default(7);
        int networkControl5Qi = default(82);

        int gamingQfiToRadioBearer = default(1);
        int videoQfiToRadioBearer = default(1);
        int voipQfiToRadioBearer = default(1);
        int dataQfiToRadioBearer = default(1);
        int networkControlQfiToRadioBearer = default(1);

        // TSN
        int tsn0_5Qi = default(10);
        int tsn1_5Qi = default(11);
        int tsn2_5Qi = default(12);
        int tsn3_5Qi = default(13);
        int tsn4_5Qi = default(14);
        int tsn5_5Qi = default(15);
        int tsn6_5Qi = default(16);
        int tsn7_5Qi = default(17);


        int tsn0Qfi = default(10);
        int tsn1Qfi = default(11);
        int tsn2Qfi = default(12);
        int tsn3Qfi = default(13);
        int tsn4Qfi = default(14);
        int tsn5Qfi = default(15);
        int tsn6Qfi = default(16);
        int tsn7Qfi = default(17);

        int tsn0QfiToRadioBearer = default(10);
        int tsn1QfiToRadioBearer = default(11);
        int tsn2QfiToRadioBearer = default(12);
        int tsn3QfiToRadioBearer = default(13);
        int tsn4QfiToRadioBearer = default(14);
        int tsn5QfiToRadioBearer = default(15);
        int tsn6QfiToRadioBearer = default(16);
        int tsn7QfiToRadioBearer = default(17);
        @display("i=block/classifier");
}

simple QosHandlerUE extends QosHandler
{
    parameters:
        @class("QosHandlerUE");
        @display("i=block/classifier");

}

simple QosHandlerGNB extends QosHandler
{
    parameters:
        @class("QosHandlerGNB");
        @display("i=block/classifier");

}

simple QosHandlerUPF extends QosHandler
{
    parameters:
        @class("QosHandlerUPF");
        @display("i=block/classifier");

}
