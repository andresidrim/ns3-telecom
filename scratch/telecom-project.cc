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

    NodeContainer nodes; // Dispositivos IoT (Default = 30)
    nodes.Create(nNodes);

    NodeContainer sinkNode; // Coletor dos dados
    sinkNode.Create(1);


    // Posiciona os nós aleatoriamente dentro de uma área de 200m por 200m
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=200]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=200]")
    );
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // Nós estáticos
    mobility.Install(nodes);

    MobilityHelper sinkMob;
    sinkMob.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // Coletor estático
    sinkMob.Install(sinkNode);

    // Configuração do canal de Wi-Fi
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel"); // Atraso constante
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel"); // Adequado para ambientes urbanos

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.Set("RxGain", DoubleValue(0)); // Ganho fixo 0
    phy.Set("TxPowerStart", DoubleValue(txPower));
    phy.Set("TxPowerEnd", DoubleValue(txPower));

    // Configuração do Wi-Fi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n); // Escolha do padrão físico 802.11n
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue(dataMode),
        "ControlMode", StringValue("HtMcs0")
    );

    // Configuração da camada MAC
    WifiMacHelper mac;
    Ssid ssid = Ssid("rede-iot");

    // Configura os nós como estação Wi-Fi
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false)); // Associa os nós a "rede-iot"
    NetDeviceContainer staDevices = wifi.Install(phy, mac, nodes);

    // Configura o coletor como ponto de acesso (AP)
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid)); // Associa o coletor a "rede-iot"
    NetDeviceContainer apDevice = wifi.Install(phy, mac, sinkNode);

    // Instalcação da pilha de internet em todos os nós
    InternetStackHelper stack;
    stack.Install(nodes);
    stack.Install(sinkNode);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0"); // Configuracao do ipv4 da rede

    // Todos os nós e coletor ficam na mesma sub rede
    Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface = address.Assign(apDevice);

    uint16_t port = 5000;

    UdpServerHelper server(port); // Criacao de servidor UDP escutando a porta 5000
    ApplicationContainer serverApp = server.Install(sinkNode.Get(0));
    // Começa a rodar com 1 segundo de simulação e vai até 30 segundos (simTime)
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(simTime));

    // Cliente UDP envia pacotes para o endereço do AP na porta 5000
    UdpClientHelper client(apInterface.GetAddress(0), port);
    client.SetAttribute("MaxPackets", UintegerValue(1000000000)); // Número máximo de pacotes
    client.SetAttribute("Interval", TimeValue(MilliSeconds(packetIntervalMs))); // Intervalo entre envios de pacote
    client.SetAttribute("PacketSize", UintegerValue(100)); // Pacotes de 100 bytes

    ApplicationContainer clientApps;
    // Cada nó recebe uma instancia do cliente UDP
    for (uint32_t i = 0; i < nodes.GetN(); i++) {
        clientApps.Add(client.Install(nodes.Get(i)));
    }

    // Começam a transmitir com 2 segundos de simulação
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime));

    // Instalação de Flow Monitor em todos os nós. Contabiliza as estatisticas de cada fluxo (pacotes transmitidos, recebidos, atrasos, etc)
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime)); // Roda até o fim da simulação
    Simulator::Run();

    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    double sumDelaySeconds = 0.0;
    uint64_t totalRxBytes = 0;

    // Soma do total de pacotes enviados, recebidos e atraso
    for (auto &kv : stats) {
        const FlowMonitor::FlowStats &flow = kv.second;
        totalTxPackets += flow.txPackets;
        totalRxPackets += flow.rxPackets;
        totalRxBytes   += flow.rxBytes;
        if (flow.rxPackets > 0) sumDelaySeconds += flow.delaySum.GetSeconds();
    }

    // Calculo das metricas solicitadas
    double pdr = totalTxPackets > 0 ? double(totalRxPackets) / totalTxPackets : 0.0;
    double avgDelay = totalRxPackets > 0 ? sumDelaySeconds / totalRxPackets : 0.0;
    double throughput = simTime > 0 ? (double(totalRxBytes) * 8.0) / simTime : 0.0;

    // Escrita no CSV
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
