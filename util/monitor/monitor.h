#ifndef MONITOR_H
#define MONITOR_H
#ifdef __cplusplus
extern "C"{
#endif
	#define NUM_FIELDS 7
	#define MAX_GPUS 16
	#define MAX_FIELDS 16
	#define MAX_BUF 256
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
		const char* result_file_path;
	}dcgm_overhear_ctx_t;
	void start_monitoring(const char *csv_path,const unsigned short mode);
	void stop_monitoring(void);
	int start_monitor_overhead(const char *result_path,const unsigned short mode);
	void stop_monitor_overhead(void);
#ifdef __cplusplus
}
#endif

#endif
