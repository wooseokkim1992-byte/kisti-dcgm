#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include "action.h"
typedef int (*run_benchmark_t)(const char*, char *const []);
typedef void (*start_monitoring_t)(const char*);

int main(){
	int bench_pid = fork();
	if(bench_pid==0){
		char *kmeans_location = "../gpu-rodinia/cuda/kmeans";
		size_t len = strlen(kmeans_location) + strlen("/kmeans")+1;
		char *kmeans_exe=malloc(len);
		if(!kmeans_exe){
			exit(1);
		}
		char *kmeans_data_location = "../gpu-rodinia/data/kmeans/inpuGen/3000000_34f.txt";
		strcpy(kmeans_exe,kmeans_location);
		strcat(kmeans_exe,"/kmeans");
		printf("kmeans_exe : %s",kmeans_exe);		
		void *bench_handle = dlopen("./benchmark/libbenchmark.so",RTLD_LAZY);
		char *err = dlerror();
		if(err){
			dlclose(bench_handle);
			printf("dlopen failed : %s\n",err);
			free(kmeans_exe);
			exit(1);
		}
		run_benchmark_t run_bench = (run_benchmark_t)dlsym(bench_handle,"run_benchmark");	
		if(!run_bench){
			dlclose(bench_handle);
			free(kmeans_exe);
			printf("no necessary function exists\n");
			exit(1);
		}
		char *args[] = {
      			kmeans_exe,"-o",
        		"-i", kmeans_data_location,
				"-l","1000",
        		NULL
    		};
		run_bench((const char*)kmeans_exe,args);
		exit(0);
	}
	int monitor_pid = fork();
	if(monitor_pid==0){
		const char *result_dir = "../result/metric";
		const char *file_name = "/kmeans.csv";
		size_t len = strlen(result_dir)+strlen("/kmeans.csv")+1;
		char *csv=malloc(len);
		if(!csv){
			exit(1);
		}
		strcpy(csv,result_dir);
		strcat(csv,"/kmeans.csv");
		printf("csv path : %s",csv);
		void *monitor_handle = dlopen("./monitor/libmonitor.so",RTLD_LAZY);
		if(!monitor_handle){
			free(csv);
		}
		char *dlErr = dlerror();
		if(dlErr){
			free(csv);
			dlclose(monitor_handle);
			exit(1);
		}
		start_monitoring_t start_monitoring = (start_monitoring_t)dlsym(monitor_handle,"start_monitoring");
		start_monitoring(csv);
		free(csv);
		dlclose(monitor_handle);
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
