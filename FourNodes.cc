#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TrafficControlExample");

int main (int argc, char *argv[])
{
  double simulationTime = 10; //seconds
  std::string transportProt = "Udp";
  std::string socketType;

  if (transportProt.compare ("Tcp") == 0)
    {
      socketType = "ns3::TcpSocketFactory";
    }
  else
    {
      socketType = "ns3::UdpSocketFactory";
    }

  NodeContainer nodes;
  nodes.Create (4);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  NetDeviceContainer devices02;
  devices02 = pointToPoint.Install (nodes.Get(0),nodes.Get(2));
NetDeviceContainer devices12;
  devices12 = pointToPoint.Install (nodes.Get(1),nodes.Get(2));
NetDeviceContainer devices23;
  devices23 = pointToPoint.Install (nodes.Get(2),nodes.Get(3));

  InternetStackHelper stack;
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces02 = address.Assign (devices02);
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces12 = address.Assign (devices12);
 address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces23 = address.Assign (devices23);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Udp Flow
  uint16_t port = 7;
  Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper (socketType, localAddress);
  ApplicationContainer sinkApp = packetSinkHelper.Install (nodes.Get (3));

  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (simulationTime + 0.1));

  uint32_t payloadSize = 1448;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  OnOffHelper onoff (socketType, Ipv4Address::GetAny ());
  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  onoff.SetAttribute ("DataRate", StringValue ("50Mbps")); //bit/s
  ApplicationContainer apps;

  InetSocketAddress rmt (interfaces23.GetAddress (1), port);
  rmt.SetTos (0xb8);
  AddressValue remoteAddress (rmt);
  onoff.SetAttribute ("Remote", remoteAddress);
  apps.Add (onoff.Install (nodes.Get (1)));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (simulationTime + 0.1));

 //TCP Flow
  uint16_t porttcp = 9;
  Address localAddresstcp (InetSocketAddress (Ipv4Address::GetAny (), porttcp));
  PacketSinkHelper packetSinkHelpertcp ("ns3::TcpSocketFactory", localAddresstcp);
  ApplicationContainer sinkApptcp = packetSinkHelpertcp.Install (nodes.Get (3));

  sinkApptcp.Start (Seconds (0.2));
  sinkApptcp.Stop (Seconds (simulationTime + 0.1));

  OnOffHelper onofftcp ("ns3::TcpSocketFactory", Ipv4Address::GetAny ());
  onofftcp.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onofftcp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onofftcp.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  onofftcp.SetAttribute ("DataRate", StringValue ("50Mbps")); //bit/s
  ApplicationContainer appstcp;

  InetSocketAddress rmttcp (interfaces23.GetAddress (1), porttcp);
  rmttcp.SetTos (0xb8);
  AddressValue remoteAddresstcp (rmttcp);
  onofftcp.SetAttribute ("Remote", remoteAddresstcp);
  appstcp.Add (onofftcp.Install (nodes.Get (0)));
  appstcp.Start (Seconds (1.0));
  appstcp.Stop (Seconds (simulationTime + 0.1));

 
 
FlowMonitorHelper flowmon;
Ptr<FlowMonitor> monitor = flowmon.InstallAll();
Simulator::Stop (Seconds (simulationTime + 5));
Simulator::Run ();

Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
// std::cout << " Dropped Packets: " << stats[1].lostPackets << std::endl;
for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end(); ++iter)
{
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
    NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " <<
    t.destinationAddress);
    NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
}

  Simulator::Destroy ();

   return 0;
}
