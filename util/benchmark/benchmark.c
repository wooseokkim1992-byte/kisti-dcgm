#include <unistd.h>
#include <stdio.h>
#include "benchmark.h"
#include <regex.h>

int is_csv(const char *filename){
	regex_t regex;
	int ret;
	ret = regcomp(&regex,"*\\.csv$",REG_EXTENDED);
	if(ret) return 0;
	ret = regexec(&regex,filename,0,NULL,0);
	regfree(&regex);
	return ret == 0;
}


int run_benchmark(const char *exe_dir,const char *exec_path,char *const argv[]){
	printf("Start Rodinia benchmark\n");
	printf("dir : %s\n",exe_dir);
	printf("path : %s\n",exec_path);
	if(is_csv(exec_path)){
		fprintf(stderr,"exec path should end with .csv\n");
		return -1;
	}
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
