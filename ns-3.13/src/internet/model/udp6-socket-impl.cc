/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/udp6-socket-factory.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "udp6-socket-impl.h"
#include "udp6-l4-protocol.h"
#include "ipv6-end-point.h"
#include <limits>

NS_LOG_COMPONENT_DEFINE ("Udp6SocketImpl");

namespace ns3 {

static const uint32_t MAX_IPV6_UDP_DATAGRAM_SIZE = 65507;

// Add attributes generic to all UdpSockets to base class UdpSocket
TypeId
Udp6SocketImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Udp6SocketImpl")
    .SetParent<UdpSocket> ()
    .AddConstructor<Udp6SocketImpl> ()
    .AddTraceSource ("Drop", "Drop UDP packet due to receive buffer overflow",
                     MakeTraceSourceAccessor (&Udp6SocketImpl::m_dropTrace))
    .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&Udp6SocketImpl::m_icmpCallback),
                   MakeCallbackChecker ())
  ;
  return tid;
}

Udp6SocketImpl::Udp6SocketImpl ()
  : m_endPoint (0),
    m_node (0),
    m_udp (0),
    m_errno (ERROR_NOTERROR),
    m_shutdownSend (false),
    m_shutdownRecv (false),
    m_connected (false),
    m_rxAvailable (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_allowBroadcast = false;
}

Udp6SocketImpl::~Udp6SocketImpl ()
{
  NS_LOG_FUNCTION_NOARGS ();

  // XXX todo:  leave any multicast groups that have been joined
  m_node = 0;
  if (m_endPoint != 0)
    {
      NS_ASSERT (m_udp != 0);
      /**
       * Note that this piece of code is a bit tricky:
       * when DeAllocate is called, it will call into
       * Ipv4EndPointDemux::Deallocate which triggers
       * a delete of the associated endPoint which triggers
       * in turn a call to the method ::Destroy below
       * will will zero the m_endPoint field.
       */
      NS_ASSERT (m_endPoint != 0);
      m_udp->DeAllocate (m_endPoint);
      NS_ASSERT (m_endPoint == 0);
    }
  m_udp = 0;
}

void 
Udp6SocketImpl::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;

}
void 
Udp6SocketImpl::SetUdp (Ptr<Udp6L4Protocol> udp)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_udp = udp;
}


enum Socket::SocketErrno
Udp6SocketImpl::GetErrno (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_errno;
}

enum Socket::SocketType
Udp6SocketImpl::GetSocketType (void) const
{
  return NS3_SOCK_DGRAM;
}

Ptr<Node>
Udp6SocketImpl::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void 
Udp6SocketImpl::Destroy (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = 0;
  m_endPoint = 0;
  m_udp = 0;
}

int
Udp6SocketImpl::FinishBind (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_endPoint == 0)
    {
      return -1;
    }
  m_endPoint->SetRxCallback (MakeCallback (&Udp6SocketImpl::ForwardUp, Ptr<Udp6SocketImpl> (this)));
  m_endPoint->SetIcmpCallback (MakeCallback (&Udp6SocketImpl::ForwardIcmp, Ptr<Udp6SocketImpl> (this)));
  m_endPoint->SetDestroyCallback (MakeCallback (&Udp6SocketImpl::Destroy, Ptr<Udp6SocketImpl> (this)));
  return 0;
}

int
Udp6SocketImpl::Bind (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_endPoint = m_udp->Allocate ();
  return FinishBind ();
}

