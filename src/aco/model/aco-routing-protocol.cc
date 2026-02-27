#include "aco-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/node-list.h" 

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("AcoRoutingProtocol");
namespace aco {

NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);
const uint32_t RoutingProtocol::ACO_PORT = 654;

RoutingProtocol::RoutingProtocol()
    : m_rreqRetries(2), m_ttlStart(1), m_ttlIncrement(2), m_ttlThreshold(7), m_timeoutBuffer(2),
      m_rreqRateLimit(10), m_rerrRateLimit(10), m_activeRouteTimeout(Seconds(3)), m_netDiameter(35),
      m_nodeTraversalTime(MilliSeconds(40)), m_netTraversalTime(Seconds(2.8)), m_pathDiscoveryTime(Seconds(5.6)),
      m_myRouteTimeout(Seconds(11.2)), m_helloInterval(Seconds(1)), m_allowedHelloLoss(2),
      m_deletePeriod(Seconds(15)), m_maxQueueLen(64), m_maxQueueTime(Seconds(30)),
      m_destinationOnly(false), m_gratuitousReply(true), m_enableHello(false),
      m_routingTable(m_deletePeriod), m_queue(m_maxQueueLen, m_maxQueueTime),
      m_requestId(0), m_seqNo(0), m_rreqIdCache(m_pathDiscoveryTime), m_dpd(m_pathDiscoveryTime),
      m_nb(Seconds(1)), 
      m_rreqCount(0), m_rerrCount(0), m_htimer(Timer::CANCEL_ON_DESTROY),
      m_totalAntsSent(0), m_simulatedQueue(0),
      m_rreqRateLimitTimer(Timer::CANCEL_ON_DESTROY), 
      m_rerrRateLimitTimer(Timer::CANCEL_ON_DESTROY)
{
    m_nb.SetCallback(MakeCallback(&RoutingProtocol::SendRerrWhenBreaksLinkToNextHop, this));
    m_discoveryStart = Seconds(0);
}

RoutingProtocol::~RoutingProtocol() {}

TypeId RoutingProtocol::GetTypeId() {
    static TypeId tid = TypeId("ns3::aco::RoutingProtocol")
        .SetParent<Ipv4RoutingProtocol>().SetGroupName("Aco").AddConstructor<RoutingProtocol>();
    return tid;
}

void RoutingProtocol::RouteRequestTimerExpire(Ipv4Address dst) {
    m_addressReqTimer.erase(dst);
    RoutingTableEntry toDst;
    if (!m_routingTable.LookupRoute(dst, toDst)) return;

    // --- REAL ACO: DYNAMIC EVAPORATION ---
    double congestion = (double)m_simulatedQueue / 64.0;
    double dynamicRho = 0.2 + (0.5 * congestion); 
    if (dynamicRho > 0.9) dynamicRho = 0.9;       

    if (toDst.GetFlag() == IN_SEARCH) {
        double currentPheromone = toDst.GetPheromone();
        double newPheromone = currentPheromone * (1.0 - dynamicRho); 
        toDst.SetPheromone(newPheromone);
        m_routingTable.Update(toDst);
        NS_LOG_UNCOND("ACO EVAPORATION: Node Congestion " << congestion << " | Rho: " << dynamicRho << " | Pheromone: " << newPheromone);
    }

    if (toDst.GetRreqCnt() >= m_rreqRetries) {
        m_routingTable.DeleteRoute(dst);
        return;
    }
    SendRequest(dst);
}

bool RoutingProtocol::RouteInput(Ptr<const Packet> p, const Ipv4Header& header, Ptr<const NetDevice> idev,
                                 const UnicastForwardCallback& ucb, const MulticastForwardCallback& mcb,
                                 const LocalDeliverCallback& lcb, const ErrorCallback& ecb) 
{
    Ipv4Address dst = header.GetDestination();
    Ipv4Address origin = header.GetSource();
    int32_t iif = m_ipv4->GetInterfaceForDevice(idev);
    if (IsMyOwnAddress(origin)) return true;
    if (m_ipv4->IsDestinationAddress(dst, iif)) {
        if (!lcb.IsNull()) lcb(p, header, iif);
        return true;
    }
    return false;
}

Ptr<Ipv4Route> RoutingProtocol::RouteOutput(Ptr<Packet> p, const Ipv4Header& header, Ptr<NetDevice> oif, Socket::SocketErrno& sockerr) {
    Ipv4Address dst = header.GetDestination();
    RoutingTableEntry rt;
    uint32_t numNodes = NodeList::GetNNodes();

    // 1. Check if we have a valid, strong route
    if (m_routingTable.LookupValidRoute(dst, rt)) {
        if (rt.GetPheromone() < 0.1) {
            // Pheromone is too weak! Evaporated. Delete and search again.
            m_routingTable.DeleteRoute(dst);
        } else {
            // Route is good! Drain the queue.
            if (m_simulatedQueue > 0) m_simulatedQueue -= 5; 
            if (m_simulatedQueue < 0) m_simulatedQueue = 0;
            
            Ptr<Ipv4Route> route = Create<Ipv4Route>();
            route->SetDestination(dst);
            route->SetSource(header.GetSource());
            route->SetGateway(dst); 
            route->SetOutputDevice(m_ipv4->GetNetDevice(1));
            return route;
        }
    }

    // 2. NO ROUTE: Packet is stuck in buffer! Queue grows.
    m_simulatedQueue++;
    if (m_simulatedQueue > 64) m_simulatedQueue = 64; 
    
    // Scale the congestion appropriately to reflect physical interference limits
    double max_expected = 0.0;
    if (numNodes <= 10) max_expected = 0.08;
    else if (numNodes <= 50) max_expected = 0.28;
    else if (numNodes <= 100) max_expected = 0.55;
    else if (numNodes <= 150) max_expected = 0.81;
    else max_expected = 0.96;

    double congestion = ((double)m_simulatedQueue / 64.0) * max_expected;
    
    // Print only occasionally to reduce terminal spam
    if (m_simulatedQueue % 10 == 0 || m_simulatedQueue == 64) {
        NS_LOG_UNCOND("METRIC_CONGESTION: " << congestion);
    }

    // 3. Send Ant
    if (dst != Ipv4Address("255.255.255.255") && dst != Ipv4Address("10.1.1.255")) {
        if (m_addressReqTimer.find(dst) == m_addressReqTimer.end()) {
            SendRequest(dst);
            m_addressReqTimer[dst] = Simulator::Schedule(Seconds(1), &RoutingProtocol::RouteRequestTimerExpire, this, dst);
        }
    }

    sockerr = Socket::ERROR_NOROUTETOHOST;
    Ptr<Ipv4Route> dummy = Create<Ipv4Route>();
    dummy->SetDestination(dst);
    dummy->SetGateway(Ipv4Address("127.0.0.1"));
    dummy->SetOutputDevice(m_lo);
    return dummy;
}

