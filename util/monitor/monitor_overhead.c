
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dcgm_structs.h>
#include <dcgm_agent.h>
#include <dcgm_fields.h>
#include <nvml.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <string.h>
#include "parse_log_file.h"
#include "monitor.h"

static dcgmHandle_t handle;
static dcgmGpuGrp_t group_id;
static dcgmReturn_t result;
static dcgmFieldGrp_t field_group_id;
// dcgmProfGetMetricGroups_t metricGroups;

static nvmlDevice_t devs[MAX_GPUS];

static unsigned int gpu_cnts = MAX_GPUS;

static atomic_int running = 0;
static pthread_t collector_thread;

static unsigned short group_A_field_ids[]={
    DCGM_FI_PROF_SM_ACTIVE,
    DCGM_FI_PROF_SM_OCCUPANCY,
    DCGM_FI_PROF_DRAM_ACTIVE,
    DCGM_FI_PROF_PCIE_TX_BYTES,
    DCGM_FI_PROF_PCIE_RX_BYTES,
    DCGM_FI_PROF_GR_ENGINE_ACTIVE,
    DCGM_FI_PROF_NVLINK_TX_BYTES,
    DCGM_FI_PROF_NVLINK_RX_BYTES
};

static unsigned short group_B_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_DRAM_ACTIVE
};

static dcgmFieldGrp_t group_C_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_PCIE_TX_BYTES,
    DCGM_FI_PROF_PCIE_RX_BYTES,
};

static dcgmFieldGrp_t group_D_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_GR_ENGINE_ACTIVE,
};

static dcgmFieldGrp_t group_E_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_NVLINK_TX_BYTES,
    DCGM_FI_PROF_NVLINK_RX_BYTES
};

unsigned short nvml_setting(){
    if(nvmlInit_v2()!=NVML_SUCCESS){
        fprintf(stderr,"failed to initialize NVML!");
        return 1;
    }    
    if(nvmlDeviceGetCount_v2(&gpu_cnts)!=NVML_SUCCESS){
        fprintf(stderr,"Failed to get GPU device Count!");
        return 1;
    }
    if(gpu_cnts>=MAX_GPUS){
        gpu_cnts=MAX_GPUS;
    }
    for(int i=0;i<gpu_cnts;i++){
        if(nvmlDeviceGetHandleByIndex_v2(i,&devs[i])){
            fprintf(stderr,"Failed to get Device Handle by Index!");
            return 1;
        }
    }
    return 0;
}

void nvml_cancel(FILE **fp){
    fclose(*fp);
    nvmlShutdown();
}

int get_gpu_stats(
    nvmlDevice_t dev,
    gpu_stat_t *gpu_stat
){
    nvmlUtilization_t util;
    unsigned int power;
    if(nvmlDeviceGetUtilizationRates(dev, &util)!=NVML_SUCCESS){
        printf("Failed to Get GPU Utilization Rate!\n");
        return 1;
    }
    if(nvmlDeviceGetPowerUsage(dev, &power)!=NVML_SUCCESS){
        printf("Failed to Get GPU Power Value!\n");
        return 1;
    }

    if(nvmlDeviceGetTemperature(dev, NVML_TEMPERATURE_GPU, &gpu_stat->temperature)){
        printf("Failed to Get GPU Temperature Rate!\n");
        return 1;
    }

    if(nvmlDeviceGetClockInfo(dev, NVML_CLOCK_SM, &gpu_stat->clock)){
        printf("Failed to get device clock info\n");
        return 1;
    }  
    
    gpu_stat->gpu_util=util.gpu;
    gpu_stat->mem_util=util.memory;
    gpu_stat->power_w=(double)power/1000;
    return 0;
}

int get_gpu_stats_for_all_devices(FILE *fp){
    gpu_stat_t gpu_stat;
    long time = get_time_us();
    for(int i=0;i<gpu_cnts;i++){
        get_gpu_stats(devs[i],&gpu_stat);
        printf( "[[POWER]:%2.f\n",gpu_stat.power_w);
        printf( "[GPU UTIL]:%u\n",gpu_stat.gpu_util);
        printf( "[CLOCK] : %u\n",gpu_stat.clock);
        printf( "[TEMPERATURE] : %u\n\n",gpu_stat.temperature);
        fprintf(fp,
                "Time Stamp : %ld\n"
                "[POWER]:%2.f\n"
                "[GPU UTIL]:%u\n"
                "[Memory UTIL]:%u\n"
                "[CLOCK] : %u\n"
                "[TEMPERATURE] : %u\n\n"
            ,time,gpu_stat.power_w,gpu_stat.gpu_util,gpu_stat.mem_util,gpu_stat.clock,gpu_stat.temperature);
    }
    return 0;
}