int 
Udp6SocketImpl::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);

  if (!Inet6SocketAddress::IsMatchingType (address))
    {
      NS_LOG_ERROR ("Not IsMatchingType");
      m_errno = ERROR_INVAL;
      return -1;
    }
  Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
  Ipv6Address ipv6 = transport.GetIpv6 ();
  uint16_t port = transport.GetPort ();
  if (ipv6 == Ipv6Address::GetAny () && port == 0)
    {
      m_endPoint = m_udp->Allocate ();
    }
  else if (ipv6 == Ipv6Address::GetAny () && port != 0)
    {
      m_endPoint = m_udp->Allocate (port);
    }
  else if (ipv6 != Ipv6Address::GetAny () && port == 0)
    {
      m_endPoint = m_udp->Allocate (ipv6);
    }
  else if (ipv6 != Ipv6Address::GetAny () && port != 0)
    {
      m_endPoint = m_udp->Allocate (ipv6, port);
    }

  return FinishBind ();
}

int 
Udp6SocketImpl::ShutdownSend (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_shutdownSend = true;
  return 0;
}

int 
Udp6SocketImpl::ShutdownRecv (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_shutdownRecv = true;
  return 0;
}

int
Udp6SocketImpl::Close (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_shutdownRecv == true && m_shutdownSend == true)
    {
      m_errno = Socket::ERROR_BADF;
      return -1;
    }
  m_shutdownRecv = true;
  m_shutdownSend = true;
  return 0;
}

int
Udp6SocketImpl::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
  m_defaultAddress = transport.GetIpv6 ();
  m_defaultPort = transport.GetPort ();
  NotifyConnectionSucceeded ();
  m_connected = true;

  return 0;
}

int 
Udp6SocketImpl::Listen (void)
{
  m_errno = Socket::ERROR_OPNOTSUPP;
  return -1;
}

int 
Udp6SocketImpl::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p << flags);

  if (!m_connected)
    {
      m_errno = ERROR_NOTCONN;
      return -1;
    }
  return DoSend (p);
}

int 
Udp6SocketImpl::DoSend (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return -1;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  if (m_shutdownSend)
    {
      m_errno = ERROR_SHUTDOWN;
      return -1;
    } 

  return DoSendTo (p, m_defaultAddress, m_defaultPort);
}

int
Udp6SocketImpl::DoSendTo (Ptr<Packet> p, const Address &address)
{
  NS_LOG_FUNCTION (this << p << address);

  if (!m_connected)
    {
      NS_LOG_LOGIC ("Not connected");
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address ipv6 = transport.GetIpv6 ();
      uint16_t port = transport.GetPort ();
      return DoSendTo (p, ipv6, port);
    }
  else
    {
      // connected UDP socket must use default addresses
      NS_LOG_LOGIC ("Connected");
      return DoSendTo (p, m_defaultAddress, m_defaultPort);
    }
}

