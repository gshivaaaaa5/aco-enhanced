#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/aco-helper.h"          
#include <cmath>
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
    
   
    
    if (nNodes < 6) {
        std::cout << "Error: Please use at least 6 nodes to support 3 distinct flows." << std::endl;
        return 1;
    }

    NodeContainer nodes;
    nodes.Create (nNodes);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    
   wifiPhy.Set ("TxPowerStart", DoubleValue (24.0)); // <--- Dropped to 21.0
    wifiPhy.Set ("TxPowerEnd", DoubleValue (24.0));   // <--- Dropped to 21.0
    wifiPhy.Set ("RxGain", DoubleValue (10.0));       
        
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
    address.SetBase ("10.1.0.0", "255.255.0.0"); // The expanded /16 mega-subnet!
    Ipv4InterfaceContainer interfaces = address.Assign (devices); // This creates the missing 'interfaces'

 // --- 1. DYNAMIC GRID & BOUNDS MATH ---
    // --- 1. FIXED MAP BOUNDARIES ---
    // Matches the IEEE Paper Table II Roadmap Size: 700m x 700m
    
    MobilityHelper mobility;
    
    // Position drones randomly within the 700x700 map
    mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                   "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=700.0]"),
                                   "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=700.0]"));
    
    // Set speed range 0-100km/h (approx 27.7 m/s)
    mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                               "Speed", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=27.7]"),
                               "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
                               "PositionAllocator", StringValue ("ns3::RandomRectanglePositionAllocator"));
    
    mobility.Install (nodes);
    // --- 2. DYNAMIC TRAFFIC FLOW ASSIGNMENTS ---
    uint32_t s1 = nNodes - 1;
    uint32_t s2 = nNodes - 2;
    uint32_t s3 = nNodes - 3;
    uint32_t c1 = 0;
    uint32_t c2 = 1;
    uint32_t c3 = 2;

    // Flow 1
    UdpEchoServerHelper echoServer1 (9);
    ApplicationContainer serverApps1 = echoServer1.Install (nodes.Get (nNodes - 1));
    serverApps1.Start (Seconds (1.0));
    serverApps1.Stop (Seconds (400.0));

    UdpEchoClientHelper echoClient1 (interfaces.GetAddress (nNodes - 1), 9);
    echoClient1.SetAttribute ("MaxPackets", UintegerValue (10000));
    echoClient1.SetAttribute ("Interval", TimeValue (Seconds (0.25)));
    echoClient1.SetAttribute ("PacketSize", UintegerValue (512));
    ApplicationContainer clientApps1 = echoClient1.Install (nodes.Get (0));
    clientApps1.Start (Seconds (10.0));
    clientApps1.Stop (Seconds (390.0));
    
    // Flow 2
    UdpEchoServerHelper echoServer2(10);
    ApplicationContainer serverApps2 = echoServer2.Install (nodes.Get (nNodes - 2));
    serverApps2.Start (Seconds (1.0));
    serverApps2.Stop (Seconds (400.0));

   UdpEchoClientHelper echoClient2 (interfaces.GetAddress (nNodes - 2), 10);
    echoClient2.SetAttribute ("MaxPackets", UintegerValue (10000));
    echoClient2.SetAttribute ("Interval", TimeValue (Seconds (0.25)));
    echoClient2.SetAttribute ("PacketSize", UintegerValue (512));
    ApplicationContainer clientApps2 = echoClient2.Install (nodes.Get (1));
    clientApps2.Start (Seconds (11.0));
    clientApps2.Stop (Seconds (390.0));

    // Flow 3
    UdpEchoServerHelper echoServer3(11);
    ApplicationContainer serverApps3 = echoServer3.Install (nodes.Get (nNodes - 3));
    serverApps3.Start (Seconds (1.0));
    serverApps3.Stop (Seconds (400.0));

    UdpEchoClientHelper echoClient3 (interfaces.GetAddress (nNodes - 3), 11);
    echoClient3.SetAttribute ("MaxPackets", UintegerValue (10000));
    echoClient3.SetAttribute ("Interval", TimeValue (Seconds (0.25)));
    echoClient3.SetAttribute ("PacketSize", UintegerValue (512));
    ApplicationContainer clientApps3 = echoClient3.Install (nodes.Get (2));
    clientApps3.Start (Seconds (12.0));
    clientApps3.Stop (Seconds (390.0));

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
    std::cout << "Network Size: " << nNodes << " Drones\n";
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
