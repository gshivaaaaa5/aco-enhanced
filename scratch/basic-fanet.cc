#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/aodv-module.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"

using namespace ns3;

AnimationInterface* animPtr = nullptr;

// Called when UAV0 receives a packet
void RxCallback (Ptr<const Packet> packet, const Address &address)
{
  std::cout << "📥 Packet received at "
            << Simulator::Now ().GetSeconds ()
            << "s, size = " << packet->GetSize () << " bytes" << std::endl;

  // Color receiver (UAV0) blue
  if (animPtr)
    animPtr->UpdateNodeColor (0, 0, 0, 255);
}

int main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer uavs;
  uavs.Create (5);

  // ------------------ POSITION + MOBILITY ------------------
  Ptr<RandomRectanglePositionAllocator> posAlloc =
    CreateObject<RandomRectanglePositionAllocator> ();

  posAlloc->SetX (CreateObjectWithAttributes<UniformRandomVariable>
                  ("Min", DoubleValue (0.0), "Max", DoubleValue (500.0)));
  posAlloc->SetY (CreateObjectWithAttributes<UniformRandomVariable>
                  ("Min", DoubleValue (0.0), "Max", DoubleValue (500.0)));

  MobilityHelper mobility;
  mobility.SetPositionAllocator (posAlloc);
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                             "Speed", StringValue ("ns3::UniformRandomVariable[Min=10|Max=20]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"),
                             "PositionAllocator", PointerValue (posAlloc));
  mobility.Install (uavs);

  // ------------------ WIFI ------------------
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper phy;
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  phy.Set ("TxPowerStart", DoubleValue (30));
  phy.Set ("TxPowerEnd", DoubleValue (30));
  phy.SetChannel (channel.Create ());

  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (phy, mac, uavs);

  // ------------------ ROUTING + INTERNET ------------------
  AodvHelper aodv;
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv);
  stack.Install (uavs);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // ------------------ APPLICATIONS ------------------
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (uavs.Get (0));
  serverApps.Start (Seconds (5.0));
  serverApps.Stop (Seconds (30.0));

  Ptr<UdpEchoServer> server = serverApps.Get (0)->GetObject<UdpEchoServer> ();
  server->TraceConnectWithoutContext ("Rx", MakeCallback (&RxCallback));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (25));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (uavs.Get (1));
  clientApps.Start (Seconds (8.0));
  clientApps.Stop (Seconds (30.0));

  // ------------------ NETANIM ------------------
  animPtr = new AnimationInterface ("fanet.xml");
  animPtr->EnablePacketMetadata (true);

  for (uint32_t i = 0; i < uavs.GetN (); i++)
  {
    animPtr->UpdateNodeDescription (uavs.Get (i), "UAV");
    animPtr->UpdateNodeColor (uavs.Get (i), 0, 255, 0);
  }

  // Sender UAV1 = Red
  animPtr->UpdateNodeColor (1, 255, 0, 0);

  // ------------------ RUN ------------------
  Simulator::Stop (Seconds (30.0));
  Simulator::Run ();
  Simulator::Destroy ();

  delete animPtr;
  return 0;
}
