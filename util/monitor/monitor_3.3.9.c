
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <dcgm_structs.h>
#include <stdlib.h>
#include "dcgm_agent.h"
#include "dcgm_fields.h"
#include "monitor.h"

#define NUM_FIELDS 7
#define MAX_GPUS 16
#define MAX_FIELDS 16

static volatile sig_atomic_t keepRunning = 1;

typedef struct {
		FILE *fp;
        dcgm_field_eid_t entityId[MAX_FIELDS];
        double power[MAX_FIELDS];
        long long gpu_temp[MAX_FIELDS];
        long long gpu_frame_buffer_used[MAX_FIELDS];
        long long sm_clock_freq[MAX_FIELDS];
        long long pcie_replay_cnt[MAX_FIELDS]; 
	} monitor_ctx_t;

void stop_monitoring(void){
	keepRunning = 0;
}

static void handle_sigterm(int sig){
	(void)sig;
	keepRunning = 0; 
}

void disconnect_fn(dcgmHandle_t *handle,dcgmFieldGrp_t *field_group_id,dcgmGpuGrp_t *group_id,FILE **fp){
    if(fp){
        fflush(*fp);
        fclose(*fp);
    }
    if(handle){
        dcgmFieldGroupDestroy(*handle, *field_group_id);
        dcgmGroupDestroy(*handle, *group_id);
        dcgmDisconnect(*handle);
        dcgmShutdown();
    }
}

FILE* initialize_file(const char *csv_path){
    FILE *fp = fopen(csv_path,"w");
    if(!fp){
        perror("failed open file");
        return NULL;
    }
    const char *field_names[] = {
        "Power(W)",
        "GPU_Temp(C)",
        "FB_Used(Bytes)",
        "SM_Clock(MHz)",
        "PCIe_Replays"
        // "GPU_Util(%)",
        // "SM_Active(%)",
        // "Mem_Copy_Util(%)",
        // "DRAM_Active(%)",
        // "PCIe_TX(MB/s)",
        // "PCIe_RX(MB/s)",
    };
    fprintf(fp,"GPU ID,");
    const int arr_size = sizeof(field_names)/sizeof(field_names[0]);
    for(int i=0;i<arr_size;i++){
        fprintf(fp,"%s",field_names[i]);
        if(i!=arr_size-1){
            fprintf(fp,",");
        }
    }
    fprintf(fp,"\n");
    fflush(fp);
    return fp;
}

int collect_callback(dcgm_field_entity_group_t entityGroupId,
                     dcgm_field_eid_t entityId,
                     dcgmFieldValue_v1 *values,
                     int numValues,
                     void *userData)
{
    monitor_ctx_t *ctx = (monitor_ctx_t *)userData;
    printf("entity id : %d\n",entityId);
    printf("entity group id : %d\n",entityGroupId);
    ctx->entityId[entityId]=entityId;
    for (int i = 0; i < numValues; i++) {

        switch (values[i].fieldId) {
            case DCGM_FI_DEV_POWER_USAGE: //  mW->W
                printf("DCGM_FI_DEV_POWER_USAGE : %d\n",values[i].fieldType);
                ctx->power[entityId]=(double)values[i].value.dbl;
                printf("%.2f,",ctx->power[entityId]);
                break;
            case DCGM_FI_DEV_GPU_TEMP:
                printf("DCGM_FI_DEV_GPU_TEMP : %d\n",values[i].fieldType);
                ctx->gpu_temp[entityId]=(long long)values[i].value.i64;
                printf("%lld,",ctx->gpu_temp[entityId]);
                break;
            case DCGM_FI_DEV_FB_USED: // MB
                printf("DCGM_FI_DEV_FB_USED : %d\n",values[i].fieldType);
                ctx->gpu_frame_buffer_used[entityId]=(long long)values[i].value.i64;
                printf("%lld,",ctx->gpu_frame_buffer_used[entityId]);
                break;
            case DCGM_FI_DEV_SM_CLOCK: // MHz
                printf("DCGM_FI_DEV_SM_CLOCK : %d\n",values[i].fieldType);
                ctx->sm_clock_freq[entityId]=(long long)values[i].value.i64;
                printf("%lld,",ctx->sm_clock_freq[entityId]);
                break;
            case DCGM_FI_DEV_PCIE_REPLAY_COUNTER:
                printf("DCGM_FI_DEV_PCIE_REPLAY_COUNTER : %d\n",values[i].fieldType);
                ctx->pcie_replay_cnt[entityId]=(long long)values[i].value.i64;
                printf("%lld,",ctx->pcie_replay_cnt[entityId]);
                break;
        }
        printf("\n");
    }
    return 0;
}

