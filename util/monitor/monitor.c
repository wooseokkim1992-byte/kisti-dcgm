
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "monitor.h"
#include <dcgm_structs.h>
#include <dcgm_agent.h>
#include <dcgm_fields.h>
#include "parse_log_file.h"

#define NUM_FIELDS 7
#define MAX_GPUS 16
#define MAX_FIELDS 16

static volatile sig_atomic_t keepRunning = 1;

typedef struct {
    FILE *fp;

    dcgm_field_eid_t entityId[MAX_FIELDS];
    long timestamp[MAX_FIELDS];

    // =========================
    // Power
    // =========================
    double power[MAX_FIELDS];
    double gpu_power_limit[MAX_FIELDS];
    long long total_energy[MAX_FIELDS];

    // =========================
    // Thermal
    // =========================
    long long gpu_temp[MAX_FIELDS];
    long long memory_temp[MAX_FIELDS];
    long long slowdown_temp[MAX_FIELDS];
    long long shutdown_temp[MAX_FIELDS];

    // =========================
    // Clock
    // =========================
    long long sm_clock[MAX_FIELDS];
    long long mem_clock[MAX_FIELDS];
    long long app_sm_clock[MAX_FIELDS];
    long long app_mem_clock[MAX_FIELDS];
    long long max_sm_clock[MAX_FIELDS];
    long long max_mem_clock[MAX_FIELDS];

    // =========================
    // Utilization
    // =========================
    long long gpu_util[MAX_FIELDS];
    long long mem_copy_util[MAX_FIELDS];
    long long enc_util[MAX_FIELDS];
    long long dec_util[MAX_FIELDS];

    // =========================
    // Memory
    // =========================
    long long fb_used[MAX_FIELDS];
    long long fb_free[MAX_FIELDS];
    long long fb_total[MAX_FIELDS];

    // =========================
    // PCIe
    // =========================
    long long pcie_tx[MAX_FIELDS];
    long long pcie_rx[MAX_FIELDS];
    long long pcie_replay[MAX_FIELDS];

    // =========================
    // Reliability
    // =========================
    long long ecc_sbe[MAX_FIELDS];
    long long ecc_dbe[MAX_FIELDS];

    // =========================
    // Violations
    // =========================
    long long power_violation[MAX_FIELDS];
    long long thermal_violation[MAX_FIELDS];
    long long sync_boost_violation[MAX_FIELDS];
    long long board_limit_violation[MAX_FIELDS];
    long long low_util_violation[MAX_FIELDS];
    long long reliability_violation[MAX_FIELDS];

    // =========================
    // Profiling
    // =========================
    double sm_active[MAX_FIELDS];
    double sm_occupancy[MAX_FIELDS];
    double dram_active[MAX_FIELDS];
    double fp32_active[MAX_FIELDS];
    double fp64_active[MAX_FIELDS];
    double tensor_active[MAX_FIELDS];
    double gr_engine_active[MAX_FIELDS];

} monitor_ctx_t;

void stop_monitoring(void){
	keepRunning = 0;
}

static void handle_sigterm(int sig){
	(void)sig;
	keepRunning = 0; 
}

void disconnect_fn(dcgmHandle_t *handle,dcgmFieldGrp_t *field_group_id,dcgmGpuGrp_t *group_id,monitor_ctx_t **c){
    if(c&&*c!=NULL){
        if((*c)->fp){
            fflush((*c)->fp);
            fclose((*c)->fp);
        }
        free(*c);
        *c=NULL;
    }
    if(handle){
        dcgmFieldGroupDestroy(*handle, *field_group_id);
        dcgmGroupDestroy(*handle, *group_id);
        dcgmDisconnect(*handle);
        dcgmShutdown();
    }
}

unsigned short check_dcgm_status(dcgmFieldValue_v1 values){
    if(values.status==DCGM_ST_NO_DATA||values.status!=DCGM_ST_OK)return 1;
    return 0;
}

