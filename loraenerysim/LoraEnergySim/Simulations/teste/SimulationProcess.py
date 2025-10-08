import simpy
from tqdm import tqdm
from Framework.PropagationModel import *
from Framework.AirInterface import AirInterface
from Framework.EnergyProfile import EnergyProfile
from Framework.Gateway import Gateway
from Framework.LoRaParameters import LoRaParameters
from Framework.Node import Node
from Framework.SNRModel import SNRModel
from Simulations.GlobalConfig import *
from Framework.Location import *


class Result:
    def __init__(self, mean_energy, energy_tx, n_packets, payload, sf, energy_tracking):
        self.mean_energy = mean_energy
        self.energy_tx = energy_tx
        self.n_packets = n_packets
        self.payload = payload
        self.sf = sf
        self.energy_tracking = energy_tracking



# EnergyProfile do LES
tx_power_mW = {2: 91.8, 5: 95.9, 8: 101.6, 11: 120.8, 14: 146.5}

rx_measurements = {'pre_mW': 8.2, 'pre_ms': 3.4, 'rx_lna_on_mW': 39,
                   'rx_lna_off_mW': 34,
                   'post_mW': 8.3, 'post_ms': 10.7}

# EnergyProfile do NS
# tx_power_mW = {2: 91.8, 5: 95.9, 8: 101.6, 11: 120.8, 14: 92.44}

# rx_measurements = {'pre_mW': 34.67, 'pre_ms': 3.4, 'rx_lna_on_mW': 36.34,
#                    'rx_lna_off_mW': 36.34,
#                    'post_mW': 34.67, 'post_ms': 10.7}

# Mapa do chatgpt
# pre_mW -> standby
# rx_lna_on_mW -> rx_power
# rx_lna_of_mW -> rx_power
# post_mW -> standby
# Valor antigo de proc_mW = 23.1e-3



def progress(env, sim_time, pbar):
    while env.now < sim_time:
        yield env.timeout(1)
        pbar.update(1)

def run_helper(args):
    return run(*args)


def SetSpreadingFactorUp(air_interface, loc_node):
    dist = Location.distance(air_interface.gateway.location, loc_node)
    sensitivity = air_interface.prop_model.calcRxPower(False, 14, dist)
    if sensitivity > Gateway.SENSITIVITY[7]: return 7
    elif sensitivity > Gateway.SENSITIVITY[8]: return 8
    elif sensitivity > Gateway.SENSITIVITY[9]: return 9
    elif sensitivity > Gateway.SENSITIVITY[10]: return 10
    elif sensitivity > Gateway.SENSITIVITY[11]: return 11
    else: return 12


def run(locs, p_size, sigma, sim_time, gateway_location, num_nodes, transmission_rate, confirmed_messages, adr):
    sim_env = simpy.Environment()
    gateway = Gateway(sim_env, gateway_location, max_snr_adr=True, avg_snr_adr=False)
    nodes = []
    
    air_interface = AirInterface(gateway, LogShadow(std=sigma), SNRModel(), sim_env)

    # Todo: confirm if sum of percents is 1
    if sum(percent_payload_size) != 1:
        ValueError('Payload sizes not englobing the total network: {}/1'.format(sum(percent_payload_size)))

    #Split the nodes per payload
    nodes_per_payload = [int(p * num_nodes) for p in percent_payload_size]
    
    diff_nodes = num_nodes - sum(nodes_per_payload) 
    if  diff_nodes > 0: nodes_per_payload[-1] += diff_nodes #in case of odd number of devices, put the remaining devices into the last group
    
    transmission_per_node = [transmission_rate[i] for i in range(len(nodes_per_payload)) for _ in range(nodes_per_payload[i])]
    for i in range(len(nodes_per_payload)):
        
        init = len(nodes)
        end = len(nodes) + nodes_per_payload[i]
        # print(init, end, nodes_per_payload)
        for node_id in range(init, end):
            # energy_profile = EnergyProfile(18.48e-3, 49.5, tx_power_mW,
            #                            rx_power=rx_measurements)
            energy_profile = EnergyProfile(5.7e-3 , 15, tx_power_mW,
                                       rx_power=rx_measurements)
            
            _sf = SetSpreadingFactorUp(air_interface, locs[node_id])
            # _sf = np.random.choice(LoRaParameters.SPREADING_FACTORS)
            if start_with_fixed_sf:
                _sf = start_sf
            lora_param = LoRaParameters(freq=np.random.choice(LoRaParameters.DEFAULT_CHANNELS),
                                        sf=_sf,
                                        bw=125, cr=5, crc_enabled=1, de_enabled=0, header_implicit_mode=0, tp=14)
            # sleep_time=(8 * p_size / transmission_rate)
            
            node = Node(node_id, energy_profile, lora_param, sleep_time=(8 * p_size[i] / transmission_per_node[node_id]),
                        process_time=5,
                        adr=adr,
                        location=locs[node_id],
                        base_station=gateway, env=sim_env, payload_size=p_size[i], air_interface=air_interface,
                        confirmed_messages=confirmed_messages)
            # print("Nó ", node.id," criado com payload de: ", node.payload_size)    
            nodes.append(node)
            sim_env.process(node.run())
    
    with tqdm(total=sim_time, desc="Simulation Time", unit="t") as pbar:
        sim_env.process(progress(sim_env, sim_time, pbar))
        sim_env.run(until=sim_time)

    # Simulation is done.
    # process data

    results = list()
    for n in nodes:
        results.append(Result(n.energy_per_bit(), n.total_energy_consumed(), n.packets_sent, n.payload_size, n.lora_param.sf, n.energy_tracking))
        

    data_mean_nodes = Node.get_mean_simulation_data_frame(nodes, name=sigma) / (
        num_nodes)

    # data_gateway = gateway.get_simulation_data(name=sigma) / num_nodes
    data_gateway = gateway.get_simulation_data(name=sigma)

    data_gateway_per_node = gateway.get_node_data()
    data_air_interface = air_interface.get_simulation_data(name=sigma)

    # eff_en = data_mean_nodes['TotalEnergy'][sigma] / (p_size*data_mean_nodes['UniquePackets'][sigma])
    # print('Eb {} for Size:{} and Sigma:{}'.format(eff_en, p_size, sigma))

    return {
        'results': results,
        'mean_nodes': data_mean_nodes,
        'gateway': data_gateway,
        'gw_per_node': data_gateway_per_node,
        'air_interface': data_air_interface,
        'path_loss_std': sigma,
        'payload_size': payload_sizes,
        
    }
