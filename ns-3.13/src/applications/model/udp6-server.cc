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
#include "packet-loss-counter.h"

#include "seq-ts-header.h"
#include "udp6-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Udp6Server");
NS_OBJECT_ENSURE_REGISTERED (Udp6Server);


TypeId
Udp6Server::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Udp6Server")
    .SetParent<Application> ()
    .AddConstructor<Udp6Server> ()
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Udp6Server::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&Udp6Server::GetPacketWindowSize,
                                         &Udp6Server::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
  ;
  return tid;
}

Udp6Server::Udp6Server ()
  : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received=0;
}

Udp6Server::~Udp6Server ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
Udp6Server::GetPacketWindowSize () const
{
  return m_lossCounter.GetBitMapSize ();
}

void
Udp6Server::SetPacketWindowSize (uint16_t size)
{
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
Udp6Server::GetLost (void) const
{
  return m_lossCounter.GetLost ();
}

uint32_t
Udp6Server::GetReceived (void) const
{

  return m_received;

}

void
Udp6Server::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
Udp6Server::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::Udp6SocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),
                                                   m_port);
      m_socket->Bind (local);
    }

  m_socket->SetRecvCallback (MakeCallback (&Udp6Server::HandleRead, this));

}

void
Udp6Server::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
Udp6Server::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from))
    {
      if (packet->GetSize () > 0)
        {
          SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t currentSequenceNumber = seqTs.GetSeq ();
          NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                       " bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
                       " Sequence Number: " << currentSequenceNumber <<
                       " Uid: " << packet->GetUid () <<
                       " TXtime: " << seqTs.GetTs () <<
                       " RXtime: " << Simulator::Now () <<
                       " Delay: " << Simulator::Now () - seqTs.GetTs ());

          m_lossCounter.NotifyReceived (currentSequenceNumber);
          m_received++;
        }
    }
}

} // Namespace ns3
