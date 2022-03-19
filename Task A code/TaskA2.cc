#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include <ns3/lr-wpan-error-model.h>
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TaskA2");

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate, uint32_t app_id);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  uint32_t        m_app_id;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
    m_app_id (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate, uint32_t app_id)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_dataRate = dataRate;
  m_app_id = app_id;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind ();
    }
  else
    {
      m_socket->Bind6 ();
    }
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);
  m_packetsSent += 1;
  ScheduleTx();
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << oldCwnd << " " << newCwnd << std::endl;
}



int main(int argc, char** argv){
  uint32_t wirelessNodeCount = 100;
  std::string tcpVariant = "ns3::TcpNewReno";
  uint32_t speed = 5;  
  uint32_t nFlows = 20;
  uint32_t sinkPort = 1;
  uint32_t packetSize = 100;
  uint32_t packetPerSecond = 100;
  double startTime = 0.0;
  double stopTime = 100.0;
  uint32_t dataRate = packetSize*packetPerSecond*8;
  uint32_t variationType = 0;


  CommandLine cmd (__FILE__);
  cmd.AddValue ("nodes", "Number of nodes", wirelessNodeCount);
  cmd.AddValue ("nFlows", "Number of flows", nFlows);
  cmd.AddValue ("packetsPerSecond", "Packets sent per second for each sender ", packetPerSecond);
  cmd.AddValue("speed", "Speed of nodes", speed);
  cmd.AddValue("variationType", "variation type", variationType);

  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (tcpVariant));

  NodeContainer wirelessNodes;
  wirelessNodes.Create(wirelessNodeCount);

  MobilityHelper mobilityHelper;
  uint32_t gridwidth = static_cast<uint32_t> (std::sqrt(wirelessNodeCount));

  // ----------------------------------- 3D mobility model ---------------------------------------
  mobilityHelper.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (1.0),
                                 "DeltaY", DoubleValue (1.0),
                                 "GridWidth", UintegerValue (gridwidth),
                                 "LayoutType", StringValue ("RowFirst"));

  double x_max = gridwidth*1.0;
  double y_max = ((wirelessNodeCount*1.0)/gridwidth)*1.0;
  double z_max = 25.0;
  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << speed << "]";
  mobilityHelper.SetMobilityModel("ns3::GaussMarkovMobilityModel",
                                 "MeanVelocity", StringValue(ssSpeed.str()),
                                 "Bounds", BoxValue(Box(-10, x_max, -10, y_max, 0, z_max)),
                                 "TimeStep", TimeValue(Seconds(5)));
  mobilityHelper.Install (wirelessNodes);


  LrWpanHelper lrwPanHelper;
  NetDeviceContainer lrwPanDevices = lrwPanHelper.Install(wirelessNodes);
  lrwPanHelper.AssociateToPan(lrwPanDevices, 0);

  InternetStackHelper ipv6Helper;
  ipv6Helper.Install(wirelessNodes);

  SixLowPanHelper sixLowPanHelper;
  NetDeviceContainer sixLowPanDevices = sixLowPanHelper.Install(lrwPanDevices);


  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:f00d::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer wirelessDeviceInterfaces;
  wirelessDeviceInterfaces = ipv6.Assign (sixLowPanDevices);
  wirelessDeviceInterfaces.SetForwarding (0, true);
  wirelessDeviceInterfaces.SetDefaultRouteInAllNodes (0);


  for(uint32_t i=0; i<sixLowPanDevices.GetN();i++){
    Ptr<NetDevice> device = sixLowPanDevices.Get(i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (200));
  }

  int i = 1;
  int appNo = 0;

  for(uint32_t flow=1; flow<=nFlows; flow++){
    Address sinkAddress = Inet6SocketAddress (Inet6SocketAddress (wirelessDeviceInterfaces.GetAddress (i-1, 0), sinkPort) );
    PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",Inet6SocketAddress (Ipv6Address::GetAny (), sinkPort));
    sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    ApplicationContainer sinkApps = sinkApp.Install (wirelessNodes.Get(i-1));
    sinkApps.Start (Seconds (0.1));
    sinkApps.Stop (Seconds (stopTime));

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (wirelessNodes.Get (i), TcpSocketFactory::GetTypeId ());
    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3TcpSocket, sinkAddress, packetSize, DataRate (dataRate), appNo);
    wirelessNodes.Get (i)->AddApplication (app);
    app->SetStartTime (Seconds (startTime));
    app->SetStopTime (Seconds (stopTime));

    std::ostringstream oss;
    oss << "./output/flow" << flow <<  ".cwnd";
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss.str());
    ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    
    sinkPort++;
    i++;
    appNo++;
    if( i % wirelessNodeCount == 0){
      i = 1;
    }
  }


  FlowMonitorHelper flowMonitorHelper;
  Ptr<FlowMonitor> flowMonitor = flowMonitorHelper.InstallAll();


  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  Ptr<Ipv6FlowClassifier> ipv6Classifier = DynamicCast<Ipv6FlowClassifier> (flowMonitorHelper.GetClassifier6 ());
  FlowMonitor::FlowStatsContainer flowStats = flowMonitor->GetFlowStats ();


  double PacketsSent = 0;
  double PacketsReceived = 0;
  double PacketsLost = 0;
  double AvgThroughput = 0;
  double CurrThroughput = 0;
  double j = 1;
  double total = 0;
  double delay = 0;
  Time StartTime,FinishTime;

  for(auto itr=flowStats.begin(); itr != flowStats.end(); itr++){
      total++;
  }

  auto itr=flowStats.begin();
  StartTime = itr->second.timeFirstTxPacket;


  for(; itr != flowStats.end(); itr++){
      Ipv6FlowClassifier::FiveTuple t = ipv6Classifier->FindFlow(itr->first);
      if(itr->second.rxBytes != 0) CurrThroughput = (itr->second.rxBytes*8.0/(itr->second.timeLastRxPacket.GetSeconds()*1.0 - itr->second.timeFirstTxPacket.GetSeconds()*1.0)/(1024*1.0));


      NS_LOG_UNCOND("\nFlow ID: " << itr->first);
      NS_LOG_UNCOND("Src Addr: " << t.sourceAddress << "\t Destination Addr: " << t.destinationAddress);
      NS_LOG_UNCOND("Sent Packets: " << itr->second.txPackets );
      NS_LOG_UNCOND("Received Packets: " << itr->second.rxPackets ); 
      NS_LOG_UNCOND("Lost Packets: " << itr->second.lostPackets ); 
      NS_LOG_UNCOND("Packet Delivery Ratio: " << itr->second.rxPackets*100.0/itr->second.txPackets << "%");
      NS_LOG_UNCOND("Packet Drop Ratio: " << (itr->second.txPackets - itr->second.rxPackets)*100.0/itr->second.txPackets << "%");
      if(itr->second.rxPackets > 0) NS_LOG_UNCOND("Delay: " << itr->second.delaySum/itr->second.rxPackets);
      if(itr->second.rxBytes != 0) NS_LOG_UNCOND("Average Throughput: " << CurrThroughput << "Kbps");


      PacketsSent += itr->second.txPackets;
      PacketsReceived += itr->second.rxPackets ;
      PacketsLost +=  itr->second.txPackets - itr->second.rxPackets;
      if(itr->second.rxBytes != 0) AvgThroughput += CurrThroughput;
      FinishTime = itr->second.timeLastRxPacket;
      delay = delay + itr->second.delaySum.GetSeconds();
      j++;
    }

    // AvgThroughput/=j;
    // AvgThroughput = (double)PacketsReceived*8*1.0/((double)FinishTime.GetSeconds()- (double)StartTime.GetSeconds());
    NS_LOG_UNCOND("\n\nTOTAL SIMULATION: ");
    NS_LOG_UNCOND("Sent Packets: " << PacketsSent);
    NS_LOG_UNCOND("Received Packets: " << PacketsReceived);
    NS_LOG_UNCOND("Lost Packets: " << PacketsLost );
    NS_LOG_UNCOND("Packet Delivery Ratio: " << PacketsReceived*100.0/PacketsSent << "%");
    NS_LOG_UNCOND("Packet Drop Ratio: " << PacketsLost*100.0/PacketsSent << "%");
    NS_LOG_UNCOND("Average Throughput: " <<AvgThroughput<< "Kbps");
    NS_LOG_UNCOND("start time = " << StartTime.GetSeconds() << "Finish time = " << FinishTime.GetSeconds());
    

    if(variationType == 0){
      std::ofstream file1("./simulation_output/taska2/nodeVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska2/nodeVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska2/nodeVary/delivery.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska2/nodeVary/drop.dat", std::ios::out | std::ios::app);

      file1 << wirelessNodeCount << " " << AvgThroughput << std::endl;
      file2 << wirelessNodeCount << " " << delay << std::endl;
      file3 << wirelessNodeCount << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file4 << wirelessNodeCount << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }

    else if(variationType == 1){
      std::ofstream file1("./simulation_output/taska2/flowVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska2/flowVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska2/flowVary/delivery.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska2/flowVary/drop.dat", std::ios::out | std::ios::app);

      file1 << nFlows << " " << AvgThroughput << std::endl;
      file2 << nFlows << " " << delay << std::endl;
      file3 << nFlows << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file4 << nFlows << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }
    else if(variationType == 2){
      std::ofstream file1("./simulation_output/taska2/speedVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska2/speedVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska2/speedVary/delivery.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska2/speedVary/drop.dat", std::ios::out | std::ios::app);

      file1 << speed << " " << AvgThroughput << std::endl;
      file2 << speed << " " << delay << std::endl;
      file3 << speed << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file4 << speed << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }
    else if(variationType == 3){
      std::ofstream file1("./simulation_output/taska2/packetsPerSecondVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska2/packetsPerSecondVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska2/packetsPerSecondVary/delivery.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska2/packetsPerSecondVary/drop.dat", std::ios::out | std::ios::app);

      file1 << packetPerSecond << " " << AvgThroughput << std::endl;
      file2 << packetPerSecond << " " << delay << std::endl;
      file3 << packetPerSecond << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file4 << packetPerSecond << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }

    Simulator::Destroy ();

    return 0;

}