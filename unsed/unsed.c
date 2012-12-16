#include "readlines.h"
#include <unistd.h>
#include <stdio.h>
#include <pcre.h>
#include <string.h>

#define MAX_LEN 256

char* parse_pattern(char* str, char* pattern, int greedy){
	
	
	
	return str;
}

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("No pattern\n");
		return -1;
	}
	struct RL* rl = rl_open(1,MAX_LEN);
	int i;
	for (i = 1; i < argc; i++){
		//get pattern
		strtok(argv[i], "/"); 
		char* pattern_tmp = strtok(NULL, "/");//skip s/
		char* pattern = malloc(strlen(pattern_tmp)+1);
		strcpy(pattern, pattern_tmp);
		pattern_tmp = strtok(NULL, "/");
		char* repl = malloc(strlen(pattern_tmp)+1);
		strcpy(repl, pattern_tmp);
		int greedy = (strtok(NULL, "/") != NULL);
		printf("%s\n%s\n%d\n", pattern, repl, greedy);
	}
	//test
	
	return 0;
}
