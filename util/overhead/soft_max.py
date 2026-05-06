import ctypes
import torch
import time
import threading
import sys
# ============================
# load libmonitor
# ============================
lib = ctypes.CDLL("../monitor/libmonitor.so")

lib.start_monitor_overhead.argtypes = [ctypes.c_char_p,ctypes.c_char_p, ctypes.c_ushort]
lib.start_monitor_overhead.restype = ctypes.c_int

lib.stop_monitor_overhead.argtypes = []
lib.stop_monitor_overhead.restype = None

# ============================
# config
# ============================
BATCH = 1024*4
DIM = 8192
ITER = 100000
WARMUP = 20

# ============================
# worker (per GPU)
# ============================
def worker(device_id, results, idx):
    device = f"cuda:{device_id}"

    x = torch.randn(BATCH, DIM, device=device)

    # warmup
    for _ in range(WARMUP):
        torch.softmax(x, dim=-1)

    torch.cuda.synchronize(device)

    start = time.time()

    for _ in range(ITER):
        torch.softmax(x, dim=-1)
    # softmax 가 GPU 에서 전부 끝날 때 까지 기다림.
    torch.cuda.synchronize(device)

    end = time.time()

    avg = (end - start) * 1000 / ITER
    results[idx] = avg

    print(f"[GPU {device_id}] Avg latency: {avg:.3f} ms")

# ============================
# multi GPU runner
# ============================
def run_multi_gpu_softmax():
    gpu_count = torch.cuda.device_count()

    print(f"Using {gpu_count} GPUs")

    threads = []
    results = [0.0] * gpu_count

    # 동시에 실행
    for i in range(gpu_count):
        t = threading.Thread(target=worker, args=(i, results, i))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    print("\n=== Summary ===")
    for i, r in enumerate(results):
        print(f"GPU {i}: {r:.3f} ms")

    print(f"Avg (all GPU): {sum(results)/len(results):.3f} ms")

# ============================
# main
# ============================
if __name__ == "__main__":
    print(sys.executable)
    print("=== DCGM Embed + Multi GPU Softmax ===")
    cpu_overhead_csv_file = "soft_max_cpu_overhead.csv".encode("utf-8")
    gpu_overhead_csv_file = "soft_max_gpu_overhead.csv".encode("utf-8")

    # 🔥 DCGM start
    if lib.start_monitor_overhead(gpu_overhead_csv_file,cpu_overhead_csv_file,1) != 0:
        raise RuntimeError("dcgm_start failed")

    # 🔥 multi GPU workload
    run_multi_gpu_softmax()

    # 🔥 DCGM stop
    lib.stop_monitor_overhead()