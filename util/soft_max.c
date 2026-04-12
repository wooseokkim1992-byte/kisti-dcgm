#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "action.h"
#include "parse_log_file.h"
//typedef int (*run_benchmark_t)(const char*, char *const []);
//typedef void (*start_monitoring_t)(const char*);

#define PATH_MAX 4096

int main(){
	time_range *t_range=malloc(sizeof(time_range));
	int bench_pid = fork();
	long start = get_time_us();

	if(bench_pid==0){
        setenv("PYTHONPATH", "/home/wskim/miniconda3/envs/torch_gpu/lib/python3.10/site-packages", 1);
		char *exe_dir="./python_benchmarks";
		char *exe_path= "/usr/bin/python3";
        char *file = "soft_max.py";
		char *args[] = {
                exe_path,
      			file,
        		NULL
    		};
		exec_benchmark(exe_dir,exe_path,args);
		exit(0);
	}
	int monitor_pid = fork();
	if(monitor_pid==0){
		const char *file_name = "soft_max.csv";
		do_monitor(file_name);
		exit(0);
	}


	sleep(1);
	waitpid(bench_pid,NULL,0);
	printf("child process %d ended\n",bench_pid);
	kill(monitor_pid,SIGTERM);
	waitpid(monitor_pid,NULL,0);
	long end = get_time_us();
	t_range->start=start;
	t_range->end=end;
	printf("child process %d ended\n",monitor_pid);
	parse_stats_targeted_power(t_range,"soft_max.txt");
	return 0;
}
