import torch
import torch.nn as nn
import time

device = 'cuda' if torch.cuda.is_available() else 'cpu'

# ⚡ 배치 크기 및 입력 크기 정의
batch_size = 512
channels = 256
height = 128
width = 128

# 입력 텐서 생성
x = torch.randn(batch_size, channels, height, width, device=device)

# MaxPool2d 레이어
maxpool = nn.MaxPool2d(kernel_size=4, stride=2).to(device)

start = time.time()
for _ in range(1000):
    y = maxpool(x)
torch.cuda.synchronize()
print(f"MaxPool2d: {time.time()-start:.4f} sec")