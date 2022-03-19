/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas 
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
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 * and Greeshma Umapathi
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "tcp-congestion-ops.h"
#include "tcp-westwood.h"

#include "ns3/tcp-recovery-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"
#include "ns3/event-id.h"

namespace ns3{

class TcpLogWestwoodPlus : public TcpWestwood
{
public: 
    static TypeId GetTypeId (void);
    TcpLogWestwoodPlus(void);
    TcpLogWestwoodPlus(const TcpLogWestwoodPlus& sock);
    virtual ~TcpLogWestwoodPlus (void);
    virtual std::string GetName () const;
    virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
    virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);
    virtual void CwndEvent (Ptr<TcpSocketState> tcb,
                          const TcpSocketState::TcpCAEvent_t event);
    virtual Ptr<TcpCongestionOps> Fork ();

protected:
      virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
      virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

      uint32_t m_Wmax;
      double m_Alpha;
    //   bool m_recovery;
    //   bool m_foundDupacks;
    //   bool m_SlowStartPhase;
    //   bool m_congAvoidancePhase;
      bool m_AlphaIncrease;
};

}