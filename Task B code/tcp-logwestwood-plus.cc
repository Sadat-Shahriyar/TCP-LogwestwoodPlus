#include "tcp-logwestwood-plus.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
#include "tcp-socket-base.h"


NS_LOG_COMPONENT_DEFINE ("TcpWestwoodPlus");

namespace ns3{
NS_OBJECT_ENSURE_REGISTERED (TcpLogWestwoodPlus);

TypeId
TcpLogWestwoodPlus::GetTypeId (void)
{
static TypeId tid = TypeId("ns3::TcpLogWestoodPlus")
    .SetParent<TcpWestwood>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLogWestwoodPlus>()
;
return tid;
}


TcpLogWestwoodPlus::TcpLogWestwoodPlus (void) :
    TcpWestwood (),
    m_Wmax(0),
    m_Alpha (2.0),
    // m_recovery (false),
    // m_foundDupacks (false),
    // m_SlowStartPhase(false),
    // m_congAvoidancePhase(false),
    m_AlphaIncrease (false)
    
    {
        NS_LOG_FUNCTION (this);
    }

TcpLogWestwoodPlus::TcpLogWestwoodPlus (const TcpLogWestwoodPlus& sock) :
    TcpWestwood (sock),
    m_Wmax(sock.m_Wmax),
    m_Alpha(sock.m_Alpha),
    // m_recovery (sock.m_recovery),
    // m_foundDupacks( sock.m_foundDupacks),
    // m_SlowStartPhase(sock.m_SlowStartPhase),
    // m_congAvoidancePhase (sock.m_congAvoidancePhase)
    m_AlphaIncrease (sock.m_AlphaIncrease)
    {
        NS_LOG_FUNCTION (this);
    }

TcpLogWestwoodPlus::~TcpLogWestwoodPlus (void)
{
}

double max(double a, double b){
    if(a < b) return b;
    else return a;
}

void
TcpLogWestwoodPlus::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  // NS_LOG_UNCOND("Congestion avoidnace, cwnd = " << tcb->m_cWnd);

  if (segmentsAcked > 0)
    {
      if(tcb->m_cWnd > m_Wmax){
          double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
          adder = std::max (1.0, adder);
          tcb->m_cWnd += static_cast<uint32_t> (adder);
          NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                       " ssthresh " << tcb->m_ssThresh);
      }
      else{

          double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
          adder = std::max (1.0, adder);


          // NS_LOG_UNCOND(m_Alpha << " " << m_Wmax);
          double val = ((double)m_Wmax - (double)tcb->m_cWnd)/(((double)tcb->m_cWnd) * m_Alpha);
          // NS_LOG_UNCOND("-----------------------test:" << val << "------------------------------");
          tcb->m_cWnd += (uint32_t) max(val, adder);
          NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                       " ssthresh " << tcb->m_ssThresh);
          
          if(m_AlphaIncrease) m_Alpha = m_Alpha * 2;
          else{
            //   if(m_Alpha/2 >= 2) m_Alpha = m_Alpha/2;
            m_Alpha = m_Alpha/2;
            if(m_Alpha < 0.01) m_Alpha = 0.01;
          }
      }
    }
}



uint32_t
TcpLogWestwoodPlus::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  // NS_LOG_UNCOND("Slow start");


  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}


void
TcpLogWestwoodPlus::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      CongestionAvoidance (tcb, segmentsAcked);
    }
}


Ptr<TcpCongestionOps>
TcpLogWestwoodPlus::Fork ()
{
  return CreateObject<TcpLogWestwoodPlus> (*this);
}

void
TcpLogWestwoodPlus::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
      NS_LOG_FUNCTION (this << tcb << newState);
      if(newState == TcpSocketState::CA_OPEN){
          m_Alpha = 2;
          m_AlphaIncrease = false;
      }
      if(newState == TcpSocketState::CA_DISORDER){
          m_AlphaIncrease = true;
      }
      if(newState == TcpSocketState::CA_RECOVERY){
          m_Wmax = tcb->m_cWnd;
      }
    // if(newState == TcpSocketState::CA_DISORDER){
    //     m_foundDupacks = true;
    //     m_Alpha = m_Alpha/2;
    // }
    // else if(newState == TcpSocketState::CA_OPEN){
        
    //     // m_Alpha=2;
    //     m_foundDupacks = false;
    //     m_recovery = false;

    // }
    // else if(newState == TcpSocketState::CA_RECOVERY){
    //     m_Wmax = tcb->m_cWnd;
    //     m_recovery = true;
    // }
}

void 
TcpLogWestwoodPlus::CwndEvent(Ptr<TcpSocketState> tcb,
                          const TcpSocketState::TcpCAEvent_t event)
{
    NS_LOG_FUNCTION (this << tcb << event);

    
    if(event == TcpSocketState::CA_EVENT_NON_DELAYED_ACK){
        if(m_AlphaIncrease) {
            double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
            adder = std::max (1.0, adder);


            std::cout << m_Alpha << " inside cwndevent" << std::endl;
            m_Alpha = m_Alpha * 2;
            double val = ((double)m_Wmax - (double)tcb->m_cWnd)/(((double)tcb->m_cWnd) * m_Alpha);
            tcb->m_cWnd += (uint32_t) max(val, adder);
        }
    }
    // else if(event == TcpSocketState::CA_EVENT_COMPLETE_CWR){
    //     m_recovery = false;
    //     m_Alpha = 2;

    // }
    // else if(event == TcpSocketState::CA_EVENT_DELAYED_ACK){
    //     if(!m_recovery){
    //         if(m_foundDupacks){
    //             if(tcb->m_cWnd < m_Wmax){
    //                 m_Alpha = m_Alpha*2.0;
    //             }

    //         }
    //         else{
    //             if(m_Alpha/2.0 >= 0.0) m_Alpha = m_Alpha/2.0;
    //         }
    //     }
    //     else{
    //         m_Alpha = 2;
    //     }

    // }
    // else if(event == TcpSocketState::CA_EVENT_NON_DELAYED_ACK){
    //     if(!m_recovery){
    //         if(m_foundDupacks){
    //             if(tcb->m_cWnd < m_Wmax){
    //                 m_Alpha = m_Alpha*2.0;
    //                 NS_LOG_UNCOND(m_Alpha);
    //             }

    //         }
    //         else{
    //             if(m_Alpha/2.0 >= 0.0) m_Alpha = m_Alpha/2.0;
    //             NS_LOG_UNCOND(m_Alpha);
    //         }
    //     }
    //     else{
    //         m_Alpha = 2;
    //     }
    // }


}

std::string
TcpLogWestwoodPlus::GetName () const
{
  return "TcpLogWestwoodPlus";
}


}
