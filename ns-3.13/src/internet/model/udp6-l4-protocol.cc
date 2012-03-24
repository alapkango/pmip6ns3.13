/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 *
 * Copyed by Hyon-Young Choi <commani@gmail.com>
 * UDPv6 Implementation with MOFI Project
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/boolean.h"
#include "ns3/object-vector.h"
#include "ns3/ipv6-route.h"

#include "udp6-l4-protocol.h"
#include "udp-header.h"
#include "udp6-socket-factory-impl.h"
#include "ipv6-end-point-demux.h"
#include "ipv6-end-point.h"
#include "ipv6-l3-protocol.h"
#include "udp6-socket-impl.h"

NS_LOG_COMPONENT_DEFINE ("Udp6L4Protocol");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Udp6L4Protocol);

/* see http://www.iana.org/assignments/protocol-numbers */
const uint8_t Udp6L4Protocol::PROT_NUMBER = 17;

TypeId 
Udp6L4Protocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Udp6L4Protocol")
    .SetParent<Ipv6L4Protocol> ()
    .AddConstructor<Udp6L4Protocol> ()
    .AddAttribute ("SocketList", "The list of sockets associated to this protocol.",
                   ObjectVectorValue (),
                   MakeObjectVectorAccessor (&Udp6L4Protocol::m_sockets),
                   MakeObjectVectorChecker<Udp6SocketImpl> ())
  ;
  return tid;
}

Udp6L4Protocol::Udp6L4Protocol ()
  : m_endPoints (new Ipv6EndPointDemux ())
{
  NS_LOG_FUNCTION_NOARGS ();
}

Udp6L4Protocol::~Udp6L4Protocol ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void 
Udp6L4Protocol::SetNode (Ptr<Node> node)
{
  m_node = node;
}

/*
 * This method is called by AddAgregate and completes the aggregation
 * by setting the node in the udp stack and link it to the ipv4 object
 * present in the node along with the socket factory
 */
void
Udp6L4Protocol::NotifyNewAggregate ()
{
  if (m_node == 0)
    {
      Ptr<Node> node = this->GetObject<Node> ();
      if (node != 0)
        {
          Ptr<Ipv6L3Protocol> ipv6 = this->GetObject<Ipv6L3Protocol> ();
          if (ipv6 != 0)
            {
              this->SetNode (node);
              ipv6->Insert (this);
              Ptr<Udp6SocketFactoryImpl> udpFactory = CreateObject<Udp6SocketFactoryImpl> ();
              udpFactory->SetUdp (this);
              node->AggregateObject (udpFactory);
            }
        }
    }
  Object::NotifyNewAggregate ();
}

int 
Udp6L4Protocol::GetProtocolNumber (void) const
{
  return PROT_NUMBER;
}


void
Udp6L4Protocol::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  for (std::vector<Ptr<Udp6SocketImpl> >::iterator i = m_sockets.begin (); i != m_sockets.end (); i++)
    {
      *i = 0;
    }
  m_sockets.clear ();

  if (m_endPoints != 0)
    {
      delete m_endPoints;
      m_endPoints = 0;
    }
  m_node = 0;
  
  Ipv6L4Protocol::DoDispose ();
}

Ptr<Socket>
Udp6L4Protocol::CreateSocket (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Udp6SocketImpl> socket = CreateObject<Udp6SocketImpl> ();
  socket->SetNode (m_node);
  socket->SetUdp (this);
  m_sockets.push_back (socket);
  return socket;
}

Ipv6EndPoint *
Udp6L4Protocol::Allocate (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_endPoints->Allocate ();
}

Ipv6EndPoint *
Udp6L4Protocol::Allocate (Ipv6Address address)
{
  NS_LOG_FUNCTION (this << address);
  return m_endPoints->Allocate (address);
}

Ipv6EndPoint *
Udp6L4Protocol::Allocate (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);
  return m_endPoints->Allocate (port);
}

Ipv6EndPoint *
Udp6L4Protocol::Allocate (Ipv6Address address, uint16_t port)
{
  NS_LOG_FUNCTION (this << address << port);
  return m_endPoints->Allocate (address, port);
}
Ipv6EndPoint *
Udp6L4Protocol::Allocate (Ipv6Address localAddress, uint16_t localPort,
                         Ipv6Address peerAddress, uint16_t peerPort)
{
  NS_LOG_FUNCTION (this << localAddress << localPort << peerAddress << peerPort);
  return m_endPoints->Allocate (localAddress, localPort,
                                peerAddress, peerPort);
}

