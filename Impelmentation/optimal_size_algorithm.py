import torch
import torch.nn as nn
import torch.optim as optim
import json
import struct
import math

SECTION_BITS = 8
BLOCK_BITS = 16
INDICATOR_BITS = 1

# generate one dimension diff based on two models
def optimal_size_algorithm(base_path, update_path):
  base_model = torch.load(base_path)
  update_model = torch.load(update_path)
  weights = ['conv1.weight', 'conv2.weight', 'fc1.vector', 'fc2.weight']
  diff = []
  total_size = 0
  optimal_size = 0 # in bytes
  optimal_section = 0

  # generate difference
  for w in weights:
    size = 1
    for i in base_model[w].shape:
      size = size * i
    base = base_model[w].reshape([size])
    update = update_model[w].reshape([size])
    total_size = total_size + size

    for i in range(size):
      if base[i] == update[i]:
        diff.append(float('nan'))
      else:
        diff.append(float(update[i]))

  # calculate optimal
  for section in range(1, pow(2, SECTION_BITS - INDICATOR_BITS)): 
    max_index = math.ceil(total_size / section) # the number elements in a section
    index_bits = math.ceil(math.log2(max_index))
    if (index_bits + INDICATOR_BITS > BLOCK_BITS - 1): # at least 1 bit length
      continue
    length = pow(2, BLOCK_BITS - INDICATOR_BITS - index_bits)
    total = 0 # update size
    #print(index_bits, length)
    for s in range(section):
      total = total + 1 # indicator + weight = 1 byte
      continuous = 0 # continuous data points
      for i in range(s * max_index, min((s + 1) * max_index, total_size)):       
        if diff[i] == float('nan'):
          continuous = 0
        else:
          if not (continuous > 0 and continuous < length):
            continuous = 0
            total = total + 2 # indicator + index + length = 2 bytes
          total = total + 2 # data point = 2 bytes
          continuous = continuous + 1
    if optimal_size == 0 or total < optimal_size:
      optimal_size = total
      optimal_section = section

  return optimal_section


base_path = 'mnist_cnn_data_range_9_9435_sparsity_50.pt'
update_path = 'mnist_cnn_data_range_9_9421_sparsity_60.pt'
section = optimal_size_algorithm(base_path, update_path);
print(section)