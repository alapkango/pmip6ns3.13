/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Proxy Mobile IPv6 (PMIPv6) (RFC5213) Implementation
 *
 * Copyright (c) 2010 KUT, ETRI
 * (Korea Univerity of Technology and Education)
 * (Electronics and Telecommunications Research Institute)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hyon-Young Choi <commani@gmail.com>
 */

#ifndef BINDING_CACHE_H
#define BINDING_CACHE_H

#include <stdint.h>

#include <list>

#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/net-device.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/timer.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/identifier.h"

namespace ns3
{

class BindingCache : public Object
{
public:
  class Entry;
  
  static TypeId GetTypeId ();

  BindingCache();
  
  ~BindingCache();
  
  BindingCache::Entry *Lookup(Identifier mnId, std::list<Ipv6Address> hnpList, bool &allMatched);
  BindingCache::Entry *Lookup(Identifier mnId, uint8_t att, Identifier mnLinkId);
  BindingCache::Entry *Lookup(Identifier mnId);
  
  BindingCache::Entry *Add(Identifier mnId);
  
  void Remove(BindingCache::Entry *entry);
  
  void Flush();
  
  Ptr<Node> GetNode() const;
  void SetNode(Ptr<Node> node);
  
  class Entry
  {
  public:
    Entry(Ptr<BindingCache> bcache);
    
    Entry *Copy();
    
    bool IsUnreachable() const;
    bool IsDeregistering() const;
    bool IsRegistering() const;
    bool IsReachable() const;
    
    void MarkUnreachable();
    void MarkDeregistering();
    void MarkRegistering();
    void MarkReachable();
    
    void StartReachableTimer();
    void StopReachableTimer();
    
    void StartDeregisterTimer();
    void StopDeregisterTimer();
    
    void StartRegisterTimer();
    void StopRegisterTimer();
    
    void FunctionReachableTimeout();
    void FunctionDeregisterTimeout();
    void FunctionRegisterTimeout();
    
    bool Match(Identifier mnId, std::list<Ipv6Address> hnpList, bool &allMatched) const;
    bool Match(Identifier mnId, uint8_t att, Identifier mnLinkId) const;
    
    Identifier GetMnIdentifier() const;
    void SetMnIdentifier(Identifier mnId);
    
    Identifier GetMnLinkIdentifier() const;
    void SetMnLinkIdentifier(Identifier mnLinkId);
    
    std::list<Ipv6Address> GetHomeNetworkPrefixes() const;
    void SetHomeNetworkPrefixes(std::list<Ipv6Address> hnpList);
    
    Ipv6Address GetMagLinkAddress() const;
    void SetMagLinkAddress(Ipv6Address lla);
    
    Ipv6Address GetProxyCoa() const;
    void SetProxyCoa(Ipv6Address pcoa);
    
    int16_t GetTunnelIfIndex() const;
    void SetTunnelIfIndex(int16_t tunnelif);
    
    uint8_t GetAccessTechnologyType() const;
    void SetAccessTechnologyType(uint8_t att);
    
    uint8_t GetHandoffIndicator() const;
    void SetHandoffIndicator(uint8_t hi);
    
    Time GetLastBindingUpdateTime() const;
    void SetLastBindingUpdateTime(Time tm);
    
    Time GetReachableTime() const;
    void SetReachableTime(Time tm);
    
    uint16_t GetLastBindingUpdateSequence() const;
    void SetLastBindingUpdateSequence(uint16_t seq);
    
    Entry *GetNext() const;
    void SetNext(Entry *entry);
    
    Entry *GetTentativeEntry() const;
    void SetTentativeEntry(Entry *entry);
    
    Ipv6Address GetOldProxyCoa() const;
    
  private:
    Ptr<BindingCache> m_bCache;
    
    enum BindingCacheState_e {
      UNREACHABLE,
      DEREGISTERING,
      REGISTERING,
      REACHABLE
    };
    
    BindingCacheState_e m_state;
    
    Identifier m_mnIdentifier;
    
    Identifier m_mnLinkIdentifier;
    
    std::list<Ipv6Address> m_homeNetworkPrefixes;
    
    Ipv6Address m_magLinkAddress;
    
    Ipv6Address m_proxyCoa;
    
    int16_t m_tunnelIfIndex;
    
    uint8_t m_accessTechnologyType;
    
    uint8_t m_handoffIndicator;
    
    Time m_lastBindingUpdateTime;
    uint16_t m_lastBindingUpdateSequence;
    
    Time m_reachableTime;
    Timer m_reachableTimer;
    Timer m_deregisterTimer;
    Timer m_registerTimer;
    
    Entry *m_next;
    
    //internal
    
    Entry *m_tentativeEntry;
    Ipv6Address m_oldProxyCoa;

  };
  
protected:

private:
  typedef sgi::hash_map<Identifier, BindingCache::Entry *, IdentifierHash> BCache;
  typedef sgi::hash_map<Identifier, BindingCache::Entry *, IdentifierHash>::iterator BCacheI;
  
  void DoDispose();
  
  BCache m_bCache;
  
  Ptr<Node> m_node;
};

} /* ns3 */

#endif /* BINDING_CACHE_H */
