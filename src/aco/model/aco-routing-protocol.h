#ifndef ACO_ROUTING_PROTOCOL_H
#define ACO_ROUTING_PROTOCOL_H

#include "aco-rtable.h"
#include "aco-rqueue.h"
#include "aco-packet.h"
#include "aco-neighbor.h"
#include "aco-dpd.h"
#include "aco-id-cache.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/timer.h"
#include "ns3/event-id.h"
#include <map>
#include <vector>

namespace ns3 {
namespace aco {

/**
 * \ingroup aco
 * \brief ACO Routing Protocol Link Layer
 */
class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);
  static const uint32_t ACO_PORT;

  RoutingProtocol ();
  virtual ~RoutingProtocol ();
  virtual void DoDispose ();

  // Inherited from Ipv4RoutingProtocol
  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  // Change this in aco-routing-protocol.h
virtual bool RouteInput (Ptr<const Packet> p, 
                         const Ipv4Header &header, 
                         Ptr<const NetDevice> idev,
                         const UnicastForwardCallback &ucb,           // Added const &
                         const MulticastForwardCallback &mcb,         // Added const &
                         const LocalDeliverCallback &lcb,             // Added const &
                         const ErrorCallback &ecb);                   // Added const &
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  // Attributes Accessors
  void SetMaxQueueLen (uint32_t len);
  uint32_t GetMaxQueueLen () const;
  void SetMaxQueueTime (Time t);
  Time GetMaxQueueTime () const;
  void SetGratuitousReplyFlag (bool f);
  bool GetGratuitousReplyFlag () const;
  void SetDestinationOnlyFlag (bool f);
  bool GetDestinationOnlyFlag () const;
  void SetHelloEnable (bool f);
  bool GetHelloEnable () const;
  void SetBroadcastEnable (bool f);
  bool GetBroadcastEnable () const;

  int64_t AssignStreams (int64_t stream);

  // Timer Expiration Handlers
  void RouteRequestTimerExpire (Ipv4Address dst);

protected:
  virtual void DoInitialize (void);

private:

  uint32_t m_rreqRetries;
  uint16_t m_ttlStart;
  uint16_t m_ttlIncrement;
  uint16_t m_ttlThreshold;
  uint16_t m_timeoutBuffer;
  uint32_t m_rreqRateLimit;
  uint32_t m_rerrRateLimit;
  Time m_activeRouteTimeout;
  uint32_t m_netDiameter;
  Time m_nodeTraversalTime;
  Time m_netTraversalTime;
  Time m_pathDiscoveryTime;
  Time m_myRouteTimeout;
  Time m_helloInterval;
  uint16_t m_allowedHelloLoss;
  Time m_deletePeriod;
  Time m_nextHopWait;
  Time m_blackListTimeout;
  uint32_t m_maxQueueLen;
  Time m_maxQueueTime;
  bool m_destinationOnly;
  bool m_gratuitousReply;
  bool m_enableHello;
  bool m_enableBroadcast;
  Time m_discoveryStart;    // To track when a route search begins
  uint32_t m_totalAntsSent; // To track the total number of RREQs (Ants)
  int m_simulatedQueue

  // IP Layer and Sockets
  Ptr<Ipv4> m_ipv4;
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketSubnetBroadcastAddresses;
  Ptr<NetDevice> m_lo;

  // Protocol State Components
  RoutingTable m_routingTable;
  RequestQueue m_queue;
  uint32_t m_requestId;
  uint32_t m_seqNo;
  IdCache m_rreqIdCache;
  DuplicatePacketDetection m_dpd;
  Neighbors m_nb;
  uint32_t m_rreqCount;
  uint32_t m_rerrCount;

  // Timers and Event IDs
  Timer m_htimer;
  Timer m_rreqRateLimitTimer;
  Timer m_rerrRateLimitTimer;
  
  // Tracking RREQ retries using EventId for safety/efficiency
  std::map<Ipv4Address, EventId> m_addressReqTimer;

  Time m_lastBcastTime;
  Ptr<UniformRandomVariable> m_uniformRandomVariable;

  // Internal Helper Methods
  void Start ();
  void DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  void SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route);
  void SendRequest (Ipv4Address dst);
  void ScheduleRreqRetry (Ipv4Address dst);
  void SendReply (const RreqHeader & header, const RoutingTableEntry & toOrigin);
  void SendReplyByIntermediateNode (RoutingTableEntry & toDst, RoutingTableEntry & toOrigin, bool gratRep);
  void SendReplyAck (Ipv4Address neighbor);
  void SendHello ();
  void SendRerrWhenBreaksLinkToNextHop (Ipv4Address nextHop);
  void SendRerrWhenNoRouteToForward (Ipv4Address dst, uint32_t dstSeqNo, Ipv4Address origin);
  void SendRerrMessage (Ptr<Packet> packet, std::vector<Ipv4Address> precursors);
  void SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);

  // Packet Reception
  void RecvAco (Ptr<Socket> socket);
  void RecvRequest (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);
  void RecvReply (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender);
  void RecvReplyAck (Ipv4Address neighbor);
  void RecvError (Ptr<Packet> p, Ipv4Address src);
  
  // Route Management Helpers
  bool UpdateRouteLifeTime (Ipv4Address addr, Time lifetime);
  void UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver);
  void ProcessHello (const RrepHeader & rrepHeader, Ipv4Address receiver);
  void AckTimerExpire (Ipv4Address neighbor, Time blacklistTimeout);
  void RreqRateLimitTimerExpire ();
  void RerrRateLimitTimerExpire ();
  void HelloTimerExpire ();

  bool IsMyOwnAddress (Ipv4Address src);
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif) const;
  Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
  Ptr<Socket> FindSubnetBroadcastSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
};

} // namespace aco
} // namespace ns3

#endif /* ACO_ROUTING_PROTOCOL_H */
