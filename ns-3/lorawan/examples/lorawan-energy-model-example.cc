/*
 * Copyright (c) 2017 University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

/*
 * This script simulates a simple network to explain how the Lora energy model
 * works.
 */

#include "ns3/basic-energy-source-helper.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/file-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/log.h"
#include "ns3/lora-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/names.h"
#include "ns3/node-container.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/simulator.h"
#include "ns3/config.h"
#include <algorithm>
#include <ctime>
#include "ns3/propagation-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/lorawan-module.h"

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("LoraEnergyModelExample");

std::map<int, std::map <int, int>> sent_packets;

struct PacketInfo {
    int id;        // id node
    bool rcv;       // if message was received or not
};

std::map<int, PacketInfo> rcv_packets;
std::map<int, int> id_packets; //real packets received per device id

std::map<int, int> delivered_packets;
std::map<int, int> delivered_packets_nodes;


std::map<int, double> energy_node; //energy per node

std::map<int, std::map<std::string, double>> energy_per_state; //energy per node and state



std::string filename = "tempos.txt";
std::string devices_locations = "/home/marianna/locations.txt";



//Count Sent Packet per device and payload
void PacketTraceDevice(Ptr<Packet const> pacote){
    std::ofstream outfile(filename, std::ios::app); // modo append
    Time sendTime = Simulator::Now ();


    uint32_t id =  Simulator::GetContext (); //device id
    uint32_t size = pacote->GetSize(); //packet size
    outfile << "Hora de envio, nó " <<  id << " e payload " << size << " : " << sendTime.GetMilliSeconds() << "\n"; 
    sent_packets[id][size]++; 

    uint64_t packet_id = pacote->GetUid(); //packet id
    rcv_packets[packet_id].rcv =false; //set the message was sent but not received (yet)
    rcv_packets[packet_id].id = id; 
}

//Count Received Packet per device, gateway and payload
void PacketTraceGW(Ptr<Packet const> pacote){
    rcv_packets[pacote->GetUid()].rcv = true; //set the message was received  
    id_packets[rcv_packets[pacote->GetUid()].id] ++;

    uint32_t size = pacote->GetSize(); //packet size
    delivered_packets[size]++;  
}

//Read locations from a txt file
std::vector<std::pair<int, int>> reading_locations(){
    std::ifstream file(devices_locations);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo!\n";
        return {};
    }

    std::string line, content;
    while (std::getline(file, line)) {
        content += line;
    }
    file.close();

    std::vector<std::pair<int, int>> positions;
    int x, y;
    bool lendoX = true;
    std::stringstream num;

    for (char ch : content) {
        if (std::isdigit(ch) || ch == '-') {
            num << ch;
        } else if (ch == ',' || ch == ']') {
            if (!num.str().empty()) {
                int valor = std::stoi(num.str());
                num.str("");
                num.clear();

                if (lendoX) {
                    x = valor;
                    lendoX = false;
                } else {
                    y = valor;
                    lendoX = true;
                    positions.emplace_back(x, y);
                }
            }
        }
    }
    return positions;    
}

