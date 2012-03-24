/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * UDP6 Implementation
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
 
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "udp6-client.h"
#include "seq-ts-header.h"
#include <stdlib.h>
#include <stdio.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Udp6Client");
NS_OBJECT_ENSURE_REGISTERED (Udp6Client);

TypeId
Udp6Client::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Udp6Client")
    .SetParent<Application> ()
    .AddConstructor<Udp6Client> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Udp6Client::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&Udp6Client::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Ipv6Address of the outbound packets",
                   Ipv6AddressValue (),
                   MakeIpv6AddressAccessor (&Udp6Client::m_peerAddress),
                   MakeIpv6AddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Udp6Client::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&Udp6Client::m_size),
                   MakeUintegerChecker<uint32_t> (12,1500))
    .AddAttribute ("IncrementalSize",
                   "Size of packets incremented. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Udp6Client::m_incrementalSize),
                   MakeUintegerChecker<uint32_t> (0,1500))
  ;
  return tid;
}

Udp6Client::Udp6Client ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

Udp6Client::~Udp6Client ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Udp6Client::SetRemote (Ipv6Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
Udp6Client::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
Udp6Client::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::Udp6SocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Bind ();
      m_socket->Connect (Inet6SocketAddress (m_peerAddress, m_peerPort));
	  
	  m_realSize = m_size;
    }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &Udp6Client::Send, this);
}

void
Udp6Client::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
}

void
Udp6Client::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (m_realSize-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);

  if ((m_socket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_realSize << " bytes to "
                                    << m_peerAddress << " Uid: " << p->GetUid ()
                                    << " Time: " << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_realSize << " bytes to "
                                          << m_peerAddress);
    }

  m_realSize += m_incrementalSize;

  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &Udp6Client::Send, this);
    }
}

} // Namespace ns3
