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
import inet.node.ethernet.Eth100M;
import inet.visualizer.common.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import simu5g.common.binder.GlobalData;

package tsnfivegcomm.simulations.p5g_tsn.multiple_UEs;

network ScenarioMultiStream extends TsnNetworkBase
{
    int numUe = default(2);
    bool useQosModel = default(true);
    bool packetDropEnabled = default(true);
    string qosModelPriority = default("priority");
    double lambdaPriority = default(1);
    double lambdaRemainDelayBudget = default(0);
    double lambdaCqi = default(0);
    double lambdaRtx = default(0);
    double lambdaByteSize = default(0);

// 	int numUe = default(1);
//    bool useQosModel = default(true);
//    bool packetDropEnabled = default(true);
//    string qosModelPriority = default("pdb");
//    double lambdaPriority = default(0);
//    double lambdaRemainDelayBudget = default(1);
//    double lambdaCqi = default(0);
//    double lambdaRtx = default(0);
//    double lambdaByteSize = default(0);


    bool tsnEnabled = default(false);
    @display("bgb=3401.0579,2249.262");
    submodules:

        globalData: GlobalData {
            @display("p=342.456,1928.568");
        }
        tsnSwitch1_2: TsnSwitch {
            @display("p=1121.208,424.974");
        }
        tsnDevice1: TsnDevice {
            @display("p=795.69604,391.82");
        }
        tsnDevice2: TsnDevice {
            @display("p=1075.998,738.43");
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
            @display("p=1865.33,427.396");
        }
        gnb: gNodeB {
            @display("p=2446.758,423.564;is=vl");
        }
        linkVisualizer: DataLinkVisualizer {
            @display("p=126.77,1155.418");
        }
        nwtt: Nwtt {
            @display("p=1488.642,427.396");
        }
        ipvisualizer: IntegratedVisualizer {
            @display("p=170.234,1408.958");
        }
        configuratortwo: Ipv4NetworkConfigurator {
            @display("p=383.932,1615.412");
        }
        dsttUe3: DsttUe {
            @display("p=2158.374,892.188");
        }
        tsnSwitch3: TsnSwitch {
            @display("p=2158.374,1239.15");
        }
        tsnDevice3: TsnDevice {
            @display("p=2158.374,1586.112");
        }
        dsttUe4: DsttUe {
            @display("p=2685.576,892.188");
        }
        tsnSwitch4: TsnSwitch {
            @display("p=2685.576,1239.15");
        }
        tsnDevice4: TsnDevice {
            @display("p=2685.576,1586.112");
        }
    connections:
        tsnSwitch1_2.ethg++ <--> Eth100M <--> tsnDevice1.ethg++;
        tsnSwitch1_2.ethg++ <--> Eth100M <--> tsnDevice2.ethg++;
        upf.pppg++ <--> Eth100M <--> gnb.ppp;
        tsnSwitch1_2.ethg++ <--> Eth100M <--> nwtt.ethg++;
        nwtt.pppg++ <--> Eth100M <--> upf.filterGate;
        dsttUe3.custom_egate++ <--> Eth100M <--> tsnSwitch3.ethg++;
        tsnSwitch3.ethg++ <--> Eth100M <--> tsnDevice3.ethg++;
        dsttUe4.custom_egate++ <--> Eth100M <--> tsnSwitch4.ethg++;
        tsnSwitch4.ethg++ <--> Eth100M <--> tsnDevice4.ethg++;
}

