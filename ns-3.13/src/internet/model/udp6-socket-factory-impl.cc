/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
#include "udp6-socket-factory-impl.h"
#include "udp6-l4-protocol.h"
#include "ipv6-route.h"
#include "ns3/socket.h"
#include "ns3/assert.h"

namespace ns3 {

Udp6SocketFactoryImpl::Udp6SocketFactoryImpl ()
  : m_udp (0)
{
}
Udp6SocketFactoryImpl::~Udp6SocketFactoryImpl ()
{
  NS_ASSERT (m_udp == 0);
}

void
Udp6SocketFactoryImpl::SetUdp (Ptr<Udp6L4Protocol> udp)
{
  m_udp = udp;
}

Ptr<Socket>
Udp6SocketFactoryImpl::CreateSocket (void)
{
  return m_udp->CreateSocket ();
}

void 
Udp6SocketFactoryImpl::DoDispose (void)
{
  m_udp = 0;
  Udp6SocketFactory::DoDispose ();
}

} // namespace ns3
