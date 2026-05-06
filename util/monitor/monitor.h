#ifndef MONITOR_H
#define MONITOR_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"{
#endif
	#include <limits.h>
	#include <stdio.h>
	#include <dcgm_fields.h>

	#define NUM_FIELDS 7
	#define MAX_GPUS 16
	#define MAX_FIELDS 16
	#define MAX_BUF 256
	
	extern const unsigned short basic_field_ids[];
	extern size_t basic_field_size;
	typedef struct {
		FILE *fp;

		dcgm_field_eid_t entityId[MAX_FIELDS];
		long timestamp[MAX_FIELDS];

		// =========================
		// Power
		// =========================
		double power[MAX_FIELDS];
		double gpu_power_limit[MAX_FIELDS];
		long long total_energy[MAX_FIELDS];

		// =========================
		// Thermal
		// =========================
		long long gpu_temp[MAX_FIELDS];
		long long memory_temp[MAX_FIELDS];
		long long slowdown_temp[MAX_FIELDS];
		long long shutdown_temp[MAX_FIELDS];

		// =========================
		// Clock
		// =========================
		long long sm_clock[MAX_FIELDS];
		long long mem_clock[MAX_FIELDS];
		long long app_sm_clock[MAX_FIELDS];
		long long app_mem_clock[MAX_FIELDS];
		long long max_sm_clock[MAX_FIELDS];
		long long max_mem_clock[MAX_FIELDS];

		// =========================
		// Utilization
		// =========================
		long long gpu_util[MAX_FIELDS];
		long long mem_copy_util[MAX_FIELDS];
		long long enc_util[MAX_FIELDS];
		long long dec_util[MAX_FIELDS];

		// =========================
		// Memory
		// =========================
		long long fb_used[MAX_FIELDS];
		long long fb_free[MAX_FIELDS];
		long long fb_total[MAX_FIELDS];

		// =========================
		// PCIe
		// =========================
		long long pcie_tx_throughput[MAX_FIELDS];
		long long pcie_rx_throughput[MAX_FIELDS];
		long long pcie_replay[MAX_FIELDS];

		// =========================
		// Reliability
		// =========================
		long long ecc_sbe[MAX_FIELDS];
		long long ecc_dbe[MAX_FIELDS];

		// =========================
		// Violations
		// =========================
		long long power_violation[MAX_FIELDS];
		long long thermal_violation[MAX_FIELDS];
		long long sync_boost_violation[MAX_FIELDS];
		long long board_limit_violation[MAX_FIELDS];
		long long low_util_violation[MAX_FIELDS];
		long long reliability_violation[MAX_FIELDS];

		// =========================
		// Profiling
		// =========================
		double sm_active[MAX_FIELDS];
		double sm_occupancy[MAX_FIELDS];
		double tensor_active[MAX_FIELDS];
		double fp64_active[MAX_FIELDS];
		double fp32_active[MAX_FIELDS];
		double fp16_active[MAX_FIELDS];
		double dram_active[MAX_FIELDS];
		double pcie_tx[MAX_FIELDS];
		double pcie_rx[MAX_FIELDS];
		double gr_engine_active[MAX_FIELDS];
		double nvlink_tx_bytes[MAX_FIELDS];
		double nvlink_rx_bytes[MAX_FIELDS];

	} monitor_ctx_t;
	typedef struct {
		unsigned long long user,nice,system;
		unsigned long long idle, iowait;
		unsigned long long irq, softirq,steal;
	} cpu_proc_stat_t;
	typedef struct {
		unsigned long long utime;
		unsigned long long stime;
	} cpu_self_stat_t;

	typedef struct{
		double power_w;
		unsigned int temperature;
		unsigned int gpu_util,mem_util;
		unsigned int clock;
	} gpu_stat_t;

	typedef struct {
		char gpu_stat_csv_file[PATH_MAX];
		char cpu_stat_csv_file[PATH_MAX];
	}dcgm_overhead_ctx_t;

	void start_monitoring(const char *csv_path,const unsigned short mode);
	void stop_monitoring(void);
	int start_monitor_overhead(const char *gpu_stat_csv_file,const char *cpu_stat_csv_file,const unsigned short mode);
	void stop_monitor_overhead(void);
	unsigned short* set_field_ids(unsigned short mode,const unsigned short*basic_field_ids,size_t count,int *final_count);
#ifdef __cplusplus
}
#endif

#endif
