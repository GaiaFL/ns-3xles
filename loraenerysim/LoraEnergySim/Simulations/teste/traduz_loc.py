import pickle
import argparse
import os
import json

parser = argparse.ArgumentParser(description="Recebe o número de nós e usa como parte do nome do arquivo.")
parser.add_argument('--node', type=int, required=True, help='Número para usar no nome do arquivo')

args = parser.parse_args()
node = args.node

locations_file = f"/home/marianna/LoraEnergySim/locations/{node}_locations_1_sim.pkl"

path = "/home/marianna/locations.txt"
locals = list()
with open(locations_file, 'rb') as file_handler:
    locations_per_simulation = pickle.load(file_handler)
    num_of_simulations = len(locations_per_simulation)
    num_nodes = len(locations_per_simulation[0])

for loc in locations_per_simulation[0]:
    
    locals.append((loc.x, loc.y))

os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, 'w') as f:
    json.dump(locals, f)