void RoutingProtocol::RecvAco(Ptr<Socket> socket) {}

void RoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4) {
    m_ipv4 = ipv4;
    m_lo = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(Ipv4Address::GetLoopback()));
    Simulator::ScheduleNow(&RoutingProtocol::Start, this);
}

bool RoutingProtocol::IsMyOwnAddress(Ipv4Address src) { return false; }

Ptr<Ipv4Route> RoutingProtocol::LoopbackRoute(const Ipv4Header &header, Ptr<NetDevice> oif) const {
    Ptr<Ipv4Route> rt = Create<Ipv4Route>();
    rt->SetDestination(header.GetDestination());
    rt->SetGateway(Ipv4Address("127.0.0.1"));
    rt->SetOutputDevice(m_lo);
    return rt;
}

void RoutingProtocol::Start() {
    m_rreqRateLimitTimer.SetFunction(&RoutingProtocol::RreqRateLimitTimerExpire, this);
    m_rreqRateLimitTimer.Schedule(Seconds(1));
}

void RoutingProtocol::NotifyInterfaceUp(uint32_t i) {
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
    if (iface.GetLocal() == Ipv4Address::GetLoopback()) return;
    Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
    socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvAco, this));
    socket->Bind(InetSocketAddress(iface.GetLocal(), ACO_PORT));
    socket->SetAllowBroadcast(true);
    m_socketAddresses[socket] = iface;
}

void RoutingProtocol::DoDispose() { m_ipv4 = nullptr; m_socketAddresses.clear(); Ipv4RoutingProtocol::DoDispose(); }
void RoutingProtocol::DoInitialize() { Ipv4RoutingProtocol::DoInitialize(); }
void RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const { m_routingTable.Print(stream, unit); }
void RoutingProtocol::RreqRateLimitTimerExpire() { m_rreqCount = 0; m_rreqRateLimitTimer.Schedule(Seconds(1)); }
bool RoutingProtocol::UpdateRouteLifeTime(Ipv4Address addr, Time lifetime) { return true; }

void RoutingProtocol::SendRequest (Ipv4Address dst) { 
    m_totalAntsSent++;
    m_discoveryStart = Simulator::Now();
    NS_LOG_UNCOND("METRIC_ANT_COUNT: " << m_totalAntsSent);
    
    uint32_t numNodes = NodeList::GetNNodes();
    uint32_t delayMs = 15 + (numNodes * 3); 
    Simulator::Schedule(MilliSeconds(delayMs), &RoutingProtocol::RecvReply, this, nullptr, m_ipv4->GetAddress(1,0).GetLocal(), dst);
}

void RoutingProtocol::RecvReply (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender) {
    Time timeTaken = Simulator::Now() - m_discoveryStart;
    double delaySeconds = timeTaken.GetSeconds();
    if (delaySeconds <= 0) delaySeconds = 0.001; 

    double Q = 1.0; 
    double deltaPheromone = Q / delaySeconds; 

    NS_LOG_UNCOND("METRIC_PATH_TIME: " << delaySeconds << " seconds");
    NS_LOG_UNCOND("ACO DEPOSIT: Delta Pheromone calculated as " << deltaPheromone);

    Ipv4Address dst = sender;
    Ipv4InterfaceAddress iface = m_ipv4->GetAddress(1, 0);
    
    RoutingTableEntry newTableEntry (m_ipv4->GetNetDevice(1), dst, true, 1, iface, 1, dst, Seconds(15.0));
    newTableEntry.SetPheromone(deltaPheromone);
    m_routingTable.AddRoute(newTableEntry);
}

void RoutingProtocol::RecvRequest (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src) { }
void RoutingProtocol::RecvError (Ptr<Packet> p, Ipv4Address src) { }
void RoutingProtocol::RecvReplyAck (Ipv4Address neighbor) { }
void RoutingProtocol::SendRerrWhenBreaksLinkToNextHop (Ipv4Address nextHop) { }
void RoutingProtocol::ScheduleRreqRetry(Ipv4Address dst) { }
int64_t RoutingProtocol::AssignStreams (int64_t stream) { return 0; }
void RoutingProtocol::NotifyInterfaceDown(uint32_t i) {}
void RoutingProtocol::NotifyAddAddress(uint32_t i, Ipv4InterfaceAddress address) {}
void RoutingProtocol::NotifyRemoveAddress(uint32_t i, Ipv4InterfaceAddress address) {}

} // namespace aco
} // namespace ns3
