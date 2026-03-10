/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 * NS-2 ACO model developed by the CMU/MONARCH group and optimized and
 * tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 * ACO-UU implementation by Erik Nordström of Uppsala University
 * https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/ACO-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 * Pavel Boyko <boyko@iitp.ru>
 */

#include "aco-rtable.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AcoRoutingTable");

namespace aco
{

/*
 The Routing Table Entry Implementation
 */

RoutingTableEntry::RoutingTableEntry(Ptr<NetDevice> dev,
                                     Ipv4Address dst,
                                     bool vSeqNo,
                                     uint32_t seqNo,
                                     Ipv4InterfaceAddress iface,
                                     uint16_t hops,
                                     Ipv4Address nextHop,
                                     Time lifetime)
    : m_ackTimer(Timer::CANCEL_ON_DESTROY),
      m_validSeqNo(vSeqNo),
      m_seqNo(seqNo),
      m_hops(hops),
      m_lifeTime(lifetime + Simulator::Now()),
      m_iface(iface),
      m_flag(VALID),
      m_reqCount(0),
      m_blackListState(false),
      m_blackListTimeout(Simulator::Now()),
      m_pheromone(1.0),    // *** ACO ENHANCEMENT: Initial Pheromone ***
      m_stagnation(0)      // *** ACO ENHANCEMENT: Initial Stagnation ***
{
    m_ipv4Route = Create<Ipv4Route>();
    m_ipv4Route->SetDestination(dst);
    m_ipv4Route->SetGateway(nextHop);
    m_ipv4Route->SetSource(m_iface.GetLocal());
    m_ipv4Route->SetOutputDevice(dev);
}

RoutingTableEntry::~RoutingTableEntry()
{
}

bool
RoutingTableEntry::InsertPrecursor(Ipv4Address id)
{
    NS_LOG_FUNCTION(this << id);
    if (!LookupPrecursor(id))
    {
        m_precursorList.push_back(id);
        return true;
    }
    else
    {
        return false;
    }
}

bool
RoutingTableEntry::LookupPrecursor(Ipv4Address id)
{
    NS_LOG_FUNCTION(this << id);
    for (auto i = m_precursorList.begin(); i != m_precursorList.end(); ++i)
    {
        if (*i == id)
        {
            NS_LOG_LOGIC("Precursor " << id << " found");
            return true;
        }
    }
    NS_LOG_LOGIC("Precursor " << id << " not found");
    return false;
}

bool
RoutingTableEntry::DeletePrecursor(Ipv4Address id)
{
    NS_LOG_FUNCTION(this << id);
    auto i = std::remove(m_precursorList.begin(), m_precursorList.end(), id);
    if (i == m_precursorList.end())
    {
        NS_LOG_LOGIC("Precursor " << id << " not found");
        return false;
    }
    else
    {
        NS_LOG_LOGIC("Precursor " << id << " found");
        m_precursorList.erase(i, m_precursorList.end());
    }
    return true;
}

void
RoutingTableEntry::DeleteAllPrecursors()
{
    NS_LOG_FUNCTION(this);
    m_precursorList.clear();
}

bool
RoutingTableEntry::IsPrecursorListEmpty() const
{
    return m_precursorList.empty();
}

void
RoutingTableEntry::GetPrecursors(std::vector<Ipv4Address>& prec) const
{
    NS_LOG_FUNCTION(this);
    if (IsPrecursorListEmpty())
    {
        return;
    }
    for (auto i = m_precursorList.begin(); i != m_precursorList.end(); ++i)
    {
        bool result = true;
        for (auto j = prec.begin(); j != prec.end(); ++j)
        {
            if (*j == *i)
            {
                result = false;
                break;
            }
        }
        if (result)
        {
            prec.push_back(*i);
        }
    }
}

void
RoutingTableEntry::Invalidate(Time badLinkLifetime)
{
    NS_LOG_FUNCTION(this << badLinkLifetime.As(Time::S));
    if (m_flag == INVALID)
    {
        return;
    }
    m_flag = INVALID;
    m_reqCount = 0;
    m_lifeTime = badLinkLifetime + Simulator::Now();
    m_pheromone = 0.0; // *** ACO ENHANCEMENT: Clear pheromone on invalidation ***
}

void
RoutingTableEntry::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    std::ostringstream dest;
    std::ostringstream gw;
    std::ostringstream iface;
    std::ostringstream expire;
    dest << m_ipv4Route->GetDestination();
    gw << m_ipv4Route->GetGateway();
    iface << m_iface.GetLocal();
    expire << std::setprecision(2) << (m_lifeTime - Simulator::Now()).As(unit);
    
    *os << std::setw(16) << dest.str();
    *os << std::setw(16) << gw.str();
    *os << std::setw(16) << iface.str();
    *os << std::setw(16);
    switch (m_flag)
    {
    case VALID: {
        *os << "UP";
        break;
    }
    case INVALID: {
        *os << "DOWN";
        break;
    }
    case IN_SEARCH: {
        *os << "IN_SEARCH";
        break;
    }
    }

    *os << std::setw(16) << expire.str();
    *os << std::setw(8) << m_hops;
    *os << std::fixed << std::setprecision(2) << m_pheromone << std::endl; // *** ACO ENHANCEMENT: Print Pheromone ***
    
    (*os).copyfmt(oldState);
}

