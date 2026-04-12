#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include "action.h"
#include <string.h>

#define PATH_MAX 4096
typedef  int (*run_benchmark_t)(const char *,const char *,char *const[]);
typedef void (*start_monitoring_t)(const char *);

char* reconstruct_path(const char *src){
    char cwd[PATH_MAX];
    char *reconstructed_path;
    if(getcwd(cwd,PATH_MAX)==NULL){
        perror("getcwd failed\n");
        return NULL;
    }
    size_t len = strlen(cwd)+strlen(src)+2;
    reconstructed_path=malloc(len);
    if(snprintf(reconstructed_path,len,"%s/%s",cwd,src)>=len){
        perror("path truncated!\n");
        free(reconstructed_path);
        return NULL;
    }
    return reconstructed_path;
}

void exec_benchmark(const char *exe_dir,const char *exe_path,char *args[]){
    const char *benchmark_dir="benchmark/libbenchmark.so";
    char *benchmark_path = reconstruct_path(benchmark_dir);
    if(benchmark_path==NULL){
        exit(1);
    }
    printf("benchmark path : %s \n",benchmark_path);
    dlerror();//initializing previous error
    void *bench_handle = dlopen(benchmark_path,RTLD_LAZY);
    free(benchmark_path);
    printf("exec benchmark\n");
    char *err = dlerror();
    if(err){
        printf("dlsym error:%s\n", err);
        exit(1);
    }
    run_benchmark_t run_benchmark = (run_benchmark_t)dlsym(bench_handle,"run_benchmark");
    err = dlerror();
    if(err){
        dlclose(bench_handle);
        printf("dlsym error:%s\n", err);
        exit(1);
    }
    run_benchmark(exe_dir,exe_path,args);
    dlclose(bench_handle);
}

void do_monitor(const char *filename){
    char *monitor_dll_path=reconstruct_path("monitor/libmonitor.so");
    if(monitor_dll_path==NULL){
        exit(1);
    }
    char csv_path[PATH_MAX];
    if(snprintf(csv_path,sizeof(csv_path),"../result/metric/%s",filename)>sizeof(csv_path)){
        perror("filename truncated!\n");
        exit(1);
    }

    printf("dll path %s\n",monitor_dll_path);
    printf("csv_path %s\n",csv_path);

    dlerror();
    void *monitor_handle=dlopen(monitor_dll_path,RTLD_LAZY);
    char *dl_error=dlerror();
    free(monitor_dll_path);
    if(dl_error){
        printf("dlsym error:%s\n", dl_error);
        exit(1);
    }
    start_monitoring_t start_monitoring = (start_monitoring_t)dlsym(monitor_handle,"start_monitoring");
    dl_error=dlerror();
    if(dl_error){
        dlclose(monitor_handle);
        printf("dlsym error:%s\n", dl_error);
        exit(1);
    }
    start_monitoring(csv_path);
    dlclose(monitor_handle);
}
