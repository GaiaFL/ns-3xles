import os
import pickle
import json
from Simulations.GlobalConfig import locations_file, num_of_simulations, num_locations, cell_size
from Framework.Location import Location


path = "/home/marianna/locations.txt"
locations_per_les = list()
locations_per_ns= list()

for num_sim in range(num_of_simulations):
    locations_ns = list()
    locations_les = list()
    for i in range(num_locations):
        l = Location(min=0, max=cell_size, indoor=False)
        locations_les.append(l)
        locations_ns.append((l.x, l.y))
    locations_per_les.append(locations_les)
    locations_per_ns.append(locations_ns)


# Arquivo do NS
os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, 'w') as f:
    json.dump(locations_per_ns, f)

# Arquivo do LES

os.makedirs(os.path.dirname(locations_file), exist_ok=True)
with open(locations_file, 'wb') as f:
    pickle.dump(locations_per_les, f)