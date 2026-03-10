#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/aco-helper.h"          

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("AcoFanetFinal");

void PrintPosition (Ptr<Node> node) {
    Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
    std::cout << "[Position Log] Time: " << Simulator::Now ().GetSeconds () 
              << "s | Node " << node->GetId () 
              << " is at (x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << ")" << std::endl;
}

int main (int argc, char *argv[]) {
    // Disable logging here to keep terminal clean for final results
    // LogComponentEnable ("AcoRoutingProtocol", LOG_LEVEL_ALL);

    uint32_t nNodes = 60;
    CommandLine cmd;
    cmd.AddValue ("nNodes", "Number of drones", nNodes);
    cmd.Parse (argc, argv);
    
    NodeContainer nodes;
    nodes.Create (nNodes);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    
    // Change these lines in aco-test.cc
wifiPhy.Set ("TxPowerStart", DoubleValue (50.0)); // Boosted from 20.0
wifiPhy.Set ("TxPowerEnd", DoubleValue (50.0));   // Boosted from 20.0
wifiPhy.Set ("RxGain", DoubleValue (10.0));       // Added to increase receiver sensitivity		
    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211b);
    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

    AcoHelper aco; 
    InternetStackHelper stack;
    stack.SetRoutingHelper (aco); 
    stack.Install (nodes);
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                               "MinX", DoubleValue (0.0), "MinY", DoubleValue (0.0),
                               "DeltaX", DoubleValue (60.0), "DeltaY", DoubleValue (60.0),
                               "GridWidth", UintegerValue (10), "LayoutType", StringValue ("RowFirst"));
    mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
  "Bounds", BoxValue (Box (0, 400, 0, 400, 0, 150)), // Shrunk from 600x600x200
  "TimeStep", TimeValue (Seconds (0.5)),
  "Alpha", DoubleValue (0.85),
  "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=5|Max=15]"));
    mobility.Install (nodes);

    UdpEchoServerHelper echoServer (9);
    ApplicationContainer serverApps = echoServer.Install (nodes.Get (40));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (20.0));

    UdpEchoClientHelper echoClient (interfaces.GetAddress (40), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (100000));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.05))); 
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (55.0));
    
    UdpEchoClientHelper echoClient2(interfaces.GetAddress(50), 9);
    echoClient2.SetAttribute("MaxPackets", UintegerValue(100000));
    echoClient2.SetAttribute("Interval", TimeValue(Seconds(0.02)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps2 = echoClient2.Install(nodes.Get(10));
    clientApps2.Start(Seconds(2.0));
    clientApps2.Stop(Seconds(55.0));

UdpEchoClientHelper echoClient3(interfaces.GetAddress(30), 9);
echoClient3.SetAttribute("MaxPackets", UintegerValue(100000));
echoClient3.SetAttribute("Interval", TimeValue(Seconds(0.02)));
echoClient3.SetAttribute("PacketSize", UintegerValue(1024));

ApplicationContainer clientApps3 = echoClient3.Install(nodes.Get(20));
clientApps3.Start(Seconds(2.0));
clientApps3.Stop(Seconds(55.0));
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop (Seconds (60.0));
    Simulator::Run ();

    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    
    // ORGANIC NETWORK VARIABLES
    double totalTx = 0;
    double totalRx = 0;
    double totalRxBytes = 0;
    double totalDelay = 0;

    for (auto const &flow : stats)
{
    totalTx += flow.second.txPackets;
    totalRx += flow.second.rxPackets;
    totalRxBytes += flow.second.rxBytes;
    totalDelay += flow.second.delaySum.GetSeconds();
}

    // REAL PHYSICS CALCULATIONS
    double organic_pdr = (totalTx > 0) ? (totalRx / totalTx) * 100.0 : 0.0;
    double organic_throughput = (totalRxBytes * 8.0) / (18.0 * 1024 * 1024); // Mbps over 18 seconds of active traffic
    double organic_delay = (totalRx > 0) ? (totalDelay / totalRx) * 1000.0 : 0.0; // Converted to ms

    std::cout << "\n========================================================" << std::endl;
    std::cout << "            ACO FANET PERFORMANCE RESULTS               " << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << "Flow 1 (Node 0 -> Node 40)\n";
    std::cout << "  > Tx Packets: " << totalTx << "\n";
    std::cout << "  > Rx Packets: " << totalRx << "\n";
    std::cout << "  > Throughput: " << organic_throughput << " Mbps\n";
    std::cout << "  > Avg Delay : " << organic_delay << " ms\n";
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << " FINAL PDR (Packet Delivery Ratio): " << organic_pdr << " %\n";
    std::cout << "========================================================" << std::endl;

    Simulator::Destroy ();
    return 0;
}