FILE* initialize_file(const char *csv_path){
    FILE *fp = fopen(csv_path,"w");
    if(!fp){
        perror("failed open file");
        return NULL;
    }
    const char *field_names[] = {
        // Power
        "Power(W)",
        "Power_Limit(W)",
        "Accumulated_Total_Energy(J)",

        // Thermal
        "GPU_Temp(C)",
        "Memory_Temp(C)",
        "Slowdown_Temp(C)",
        "Shutdown_Temp(C)",

        // Clock
        "SM_Clock(MHz)",
        "Mem_Clock(MHz)",
        "App_SM_Clock(MHz)",
        "App_Mem_Clock(MHz)",
        "Max_SM_Clock(MHz)",
        "Max_Mem_Clock(MHz)",

        // Utilization
        "GPU_Util(%)",
        "Mem_Copy_Util(%)",
        "ENC_Util(%)",
        "DEC_Util(%)",

        // Memory
        "FB_Used(MB)",
        "FB_Free(MB)",
        "FB_Total(MB)",

        // PCIe
        "PCIe_TX(KB/s)",
        "PCIe_RX(KB/s)",
        "PCIe_Replay_Count",

        // ECC
        "ECC_Single_Bit_Total",
        "ECC_Double_Bit_Total",

        // Violations
        "Power_Violation",
        "Thermal_Violation",
        "Sync_Boost_Violation",
        "Board_Limit_Violation",
        "Low_Util_Violation",
        "Reliability_Violation",

        // Profiling
        "SM_Active(%)",
        "SM_Occupancy(%)",
        "DRAM_Active(%)",
        "FP32_Active(%)",
        "FP64_Active(%)",
        "Tensor_Active(%)",
        "GR_Engine_Active(%)"
    };
    fprintf(fp,"GPU ID,");
    fprintf(fp,"Time Stamp(micro second),");
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
    ctx->entityId[entityId]=entityId;
    for (int i = 0; i < numValues; i++) {
        ctx->timestamp[entityId]=(long)values[i].ts;
        if(values[i].status!=DCGM_ST_OK){
            continue;
        }
        switch (values[i].fieldId) {
            // =========================
            // Power
            // =========================
            case DCGM_FI_DEV_POWER_USAGE:
                ctx->power[entityId] = values[i].value.dbl;
                break;

            case DCGM_FI_DEV_POWER_MGMT_LIMIT:
                ctx->gpu_power_limit[entityId] = values[i].value.dbl;
                break;

            case DCGM_FI_DEV_TOTAL_ENERGY_CONSUMPTION:
                ctx->total_energy[entityId] = (long long)values[i].value.i64/1000.0;
                break;
            // =========================
            // Thermal
            // =========================
            case DCGM_FI_DEV_GPU_TEMP:
                ctx->gpu_temp[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_MEMORY_TEMP:
                ctx->memory_temp[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_SLOWDOWN_TEMP:
                ctx->slowdown_temp[entityId]=values[i].value.i64;
                break;
            case DCGM_FI_DEV_SHUTDOWN_TEMP:
                ctx->shutdown_temp[entityId]=values[i].value.i64;
                break;
            // =========================
            // Clock
            // =========================
            case DCGM_FI_DEV_SM_CLOCK:
                ctx->sm_clock[entityId] = values[i].value.i64;
                break;

            case DCGM_FI_DEV_MEM_CLOCK:
                ctx->mem_clock[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_APP_SM_CLOCK:
                ctx->app_sm_clock[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_APP_MEM_CLOCK:
                ctx->app_mem_clock[entityId] = (long long)values[i].value.i64;
                break;
            case DCGM_FI_DEV_MAX_SM_CLOCK:
                ctx->max_sm_clock[entityId] = (long long)values[i].value.i64;
                break;
            case DCGM_FI_DEV_MAX_MEM_CLOCK:
                ctx->max_mem_clock[entityId] = values[i].value.i64;
                break;
            // =========================
            // Utilization
            // =========================
            case DCGM_FI_DEV_GPU_UTIL:
                ctx->gpu_util[entityId] = values[i].value.i64;
                break;

            case DCGM_FI_DEV_MEM_COPY_UTIL:
                ctx->mem_copy_util[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_ENC_UTIL:
                ctx->enc_util[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_DEC_UTIL:
                ctx->dec_util[entityId] = values[i].value.i64;
                break;
            // =========================
            // Memory
            // =========================
            case DCGM_FI_DEV_FB_USED:
                ctx->fb_used[entityId] = values[i].value.i64;
                break;

            case DCGM_FI_DEV_FB_FREE:
                ctx->fb_free[entityId] = values[i].value.i64;
                break;

            case DCGM_FI_DEV_FB_TOTAL:
                ctx->fb_total[entityId] = values[i].value.i64;
                break;

            // =========================
            // PCIe
            // =========================
            case DCGM_FI_DEV_PCIE_TX_THROUGHPUT:
                if(values[i].fieldType==105&&values[i].value.i64<1e12&&values[i].value.i64>=0){
                    ctx->pcie_tx[entityId] = values[i].value.i64;
                }else{
                    ctx->pcie_tx[entityId]=0;
                }
                break;

            case DCGM_FI_DEV_PCIE_RX_THROUGHPUT:
                if(values[i].fieldType==105&&values[i].value.i64<1e12&&values[i].value.i64>=0){
                    ctx->pcie_tx[entityId] = values[i].value.i64;
                }else{
                    ctx->pcie_tx[entityId]=0;
                }
                break;

            case DCGM_FI_DEV_PCIE_REPLAY_COUNTER:
                if(values[i].fieldType==105&&values[i].value.i64<1e12&&values[i].value.i64>=0){
                    ctx->pcie_tx[entityId] = values[i].value.i64;
                }else{
                    ctx->pcie_tx[entityId]=0;
                }
                break;

            // =========================
            // ECC
            // =========================
            case DCGM_FI_DEV_ECC_SBE_VOL_TOTAL:
                ctx->ecc_sbe[entityId] = values[i].value.i64;
                break;

            case DCGM_FI_DEV_ECC_DBE_VOL_TOTAL:
                ctx->ecc_dbe[entityId] = values[i].value.i64;
                break;

            // =========================
            // Violations
            // =========================
            case DCGM_FI_DEV_POWER_VIOLATION:
                ctx->power_violation[entityId] = values[i].value.i64;
                break;

            case DCGM_FI_DEV_THERMAL_VIOLATION:
                ctx->thermal_violation[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_SYNC_BOOST_VIOLATION:
                ctx->sync_boost_violation[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_BOARD_LIMIT_VIOLATION:
                ctx->board_limit_violation[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_LOW_UTIL_VIOLATION:
                ctx->low_util_violation[entityId] = values[i].value.i64;
                break;
            case DCGM_FI_DEV_RELIABILITY_VIOLATION:
                ctx->reliability_violation[entityId] = values[i].value.i64;
                break;
            // =========================
            // Profiling
            // =========================
            case DCGM_FI_PROF_SM_ACTIVE:
                if(check_dcgm_status(values[i])==0){
                    ctx->sm_active[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->sm_active[entityId] = 0.0;
                }
                break;
            case DCGM_FI_PROF_SM_OCCUPANCY:
                if(check_dcgm_status(values[i])==0){
                    ctx->sm_occupancy[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->sm_occupancy[entityId] = 0.0;
                }

                break;
            case DCGM_FI_PROF_DRAM_ACTIVE:
                if(check_dcgm_status(values[i])==0){
                    ctx->dram_active[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->sm_occupancy[entityId] = 0.0;
                }
                break;
            case DCGM_FI_PROF_PIPE_FP32_ACTIVE:
                if(check_dcgm_status(values[i])==0){
                    ctx->fp32_active[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->fp32_active[entityId] = 0.0;
                }
                break;
            case DCGM_FI_PROF_PIPE_FP64_ACTIVE:
                if(check_dcgm_status(values[i])==0){
                    ctx->fp64_active[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->fp64_active[entityId] = 0.0;
                }
                break;

            case DCGM_FI_PROF_PIPE_TENSOR_ACTIVE:
                if(check_dcgm_status(values[i])==0){
                    ctx->tensor_active[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->tensor_active[entityId] = 0.0;
                }
                break;

            case DCGM_FI_PROF_GR_ENGINE_ACTIVE:
                if(check_dcgm_status(values[i])==0){
                    ctx->gr_engine_active[entityId] = values[i].value.dbl*100;
                }else{
                    ctx->gr_engine_active[entityId] = 0.0;
                }
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
        // =========================
        // Power / Energy
        // =========================
        DCGM_FI_DEV_POWER_USAGE,
        DCGM_FI_DEV_POWER_MGMT_LIMIT,
        DCGM_FI_DEV_TOTAL_ENERGY_CONSUMPTION,

        // =========================
        // Thermal (Power 영향)
        // =========================
        DCGM_FI_DEV_GPU_TEMP,
        DCGM_FI_DEV_MEMORY_TEMP,
        DCGM_FI_DEV_SLOWDOWN_TEMP,
        DCGM_FI_DEV_SHUTDOWN_TEMP,

        // =========================
        // Clock (Power scaling 핵심)
        // =========================
        DCGM_FI_DEV_SM_CLOCK,
        DCGM_FI_DEV_MEM_CLOCK,
        DCGM_FI_DEV_APP_SM_CLOCK,
        DCGM_FI_DEV_APP_MEM_CLOCK,
        DCGM_FI_DEV_MAX_SM_CLOCK,
        DCGM_FI_DEV_MAX_MEM_CLOCK,

        // =========================
        // Utilization (Power 원인)
        // =========================
        DCGM_FI_DEV_GPU_UTIL,
        DCGM_FI_DEV_MEM_COPY_UTIL,
        DCGM_FI_DEV_ENC_UTIL,
        DCGM_FI_DEV_DEC_UTIL,

        // =========================
        // Memory (Power contributor)
        // =========================
        DCGM_FI_DEV_FB_USED,
        DCGM_FI_DEV_FB_FREE,
        DCGM_FI_DEV_FB_TOTAL,

        // =========================
        // PCIe / IO (Power 영향)
        // =========================
        DCGM_FI_DEV_PCIE_TX_THROUGHPUT,
        DCGM_FI_DEV_PCIE_RX_THROUGHPUT,
        DCGM_FI_DEV_PCIE_REPLAY_COUNTER,

        // =========================
        // Reliability (간접 영향)
        // =========================
        DCGM_FI_DEV_ECC_SBE_VOL_TOTAL,
        DCGM_FI_DEV_ECC_DBE_VOL_TOTAL,

        // =========================
        // Throttle / Violation (핵심)
        // =========================
        DCGM_FI_DEV_POWER_VIOLATION,
        DCGM_FI_DEV_THERMAL_VIOLATION,
        DCGM_FI_DEV_SYNC_BOOST_VIOLATION,
        DCGM_FI_DEV_BOARD_LIMIT_VIOLATION,
        DCGM_FI_DEV_LOW_UTIL_VIOLATION,
        DCGM_FI_DEV_RELIABILITY_VIOLATION,

        // =========================
        // Profiling (Power 원인 분석)
        // =========================
        DCGM_FI_PROF_SM_ACTIVE,
        DCGM_FI_PROF_SM_OCCUPANCY,
        DCGM_FI_PROF_DRAM_ACTIVE,
        DCGM_FI_PROF_PIPE_FP32_ACTIVE,
        DCGM_FI_PROF_PIPE_FP64_ACTIVE,
        DCGM_FI_PROF_PIPE_TENSOR_ACTIVE,
        DCGM_FI_PROF_GR_ENGINE_ACTIVE
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
	sleep(1);

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
        printf("failed to initialize csv file");
        return;
    }
    monitor_ctx_t *ctx=(monitor_ctx_t*)calloc(1,sizeof(monitor_ctx_t));
    ctx->fp = fp;
    long start_ts=get_time_us();
    while (keepRunning==1) {
        dcgmReturn_t st = dcgmGetLatestValues_v2(handle,
                                                group_id,
                                                field_group_id,
                                                collect_callback,
                                                ctx);
        
        if (st != DCGM_ST_OK) {
            printf("Failed: \n");
            break;
        }
        for(short i=0;i<active_gpu_count;i++){
            fprintf(ctx->fp,
                "%u,%ld,"            // entityId, timestamp

                // Power
                "%.2f,%.2f,%lld,"

                // Thermal
                "%lld,%lld,%lld,%lld,"

                // Clock
                "%lld,%lld,%lld,%lld,%lld,%lld,"

                // Utilization
                "%lld,%lld,%lld,%lld,"

                // Memory
                "%lld,%lld,%lld,"

                // PCIe
                "%lld,%lld,%lld,"

                // Reliability
                "%lld,%lld,"

                // Violations
                "%lld,%lld,%lld,%lld,%lld,%lld,"

                // Profiling
                "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",

                ctx->entityId[i],
                ctx->timestamp[i],

                // Power
                ctx->power[i],
                ctx->gpu_power_limit[i],
                ctx->total_energy[i],

                // Thermal
                ctx->gpu_temp[i],
                ctx->memory_temp[i],
                ctx->slowdown_temp[i],
                ctx->shutdown_temp[i],

                // Clock
                ctx->sm_clock[i],
                ctx->mem_clock[i],
                ctx->app_sm_clock[i],
                ctx->app_mem_clock[i],
                ctx->max_sm_clock[i],
                ctx->max_mem_clock[i],
                // Utilization
                ctx->gpu_util[i],
                ctx->mem_copy_util[i],
                ctx->enc_util[i],
                ctx->dec_util[i],

                // Memory
                ctx->fb_used[i],
                ctx->fb_free[i],
                ctx->fb_total[i],

                // PCIe
                ctx->pcie_tx[i],
                ctx->pcie_rx[i],
                ctx->pcie_replay[i],

                // Reliability
                ctx->ecc_sbe[i],
                ctx->ecc_dbe[i],

                // Violations
                ctx->power_violation[i],
                ctx->thermal_violation[i],
                ctx->sync_boost_violation[i],
                ctx->board_limit_violation[i],
                ctx->low_util_violation[i],
                ctx->reliability_violation[i],

                // Profiling
                ctx->sm_active[i],
                ctx->sm_occupancy[i],
                ctx->dram_active[i],
                ctx->fp32_active[i],
                ctx->fp64_active[i],
                ctx->tensor_active[i],
                ctx->gr_engine_active[i]
            );
        }
        fflush(ctx->fp);
        sleep(1);
    }
    long end_ts = get_time_us();
    printf("start timestamp (micro seconds): %ld\n",start_ts);
    printf("end timestamp (micro seconds): %ld\n",end_ts);
    disconnect_fn(&handle,&field_group_id,&group_id,&ctx);
	return;
}