void start_monitoring(const char *csv_path){
    short active_gpu_count=0;
	signal(SIGTERM,handle_sigterm);
	dcgmReturn_t result;
    dcgmHandle_t handle;
    dcgmGpuGrp_t group_id;
    dcgmFieldGrp_t field_group_id;

    printf("[MONITOR] DCGM Init\n");
	
	result=dcgmInit();
	if(result!=DCGM_ST_OK){
		printf("dcgmInit failed\n");
		return;
	}

	result=dcgmConnect("127.0.0.1",&handle);
	if(result!=DCGM_ST_OK){
		printf("dcgmInit failed\n");
		return;
	}

	// bring active GPU
	unsigned int gpu_ids[MAX_GPUS];
	int gpu_count = MAX_GPUS;

	if(dcgmGetAllSupportedDevices(handle,gpu_ids,&gpu_count)!=DCGM_ST_OK){
		printf("Failed to get GPUs\n");
        return;	
	}

	result=dcgmGroupCreate(handle,DCGM_GROUP_EMPTY,"active_gpus",&group_id);
    if(result!=DCGM_ST_OK){
        printf("GroupCreate failed: \n");
        printf("Failed to create Group\n");
        return;
    }
	 for (int i = 0; i < gpu_count; i++) {
        dcgmGroupAddDevice(handle, group_id, gpu_ids[i]);
        printf("Add GPU %d \n", gpu_ids[i]);
        active_gpu_count++;
    }

	//Field 정의
	unsigned short field_ids[] = {
    // 🔥 Power (target or label)
        DCGM_FI_DEV_POWER_USAGE,        // GPU 전력 사용량 (mW 단위)
        DCGM_FI_DEV_GPU_TEMP,
        DCGM_FI_DEV_FB_USED,
        DCGM_FI_DEV_SM_CLOCK,
        DCGM_FI_DEV_PCIE_REPLAY_COUNTER
    };
    
	int field_count = sizeof(field_ids)/sizeof(field_ids[0]);
	result=dcgmFieldGroupCreate(handle,field_count,field_ids,"directly observed fields",&field_group_id);
    if(result!=DCGM_ST_OK){
        printf("dcgm field group create failed!\n");
        return;
    }
	result=dcgmWatchFields(handle,group_id,field_group_id,1000,3600.0,0);
    if(result!=DCGM_ST_OK){
        printf("dcgm watch fields failed!\n");
        return;
    }

	// warm-up
	sleep(10);

	//Entity 준비
	dcgmGroupInfo_t group_info;
	memset(&group_info,0,sizeof(group_info));
    group_info.version = dcgmGroupInfo_version;

	result=dcgmGroupGetInfo(handle,group_id,&group_info);
	if(result!=DCGM_ST_OK){
        printf("get dcgm group info failed!\n");
        return;
    }

    // for(int i=0;i<gpu_count;i++){
    //     dcgmStartEmbeddedNvLinkProfiling(handle,gpu_ids[i],field_group_id);
    // }
    sleep(1);
	
    FILE *fp = initialize_file(csv_path);
    if(!fp){
        return;
    }
    monitor_ctx_t ctx;
    ctx.fp = fp;

    while (keepRunning==1) {

        dcgmReturn_t st = dcgmGetLatestValues_v2(handle,
                                                group_id,
                                                field_group_id,
                                                collect_callback,
                                                &ctx);
        
        if (st != DCGM_ST_OK) {
            printf("Failed: \n");
            break;
        }
        for(short i=0;i<active_gpu_count;i++){
            fprintf(ctx.fp,"%d,%.2f,%lld,%lld,%lld,%lld\n",ctx.entityId[i],ctx.power[i],ctx.gpu_temp[i],ctx.gpu_frame_buffer_used[i],ctx.sm_clock_freq[i],ctx.pcie_replay_cnt[i]);
        }
        fflush(fp);
        sleep(1);
    }
    disconnect_fn(&handle,&field_group_id,&group_id,&fp);
	return;
}

