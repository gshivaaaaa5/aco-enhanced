#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BasicVanet");

int main (int argc, char *argv[])
{
    uint32_t numVehicles = 5;
    double simTime = 10.0;

    LogComponentEnable ("BasicVanet", LOG_LEVEL_INFO);

    // 1. Create vehicle nodes
    NodeContainer vehicles;
    vehicles.Create (numVehicles);

    // 2. Mobility model (vehicles moving in a straight line)
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
    mobility.Install (vehicles);

    for (uint32_t i = 0; i < numVehicles; ++i)
    {
        Ptr<ConstantVelocityMobilityModel> mob =
            vehicles.Get(i)->GetObject<ConstantVelocityMobilityModel>();
        mob->SetPosition (Vector (i * 20.0, 0.0, 0.0));
        mob->SetVelocity (Vector (5.0, 0.0, 0.0)); // 5 m/s
    }

    // 3. Configure WiFi (802.11p - VANET)
    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211p);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();

    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    WifiMacHelper mac;
    mac.SetType ("ns3::AdhocWifiMac");

    NetDeviceContainer devices;
    devices = wifi.Install (phy, mac, vehicles);

    // 4. Install Internet stack
    InternetStackHelper internet;
    internet.Install (vehicles);

    // 5. Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

    // 6. Create UDP application (Vehicle 0 → Vehicle 4)
    uint16_t port = 4000;

    UdpServerHelper server (port);
    ApplicationContainer serverApp = server.Install (vehicles.Get (4));
    serverApp.Start (Seconds (1.0));
    serverApp.Stop (Seconds (simTime));

    UdpClientHelper client (interfaces.GetAddress (4), port);
    client.SetAttribute ("MaxPackets", UintegerValue (50));
    client.SetAttribute ("Interval", TimeValue (Seconds (0.2)));
    client.SetAttribute ("PacketSize", UintegerValue (512));

    ApplicationContainer clientApp = client.Install (vehicles.Get (0));
    clientApp.Start (Seconds (2.0));
    clientApp.Stop (Seconds (simTime));

    // 7. Run simulation
    Simulator::Stop (Seconds (simTime));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
