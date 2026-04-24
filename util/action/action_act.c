#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include "action_act.h"
#include <string.h>

typedef  int (*run_benchmark_t)(const char *,const char *,char *const[]);
typedef void (*start_monitoring_t)(const char *,const unsigned short);
typedef void (*start_monitor_overhead_t)(const char *,const unsigned short);

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

void do_monitor(const char *filename,const unsigned short mode,const char *monitor_mode){
    printf("mode : %hu\n",mode);
    char *monitor_dll_path=reconstruct_path("monitor/libmonitor.so");
    if(monitor_dll_path==NULL){
        exit(1);
    }
    char result_file_path[PATH_MAX];
    char result_dir_path[PATH_MAX];
    if(strcmp(monitor_mode,"monitor")==0){
        strcpy(result_dir_path,"../result/metric");
    }else if(strcmp(monitor_mode,"overhead")==0){
        strcpy(result_dir_path,"../result/overhead");
    }else{
        fprintf(stderr,"Inappropirate monitor mode name. It should be \"monitor\" or \"overhead\"\n");
        exit(1);
    }
    if(snprintf(result_file_path,sizeof(result_file_path),"%s/%s",result_dir_path,filename)>sizeof(result_file_path)){
        perror("filename truncated!\n");
        exit(1);
    }

    printf("dll path %s\n",monitor_dll_path);
    printf("csv_path %s\n",result_file_path);

    dlerror();
    void *monitor_handle=dlopen(monitor_dll_path,RTLD_LAZY);
    char *dl_error=dlerror();
    free(monitor_dll_path);
    if(dl_error){
        printf("dlsym error:%s\n", dl_error);
        exit(1);
    }
    if(strcmp(monitor_mode,"monitor")==0){
        start_monitoring_t start_monitoring = (start_monitoring_t)dlsym(monitor_handle,"start_monitoring");
        dl_error=dlerror();
        if(dl_error){
            dlclose(monitor_handle);
            fprintf(stderr,"dlsym error:%s\n", dl_error);
            exit(1);
        }
        start_monitoring(result_file_path,mode);
    }else if(strcmp(monitor_mode,"overhead")==0){
        start_monitor_overhead_t start_monitor_overhead = (start_monitor_overhead_t)dlsym(monitor_handle,"start_monitor_overhead");
        dl_error=dlerror();
        if(dl_error){
            fprintf(stderr,"dlsym error:%s\n", dl_error);
            dlclose(monitor_dll_path);
            exit(1);
        }
        start_monitor_overhead(result_file_path,mode);
    }
    dlclose(monitor_handle);
    
}