int
Udp6SocketImpl::DoSendTo (Ptr<Packet> p, Ipv6Address dest, uint16_t port)
{
  NS_LOG_FUNCTION (this << p << dest << port);
  if (m_boundnetdevice)
    {
      NS_LOG_LOGIC ("Bound interface number " << m_boundnetdevice->GetIfIndex ());
    }
  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return -1;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  if (m_shutdownSend)
    {
      m_errno = ERROR_SHUTDOWN;
      return -1;
    }

  if (p->GetSize () > GetTxAvailable () )
    {
      m_errno = ERROR_MSGSIZE;
      return -1;
    }

  Ptr<Ipv6> ipv6 = m_node->GetObject<Ipv6> ();

  // Locally override the IP TTL for this socket
  // We cannot directly modify the TTL at this stage, so we set a Packet tag
  // The destination can be either multicast, unicast/anycast, or
  // either all-hosts broadcast or limited (subnet-directed) broadcast.
  // For the latter two broadcast types, the TTL will later be set to one
  // irrespective of what is set in these socket options.  So, this tagging
  // may end up setting the TTL of a limited broadcast packet to be
  // the same as a unicast, but it will be fixed further down the stack
  if (m_ipMulticastTtl != 0 && dest.IsMulticast ())
    {
      SocketIpTtlTag tag;
      tag.SetTtl (m_ipMulticastTtl);
      p->AddPacketTag (tag);
    }
  else if (m_ipTtl != 0 && !dest.IsMulticast ())
    {
      SocketIpTtlTag tag;
      tag.SetTtl (m_ipTtl);
      p->AddPacketTag (tag);
    }
  {
    SocketSetDontFragmentTag tag;
    bool found = p->RemovePacketTag (tag);
    if (!found)
      {
        if (m_mtuDiscover)
          {
            tag.Enable ();
          }
        else
          {
            tag.Disable ();
          }
        p->AddPacketTag (tag);
      }
  }
  //
  // If dest is set to the limited broadcast address (all ones),
  // convert it to send a copy of the packet out of every 
  // interface as a subnet-directed broadcast.
  // Exception:  if the interface has a /32 address, there is no
  // valid subnet-directed broadcast, so send it as limited broadcast
  // Note also that some systems will only send limited broadcast packets
  // out of the "default" interface; here we send it out all interfaces
  //
/*
  if (dest.IsBroadcast ())
    {
      if (!m_allowBroadcast)
        {
          m_errno = ERROR_OPNOTSUPP;
          return -1;
        }
      NS_LOG_LOGIC ("Limited broadcast start.");
      for (uint32_t i = 0; i < ipv6->GetNInterfaces (); i++ )
        {
          // Get the primary address
          Ipv6InterfaceAddress iaddr = ipv6->GetAddress (i, 0);
          Ipv6Address addri = iaddr.GetLocal ();
          if (addri == Ipv6Address ("::1"))
            continue;
          // Check if interface-bound socket
          if (m_boundnetdevice) 
            {
              if (ipv6->GetNetDevice (i) != m_boundnetdevice)
                continue;
            }
          Ipv6Mask maski = iaddr.GetMask ();
          if (maski == Ipv4Mask::GetOnes ())
            {
              // if the network mask is 255.255.255.255, do not convert dest
              NS_LOG_LOGIC ("Sending one copy from " << addri << " to " << dest
                                                     << " (mask is " << maski << ")");
              m_udp->Send (p->Copy (), addri, dest,
                           m_endPoint->GetLocalPort (), port);
              NotifyDataSent (p->GetSize ());
              NotifySend (GetTxAvailable ());
            }
          else
            {
              // Convert to subnet-directed broadcast
              Ipv4Address bcast = addri.GetSubnetDirectedBroadcast (maski);
              NS_LOG_LOGIC ("Sending one copy from " << addri << " to " << bcast
                                                     << " (mask is " << maski << ")");
              m_udp->Send (p->Copy (), addri, bcast,
                           m_endPoint->GetLocalPort (), port);
              NotifyDataSent (p->GetSize ());
              NotifySend (GetTxAvailable ());
            }
        }
      NS_LOG_LOGIC ("Limited broadcast end.");
      return p->GetSize ();
    }
  else */if (m_endPoint->GetLocalAddress () != Ipv6Address::GetAny ())
    {
      m_udp->Send (p->Copy (), m_endPoint->GetLocalAddress (), dest,
                   m_endPoint->GetLocalPort (), port, 0);
      NotifyDataSent (p->GetSize ());
      NotifySend (GetTxAvailable ());
      return p->GetSize ();
    }
  else if (ipv6->GetRoutingProtocol () != 0)
    {
      Ipv6Header header;
      header.SetDestinationAddress (dest);
      header.SetNextHeader (Udp6L4Protocol::PROT_NUMBER);
      Socket::SocketErrno errno_;
      Ptr<Ipv6Route> route;
      Ptr<NetDevice> oif = m_boundnetdevice; //specify non-zero if bound to a specific device
      // TBD-- we could cache the route and just check its validity
      route = ipv6->GetRoutingProtocol ()->RouteOutput (p, header, oif, errno_); 
      if (route != 0)
        {
          NS_LOG_LOGIC ("Route exists");
          if (!m_allowBroadcast)
            {
/*              uint32_t outputIfIndex = ipv6->GetInterfaceForDevice (route->GetOutputDevice ());
              uint32_t ifNAddr = ipv6->GetNAddresses (outputIfIndex);
              for (uint32_t addrI = 0; addrI < ifNAddr; ++addrI)
                {
                  Ipv6InterfaceAddress ifAddr = ipv6->GetAddress (outputIfIndex, addrI);
                  if (dest == ifAddr.GetBroadcast ())
                    {
                      m_errno = ERROR_OPNOTSUPP;
                      return -1;
                    }
                } */
            }

          header.SetSourceAddress (route->GetSource ());
          m_udp->Send (p->Copy (), header.GetSourceAddress (), header.GetDestinationAddress (),
                       m_endPoint->GetLocalPort (), port, route);
          NotifyDataSent (p->GetSize ());
          return p->GetSize ();
        }
      else 
        {
          NS_LOG_LOGIC ("No route to destination");
          NS_LOG_ERROR (errno_);
          m_errno = errno_;
          return -1;
        }
    }
  else
    {
      NS_LOG_ERROR ("ERROR_NOROUTETOHOST");
      m_errno = ERROR_NOROUTETOHOST;
      return -1;
    }

  return 0;
}

