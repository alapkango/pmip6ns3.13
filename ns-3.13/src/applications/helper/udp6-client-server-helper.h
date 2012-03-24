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

#ifndef UDP6_CLIENT_SERVER_HELPER_H
#define UDP6_CLIENT_SERVER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv6-address.h"
#include "ns3/udp6-server.h"
#include "ns3/udp6-client.h"

namespace ns3 {
  /**
   * \brief Create a server application which waits for input udp packets
   *        and uses the information carried into their payload to compute
   *        delay and to determine if some packets are lost.
   */
class Udp6ServerHelper
{
public:
   /**
    * Create Udp6ServerHelper which will make life easier for people trying
    * to set up simulations with udp6-client-server application.
    *
    */
  Udp6ServerHelper ();

   /**
    * Create Udp6ServerHelper which will make life easier for people trying
    * to set up simulations with udp-client-server application.
    *
    * \param port The port the server will wait on for incoming packets
    */
  Udp6ServerHelper (uint16_t port);

    /**
     * Record an attribute to be set in each Application after it is is created.
     *
     * \param name the name of the attribute to set
     * \param value the value of the attribute to set
     */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one udp server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);
  Ptr<Udp6Server> GetServer(void);
private:
  ObjectFactory m_factory;
  Ptr<Udp6Server> m_server;
};

/**
 * \brief Create a client application which sends udp packets carrying
 *  a 32bit sequence number and a 64 bit time stamp.
 *
 */
class Udp6ClientHelper
{

public:
  /**
   * Create Udp6ClientHelper which will make life easier for people trying
   * to set up simulations with udp6-client-server.
   *
   */
  Udp6ClientHelper ();

   /**
    *  Create Udp6ClientHelper which will make life easier for people trying
    * to set up simulations with udp-client-server.
    *
    * \param ip The IPv6 address of the remote udp server
    * \param port The port number of the remote udp server
    */

  Udp6ClientHelper (Ipv6Address ip, uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
     * \param c the nodes
     *
     * Create one udp client application on each of the input nodes
     *
     * \returns the applications created, one application per input node.
     */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory;
};

} // namespace ns3

#endif /* UDP6_CLIENT_SERVER_H */
