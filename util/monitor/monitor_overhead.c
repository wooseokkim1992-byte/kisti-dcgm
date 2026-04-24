#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dcgm_structs.h>
#include <dcgm_agent.h>
#include <dcgm_fields.h>
#include <nvml.h>
#include "monitor.h"

static volatile __sig_atomic_t keepRunning=1;

static const unsigned short group_A_field_ids[]={
    //sub group 1
    DCGM_FI_PROF_SM_ACTIVE,
    DCGM_FI_PROF_SM_OCCUPANCY,
    //sub group 2
    DCGM_FI_PROF_PIPE_TENSOR_ACTIVE,
    //sub group 3
    DCGM_FI_PROF_PIPE_FP32_ACTIVE,
    //sub group 4
    DCGM_FI_PROF_PIPE_FP64_ACTIVE,
    //sub group 5
    DCGM_FI_PROF_PIPE_FP16_ACTIVE
};

static const unsigned short group_B_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_DRAM_ACTIVE
};

static const unsigned short group_C_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_PCIE_TX_BYTES,
    DCGM_FI_PROF_PCIE_RX_BYTES,
};

static const unsigned short group_D_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_GR_ENGINE_ACTIVE,
};

static const unsigned short group_E_field_ids[]={
    //sub group 0
    DCGM_FI_PROF_NVLINK_TX_BYTES,
    DCGM_FI_PROF_NVLINK_RX_BYTES
};

void handle_exit_signal(int signal){
    (void)signal;
    keepRunning=0;
}

void disconnect_procedure(dcgmHandle_t *handle,dcgmGpuGrp_t *group_id,dcgmFieldGrp_t *field_group_id){
    if(handle&&*handle){
        if(*field_group_id&&field_group_id){
            dcgmFieldGroupDestroy(*handle, *field_group_id);
            *field_group_id=0;
        }
        if(*group_id&&group_id){
            dcgmGroupDestroy(*handle, *group_id);
            *group_id=0;
        }
        dcgmStopEmbedded(*handle);
        dcgmShutdown();
        (*handle)=0;
    }
}

void start_monitor_overhead(const char *result_path,const unsigned short mode){
    short active_gpu_cnt=0;
    dcgmHandle_t handle;
    dcgmReturn_t result;
    dcgmGpuGrp_t group_id;
    dcgmFieldGrp_t field_group_id;
    size_t gpu_cnts = MAX_GPUS;
    unsigned int gpu_ids[MAX_GPUS];

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
    if(dcgmGetAllSupportedDevices(handle,gpu_ids,&gpu_cnts)!=DCGM_ST_OK){
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
    for(int i=0;i<gpu_cnts;i++){
        dcgmGroupAddDevice(handle,group_id,gpu_ids[i]);
        printf("Add GPU %d \n", gpu_ids[i]);
        active_gpu_cnt++;
    }
    if(active_gpu_cnt<=0){
        fprintf(stderr,"failed to add gpu\n");
        dcgmGroupDestroy(handle,group_id);
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    if(dcgmWatchFields(handle,group_id,group_A_field_ids,1000000,3600.0,0)!=DCGM_ST_OK){
        fprintf(stderr,"failed to add gpu\n");
        dcgmGroupDestroy(handle,group_id);
        dcgmStopEmbedded(handle);
        dcgmShutdown();
        return 1;
    }
    disconnect_procedure(&handle,&group_A_field_ids,&group_id);
}