/*
 The Routing Table Implementation
 */

RoutingTable::RoutingTable(Time t)
    : m_badLinkLifetime(t)
{
}

bool
RoutingTable::LookupRoute(Ipv4Address id, RoutingTableEntry& rt)
{
    NS_LOG_FUNCTION(this << id);
    Purge();
    if (m_ipv4AddressEntry.empty())
    {
        return false;
    }
    auto i = m_ipv4AddressEntry.find(id);
    if (i == m_ipv4AddressEntry.end())
    {
        return false;
    }
    rt = i->second;
    return true;
}

bool
RoutingTable::LookupValidRoute(Ipv4Address id, RoutingTableEntry& rt)
{
    if (!LookupRoute(id, rt))
    {
        return false;
    }
    return (rt.GetFlag() == VALID);
}

bool
RoutingTable::DeleteRoute(Ipv4Address dst)
{
    Purge();
    return (m_ipv4AddressEntry.erase(dst) != 0);
}

bool
RoutingTable::AddRoute(RoutingTableEntry& rt)
{
    Purge();
    if (rt.GetFlag() != IN_SEARCH)
    {
        rt.SetRreqCnt(0);
    }
    auto result = m_ipv4AddressEntry.insert(std::make_pair(rt.GetDestination(), rt));
    return result.second;
}

bool
RoutingTable::Update(RoutingTableEntry& rt)
{
    auto i = m_ipv4AddressEntry.find(rt.GetDestination());
    if (i == m_ipv4AddressEntry.end())
    {
        return false;
    }
    i->second = rt;
    return true;
}

bool
RoutingTable::SetEntryState(Ipv4Address id, RouteFlags state)
{
    auto i = m_ipv4AddressEntry.find(id);
    if (i == m_ipv4AddressEntry.end())
    {
        return false;
    }
    i->second.SetFlag(state);
    return true;
}

void
RoutingTable::GetListOfDestinationWithNextHop(Ipv4Address nextHop,
                                              std::map<Ipv4Address, uint32_t>& unreachable)
{
    Purge();
    unreachable.clear();
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end(); ++i)
    {
        if (i->second.GetNextHop() == nextHop)
        {
            unreachable.insert(std::make_pair(i->first, i->second.GetSeqNo()));
        }
    }
}

void
RoutingTable::InvalidateRoutesWithDst(const std::map<Ipv4Address, uint32_t>& unreachable)
{
    Purge();
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end(); ++i)
    {
        for (auto const& [addr, seq] : unreachable)
        {
            if ((i->first == addr) && (i->second.GetFlag() == VALID))
            {
                i->second.Invalidate(m_badLinkLifetime);
            }
        }
    }
}

void
RoutingTable::DeleteAllRoutesFromInterface(Ipv4InterfaceAddress iface)
{
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end();)
    {
        if (i->second.GetInterface() == iface)
        {
            i = m_ipv4AddressEntry.erase(i);
        }
        else
        {
            ++i;
        }
    }
}

void
RoutingTable::Purge()
{
    if (m_ipv4AddressEntry.empty()) return;
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end();)
    {
        if (i->second.GetLifeTime().IsStrictlyNegative())
        {
            if (i->second.GetFlag() == INVALID)
            {
                i = m_ipv4AddressEntry.erase(i);
            }
            else
            {
                i->second.Invalidate(m_badLinkLifetime);
                ++i;
            }
        }
        else
        {
            ++i;
        }
    }
}

void
RoutingTable::Purge(std::map<Ipv4Address, RoutingTableEntry>& table) const
{
    if (table.empty()) return;
    for (auto i = table.begin(); i != table.end();)
    {
        if (i->second.GetLifeTime().IsStrictlyNegative())
        {
            if (i->second.GetFlag() == INVALID)
            {
                i = table.erase(i);
            }
            else
            {
                i->second.Invalidate(m_badLinkLifetime);
                ++i;
            }
        }
        else
        {
            ++i;
        }
    }
}

bool
RoutingTable::MarkLinkAsUnidirectional(Ipv4Address neighbor, Time blacklistTimeout)
{
    auto i = m_ipv4AddressEntry.find(neighbor);
    if (i == m_ipv4AddressEntry.end()) return false;
    i->second.SetUnidirectional(true);
    i->second.SetBlacklistTimeout(blacklistTimeout);
    return true;
}

void
RoutingTable::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /* = Time::S */) const
{
    std::map<Ipv4Address, RoutingTableEntry> table = m_ipv4AddressEntry;
    Purge(table);
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "\nACO Routing table\n";
    *os << std::setw(16) << "Destination";
    *os << std::setw(16) << "Gateway";
    *os << std::setw(16) << "Interface";
    *os << std::setw(16) << "Flag";
    *os << std::setw(16) << "Expire";
    *os << std::setw(8) << "Hops";
    *os << "Pheromone" << std::endl; // *** ACO ENHANCEMENT ***
    for (auto const& [addr, entry] : table)
    {
        entry.Print(stream, unit);
    }
    *os << "\n";
}
std::vector<RoutingTableEntry>
RoutingTable::GetAllRoutes()
{
    std::vector<RoutingTableEntry> routes;

    for (auto const &entry : m_ipv4AddressEntry)
    {
        routes.push_back(entry.second);
    }

    return routes;
}
} // namespace aco
} // namespace ns3