void 
Udp6L4Protocol::DeAllocate (Ipv6EndPoint *endPoint)
{
  NS_LOG_FUNCTION (this << endPoint);
  m_endPoints->DeAllocate (endPoint);
}

void 
Udp6L4Protocol::ReceiveIcmp (Ipv6Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv6Address payloadSource,Ipv6Address payloadDestination,
                            const uint8_t* payload)
{
  NS_LOG_FUNCTION (this << icmpSource << icmpTtl << icmpType << icmpCode << icmpInfo 
                        << payloadSource << payloadDestination);
  uint16_t src, dst;
  src = payload[0] << 8;
  src |= payload[1];
  dst = payload[2] << 8;
  dst |= payload[3];

  Ipv6EndPoint *endPoint = m_endPoints->SimpleLookup (payloadSource, src, payloadDestination, dst);
  if (endPoint != 0)
    {
      endPoint->ForwardIcmp (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
  else
    {
      NS_LOG_DEBUG ("no endpoint found source=" << payloadSource <<
                    ", destination="<<payloadDestination<<
                    ", src=" << src << ", dst=" << dst);
    }
}

enum Ipv6L4Protocol::RxStatus_e
Udp6L4Protocol::Receive (Ptr<Packet> packet,
                         Ipv6Address const &src, 
                         Ipv6Address const &dst,
                         Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << src << dst);
  UdpHeader udpHeader;
  if(Node::ChecksumEnabled ())
    {
      udpHeader.EnableChecksums ();
    }

//  udpHeader.InitializeChecksum (header.GetSourceAddress (), header.GetDestinationAddress (), PROT_NUMBER);

  packet->RemoveHeader (udpHeader);

  if(!udpHeader.IsChecksumOk ())
    {
      NS_LOG_INFO ("Bad checksum : dropping packet!");
      return Ipv6L4Protocol::RX_CSUM_FAILED;
    }

  NS_LOG_DEBUG ("Looking up dst " << dst << " port " << udpHeader.GetDestinationPort ()); 
  Ipv6EndPointDemux::EndPoints endPoints =
    m_endPoints->Lookup (dst, udpHeader.GetDestinationPort (),
                         src, udpHeader.GetSourcePort (), interface);
  if (endPoints.empty ())
    {
      NS_LOG_LOGIC ("RX_ENDPOINT_UNREACH");
      return Ipv6L4Protocol::RX_ENDPOINT_UNREACH;
    }
  for (Ipv6EndPointDemux::EndPointsI endPoint = endPoints.begin ();
       endPoint != endPoints.end (); endPoint++)
    {
      (*endPoint)->ForwardUp (packet->Copy (), src, 
	                          dst, udpHeader.GetSourcePort (), 
                              udpHeader.GetDestinationPort (), interface);
    }
  return Ipv6L4Protocol::RX_OK;
}

void
Udp6L4Protocol::Send (Ptr<Packet> packet, 
                     Ipv6Address saddr, Ipv6Address daddr, 
                     uint16_t sport, uint16_t dport)
{
  NS_LOG_FUNCTION (this << packet << saddr << daddr << sport << dport);
  
  Ptr<Ipv6L3Protocol> ipv6 =  m_node->GetObject<Ipv6L3Protocol> ();

  UdpHeader udpHeader;
  if(Node::ChecksumEnabled ())
    {
      udpHeader.EnableChecksums ();
//      udpHeader.InitializeChecksum (saddr,
//                                    daddr,
//                                    PROT_NUMBER);
    }
  udpHeader.SetDestinationPort (dport);
  udpHeader.SetSourcePort (sport);

  packet->AddHeader (udpHeader);

  ipv6->Send (packet, saddr, daddr, PROT_NUMBER, 0);
}

void
Udp6L4Protocol::Send (Ptr<Packet> packet, 
                     Ipv6Address saddr, Ipv6Address daddr, 
                     uint16_t sport, uint16_t dport, Ptr<Ipv6Route> route)
{
  NS_LOG_FUNCTION (this << packet << saddr << daddr << sport << dport << route);
  Ptr<Ipv6L3Protocol> ipv6 =  m_node->GetObject<Ipv6L3Protocol> ();
  
  UdpHeader udpHeader;
  if(Node::ChecksumEnabled ())
    {
      udpHeader.EnableChecksums ();
//      udpHeader.InitializeChecksum (saddr,
//                                    daddr,
//                                    PROT_NUMBER);
    }
  udpHeader.SetDestinationPort (dport);
  udpHeader.SetSourcePort (sport);

  packet->AddHeader (udpHeader);

  ipv6->Send (packet, saddr, daddr, PROT_NUMBER, route);
}

}; // namespace ns3

