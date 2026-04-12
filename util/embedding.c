#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "action.h"
//typedef int (*run_benchmark_t)(const char*, char *const []);
typedef void (*start_monitoring_t)(const char*);
#define PATH_MAX 4096

int main(){
	int bench_pid = fork();
	if(bench_pid==0){
        setenv("PYTHONPATH", "/home/wskim/miniconda3/envs/torch_gpu/lib/python3.10/site-packages", 1);
		char *exe_dir="./python_benchmarks";
		char *exe_path= "/usr/bin/python3";
        char *file = "embedding.py";
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
		const char *file_name = "embedding.csv";
		do_monitor(file_name);
		exit(0);
	}

	sleep(1);
	waitpid(bench_pid,NULL,0);
	printf("child process %d ended\n",bench_pid);
	kill(monitor_pid,SIGTERM);
	waitpid(monitor_pid,NULL,0);
	printf("child process %d ended\n",monitor_pid);
	return 0;
}
