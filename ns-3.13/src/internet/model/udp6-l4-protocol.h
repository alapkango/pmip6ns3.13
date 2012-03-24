/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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

#ifndef UDP6_L4_PROTOCOL_H
#define UDP6_L4_PROTOCOL_H

#include <stdint.h>

#include "ns3/packet.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ipv6-l4-protocol.h"

namespace ns3 {

class Node;
class Socket;
class Ipv6EndPointDemux;
class Ipv6EndPoint;
class Udp6SocketImpl;
class Ipv6Route;

/**
 * \ingroup udp
 * \brief Implementation of the UDP protocol
 */
class Udp6L4Protocol : public Ipv6L4Protocol {
public:
  static TypeId GetTypeId (void);
  static const uint8_t PROT_NUMBER;

  Udp6L4Protocol ();
  virtual ~Udp6L4Protocol ();

  void SetNode (Ptr<Node> node);

  virtual int GetProtocolNumber (void) const;

  /**
   * \return A smart Socket pointer to a UdpSocket, allocated by this instance
   * of the UDP protocol
   */
  Ptr<Socket> CreateSocket (void);

  Ipv6EndPoint *Allocate (void);
  Ipv6EndPoint *Allocate (Ipv6Address address);
  Ipv6EndPoint *Allocate (uint16_t port);
  Ipv6EndPoint *Allocate (Ipv6Address address, uint16_t port);
  Ipv6EndPoint *Allocate (Ipv6Address localAddress, uint16_t localPort,
                          Ipv6Address peerAddress, uint16_t peerPort);

  void DeAllocate (Ipv6EndPoint *endPoint);

  // called by UdpSocket.
  /**
   * \brief Send a packet via UDP
   * \param packet The packet to send
   * \param saddr The source Ipv4Address
   * \param daddr The destination Ipv4Address
   * \param sport The source port number
   * \param dport The destination port number
   */
  void Send (Ptr<Packet> packet,
             Ipv6Address saddr, Ipv6Address daddr, 
             uint16_t sport, uint16_t dport);
  void Send (Ptr<Packet> packet,
             Ipv6Address saddr, Ipv6Address daddr, 
             uint16_t sport, uint16_t dport, Ptr<Ipv6Route> route);
  /**
   * \brief Receive a packet up the protocol stack
   * \param p The Packet to dump the contents into
   * \param header IPv4 Header information
   * \param interface the interface from which the packet is coming.
   */
  // inherited from Ipv4L4Protocol
  virtual enum Ipv6L4Protocol::RxStatus_e Receive (Ptr<Packet> p,
                                                   Ipv6Address const &src, 
                                                   Ipv6Address const &dst,
                                                   Ptr<Ipv6Interface> interface);

  /**
   * \brief Receive an ICMP packet
   * \param icmpSource The IP address of the source of the packet.
   * \param icmpTtl The time to live from the IP header
   * \param icmpType The type of the message from the ICMP header
   * \param icmpCode The message code from the ICMP header
   * \param icmpInfo 32-bit integer carrying informational value of varying semantics.
   * \param payloadSource The IP source address from the IP header of the packet
   * \param payloadDestination The IP destination address from the IP header of the packet
   * \param payload Payload of the ICMP packet
   */
  virtual void ReceiveIcmp (Ipv6Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode, uint32_t icmpInfo,
                            Ipv6Address payloadSource,Ipv6Address payloadDestination,
                            const uint8_t* payload);

protected:
  virtual void DoDispose (void);
  /*
   * This function will notify other components connected to the node that a new stack member is now connected
   * This will be used to notify Layer 3 protocol of layer 4 protocol stack to connect them together.
   */
  virtual void NotifyNewAggregate ();
private:
  Ptr<Node> m_node;
  Ipv6EndPointDemux *m_endPoints;
  Udp6L4Protocol (const Udp6L4Protocol &o);
  Udp6L4Protocol &operator = (const Udp6L4Protocol &o);
  std::vector<Ptr<Udp6SocketImpl> > m_sockets;
};

}; // namespace ns3

#endif /* UDP6_L4_PROTOCOL_H */
