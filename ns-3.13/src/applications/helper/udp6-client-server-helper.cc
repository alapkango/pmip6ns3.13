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
 
#include "udp6-client-server-helper.h"
#include "ns3/udp6-server.h"
#include "ns3/udp6-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

Udp6ServerHelper::Udp6ServerHelper ()
{}

Udp6ServerHelper::Udp6ServerHelper (uint16_t port)
{
  m_factory.SetTypeId (Udp6Server::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
Udp6ServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
Udp6ServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<Udp6Server> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<Udp6Server>
Udp6ServerHelper::GetServer (void)
{
  return m_server;
}

Udp6ClientHelper::Udp6ClientHelper ()
{}

Udp6ClientHelper::Udp6ClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (Udp6Client::GetTypeId ());
  SetAttribute ("RemoteAddress", Ipv6AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
Udp6ClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
Udp6ClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<Udp6Client> client = m_factory.Create<Udp6Client> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3
