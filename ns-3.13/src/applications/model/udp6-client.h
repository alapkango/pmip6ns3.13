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
 
#ifndef UDP6_CLIENT_H
#define UDP6_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup udp6clientserver
 * \class Udp6Client
 * \brief A Udp6 client. Sends UDPv6 packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class Udp6Client : public Application
{
public:
  static TypeId
  GetTypeId (void);

  Udp6Client ();

  virtual ~Udp6Client ();

  /**
   * \brief set the remote address and port
   * \param ip remote IPv6 address
   * \param port remote port
   */
  void SetRemote (Ipv6Address ip, uint16_t port);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTransmit (Time dt);
  void Send (void);

  uint32_t m_count;
  Time m_interval;
  uint32_t m_size;
  uint32_t m_incrementalSize;
  
  uint32_t m_realSize;

  uint32_t m_sent;
  Ptr<Socket> m_socket;
  Ipv6Address m_peerAddress;
  uint16_t m_peerPort;
  EventId m_sendEvent;

};

} // namespace ns3

#endif /* UDP6_CLIENT_H */
