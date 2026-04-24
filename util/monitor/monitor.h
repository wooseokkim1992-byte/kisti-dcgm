#ifndef MONITOR_H
#define MONITOR_H
#ifdef __cplusplus
extern "C"{
#endif
	#define NUM_FIELDS 7
	#define MAX_GPUS 16
	#define MAX_FIELDS 16
	void start_monitoring(const char *csv_path,const unsigned short mode);
	void start_monitor_overhead(const char *result_path,const unsigned short mode);
	void stop_monitoring(void);
#ifdef __cplusplus
}
#endif

#endif
