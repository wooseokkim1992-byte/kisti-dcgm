CC=gcc
ACTION_PATH=./action
BENCHMARK_PATH=./benchmark
MONITOR_PATH=./monitor
PARSE_LOG_FILE_PATH=./parse_log_file
# DCGM version 4 
DCGM_INC ?= /usr/include/datacenter-gpu-manager-4
DCGM_LIB ?= /usr/lib/x86_64-linux-gnu

ACTION_OBJS = $(ACTION_PATH)/action_act.o
CFLAGS=-fPIC -Wall -O2
LDFLAGS= -L $(ACTION_PATH) -laction_act -L $(MONITOR_PATH) -lmonitor -L $(BENCHMARK_PATH) -lbenchmark -L $(PARSE_LOG_FILE_PATH) -lparse_log_file -Wl,-rpath=$(BENCHMARK_PATH) -Wl,-rpath=$(MONITOR_PATH) -ldl -lyajl
# DCGM version 4 
CFLAGS  += -I$(DCGM_INC)
LDFLAGS += -L$(DCGM_LIB) -ldcgm

NVML_H_LOCATION := /usr/local/cuda-12.2/targets/x86_64-linux/include
NVIDIA_DLL_PATH := /usr/lib/i386-linux-gnu/libnvidia-ml.so.535.288.01
NVIDIA_DLL_NAME := nvidia-ml

all: $(PARSE_LOG_FILE_PATH)/libparse_log_file.a $(BENCHMARK_PATH)/libbenchmark.so $(MONITOR_PATH)/libmonitor.so $(ACTION_PATH)/libaction_act.a soft_max batch_norm max_pool embedding 

# -------------------
# parse log file
# -------------------

$(PARSE_LOG_FILE_PATH)/parse_log_file.o: $(PARSE_LOG_FILE_PATH)/parse_log_file.c 
	$(CC) -c -fPIC $< -o $@

$(PARSE_LOG_FILE_PATH)/libparse_log_file.a: $(PARSE_LOG_FILE_PATH)/parse_log_file.o
	ar rsc $@ $^
	ranlib $@
# -------------------
# action
# -------------------
$(ACTION_PATH)/action_act.o: $(ACTION_PATH)/action_act.c 
	$(CC) $(CFLAGS) -c $< -o $@

$(ACTION_PATH)/libaction_act.a: $(ACTION_OBJS)
	ar rcs $@ $^

# -------------------
# benchmark
# -------------------

$(BENCHMARK_PATH)/benchmark.o: $(BENCHMARK_PATH)/benchmark.c
	$(CC) $(CFLAGS) -c $< -o $(BENCHMARK_PATH)/benchmark.o

$(BENCHMARK_PATH)/libbenchmark.so: $(BENCHMARK_PATH)/benchmark.o
	$(CC) -shared $^ -o $(BENCHMARK_PATH)/libbenchmark.so 

# -------------------
# monitor
# -------------------
$(MONITOR_PATH)/monitor.o: $(MONITOR_PATH)/monitor.c 
	$(CC) $(CFLAGS) -c $< -I$(PARSE_LOG_FILE_PATH) -o $@

$(MONITOR_PATH)/monitor_overhead.o: $(MONITOR_PATH)/monitor_overhead.c
	$(CC) -fPIC -c $< -I$(NVML_H_LOCATION) -L$(NVIDIA_DLL_PATH) -l$(NVIDIA_DLL_NAME)  -o $@

$(MONITOR_PATH)/libmonitor.so: $(MONITOR_PATH)/monitor.o $(MONITOR_PATH)/monitor_overhead.o $(PARSE_LOG_FILE_PATH)/parse_log_file.o
	$(CC) -shared $^ -o $@ -ldcgm

# -------------------
# soft_max
# -------------------
soft_max.o: soft_max.c
	$(CC) -c -I$(ACTION_PATH) -I$(PARSE_LOG_FILE_PATH) $< -o $@

soft_max:soft_max.o $(ACTION_PATH)/libaction_act.a $(PARSE_LOG_FILE_PATH)/libparse_log_file.a
	$(CC) soft_max.o $(LDFLAGS) -o $@ 

# -------------------
# batch_norm
# -------------------
batch_norm.o: batch_norm.c
	$(CC) -c -I$(ACTION_PATH) $< -o $@

batch_norm:batch_norm.o $(ACTION_PATH)/libaction_act.a
	$(CC) batch_norm.o $(LDFLAGS) -o $@

# -------------------
#max_pool
#-------------------
max_pool.o: max_pool.c
	$(CC) -c -I$(ACTION_PATH) $< -o $@

max_pool:max_pool.o $(ACTION_PATH)/libaction_act.a
	$(CC) max_pool.o $(LDFLAGS) -o $@

# -------------------
# embedding
# -------------------
embedding.o: embedding.c
	$(CC) -c -I$(ACTION_PATH) $< -o $@

embedding:embedding.o $(ACTION_PATH)/libaction_act.a
	$(CC) embedding.o $(LDFLAGS) -o $@

clean:
	rm -f ./soft_max ./embedding ./max_pool ./batch_norm
	rm -f $(ACTION_PATH)/*.a
	rm -f $(ACTION_PATH)/*.o
	rm -f $(BENCHMARK_PATH)/*.so
	rm -f $(BENCHMARK_PATH)/*.o
	rm -f $(MONITOR_PATH)/*.so
	rm -f $(MONITOR_PATH)/*.o
	rm -f $(PARSE_LOG_FILE_PATH)/*.a
	rm -f $(PARSE_LOG_FILE_PATH)/*.o
	rm -f *.o



