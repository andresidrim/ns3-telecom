#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include <fstream>
#include <iomanip>

using namespace ns3;

void EnsureCsvHeader(const std::string &filename) {
    std::ifstream in(filename.c_str());
    if (in.good()) return;
    in.close();
    std::ofstream out(filename.c_str(), std::ios::out);
    out << "txPower_dBm,dataMode,packetInterval_ms,simTime_s,"
           "sentPkts,recvPkts,pdr,avgDelay_s,throughput_bps"
        << std::endl;
    out.close();
}

int main(int argc, char *argv[]) {
    uint32_t nNodes = 30;
    double simTime = 30.0;
    double txPower = 16.0;
    std::string dataMode = "HtMcs7";
    double packetIntervalMs = 500.0;
    std::string csvFile = "results.csv";

    CommandLine cmd;
    cmd.AddValue("nNodes", "nNodes", nNodes);
    cmd.AddValue("simTime", "simTime", simTime);
    cmd.AddValue("txPower", "txPower", txPower);
    cmd.AddValue("dataMode", "dataMode", dataMode);
    cmd.AddValue("packetIntervalMs", "packetIntervalMs", packetIntervalMs);
    cmd.AddValue("csvFile", "csvFile", csvFile);
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(nNodes);

    NodeContainer sinkNode;
    sinkNode.Create(1);

    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=200]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=200]")
    );
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    MobilityHelper sinkMob;
    sinkMob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    sinkMob.Install(sinkNode);

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.Set("RxGain", DoubleValue(0));
    phy.Set("TxPowerStart", DoubleValue(txPower));
    phy.Set("TxPowerEnd", DoubleValue(txPower));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue(dataMode),
        "ControlMode", StringValue("HtMcs0")
    );

    WifiMacHelper mac;
    Ssid ssid = Ssid("rede-iot");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, nodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, sinkNode);

    InternetStackHelper stack;
    stack.Install(nodes);
    stack.Install(sinkNode);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface = address.Assign(apDevice);

    uint16_t port = 5000;

    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(sinkNode.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(simTime));

    UdpClientHelper client(apInterface.GetAddress(0), port);
    client.SetAttribute("MaxPackets", UintegerValue(1000000000));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(packetIntervalMs)));
    client.SetAttribute("PacketSize", UintegerValue(100));

    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < nodes.GetN(); i++) {
        clientApps.Add(client.Install(nodes.Get(i)));
    }

    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    double sumDelaySeconds = 0.0;
    uint64_t totalRxBytes = 0;

    for (auto &kv : stats) {
        const FlowMonitor::FlowStats &flow = kv.second;
        totalTxPackets += flow.txPackets;
        totalRxPackets += flow.rxPackets;
        totalRxBytes   += flow.rxBytes;
        if (flow.rxPackets > 0) sumDelaySeconds += flow.delaySum.GetSeconds();
    }

    double pdr = totalTxPackets > 0 ? double(totalRxPackets) / totalTxPackets : 0.0;
    double avgDelay = totalRxPackets > 0 ? sumDelaySeconds / totalRxPackets : 0.0;
    double throughput = simTime > 0 ? (double(totalRxBytes) * 8.0) / simTime : 0.0;

    EnsureCsvHeader(csvFile);
    std::ofstream out(csvFile.c_str(), std::ios::app);
    out << std::fixed << std::setprecision(6)
        << txPower << ","
        << dataMode << ","
        << packetIntervalMs << ","
        << simTime << ","
        << totalTxPackets << ","
        << totalRxPackets << ","
        << pdr << ","
        << avgDelay << ","
        << throughput
        << std::endl;
    out.close();

    std::cout << "TX=" << totalTxPackets
              << " RX=" << totalRxPackets
              << " PDR=" << pdr * 100
              << "% Delay=" << avgDelay
              << " Throughput=" << throughput << "bps\n";

    Simulator::Destroy();
    return 0;
}
