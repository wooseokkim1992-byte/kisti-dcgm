#ifndef ACITON_H
#define ACTION_H

#ifdef __cplusplus
extern "C"{
#endif
    void exec_benchmark(const char *exe_dir,const char *exe_path,char *args[]);
    void do_monitor(const char *csv_path,const unsigned short mode);
#ifdef __cplusplus
}
#endif

#endif