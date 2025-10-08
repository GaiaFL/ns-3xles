import numpy as np

############### SIMULATION SPECIFIC PARAMETERS ###############
start_with_fixed_sf = False
start_sf = 7

scaling_factor = 0.1
transmission_rate_id = str(scaling_factor)
#transmission_rate_bit_per_ms = scaling_factor*(12*8)/(60*60*1000)  # 12*8 bits per hour (1 typical packet per hour)
transmission_rate_bit_per_ms = [(10*8)/(5000)]  # 10*8 bits per 4s (1 typical packet per 5s)

# simulation_time = 24 * 60 * 60 * 1000 * 30/scaling_factor
simulation_time = 86400000 #em ms

# cell_size = 1000
cell_size = 100

adr = False
confirmed_messages = False

# payload_sizes = range(5, 55, 5)
payload_sizes = [10]
percent_payload_size = [1]

# path_loss_variances = [7.9]  # [0, 5, 7.8, 15, 20]
path_loss_variances = [7.7]  # [0, 5, 7.8, 15, 20]


MAC_IMPROVEMENT = False
num_locations = 1
num_of_simulations = 1
locations_file = "locations/"+"{}_locations_{}_sim.pkl".format(num_locations, num_of_simulations)
results_file = "results/{}_{}_{}_cnst_num_bytes.p".format(num_locations, confirmed_messages, num_of_simulations)

############### SIMULATION SPECIFIC PARAMETERS ###############

############### DEFAULT PARAMETERS ###############
LOG_ENABLED = False
# MAX_DELAY_BEFORE_SLEEP_MS = 500
MAX_DELAY_BEFORE_SLEEP_MS = 0

PRINT_ENABLED = False
# MAX_DELAY_START_PER_NODE_MS = np.round(simulation_time / 10)
MAX_DELAY_START_PER_NODE_MS = 0

track_changes = True
middle = np.round(cell_size / 2)
load_prev_simulation_results = True

############### DEFAULT PARAMETERS ###############