void stop_monitor_overhead(){
    running=0;
    pthread_join(collector_thread, NULL);
    if(handle){
        if(field_group_id){
            dcgmFieldGroupDestroy(handle, field_group_id);
            field_group_id=0;
        }
        if(group_id){
            dcgmGroupDestroy(handle, group_id);
            group_id=0;
        }
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        handle=0;
    }
}

unsigned long long total_cpu(cpu_proc_stat_t *s) {
    return s->user + s->nice + s->system +
           s->idle + s->iowait +
           s->irq + s->softirq + s->steal;
}

double calc_cpu_usage(
    cpu_proc_stat_t *prev_cpu_proc,cpu_proc_stat_t *curr_cpu_proc,
    cpu_self_stat_t *prev_cpu_self,cpu_self_stat_t *curr_cpu_self,
    double *user_ratio,double *system_ratio
){
    unsigned long long prev_total = total_cpu(prev_cpu_proc);
    unsigned long long curr_total = total_cpu(curr_cpu_proc);

    unsigned long long proc_prev = prev_cpu_self->stime+prev_cpu_self->utime;
    unsigned long long proc_curr = curr_cpu_self->stime+curr_cpu_self->utime;

    unsigned long long proc_delta = proc_curr-proc_prev;
    unsigned long long total_delta = curr_total - prev_total;
    if(total_delta==0){
        return 0.0;
    }
    printf("proc_delta : %lld\n",proc_delta);
    printf("total_delta : %lld\n",total_delta);
    double usage = (double) proc_delta/total_delta;
     // user / system 분해
    *user_ratio   = (double)(curr_cpu_self->utime - prev_cpu_self->utime) / total_delta * 100.0;
    *system_ratio = (double)(curr_cpu_self->stime - prev_cpu_self->stime) / total_delta * 100.0;

    return usage;
}

int read_self_stat(cpu_self_stat_t *stat){
    FILE *self_stat=fopen("/proc/self/stat","r");
    if(self_stat==NULL){
        fprintf(stderr,"Failed to open /proc/self/stat file!");
        return 1;
    }
    char *line=NULL;
    size_t len=0;
    if(getline(&line, &len, self_stat)==-1){
        fclose(self_stat);
        free(line);
        fprintf(stderr,"failed to get line");
        return 1;
    }
    fclose(self_stat);
    char comm[MAX_BUF];
    char state;
    sscanf(line,"%*d (%[^)]) %c" 
        "%*d %*d %*d %*d %*d"
        "%*u %*u %*u %*u %*u "
        "%llu %llu",
        comm, &state,&stat->utime,&stat->stime
    );
    free(line);
    return 0;
}

int read_cpu_proc_stat(cpu_proc_stat_t *stat){
    FILE *proc_stat = fopen("/proc/stat","r");
    if(proc_stat==NULL){
        fprintf(stderr,"Failed to Read /proc/stat file!");
        return 1;
    }
    char *line=NULL;
    size_t len=0;
    if(getline(&line,&len,proc_stat)==-1){
        fclose(proc_stat);
        fprintf(stderr,"Failed to read get first line in /proc/stat!");
        return 1;
    }
    fclose(proc_stat);
    printf("line : %s",line);
    line[strcspn(line,"\n")]=0;
    sscanf(
        line,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &stat->user,
        &stat->nice,
        &stat->system,
        &stat->idle,
        &stat->iowait,
        &stat->irq,
        &stat->softirq,
        &stat->steal
    );
    free(line);
    
    return 0;   
}
int dcgm_collect_callback(dcgm_field_entity_group_t entity_group_id,
                     dcgm_field_eid_t entity_id,
                     dcgmFieldValue_v1 *values,
                     int num_values,void* user_data){
    for(int i=0;i<num_values;i++){
        if(values[i].status!=DCGM_ST_OK)continue;
            printf("field ID: %u \n",values[i].fieldId);
        if(values[i].fieldType==DCGM_FT_INT64){
            printf("values : %lld\n",(long long)values[i].value.i64);
        }else if(values[i].fieldType==DCGM_FT_DOUBLE){
            printf("values : %2.f\n",(double)values[i].value.dbl);
        }
    }
    return 0;
}

