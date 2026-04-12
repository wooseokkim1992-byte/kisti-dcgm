import torch
import torch.nn as nn
import time

device = 'cuda' if torch.cuda.is_available() else 'cpu'

# 입력 데이터: 배치 64, 채널 128, 이미지 32x32
batch_size, channels, height, width = 512, 256, 128, 128
x = torch.randn(batch_size, channels, height, width, device=device)
bn = nn.BatchNorm2d(channels).to(device)

start = time.time()
for _ in range(1000):
    y = bn(x)
torch.cuda.synchronize()  # GPU 연산 동기화
print(f"BatchNorm2d: {time.time()-start:.4f} sec")