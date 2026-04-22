#ifndef MONITOR_H
#define MONITOR_H
#ifdef __cplusplus
extern "C"{
#endif
	void start_monitoring(const char *csv_path,const unsigned short mode);
	void stop_monitoring(void);
#ifdef __cplusplus
}
#endif

#endif
