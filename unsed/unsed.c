#include "readlines.h"
#include <unistd.h>
#include <stdio.h>
#include <pcre.h>
#include <string.h>

#define MAX_LEN 256

char* parse_pattern(char* str, char* pattern, int greedy){
	pcre* re;
	int options = 0;
	const char* error;
	int erroffset;
	//re = pcre_compile(pattern, options, &error, &erroffset, NULL);
	if (!re){
		printf("RE compilation failed");
		return NULL;
	}
	
	
	return str;
}

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("No pattern\n");
		return -1;
	}
	struct RL* rl = rl_open(1,MAX_LEN);
	int i;
	char** patterns = malloc((argc-1)*sizeof(char*));
	char** repls = malloc((argc-1)*sizeof(char*));
	int* greeds = malloc((argc-1)*sizeof(int));
	for (i = 1; i < argc; i++){
		//get pattern
		strtok(argv[i], "/"); 
		char* pattern_tmp = strtok(NULL, "/");//skip s/
		patterns[i] = malloc(strlen(pattern_tmp)+1);
		strcpy(patterns[i], pattern_tmp);
		pattern_tmp = strtok(NULL, "/");
		repls[i] = malloc(strlen(pattern_tmp)+1);
		strcpy(repls[i], pattern_tmp);
		greeds[i] = (strtok(NULL, "/") != NULL);
		printf("%s\n%s\n%d\n", patterns[i], repls[i], greeds[i]);
	}
	//test
	
	
	for (i = 0; i < argc -1; i++){
		free(patterns[i]);
		free(repls[i]);
	}
	free(patterns);
	free(repls);
	free(greeds);
	return 0;
}
