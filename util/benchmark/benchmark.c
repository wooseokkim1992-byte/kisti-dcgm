#include "benchmark.h"
#include <unistd.h>
#include <stdio.h>

int run_benchmark(const char *exe_dir,const char *exec_path,char *const argv[]){
	printf("Start Rodinia benchmark\n");
	printf("dir : %s\n",exe_dir);
	printf("path : %s\n",exec_path);
	if(chdir(exe_dir)){
		printf("chdir failed\n");
		perror("chdir failed\n");
		return -1;
	}
	int status = execv(exec_path,argv);
	if(status==-1){
		printf("execv failed\n");
		perror("execv failed");
		return -1;
	}
	return 0;
}
