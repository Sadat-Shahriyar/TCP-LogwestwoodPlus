
#include <stdio.h>
#include <stdlib.h>
#include<time.h>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/stats-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-module.h"
#include "ns3/callback.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TaskA1");

int recieveCount = 0;



//    Default Network Topology
//
//      Wifi 10.1.2.0
//                        AP
//      *    *    *    *   *
//      |    |    |    |   |     10.1.1.0
//      n6   n7   n8   n9   n0 -------------- n1   n2   n3   n4   n5 
//                            point-to-point  |    |    |    |    |
//                                            *    *    *    *    * 
//                                            AP
//     

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
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
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
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
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

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
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



// std::ofstream throughputFile;
// std::ofstream endToEndDelayFile;
// std::ofstream packetDeliveryRatioFile;
// std::ofstream packetDropRationFile;


int main(int argc, char** argv){
    uint32_t payloadSize = 1024;
    std::string tcpVariant = "ns3::TcpNewReno";
    int nWifi = 10;
    int nFlows = 5;
    int nPackets = 1000;
    std::string senderDataRate = "2Mbps";
    std::string bottleNeckDataRate = "10Mbps";
    std::string bottleNeckDelay = "2ms";
    double Tx_Range = 10.0;
    // double simulationTimeInSeconds = nPackets*nFlows*payloadSize*8.0/(1e6*5.0);
    double simulationTimeInSeconds = 5.0;
    // NS_LOG_UNCOND("simulation time " << simulationTimeInSeconds);
    uint32_t packetsPerSecond = 500;
    int nodes = 20;

    uint32_t variationType = 0;

    // int createFile = 0;
    // int fileCount = 0;
    // std::string variationType = "";


    CommandLine cmd (__FILE__);
    cmd.AddValue ("nodes", "Number of nodes", nodes);
    cmd.AddValue ("nFlows", "Number of flows", nFlows);
    cmd.AddValue ("packetsPerSecond", "Packets sent per second for each sender ", packetsPerSecond);
    cmd.AddValue("Tx_Range", "Coverage area", Tx_Range);
    cmd.AddValue("variationType", "variation type", variationType);
    // cmd.AddValue("createFile", "append or new data", createFile);
    // cmd.AddValue("fileCount", "file no", fileCount);
    // cmd.AddValue("variation", "variation type", variationType);
    
    cmd.Parse (argc, argv);

    nWifi = (nodes-2)/2;

    std::stringstream ss;
    ss << packetsPerSecond*payloadSize*8/1024 << "kbps";

    ss >> senderDataRate;
    NS_LOG_UNCOND(nWifi << " " << nodes << " " << senderDataRate << " " << nFlows);


    srand(time(0));

    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (tcpVariant));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

    // setup routers
    NodeContainer p2pNode;
    p2pNode.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bottleNeckDataRate));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (bottleNeckDelay));

    NetDeviceContainer pointToPointDevies;
    pointToPointDevies = pointToPoint.Install(p2pNode);



    Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (Tx_Range));
    // setup sender devices

    NodeContainer senderWifiStaNodes;
    senderWifiStaNodes.Create(nWifi);
    NodeContainer senderWifiApNode = p2pNode.Get(0);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    channel.AddPropagationLoss ("ns3::RangePropagationLossModel");
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());
    phy.SetErrorRateModel ("ns3::YansErrorRateModel");

    WifiHelper senderWifi;
    senderWifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));

    NetDeviceContainer senderStaDevices;
    senderStaDevices = senderWifi.Install (phy, mac, senderWifiStaNodes);

    mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

    NetDeviceContainer senderApDevices;
    senderApDevices = senderWifi.Install (phy, mac, senderWifiApNode);

    
    
    //setup reciever devices

    NodeContainer recieverWifiStaNodes;
    recieverWifiStaNodes.Create(nWifi);
    NodeContainer recieverWifiApNode = p2pNode.Get(1);

    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
    channel2.AddPropagationLoss ("ns3::RangePropagationLossModel");
    channel2.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    YansWifiPhyHelper phy2;
    phy2.SetChannel (channel2.Create ());
    phy2.SetErrorRateModel ("ns3::YansErrorRateModel");

    WifiHelper recieverWifi;
    recieverWifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac2;
    Ssid ssid2 = Ssid ("ns-3-ssid");
    mac2.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid2),
                "ActiveProbing", BooleanValue (false));
    
    NetDeviceContainer recieverStaDevices;
    recieverStaDevices = recieverWifi.Install (phy2, mac2, recieverWifiStaNodes);

    mac2.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));

    NetDeviceContainer recieverApDevices;
    recieverApDevices = recieverWifi.Install (phy2, mac2, recieverWifiApNode);


    //setup mobility model
    
    MobilityHelper mobility;


    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (senderWifiStaNodes);
    mobility.Install(recieverWifiStaNodes);
    mobility.Install(senderWifiApNode);
    mobility.Install(recieverWifiApNode);


    // setup internet stack
    InternetStackHelper stack;
    stack.InstallAll();

    // setup address
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer p2pInterfaces, recieverWifiInterfaces, senderWifiInterfaces;

    address.SetBase ("10.1.1.0", "255.255.255.0");
    p2pInterfaces = address.Assign (pointToPointDevies);


    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer senderStaInterfaces = address.Assign (senderStaDevices);
    // senderWifiInterfaces.Add(sifc1);
    Ipv4InterfaceContainer senderApInterface=address.Assign (senderApDevices);
    // senderWifiInterfaces.Add(sifc2);


    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer recieverStaInterfaces = address.Assign (recieverStaDevices);
    // recieverWifiInterfaces.Add(rifc1);
    Ipv4InterfaceContainer recieverApInterface = address.Assign (recieverApDevices);
    // recieverWifiInterfaces.Add(rifc2);

    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    for(NodeContainer::Iterator itr = senderWifiStaNodes.Begin(); itr != senderWifiStaNodes.End(); itr++){
      Ptr<Ipv4StaticRouting> singleWifiStaNode = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting> ((*itr)->GetObject<Ipv4>()->GetRoutingProtocol());
      singleWifiStaNode->SetDefaultRoute(senderApInterface.GetAddress(0), 1);
    }

    for(NodeContainer::Iterator itr = recieverWifiStaNodes.Begin(); itr != recieverWifiStaNodes.End(); itr++){
      Ptr<Ipv4StaticRouting> singleWifiStaNode = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting> ((*itr)->GetObject<Ipv4>()->GetRoutingProtocol());
      singleWifiStaNode->SetDefaultRoute(recieverApInterface.GetAddress(0), 1);
    }

    uint32_t i = 0;
    uint16_t sp = 8080;
    if(nFlows > nWifi) nFlows = nWifi;
    for(int flow=0;flow<nFlows; flow++){
      Address sinkAddress (InetSocketAddress (recieverStaInterfaces.GetAddress(nWifi-1-i), sp));
      PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(), sp));
      ApplicationContainer sinkApps = packetSinkHelper.Install (recieverWifiStaNodes.Get (nWifi-1-i));
      sinkApps.Start (Seconds (0));
      sinkApps.Stop (Seconds (simulationTimeInSeconds));

      Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (senderWifiStaNodes.Get (nWifi-1-i), TcpSocketFactory::GetTypeId ());
      Ptr<MyApp> app = CreateObject<MyApp> ();
      app->Setup (ns3TcpSocket, sinkAddress, payloadSize, nPackets, DataRate (senderDataRate));
      senderWifiStaNodes.Get (nWifi-1-i)->AddApplication (app);
      app->SetStartTime (Seconds (1));
      app->SetStopTime (Seconds (simulationTimeInSeconds));

      std::ostringstream oss;
      oss << "./output/flow" << flow+1 <<  ".cwnd";
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss.str());
      ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));


      i++;
      if(i%nWifi == 0){
        i = 0;
      }
    }
    
    

    FlowMonitorHelper flowMonitorHelper;
    Ptr<FlowMonitor> flowMonitor = flowMonitorHelper.InstallAll();



    Simulator::Stop (Seconds (simulationTimeInSeconds));
    Simulator::Run ();

    Ptr<Ipv4FlowClassifier> ipv4Callisfier = DynamicCast <Ipv4FlowClassifier> (flowMonitorHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer flowStats = flowMonitor->GetFlowStats();


    double PacketsSent = 0.0;
    double PacketsReceived = 0.0;
    double PacketsLost = 0.0;
    double AvgThroughput = 0.0;
    double CurrThroughput = 0.0;
    double j = 1;
    double total = 0.0;
    double delay = 0.0;
    Time StartTime,FinishTime;


    for(auto itr=flowStats.begin(); itr != flowStats.end(); itr++){
      total++;
    }

    auto itr=flowStats.begin();
    StartTime = itr->second.timeFirstTxPacket;


    for(; itr != flowStats.end(); itr++){
      // Ipv4FlowClassifier::FiveTuple t = ipv4Callisfier->FindFlow(itr->first);
      if(itr->second.rxBytes != 0) CurrThroughput = (itr->second.rxBytes*8.0/(itr->second.timeLastRxPacket.GetSeconds() - itr->second.timeFirstTxPacket.GetSeconds())/1024);

      // NS_LOG_UNCOND("\nFlow ID: " << itr->first);
      // NS_LOG_UNCOND("Src Addr: " << t.sourceAddress << "\t Destination Addr: " << t.destinationAddress);
      // NS_LOG_UNCOND("Sent Packets: " << itr->second.txPackets );
      // NS_LOG_UNCOND("Received Packets: " << itr->second.rxPackets ); 
      // NS_LOG_UNCOND("Lost Packets: " << itr->second.lostPackets ); 
      // NS_LOG_UNCOND("Packet Delivery Ratio: " << itr->second.rxPackets*100.0/itr->second.txPackets << "%");
      // NS_LOG_UNCOND("Packet Drop Ratio: " << (itr->second.txPackets - itr->second.rxPackets)*100.0/itr->second.txPackets << "%");
      // if(itr->second.rxPackets > 0) NS_LOG_UNCOND("Delay: " << itr->second.delaySum/itr->second.rxPackets);
      // if(itr->second.rxBytes != 0) NS_LOG_UNCOND("Average Throughput: " << CurrThroughput << "Kbps");


      PacketsSent += itr->second.txPackets;
      PacketsReceived += itr->second.rxPackets ;
      PacketsLost +=  itr->second.txPackets - itr->second.rxPackets;
      if(itr->second.rxBytes != 0) AvgThroughput += CurrThroughput;
      FinishTime = itr->second.timeLastRxPacket;
      delay = delay + itr->second.delaySum.GetSeconds();
      j++;
    }

    // AvgThroughput/=j;
    // AvgThroughput = PacketsReceived*8/(FinishTime.GetSeconds()-StartTime.GetSeconds());

    // NS_LOG_UNCOND("\n\nTOTAL SIMULATION: ");
    // NS_LOG_UNCOND("Sent Packets: " << PacketsSent);
    // NS_LOG_UNCOND("Received Packets: " << PacketsReceived);
    // NS_LOG_UNCOND("Lost Packets: " << PacketsLost );
    // NS_LOG_UNCOND("Packet Delivery Ratio: " << PacketsReceived*100.0/PacketsSent << "%");
    // NS_LOG_UNCOND("Packet Drop Ratio: " << PacketsLost*100.0/PacketsSent << "%");
    // NS_LOG_UNCOND("Total delay " << delay);
    // NS_LOG_UNCOND("Average Throughput: " <<AvgThroughput<< "Kbps nodes = " << nodes);
    
    if(variationType == 0){
      std::ofstream file1("./simulation_output/taska1/nodeVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska1/nodeVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska1/nodeVary/drop.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska1/nodeVary/delivery.dat", std::ios::out | std::ios::app);
      
      file1 << nodes << " " << AvgThroughput << std::endl;
      file2 << nodes << " " << delay << std::endl;
      file4 << nodes << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file3 << nodes << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }
    else if(variationType == 1){
      std::ofstream file1("./simulation_output/taska1/flowVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska1/flowVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska1/flowVary/drop.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska1/flowVary/delivery.dat", std::ios::out | std::ios::app);
      
      file1 << nFlows << " " << AvgThroughput << std::endl;
      file2 << nFlows << " " << delay << std::endl;
      file4 << nFlows << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file3 << nFlows << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }
    else if(variationType == 2){
      std::ofstream file1("./simulation_output/taska1/coverageVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska1/coverageVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska1/coverageVary/drop.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska1/coverageVary/delivery.dat", std::ios::out | std::ios::app);
      
      file1 << Tx_Range << " " << AvgThroughput << std::endl;
      file2 << Tx_Range << " " << delay << std::endl;
      file4 << Tx_Range << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file3 << Tx_Range << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }
    else if(variationType == 3){
      std::ofstream file1("./simulation_output/taska1/packetsPerSecondVary/throughtput.dat", std::ios::out | std::ios::app);
      std::ofstream file2("./simulation_output/taska1/packetsPerSecondVary/delay.dat", std::ios::out | std::ios::app);
      std::ofstream file3("./simulation_output/taska1/packetsPerSecondVary/drop.dat", std::ios::out | std::ios::app);
      std::ofstream file4("./simulation_output/taska1/packetsPerSecondVary/delivery.dat", std::ios::out | std::ios::app);
      
      file1 << packetsPerSecond << " " << AvgThroughput << std::endl;
      file2 << packetsPerSecond << " " << delay << std::endl;
      file4 << packetsPerSecond << " " << PacketsReceived*100.0/PacketsSent << std::endl;
      file3 << packetsPerSecond << " " << PacketsLost*100.0/PacketsSent << std::endl;
    }
    // if(createFile == 1){
    //   throughputFile.open("./simula")
    // }

    Simulator::Destroy ();

    return 0;
}