#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <limits.h>
#include "action_act.h"

#define PATH_MAX 4096
#define MAX_MODE_VALUE 5

int main(const int argc,const char **argv){
	if(argc!=3){
		fprintf(stderr,"%s usage : <csv_file_name> <mode 1,2,3>\n",argv[0]);
		return 1;
	}
	const unsigned short mode = strtoul(argv[2],NULL,10);
	if(mode>=USHRT_MAX||mode<1||mode>MAX_MODE_VALUE){
		fprintf(stderr,"Inappropriate Mode value. Mode should be one of these values.(1,2,3)\n");
		return 1;
	}
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
		const char *file_name = argv[1];
		do_monitor(file_name,mode);
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
