import torch
import torch.nn as nn
import time

device = 'cuda' if torch.cuda.is_available() else 'cpu'

# ⚡ 여기서 배치 사이즈 정의
batch_size = 1024

# Embedding 설정
num_embeddings, embedding_dim = 50000, 1024
x_idx = torch.randint(0, num_embeddings, (batch_size, 64), device=device)
embedding = nn.Embedding(num_embeddings, embedding_dim).to(device)

start = time.time()
for _ in range(30000):
    y = embedding(x_idx)
torch.cuda.synchronize()
print(f"Embedding: {time.time()-start:.4f} sec")