void *collector(void *arg){
    char *result_file_path = (char *)arg;
    if(result_file_path==NULL){
        fprintf(stderr,"argument should be initialized!");
        return NULL;
    }
    printf("file name : %s\n",result_file_path);
    FILE *fp = fopen(result_file_path,"w");
    if(!fp){
        fprintf(stderr,"failed to opening file!\n");
        return NULL;
    }
    cpu_proc_stat_t curr_cpu_proc,prev_cpu_proc;
    cpu_self_stat_t curr_cpu_self,prev_cpu_self;
    read_cpu_proc_stat(&prev_cpu_proc);
    read_self_stat(&prev_cpu_self);
    while(running){
        result = dcgmGetLatestValues_v2(handle,  group_id, field_group_id,  dcgm_collect_callback,NULL);
        read_cpu_proc_stat(&curr_cpu_proc);
        read_self_stat(&curr_cpu_self);
        double user_ratio=0.0;
        double system_ratio=0.0;
        double usage=calc_cpu_usage(&prev_cpu_proc,&curr_cpu_proc,&prev_cpu_self,&curr_cpu_self,&user_ratio, &system_ratio);
        printf("user_ratio : %.2f\n",user_ratio);
        printf("system_ratio : %.2f\n",system_ratio);
        printf("usage : %.2f\n",usage);
        get_gpu_stats_for_all_devices(fp);
        sleep_us(1*1000*1000);//1ms
        prev_cpu_proc=curr_cpu_proc;
        prev_cpu_self=curr_cpu_self;
    }
    nvml_cancel(&fp);
    return 0;
}

int search_supported_profile_metrics(dcgmHandle_t *handle){
    dcgmProfGetMetricGroups_t metricGroups;
    memset(&metricGroups, 0, sizeof(metricGroups));
    metricGroups.version = dcgmProfGetMetricGroups_version;
    if(dcgmProfGetSupportedMetricGroups(*handle, &metricGroups)!=DCGM_ST_OK){
        fprintf(stderr,"Failed to get Supported Metric Group!");
        return 0;
    }
    printf("Number of metric groups: %d\n", metricGroups.numMetricGroups);
    for(int i=0;i<metricGroups.numMetricGroups;i++){
        dcgmProfMetricGroupInfo_v2 *group_info = &metricGroups.metricGroups[i];
        printf("Major ID : %u\n",group_info->majorId);
        printf("Minor ID : %u\n",group_info->minorId);
        printf("Num Metrics : %d\n",group_info->numFieldIds);
        size_t field_size = group_info->numFieldIds;
        for(int j =0;j<field_size;j++){
            printf("    Metric ID : %d\n",group_info->fieldIds[j]);
        }
    }
    return 1;
}

int start_monitor_overhead(const char *result_path,const unsigned short mode){
    short active_gpu_cnt=0;
    unsigned int gpu_ids[DCGM_MAX_NUM_DEVICES];
    printf("result_path : %s\n",result_path);
    printf("[MONITOR OVERHEAD] DCGM Init\n");
    if(dcgmInit()!=DCGM_ST_OK){
        fprintf(stderr,"Failed to Initialize DCGM!\n");
        return 1;
    }
    if(dcgmStartEmbedded(DCGM_OPERATION_MODE_AUTO,&handle)!=DCGM_ST_OK){
        fprintf(stderr,"Failed to Embed DCGM!\n");
        dcgmShutdown();
        return 1;
    }
    int gpu_cnt_local=0;
    if(dcgmGetAllSupportedDevices(handle,gpu_ids,&gpu_cnt_local)!=DCGM_ST_OK){
        fprintf(stderr,"Failed to get supported devices!\n");
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    if(dcgmGroupCreate(handle,DCGM_GROUP_EMPTY,"active_gpus",&group_id)!=DCGM_ST_OK){
        fprintf(stderr,"Failed to create GPU Group!\n");
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    for(int i=0;i<gpu_cnt_local;i++){
        dcgmGroupAddDevice(handle,group_id,gpu_ids[i]);
        printf("Add GPU %d \n", gpu_ids[i]);
        active_gpu_cnt++;
    }
    gpu_cnts=active_gpu_cnt;
    if(active_gpu_cnt<=0){
        fprintf(stderr,"failed to add gpu\n");
        dcgmGroupDestroy(handle,group_id);
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    const int num_of_fields = sizeof(group_A_field_ids)/sizeof(group_A_field_ids[0]);

    if(dcgmFieldGroupCreate(handle,num_of_fields,group_A_field_ids,"fields",&field_group_id)!=DCGM_ST_OK){
        fprintf(stderr,"failed to create field group.\n");
        dcgmGroupDestroy(handle,group_id);
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    printf("group_id : %ld\n",field_group_id);
    if(dcgmWatchFields(handle,group_id,field_group_id,1000000,3600.0,0)!=DCGM_ST_OK){
        fprintf(stderr,"failed to watch fields\n");
        dcgmGroupDestroy(handle,group_id);
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    if(nvml_setting()!=0){
        return 1;
    }
    search_supported_profile_metrics(&handle);
    running=1;
    pthread_create(&collector_thread,NULL,collector, (void*)result_path);
    return 0;
}


