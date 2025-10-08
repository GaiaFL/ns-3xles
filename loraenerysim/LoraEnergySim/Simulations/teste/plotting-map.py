import pickle

import matplotlib.pyplot as plt
import matplotlib.cm as cm
import seaborn as sns
import numpy as np

import mplcursors

def plot_packet_energy(node_data):
    TE_J = node_data['energy_consumed']/ 1000
    ep = TE_J / (node_data['packets_sent'])
    print("Energy per packet: ", ep, "(J)")
    num_pacotes = np.arange(0, node_data['packets_sent'])
    energia_total = num_pacotes * ep
    label_legenda = f'Accumulated energy\n({ep:.10f} J/packet)'
    plt.plot(num_pacotes, energia_total, color='blue', label=label_legenda)
    plt.xlabel('Number of packets')
    plt.ylabel('Expended energy (J)')
    plt.title('Total energy spent vs. Number of packets transmitted')
    plt.grid(True)
    plt.legend()
    plt.show()


def plot_bit_energy(node_data, if_return=False):
    total_bytes = node_data['packets_sent'] * node_data['payload_size']
    eb = (node_data['energy_consumed']/ (total_bytes * 8))/1000
    if if_return: return eb
    
    print("Energy Consumed to Send One Bit of Data: ", eb, " J") #mj to J
    # print(node_data)
    
    num_bit = np.arange(0, total_bytes*8) #Transform bytes to bits
    energia_total = num_bit * eb
    label_legenda = f'Accumulated energy\n({eb:.10f} J/bit)'
    plt.plot(num_bit, energia_total, color='blue', label=label_legenda)
    plt.xlabel('Number of bits')
    plt.ylabel('Expended energy (J)')
    plt.title('Total energy spent vs. Number of bits transmitted')
    plt.grid(True)
    plt.legend()
    plt.show()

    return eb



file = "/home/marianna/LoraEnergySim/results/1_False_1_cnst_num_bytes.p"

results = pickle.load(open(file, "rb")) #Gateway info
path_loss_variances = results['path_loss_variances']
sigmas = results['path_loss_variances']
payload_sizes = results['payload_sizes']


# print("teste: ", results['TxRxEnergy'])

# load locations:
locations_file = "/home/marianna/LoraEnergySim/locations/1_locations_1_sim.pkl"
with open(locations_file, 'rb') as file_handler:
    locations_per_simulation = pickle.load(file_handler)
    num_of_simulations = len(locations_per_simulation)
    num_nodes = len(locations_per_simulation[0])

mean_val = dict()
std_val = dict()

print(results)
EC = 0
dispositivos = []
for id in range(num_nodes):
        id_str = str(id)
        # print(results['nodes'])
        node_data = results['nodes'][id_str]
        # print(node_data)
        print("Node ID: ", id_str, " Payload size: ", node_data['payload_size'])
        print("Total Packets Transmitted: ", node_data['packets_sent'])
        EC += node_data['energy_consumed']
        print("Total Energy Consumed Per Device:" , node_data['energy_consumed'])
        
        print()
        energia_bits = plot_bit_energy(node_data)
        # plot_packet_energy(node_data)
        dispositivos.append({
            'id': id,
            'x': float(locations_per_simulation[0][id].x),
            'y': float(locations_per_simulation[0][id].y),
            'grupo': int(node_data['payload_size']),
            'enviadas': int(node_data['packets_sent']),
            'energia_bits': float(energia_bits)
        })
        
EC = EC/1000
print(f"Total Energy Consumed on the Network: {EC} J")

# Obter grupos únicos e mapear cores
grupos = sorted(set(d['grupo'] for d in dispositivos))
cores = cm.rainbow(np.linspace(0, 1, len(grupos)))
mapa_cores = {grupo: cor for grupo, cor in zip(grupos, cores)}

# Plotagem
plt.figure(figsize=(10, 8))
xs = [d['x'] for d in dispositivos]
ys = [d['y'] for d in dispositivos]
cs = [mapa_cores[d['grupo']] for d in dispositivos]

sc = plt.scatter(xs, ys, c=cs)

gw = plt.scatter(0, 0, color='black', marker='x', s=100, label='Gateway')

# Criar legenda para os grupos
handles = [plt.Line2D([0], [0], marker='o', color='w', label=f'Payload de {g} bytes',
                      markerfacecolor=mapa_cores[g], markersize=10)
           for g in grupos]
handles.append(plt.Line2D([0], [0], marker='x', color='black', label='Gateway', linestyle=''))


plt.legend(handles=handles, title="Legenda", bbox_to_anchor=(1.05, 1), loc='upper left')

# Adicionar cursor interativo
cursor = mplcursors.cursor(sc, hover=True)

@cursor.connect("add")
def on_add(sel):
    idx = sel.index
    d = dispositivos[idx]
    sel.annotation.set(text=f"ID: {d['id']}\nEnviadas: {d['enviadas']} \nEnergia por bit: {d['energia_bits']}J")
    sel.annotation.get_bbox_patch().set(fc="white", alpha=0.8)

gw_cursor = mplcursors.cursor(gw, hover=True)

@gw_cursor.connect("add")
def on_add_gateway(sel):
    sel.annotation.set(text=f"Total de Mensagens Recebidas: {results['gateway']['PacketsReceived']}")
    sel.annotation.get_bbox_patch().set(fc="white", alpha=0.8)    

plt.xlabel("Posição X")
plt.ylabel("Posição Y")
plt.title("Posição dos {} Dispositivos".format(num_nodes))
plt.grid(True)
plt.tight_layout()
plt.show()

