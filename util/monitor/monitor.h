#ifndef MONITOR_H
#define MONITOR_H
#ifdef __cplusplus
extern "C"{
#endif
	void start_monitoring(const char *csv_path);
	void stop_monitoring(void);
#ifdef __cplusplus
}
#endif

#endif
