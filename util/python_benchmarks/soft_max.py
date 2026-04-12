#!/usr/bin/python3

import torch
import time

device = 'cuda'
batch_size = 16384
num_classes = 8192
x = torch.randn(batch_size, num_classes, device=device)

# Warmup
for _ in range(10):
    _ = torch.nn.functional.softmax(x, dim=1)

# Timing
torch.cuda.synchronize()
start = time.time()
for _ in range(50000):
    _ = torch.nn.functional.softmax(x, dim=1)
torch.cuda.synchronize()
end = time.time()

print(f"Softmax average time per forward: {(end - start)/100:.6f} sec")