// XXX maximum message size for UDP broadcast is limited by MTU
// size of underlying link; we are not checking that now.
uint32_t
Udp6SocketImpl::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // No finite send buffer is modelled, but we must respect
  // the maximum size of an IP datagram (65535 bytes - headers).
  return MAX_IPV6_UDP_DATAGRAM_SIZE;
}

int 
Udp6SocketImpl::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
  NS_LOG_FUNCTION (this << p << flags << address);
  Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
  Ipv6Address ipv6 = transport.GetIpv6 ();
  uint16_t port = transport.GetPort ();
  return DoSendTo (p, ipv6, port);
}

uint32_t
Udp6SocketImpl::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // We separately maintain this state to avoid walking the queue 
  // every time this might be called
  return m_rxAvailable;
}

Ptr<Packet>
Udp6SocketImpl::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  if (m_deliveryQueue.empty () )
    {
      m_errno = ERROR_AGAIN;
      return 0;
    }
  Ptr<Packet> p = m_deliveryQueue.front ();
  if (p->GetSize () <= maxSize) 
    {
      m_deliveryQueue.pop ();
      m_rxAvailable -= p->GetSize ();
    }
  else
    {
      p = 0; 
    }
  return p;
}

Ptr<Packet>
Udp6SocketImpl::RecvFrom (uint32_t maxSize, uint32_t flags, 
                         Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  Ptr<Packet> packet = Recv (maxSize, flags);
  if (packet != 0)
    {
      SocketAddressTag tag;
      bool found;
      found = packet->PeekPacketTag (tag);
      NS_ASSERT (found);
      //cast found to void, to suppress 'found' set but not used,compiler warning
      //in optimized builds
      (void) found;
      fromAddress = tag.GetAddress ();
    }
  return packet;
}

int
Udp6SocketImpl::GetSockName (Address &address) const
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_endPoint != 0)
    {
      address = Inet6SocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
    }
  else
    {
      // It is possible to call this method on a socket without a name
      // in which case, behavior is unspecified
      address = Inet6SocketAddress (Ipv6Address::GetZero (), 0);
    }
  return 0;
}

int 
Udp6SocketImpl::MulticastJoinGroup (uint32_t interface, const Address &groupAddress)
{
  NS_LOG_FUNCTION (interface << groupAddress);
  /*
   1) sanity check interface
   2) sanity check that it has not been called yet on this interface/group
   3) determine address family of groupAddress
   4) locally store a list of (interface, groupAddress)
   5) call ipv4->MulticastJoinGroup () or Ipv6->MulticastJoinGroup ()
  */
  return 0;
} 

