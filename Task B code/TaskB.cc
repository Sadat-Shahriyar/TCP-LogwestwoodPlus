
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



int main(){
    uint32_t payloadSize = 1040;
    std::string tcpVariant = "ns3::TcpLogWestoodPlus";
    // std::string tcpVariant = "ns3::TcpNewReno";
    int nWifi = 20;
    int nFlows = 18;
    int nPackets = 1000;
    // uint32_t packetsPerSecond = 500;
    std::string senderDataRate = "1000Mbps";
    std::string bottleNeckDataRate = "50Mbps";
    std::string bottleNeckDelay = "2ms";
    double packetErrorRate = 1e-2;
    // int simulationTimeInSeconds = nPackets*nFlows*payloadSize*8/(1e6*5);
    int simulationTimeInSeconds = 5;
    // NS_LOG_UNCOND("simulation time " << simulationTimeInSeconds);

    // std::stringstream ss;
    // ss << packetsPerSecond*payloadSize*8/1024 << "kbps";

    // ss >> senderDataRate;


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

    // setup sender devices

    NodeContainer senderWifiStaNodes;
    senderWifiStaNodes.Create(nWifi);
    NodeContainer senderWifiApNode = p2pNode.Get(0);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());
    // phy.SetErrorRateModel ("ns3::YansErrorRateModel");

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
    YansWifiPhyHelper phy2;
    phy2.SetChannel (channel2.Create ());
    // phy2.SetErrorRateModel ("ns3::YansErrorRateModel");
    // phy2.SetErrorRateModel()

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

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (packetErrorRate));

    for(int i=nWifi+2; i<=2*nWifi+1; i++){
      // recieverStaDevices.Get(i)->SetAttribute("ns3::YansWifiPhy/PostReceptionErrorModel",  PointerValue (em));
      Config::Set("/NodeList/" + std::to_string(i) + "DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/PostReceptionErrorModel", PointerValue(em));
    }

    //setup mobility model
    
    MobilityHelper mobility;

    // mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    //                                 "MinX", DoubleValue (0.0),
    //                                 "MinY", DoubleValue (0.0),
    //                                 "DeltaX", DoubleValue (5.0),
    //                                 "DeltaY", DoubleValue (10.0),
    //                                 "GridWidth", UintegerValue (3),
    //                                 "LayoutType", StringValue ("RowFirst"));

    // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    //                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
    // mobility.Install (senderWifiStaNodes);
    // mobility.Install(recieverWifiStaNodes);

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


    uint16_t sp = 8080;
    for(int i=0;i<nFlows; i++){
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
      oss << "./output/flow" << i+1 <<  ".cwnd";
      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss.str());
      ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
    }
    
    

    FlowMonitorHelper flowMonitorHelper;
    Ptr<FlowMonitor> flowMonitor = flowMonitorHelper.InstallAll();



    Simulator::Stop (Seconds (simulationTimeInSeconds));
    Simulator::Run ();

    Ptr<Ipv4FlowClassifier> ipv4Callisfier = DynamicCast <Ipv4FlowClassifier> (flowMonitorHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer flowStats = flowMonitor->GetFlowStats();


    uint32_t packetsSent = 0;
    uint32_t packetsReceived = 0;
    uint32_t packetsLost = 0;
    uint32_t avgThroughput = 0;
    uint32_t currThroughput = 0;
    uint32_t j = 1;
    uint32_t total = 0;
    double jainsFairnessIndexThroughputSum = 0.0;
    double jainsFairnessIndexThroughputSquareSum = 0.0;
    uint32_t avgGoodPut = 0;
    uint32_t packetsRecievedExludingAckPackets = 0;
    Time startTime,finishTime;


    Header* tempHeader = new Ipv4Header ();
    uint32_t ipHeaderSize = tempHeader->GetSerializedSize ();
    delete tempHeader;
    tempHeader = new TcpHeader ();
    uint32_t tcpHeaderSize = tempHeader->GetSerializedSize ();
    delete tempHeader;


    for(auto itr=flowStats.begin(); itr != flowStats.end(); itr++){
      total++;
    }

    auto itr=flowStats.begin();
    startTime = itr->second.timeFirstTxPacket;

    for(; itr != flowStats.end(); itr++){
      Ipv4FlowClassifier::FiveTuple t = ipv4Callisfier->FindFlow(itr->first);
      if(itr->second.rxBytes != 0) currThroughput = (itr->second.rxBytes*8.0/(itr->second.timeLastRxPacket.GetSeconds() - itr->second.timeFirstTxPacket.GetSeconds())/1024);

      NS_LOG_UNCOND("\nFlow ID: " << itr->first);
      NS_LOG_UNCOND("Src Addr: " << t.sourceAddress << "\t Destination Addr: " << t.destinationAddress);
      NS_LOG_UNCOND("Sent Packets: " << itr->second.txPackets );
      NS_LOG_UNCOND("Received Packets: " << itr->second.rxPackets ); 
      NS_LOG_UNCOND("Lost Packets: " << itr->second.lostPackets ); 
      NS_LOG_UNCOND("Packet Delivery Ratio: " << itr->second.rxPackets*100.0/itr->second.txPackets << "%");
      NS_LOG_UNCOND("Packet Drop Ratio: " << (itr->second.txPackets - itr->second.rxPackets)*100.0/itr->second.txPackets << "%");
      if(itr->second.rxBytes != 0) NS_LOG_UNCOND("Delay: " << itr->second.delaySum/itr->second.rxPackets);
      if(itr->second.rxBytes != 0) NS_LOG_UNCOND("Average Throughput: " << currThroughput << "Kbps");


      packetsSent += itr->second.txPackets;
      packetsReceived += itr->second.rxPackets ;
      packetsLost +=  itr->second.txPackets - itr->second.rxPackets;
      finishTime = itr->second.timeLastRxPacket;


      if(itr->second.rxBytes != 0) avgThroughput += currThroughput;

      if(itr->second.rxBytes != 0) {
        jainsFairnessIndexThroughputSum += currThroughput*1.0;
        jainsFairnessIndexThroughputSquareSum += currThroughput*currThroughput*1.0;
      }

      if(j <= total/2 && itr->second.rxBytes != 0){
        avgGoodPut += currThroughput;
        packetsRecievedExludingAckPackets += itr->second.rxPackets;
      }

      j++;
    }

    double jainsFairnessIndex = (jainsFairnessIndexThroughputSum*jainsFairnessIndexThroughputSum*1.0)/((2*nWifi+2) * jainsFairnessIndexThroughputSquareSum*1.0);
    avgGoodPut = avgGoodPut - packetsRecievedExludingAckPackets*(tcpHeaderSize + ipHeaderSize);

    // avgThroughput = packetsReceived*8/(finishTime.GetSeconds()-startTime.GetSeconds());
    NS_LOG_UNCOND("\n\nTOTAL SIMULATION: ");
    NS_LOG_UNCOND("Sent Packets: " << packetsSent);
    NS_LOG_UNCOND("Received Packets: " << packetsReceived);
    NS_LOG_UNCOND("Lost Packets: " << packetsLost );
    NS_LOG_UNCOND("Packet Delivery Ratio: " << packetsReceived*100.0/packetsSent << "%");
    NS_LOG_UNCOND("Packet Drop Ratio: " << packetsLost*100.0/packetsSent << "%");
    NS_LOG_UNCOND("Average Throughput: " <<avgThroughput<< "Kbps");
    NS_LOG_UNCOND("Jains faireness index = " << jainsFairnessIndex << " for bottleneck bandwidth " << bottleNeckDataRate);
    NS_LOG_UNCOND("Average goodput = " << avgGoodPut << " packetErrorRate = " << packetErrorRate << " number of flows = " << nFlows*2 << " bottleneck bandwidth = "
     << bottleNeckDataRate);

    
    Simulator::Destroy ();

    return 0;
}