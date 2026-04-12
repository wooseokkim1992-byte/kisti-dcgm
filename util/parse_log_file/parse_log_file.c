#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <yajl/yajl_parse.h>
#include "parse_log_file.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DCGM_LOG_FILE "/var/log/nvidia-dcgm"
#define RESULT_FILE_PATH "../result/log"
#define MAX_FILE_PATH 255
#define BUFFER_SIZE 4096


long get_time_us(){
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    return ts.tv_sec*1000000L + ts.tv_nsec/1000;
}

int ensure_directory(const char *path){
    struct stat st;
    if(stat(path,&st)==0){
        if(S_ISDIR(st.st_mode)){
            return 0;
        }else{
            return -1;
        }
    }
    if(mkdir(path,0755)==0){
        return 0;
    }
    return -1;
}

static int handle_map_key(void *ctx, const unsigned char *key, size_t len)
{
    parse_power_usage_ctx *c = (parse_power_usage_ctx *)ctx;

    char buf[64];
    memcpy(buf, key, len);
    buf[len] = '\0';
    if (strcmp(buf, "power_usage") == 0) {
        printf("key : %s\n",buf);
        c->in_power_usage = 1;
    }
    else if (c->in_power_usage && strcmp(buf, "timestamp") == 0) {
        c->expect_timestamp = 1;
    }
    else if (c->in_power_usage && strcmp(buf, "value") == 0) {
        c->expect_value = 1;
    }
    return 1;
}

/* =========================
   NUMBER HANDLER
========================= */
static int handle_number(void *ctx, const char *s, size_t l)
{
    parse_power_usage_ctx *c = (parse_power_usage_ctx *)ctx;

    if (!c->in_power_usage)
        return 1;

    char buf[64];
    memcpy(buf, s, l);
    buf[l] = '\0';
    printf("numb : %s\n",buf);

    if (c->expect_timestamp) {
        c->current_timestamp = atol(buf);
        c->expect_timestamp = 0;
    }
    else if (c->expect_value) {
        double value = atof(buf);
        c->expect_value = 0;
        // fprintf(c->dest,
        //             "timestamp:%ld value:%.3f\n",
        //             c->current_timestamp,
        //             value);
        printf("START=%ld END=%ld TS=%ld\n",
            c->start,
            c->end,
            c->current_timestamp);
        if (c->current_timestamp >= c->start &&
            c->current_timestamp <= c->end &&
            c->dest) {

            fprintf(c->dest,
                    "timestamp:%ld value:%.3f\n",
                    c->current_timestamp,
                    value);
        }
    }

    return 1;
}

static int start_map(void *ctx){
    return 1;
}

static int end_map(void *ctx){
    return 1;
}

/* =========================
   ARRAY HANDLER
========================= */
static int handle_start_array(void *ctx)
{
    return 1;
}

static int handle_end_array(void *ctx)
{
     parse_power_usage_ctx *c = (parse_power_usage_ctx *)ctx;
    c->in_power_usage = 0;
    return 1;
}

/* =========================
   CALLBACK TABLE
========================= */
static yajl_callbacks callbacks = {
    /* null value */
    NULL,

    /* boolean (true / false) */
    NULL,

    /* integer number (정수 전용 callback)
       - JSON에서 123 같은 정수 타입 처리 */
    NULL,

    /* double / floating point number
       - 3.14 같은 실수 타입 처리
       - (yajl 버전에 따라 integer 대신 여기로 들어올 수 있음) */
    NULL,

    /* number (통합 number handler)
       - int / double 모두 여기로 들어오는 경우 많음
       - 가장 많이 사용하는 핵심 callback */
    handle_number,//handle_number,

    /* string value
       - "hello" 같은 문자열 처리 */
    NULL,
    /* start map (object 시작)
       - { 시작 시 호출 */
    start_map,

    /* map key (JSON key)
       - "timestamp", "value" 같은 key 처리 */
    handle_map_key,//handle_map_key,

    /* end map (object 끝)
       - } 닫힐 때 호출 */
    end_map,

    /* start array
       - [ 시작 시 호출 */
    handle_start_array,

    /* end array
       - ] 닫힐 때 호출 */
    handle_end_array
};

/* =========================
   PARSER CORE
========================= */
int parse_json(parse_power_usage_ctx *ctx,
               time_range *t_range,
               const yajl_callbacks *cb,
               const char *dest_path,
               const char *src_path)
{
    if (!ctx || !t_range)
        return -1;

    ctx->dest = fopen(dest_path, "w");
    if (!ctx->dest) {
        perror("dest fopen");
        return -1;
    }

    ctx->start=t_range->start;
    ctx->end=t_range->end;
    FILE *src = fopen(src_path, "r");
    if (!src) {
        perror("src fopen");
        fclose(ctx->dest);
        return -1;
    }

    yajl_handle handle = yajl_alloc(cb, NULL, ctx);

    char buffer[BUFFER_SIZE];

    while (1) {
        size_t rd = fread(buffer, 1, BUFFER_SIZE, src);
        if (rd == 0)
            break;

        yajl_status stat = yajl_parse(handle,
                                      (unsigned char *)buffer,
                                      rd);

        if (stat != yajl_status_ok) {
            unsigned char *err =
                yajl_get_error(handle, 1,
                               (unsigned char *)buffer,
                               rd);

            fprintf(stderr, "YAJL error: %s\n", err);
            yajl_free_error(handle, err);
            break;
        }
    }

    yajl_complete_parse(handle);

    yajl_free(handle);

    fclose(src);
    fclose(ctx->dest);
    ctx->dest = NULL;
    return 0;
}

/* =========================
   ENTRY FUNCTION
========================= */
int parse_stats_targeted_power(time_range *t_range,
                               const char *log_filename)
{
    if (!t_range)
        return -1;

    char src_path[MAX_FILE_PATH];
    char dest_path[MAX_FILE_PATH];
    if(ensure_directory(DCGM_LOG_FILE)){
        perror("directory created failed!\n");
        return -1;
    }
    if(ensure_directory(RESULT_FILE_PATH)){
        perror("directory created failed!\n");
        return -1;
    }
    snprintf(src_path, MAX_FILE_PATH,
             "%s/stats_targeted_power.json",
             DCGM_LOG_FILE);

    snprintf(dest_path, MAX_FILE_PATH,
             "%s/%s",
             RESULT_FILE_PATH,
             log_filename);

    printf("start: %ld\n", t_range->start);
    printf("end:   %ld\n", t_range->end);

    parse_power_usage_ctx *ctx =
        calloc(1, sizeof(parse_power_usage_ctx));
    
    if (!ctx)
        return -1;

    int ret = parse_json(ctx,
                         t_range,
                         &callbacks,
                         dest_path,
                         src_path);

    free(ctx);
    return ret;
}