int 
Udp6SocketImpl::MulticastLeaveGroup (uint32_t interface, const Address &groupAddress) 
{
  NS_LOG_FUNCTION (interface << groupAddress);
  /*
   1) sanity check interface
   2) determine address family of groupAddress
   3) delete from local list of (interface, groupAddress); raise a LOG_WARN
      if not already present (but return 0) 
   5) call ipv4->MulticastLeaveGroup () or Ipv6->MulticastLeaveGroup ()
  */
  return 0;
}

void
Udp6SocketImpl::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice);
  Socket::BindToNetDevice (netdevice); // Includes sanity check
  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  m_endPoint->BindToNetDevice (netdevice);
  return;
}

void 
Udp6SocketImpl::ForwardUp (Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, 
                           uint16_t sport, uint16_t dport, Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << saddr << daddr << sport << dport);

  if (m_shutdownRecv)
    {
      return;
    }

  // Should check via getsockopt ()..
  if (this->m_recvpktinfo)
    {
      Ipv6PacketInfoTag tag;
      packet->RemovePacketTag (tag);
      tag.SetRecvIf (incomingInterface->GetDevice ()->GetIfIndex ());
      packet->AddPacketTag (tag);
    }

  if ((m_rxAvailable + packet->GetSize ()) <= m_rcvBufSize)
    {
      Address address = Inet6SocketAddress (saddr, sport);
      SocketAddressTag tag;
      tag.SetAddress (address);
      packet->AddPacketTag (tag);
      m_deliveryQueue.push (packet);
      m_rxAvailable += packet->GetSize ();
      NotifyDataRecv ();
    }
  else
    {
      // In general, this case should not occur unless the
      // receiving application reads data from this socket slowly
      // in comparison to the arrival rate
      //
      // drop and trace packet
      NS_LOG_WARN ("No receive buffer space available.  Drop.");
      m_dropTrace (packet);
    }
}

void
Udp6SocketImpl::ForwardIcmp (Ipv6Address icmpSource, uint8_t icmpTtl, 
                            uint8_t icmpType, uint8_t icmpCode,
                            uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback.IsNull ())
    {
      m_icmpCallback (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}


void 
Udp6SocketImpl::SetRcvBufSize (uint32_t size)
{
  m_rcvBufSize = size;
}

uint32_t 
Udp6SocketImpl::GetRcvBufSize (void) const
{
  return m_rcvBufSize;
}

void 
Udp6SocketImpl::SetIpTtl (uint8_t ipTtl)
{
  m_ipTtl = ipTtl;
}

uint8_t 
Udp6SocketImpl::GetIpTtl (void) const
{
  return m_ipTtl;
}

void 
Udp6SocketImpl::SetIpMulticastTtl (uint8_t ipTtl)
{
  m_ipMulticastTtl = ipTtl;
}

uint8_t 
Udp6SocketImpl::GetIpMulticastTtl (void) const
{
  return m_ipMulticastTtl;
}

void 
Udp6SocketImpl::SetIpMulticastIf (int32_t ipIf)
{
  m_ipMulticastIf = ipIf;
}

int32_t 
Udp6SocketImpl::GetIpMulticastIf (void) const
{
  return m_ipMulticastIf;
}

void 
Udp6SocketImpl::SetIpMulticastLoop (bool loop)
{
  m_ipMulticastLoop = loop;
}

bool 
Udp6SocketImpl::GetIpMulticastLoop (void) const
{
  return m_ipMulticastLoop;
}

void 
Udp6SocketImpl::SetMtuDiscover (bool discover)
{
  m_mtuDiscover = discover;
}
bool 
Udp6SocketImpl::GetMtuDiscover (void) const
{
  return m_mtuDiscover;
}

bool
Udp6SocketImpl::SetAllowBroadcast (bool allowBroadcast)
{
  m_allowBroadcast = allowBroadcast;
  return true;
}

bool
Udp6SocketImpl::GetAllowBroadcast () const
{
  return m_allowBroadcast;
}


} //namespace ns3
