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
import inet.networks.base.TsnNetworkBase;
import inet.node.ethernet.EthernetHost;
import inet.node.ethernet.EthernetSwitch;
import inet.node.inet.StandardHost;
import inet.node.tsn.TsnClock;
import inet.node.tsn.TsnDevice;
import inet.node.tsn.TsnSwitch;
import inet.node.wireless.AccessPoint;
import simu5g.world.radio.LteChannelControl;
import inet.networklayer.ipv4.RoutingTableRecorder;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import inet.visualizer.common.DataLinkVisualizer;
import tsnfivegcomm.DsttUe;
import tsnfivegcomm.Nwtt;
import inet.node.ethernet.Eth1G;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import simu5g.common.binder.GlobalData;

package tsnfivegcomm.simulations.Heterogenous;

network NetworkHeterogenous extends TsnNetworkBase
{
    int numUe = default(1);
    bool useQosModel = default(true);
    bool packetDropEnabled = default(true);
    string qosModelPriority = default("priority");
    double lambdaPriority = default(1);
    double lambdaRemainDelayBudget = default(0);
    double lambdaCqi = default(0);
    double lambdaRtx = default(0);
    double lambdaByteSize = default(0);

    bool tsnEnabled = default(false);
    globalData.config = xmldoc("inputConfig/DsttIpMacConfig.xml");
    configurator*.config = xmldoc("inputConfig/demo.xml");

    @display("bgb=13949,5264");
    submodules:

        globalData: GlobalData {
            @display("p=342.456,1928.568");
        }
        tsnSwitchListener: TsnSwitch {
            @display("p=2252,585");
        }
        Listener: TsnDevice {
            @display("p=748,428");
        }
        channelControl: LteChannelControl {
            @display("p=62.012,789.396;is=s");
        }
        routingTableRecorder: RoutingTableRecorder {
            @display("p=525.19,1155.418");
        }
        binder: Binder {
            @display("p=189.388,881.576;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=63.688,894.984;is=s");
        }
        upf: Upf {
            @display("p=3422,1145");
        }
        gnb: gNodeB {
            @display("p=3410,1916");
        }
        linkVisualizer: DataLinkVisualizer {
            @display("p=126.77,1155.418");
        }
        nwtt: Nwtt {
            parameters:
                *.bridging.typename = "TsnRelayUnit";
                @display("p=3509,498");
        }
        ipvisualizer: IntegratedVisualizer {
            @display("p=170.234,1408.958");
        }
        configuratortwo: Ipv4NetworkConfigurator {
            @display("p=383.932,1615.412");
        }
        // ===== Group 0 =====
        dsttUe0: DsttUe {
            @display("p=4990,1070;is=s");
        }
        dsttUe1: DsttUe {
            @display("p=5550,1070;is=s");
        }
        Talker0: TsnDevice {
            @display("p=3024,3683");
        }
        TASSwitch0: TsnSwitch {
            @display("p=4535,3052");
        }
        SWTalker: TsnSwitch {
            @display("p=5878,3035");
        }
        SWTalker0: TsnSwitch {
            @display("p=4990,2091;is=s");
        }
        SWTalker1: TsnSwitch {
            @display("p=5662,2091;is=s");
        }

        // ===== Group 1 =====
        dsttUe2: DsttUe {
            @display("p=6222,1045;is=s");
        }
        dsttUe3: DsttUe {
            @display("p=7018,1070;is=s");
        }
        SWTalker2: TsnSwitch {
            @display("p=6222,2091;is=s");
        }
        SWTalker3: TsnSwitch {
            @display("p=6931,2091;is=s");
        }

                // ===== Group 2 =====
        dsttUe4: DsttUe {
            @display("p=7603,1070;is=s");
        }
        dsttUe5: DsttUe {
            @display("p=8412,1045;is=s");
        }
        SWTalker4: TsnSwitch {
            @display("p=7491,2091;is=s");
        }
        SWTalker5: TsnSwitch {
            @display("p=8250,2091;is=s");
        }

        // ===== Group 3 =====
        dsttUe6: DsttUe {
            @display("p=9009,1070;is=s");
        }
        dsttUe7: DsttUe {
            @display("p=9706,1070;is=s");
        }
        SWTalker6: TsnSwitch {
            @display("p=8885,2091;is=s");
        }
        SWTalker7: TsnSwitch {
            @display("p=9594,2091;is=s");
        }

        // ===== Group 4 =====
        dsttUe8: DsttUe {
            @display("p=10378,1070;is=s");
        }
        dsttUe9: DsttUe {
            @display("p=11075,1070;is=s");
        }
        SWTalker8: TsnSwitch {
            @display("p=10266,2091;is=s");
        }
        SWTalker9: TsnSwitch {
            @display("p=10851,2091;is=s");
        }

        // ===== Group 5 =====
        dsttUe10: DsttUe {
            @display("p=11660,1070;is=s");
        }
        dsttUe11: DsttUe {
            @display("p=12282,1070;is=s");
        }
        SWTalker10: TsnSwitch {
            @display("p=11548,2091;is=s");
        }
        SWTalker11: TsnSwitch {
            @display("p=12120,2091;is=s");
        }

        // ===== Group 6 =====
        dsttUe12: DsttUe {
            @display("p=12929,1070;is=s");
        }
        dsttUe13: DsttUe {
            @display("p=13738,1045;is=s");
        }
        SWTalker12: TsnSwitch {
            @display("p=12842,2091;is=s");
        }
        SWTalker13: TsnSwitch {
            @display("p=13626,2091;is=s");
        }

        // ===== Group 7 =====
        dsttUe14: DsttUe {
            @display("p=4380,4666;is=s");
        }
        dsttUe15: DsttUe {
            @display("p=5189,4666;is=s");
        }
        SWTalker14: TsnSwitch {
            @display("p=4405,3746;is=s");
        }
        SWTalker15: TsnSwitch {
            @display("p=5189,3746;is=s");
        }

        // ===== Group 8 =====
        dsttUe16: DsttUe {
            @display("p=5886,4666;is=s");
        }
        dsttUe17: DsttUe {
            @display("p=6558,4579;is=s");
        }
        SWTalker16: TsnSwitch {
            @display("p=5886,3746;is=s");
        }
        SWTalker17: TsnSwitch {
            @display("p=6670,3746;is=s");
        }

        // ===== Group 9 =====
        dsttUe18: DsttUe {
            @display("p=7342,4579;is=s");
        }
        dsttUe19: DsttUe {
            @display("p=8076,4579;is=s");
        }
        SWTalker18: TsnSwitch {
            @display("p=7342,3746;is=s");
        }
        SWTalker19: TsnSwitch {
            @display("p=8051,3746;is=s");
        }
        // ===== Group 10 =====
        dsttUe20: DsttUe {
            @display("p=8835,4505;is=s");
        }
        dsttUe21: DsttUe {
            @display("p=9594,4505;is=s");
        }
        SWTalker20: TsnSwitch {
            @display("p=8835,3658;is=s");
        }
        SWTalker21: TsnSwitch {
            @display("p=9594,3546;is=s");
        }

        // ===== Group 11 =====
        dsttUe22: DsttUe {
            @display("p=10291,4505;is=s");
        }
        dsttUe23: DsttUe {
            @display("p=10963,4505;is=s");
        }
        SWTalker22: TsnSwitch {
            @display("p=10291,3546;is=s");
        }
        SWTalker23: TsnSwitch {
            @display("p=10963,3546;is=s");
        }

        // ===== Group 12 =====
        dsttUe24: DsttUe {
            @display("p=11772,4530;is=s");
        }
        dsttUe25: DsttUe {
            @display("p=12394,4530;is=s");
        }
        SWTalker24: TsnSwitch {
            @display("p=11660,3546;is=s");
        }
        SWTalker25: TsnSwitch {
            @display("p=12394,3546;is=s");
        }

        // ===== Group 13 =====
        dsttUe26: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe27: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker26: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker27: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        // ===== Group 13 =====
        dsttUe28: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe29: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker28: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker29: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        // ===== Group 13 =====
        dsttUe30: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe31: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker30: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker31: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        // ===== Group 13 =====
        dsttUe32: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe33: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker32: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker33: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        // ===== Group 13 =====
        dsttUe34: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe35: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker34: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker35: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        // ===== Group 13 =====
        dsttUe36: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe37: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker36: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker37: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        // ===== Group 13 =====
        dsttUe38: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe39: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker38: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker39: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        // ===== Group 13 =====
        dsttUe40: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe41: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker40: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker41: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        dsttUe42: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe43: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker42: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker43: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        
        dsttUe44: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe45: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker44: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker45: TsnSwitch {
            @display("p=13850,3658;is=s");
        }
        
        
        dsttUe46: DsttUe {
            @display("p=13041,4530;is=s");
        }
        dsttUe47: DsttUe {
            @display("p=13738,4530;is=s");
        }
        SWTalker46: TsnSwitch {
            @display("p=13066,3546;is=s");
        }
        SWTalker47: TsnSwitch {
            @display("p=13850,3658;is=s");
        }


    connections:
        // ===== Connections Group 0 =====
        dsttUe0.custom_egate++ <--> Eth1G <--> SWTalker0.ethg++;
        dsttUe1.custom_egate++ <--> Eth1G <--> SWTalker1.ethg++;
        dsttUe2.custom_egate++ <--> Eth1G <--> SWTalker2.ethg++;
        dsttUe3.custom_egate++ <--> Eth1G <--> SWTalker3.ethg++;
        dsttUe4.custom_egate++ <--> Eth1G <--> SWTalker4.ethg++;
        dsttUe5.custom_egate++ <--> Eth1G <--> SWTalker5.ethg++;
        dsttUe6.custom_egate++ <--> Eth1G <--> SWTalker6.ethg++;
        dsttUe7.custom_egate++ <--> Eth1G <--> SWTalker7.ethg++;
        dsttUe8.custom_egate++ <--> Eth1G <--> SWTalker8.ethg++;
        dsttUe9.custom_egate++ <--> Eth1G <--> SWTalker9.ethg++;
        dsttUe10.custom_egate++ <--> Eth1G <--> SWTalker10.ethg++;
        dsttUe11.custom_egate++ <--> Eth1G <--> SWTalker11.ethg++;
        dsttUe12.custom_egate++ <--> Eth1G <--> SWTalker12.ethg++;
        dsttUe13.custom_egate++ <--> Eth1G <--> SWTalker13.ethg++;
        dsttUe14.custom_egate++ <--> Eth1G <--> SWTalker14.ethg++;
        dsttUe15.custom_egate++ <--> Eth1G <--> SWTalker15.ethg++;
        dsttUe16.custom_egate++ <--> Eth1G <--> SWTalker16.ethg++;
        dsttUe17.custom_egate++ <--> Eth1G <--> SWTalker17.ethg++;
        dsttUe18.custom_egate++ <--> Eth1G <--> SWTalker18.ethg++;
        dsttUe19.custom_egate++ <--> Eth1G <--> SWTalker19.ethg++;
        dsttUe20.custom_egate++ <--> Eth1G <--> SWTalker20.ethg++;
        dsttUe21.custom_egate++ <--> Eth1G <--> SWTalker21.ethg++;
        dsttUe22.custom_egate++ <--> Eth1G <--> SWTalker22.ethg++;
        dsttUe23.custom_egate++ <--> Eth1G <--> SWTalker23.ethg++;
        dsttUe24.custom_egate++ <--> Eth1G <--> SWTalker24.ethg++;
        dsttUe25.custom_egate++ <--> Eth1G <--> SWTalker25.ethg++;
        dsttUe26.custom_egate++ <--> Eth1G <--> SWTalker26.ethg++;
        dsttUe27.custom_egate++ <--> Eth1G <--> SWTalker27.ethg++;
        dsttUe28.custom_egate++ <--> Eth1G <--> SWTalker28.ethg++;
        dsttUe29.custom_egate++ <--> Eth1G <--> SWTalker29.ethg++;
        dsttUe30.custom_egate++ <--> Eth1G <--> SWTalker30.ethg++;
        dsttUe31.custom_egate++ <--> Eth1G <--> SWTalker31.ethg++;
        dsttUe32.custom_egate++ <--> Eth1G <--> SWTalker32.ethg++;
        dsttUe33.custom_egate++ <--> Eth1G <--> SWTalker33.ethg++;
        dsttUe34.custom_egate++ <--> Eth1G <--> SWTalker34.ethg++;
        dsttUe35.custom_egate++ <--> Eth1G <--> SWTalker35.ethg++;
        dsttUe36.custom_egate++ <--> Eth1G <--> SWTalker36.ethg++;
        dsttUe37.custom_egate++ <--> Eth1G <--> SWTalker37.ethg++;
        dsttUe38.custom_egate++ <--> Eth1G <--> SWTalker38.ethg++;
        dsttUe39.custom_egate++ <--> Eth1G <--> SWTalker39.ethg++;
        dsttUe40.custom_egate++ <--> Eth1G <--> SWTalker40.ethg++;
        dsttUe41.custom_egate++ <--> Eth1G <--> SWTalker41.ethg++;
        dsttUe42.custom_egate++ <--> Eth1G <--> SWTalker42.ethg++;
        dsttUe43.custom_egate++ <--> Eth1G <--> SWTalker43.ethg++;
        dsttUe44.custom_egate++ <--> Eth1G <--> SWTalker44.ethg++;
        dsttUe45.custom_egate++ <--> Eth1G <--> SWTalker45.ethg++;
        dsttUe46.custom_egate++ <--> Eth1G <--> SWTalker46.ethg++;
        dsttUe47.custom_egate++ <--> Eth1G <--> SWTalker47.ethg++;
        

        Talker0.ethg++ <--> Eth1G <--> TASSwitch0.ethg++;
        SWTalker.ethg++ <--> Eth1G <--> TASSwitch0.ethg++;

        SWTalker0.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker1.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker2.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker3.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker4.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker5.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker6.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker7.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker8.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker9.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker10.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker11.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker12.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker13.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker14.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker15.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker16.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker17.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker18.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker19.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker20.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker21.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker22.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker23.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker24.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker25.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker26.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker27.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker28.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker29.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker30.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker31.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker32.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker33.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker34.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker35.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker36.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker37.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker38.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker39.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker40.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker41.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker42.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker43.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker44.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker45.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker46.ethg++ <--> Eth1G <--> SWTalker.ethg++;
        SWTalker47.ethg++ <--> Eth1G <--> SWTalker.ethg++;


        Listener.ethg++ <--> Eth1G <--> tsnSwitchListener.ethg++;
        upf.pppg++ <--> Eth1G <--> gnb.ppp;
        tsnSwitchListener.ethg++ <--> Eth1G <--> nwtt.ethg++;
        nwtt.pppg++ <--> Eth1G <--> upf.filterGate;

}