int
main(int argc, char* argv[])
{
    // Set up logging 
    LogComponentEnable("LoraEnergyModelExample", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_DEBUG);
    // LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
    // LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable ("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("GatewayLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LorawanMacHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("PropagationLossModel", LOG_LEVEL_ALL);

    // LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
    // LogComponentEnable ("LorawanMacHeader", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);

    LogComponentEnableAll(LOG_PREFIX_FUNC);
    LogComponentEnableAll(LOG_PREFIX_NODE);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    /************************
     *  Create the channel  *
     ************************/

    NS_LOG_INFO("Creating the channel...");

    // Create the lora channel object
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);
    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    /************************
     *  Create the helpers  *
     ************************/

    NS_LOG_INFO("Setting up helpers...");

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    std::vector<std::pair<int, int>> nodes_positions = reading_locations();
    for ( size_t i = 0; i < nodes_positions.size(); ++i){
        allocator->Add (Vector (nodes_positions[i].first, nodes_positions[i].second, 0));
    }
    int n_devices = allocator->GetSize();
    std::cout << "Number of Devices Allocated: " << n_devices << std::endl;
    // allocator->Add(Vector(100, 0, 0));
    
    mobility.SetPositionAllocator(allocator);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Create the LoraPhyHelper
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    // Create the LorawanMacHelper
    LorawanMacHelper macHelper = LorawanMacHelper();

    // Create the LoraHelper
    LoraHelper helper = LoraHelper();
    helper.EnablePacketTracking();

    /************************
     *  Create End Devices  *
     ************************/

    NS_LOG_INFO("Creating the end device...");

    // Create a set of nodes
    NodeContainer endDevices;
    endDevices.Create(n_devices);

    // Assign a mobility model to the node
    mobility.Install(endDevices);

    // Create the LoraNetDevices of the end devices
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    NetDeviceContainer endDevicesNetDevices = helper.Install(phyHelper, macHelper, endDevices);
    
    // Create trace for each time a packet is sent by the devices
    for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
        Ptr<Node> node = *j;
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
        Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> (); 
        mac->TraceConnectWithoutContext("SentNewPacket", MakeCallback(&PacketTraceDevice));
        mac->SetMType(LorawanMacHeader::CONFIRMED_DATA_UP);
        mac->SetMaxNumberOfTransmissions(1);
    }
     

    /*********************
     *  Create Gateways  *
     *********************/

    NS_LOG_INFO("Creating the gateway...");
    NodeContainer gateways;
    gateways.Create(1);

    Ptr<ListPositionAllocator> allocatorGw = CreateObject<ListPositionAllocator>();
    allocatorGw->Add(Vector(0, 0, 0));
    mobility.SetPositionAllocator(allocatorGw);
    mobility.Install(gateways);

    // Create a netdevice for each gateway
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);


    // Create trace for each time a packet is received by the gateway
    for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
    {
        Ptr<Node> node = *j;
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
        Ptr<LorawanMac> mac = loraNetDevice->GetMac()->GetObject<LorawanMac>();
        mac->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&PacketTraceGW));
    }  

    ////////////
    // Create network serverNS
    ////////////
    // Mudou o código de criação e setagem do Network Server
    Ptr<Node> networkServer = CreateObject<Node>();

    // PointToPoint links between gateways and server
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    // Store network server app registration details for later
    P2PGwRegistration_t gwRegistration;
    for (auto gw = gateways.Begin(); gw != gateways.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    // Install the NetworkServer application on the network server
    NetworkServerHelper networkServerHelper;
    networkServerHelper.SetGatewaysP2P(gwRegistration);
    networkServerHelper.SetEndDevices(endDevices);
    networkServerHelper.Install(networkServer);

    // Install the Forwarder application on the gateways
    ForwarderHelper forwarderHelper;
    forwarderHelper.Install(gateways);

    LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateways, channel);

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    // OneShotSenderHelper oneShotSenderHelper;
    // oneShotSenderHelper.SetSendTime (Seconds (10));

    // oneShotSenderHelper.Install (endDevices);

    PeriodicSenderHelper periodicSenderHelper_group1;
    periodicSenderHelper_group1.SetPeriod(Seconds(5));
    periodicSenderHelper_group1.SetPacketSize(1);

    PeriodicSenderHelper periodicSenderHelper_group2;
    periodicSenderHelper_group2.SetPeriod(Minutes(10));
    periodicSenderHelper_group2.SetPacketSize(11);

    NodeContainer group1;
    NodeContainer group2;

    if(n_devices == 1){
        group1.Add(endDevices.Get(0));
        ApplicationContainer appContainer_group1 = periodicSenderHelper_group1.Install(group1);
        ApplicationContainer appContainer;
        appContainer.Add(appContainer_group1);
    }else{
        for(int i = 0; i < n_devices/2; i++){
            group1.Add(endDevices.Get(i));
        }
        for(int i = n_devices/2; i < n_devices; i++){
            group2.Add(endDevices.Get(i));
        }
        
        ApplicationContainer appContainer_group1 = periodicSenderHelper_group1.Install(group1);
        ApplicationContainer appContainer_group2 = periodicSenderHelper_group2.Install(group2);

        ApplicationContainer appContainer;
        appContainer.Add(appContainer_group1);
        appContainer.Add(appContainer_group2);
    }    
    /************************
     * Install Energy Model *
     ************************/

    BasicEnergySourceHelper basicSourceHelper;
    LoraRadioEnergyModelHelper radioEnergyHelper;

    // configure energy source
    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(10000)); // Energy in J
    basicSourceHelper.Set("BasicEnergySupplyVoltageV", DoubleValue(3.3));
    // Energy Profile do LES
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.0444));
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.01182));
    radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.00000173));
    radioEnergyHelper.Set("StandbyCurrentA", DoubleValue(0.00455));
    // radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (1.73e-6));

    // Energy Profile do artigo
    // radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.028011)); 
    // radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.011011));
    // radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.0000056));
    // // radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.000007)); //idle do artigo 
    // radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.0105055));
    
    

    // radioEnergyHelper.SetTxCurrentModel("ns3::ConstantLoraTxCurrentModel",
    //                                     "TxCurrent",
                                        // DoubleValue(0.0444));

    // install source on end devices' nodes
    EnergySourceContainer sources = basicSourceHelper.Install(endDevices);
    Names::Add("/Names/EnergySource", sources.Get(0));

    // install device model
    DeviceEnergyModelContainer deviceModels =
        radioEnergyHelper.Install(endDevicesNetDevices, sources);
    Names::Add("/Names/DeviceEnergyModel", deviceModels.Get(0));
    
    /**************
     * Get output *
     **************/
    FileHelper fileHelper;
    fileHelper.ConfigureFile("battery-level", FileAggregator::SPACE_SEPARATED);
    fileHelper.WriteProbe("ns3::DoubleProbe", "/Names/EnergySource/RemainingEnergy", "Output");

    fileHelper.ConfigureFile("consuming-battery", FileAggregator::SPACE_SEPARATED);
    fileHelper.WriteProbe("ns3::DoubleProbe", "/Names/DeviceEnergyModel/TotalEnergyConsumption", "Output");
    /****************
     *  Simulation  *
     ****************/
    helper.DoPrintDeviceStatus(endDevices, gateways, "output-1.txt");
    std::ofstream output(filename, std::ios::trunc);
   
    Simulator::Stop(Hours(24));

    Simulator::Run();

    
    // Calculate energy efficiency for all devices in the network

    double totalEnergyConsumed = 0.0;
    int count_node = 0;
    for (EnergySourceContainer::Iterator iter = sources.Begin (); iter != sources.End (); ++iter)

    {

        // Get total energy per node
        Ptr<EnergySource> source = *iter;
        double initialEnergy = source->GetInitialEnergy ();
        double remainingEnergy = source->GetRemainingEnergy ();
        energy_node[count_node] = (initialEnergy - remainingEnergy);

        // Get total energy per node and state
        // DeviceEnergyModelContainer dev = source->FindDeviceEnergyModels(0);
        Ptr<LoraRadioEnergyModel> lora = deviceModels.Get(count_node)->GetObject<LoraRadioEnergyModel>();
        energy_per_state[count_node] = lora->GetEnergyPerState();

        count_node += 1;
        totalEnergyConsumed += (initialEnergy - remainingEnergy);
        
    
    }
    NS_LOG_INFO ("\nTotal energy consumed by the network");
    std :: cout << totalEnergyConsumed << " J\n";


    helper.DoPrintDeviceStatus(endDevices, gateways, "output-1.txt");



    Simulator::Destroy();
    output.close();

    LoraPacketTracker& tracker = helper.GetPacketTracker();

    int iterator = n_devices;
    std::ofstream file("devices_packets.txt", std::ios::trunc);

    for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j){
        std::vector <int> output = tracker.CountPhyPacketsPerGw(Seconds(0), Hours(24), iterator);
        file << "GwID " << n_devices << "\nReceived: " << output.at(1) << "\nInterfered: " << output.at(2)
        << "\nNoMoreReceivers: " << output.at(3) << "\nUnderSensitivity: " << output.at(4) << "\nLost: " << output.at(5)
        << "\n" << "\n";
        iterator += 1;
    }

    // Count number of packets sent and received
    std::string s =  tracker.CountMacPacketsGlobally (Seconds (0), Hours(24));
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (std::getline(ss, item, ' '))
    {
        splittedStrings.push_back(item);
    }

    double sent = std::stod(splittedStrings[0]);
    double receiv = std::stod(splittedStrings[1]);
    NS_LOG_INFO ("\nNumber of Packets Sent And Received");
    std :: cout << sent << ' ' << receiv << "\n";
    
    // Get amount of packets and calculate energy used per bit on each device
    for(auto i = sent_packets.begin(); i != sent_packets.end(); i++){
        for (auto ptr = i->second.begin(); ptr != i->second.end(); ptr++){
            std :: cout << "Device " << i->first << ": " << ptr->first << " bytes of payload - Well received " << id_packets[i->first] << "/" << ptr->second  << " packets\n";
            
            double energy_per_bit = energy_node[i->first] / (ptr->second * ptr->first * 8);
            double energy_sleep = energy_per_state[i->first]["sleep"];
            double energy_standby = energy_per_state[i->first]["standby"];
            double energy_tx = energy_per_state[i->first]["tx"];
            double energy_rx = energy_per_state[i->first]["rx"];

            NS_LOG_INFO ("\nEnergy per bit and per states");
            std :: cout << "Energy per bit: " << energy_per_bit << " J\n";
            std :: cout << "Energy to sleep: " << energy_sleep << " J\n";
            std :: cout << "Energy to standby: " << energy_standby << " J\n";
            std :: cout << "Energy to tx: " << energy_tx << " J\n";
            std :: cout << "Energy to rx: " << energy_rx << " J\n";



            Vector position = allocator->GetNext();
            file << i->first << " " << position.x << " " << position.y  << " " << ptr->first << " " << ptr->second << " " << id_packets[i->first] << " " << energy_per_bit <<std::endl;
        }
    }
    file.close();

    for(auto i = delivered_packets.begin(); i != delivered_packets.end(); i++){
        std :: cout << "Gateway, " << i->first << " bytes of Payload - " << i->second  << " number of packets\n";
    }

    

    return 0;
}
