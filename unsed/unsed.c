#include "readlines.h"
#include <unistd.h>
#include <stdio.h>
#include <pcre.h>
#include <string.h>

#define MAX_LEN 256
#define BUFFER_LEN 4*MAX_LEN

char* parse_pattern(char* str, char* pattern, char* repl, int greedy){
	pcre* re;
	int options = PCRE_UTF8;
	const char* error;
	int erroffset;
	re = pcre_compile(pattern, options, &error, &erroffset, NULL);
	if (!re){
		printf("RE compilation failed");
		return NULL;
	}
	int count = 0;
	int ovector[30];

    int offset = 0;
    char* buf;
    int strpos = 0;
    int bufpos = 0;
    buf = malloc(BUFFER_LEN);
    memset(buf, 0, BUFFER_LEN);
	//courtesy of dedoz
	while((offset < strlen(str)) && (count = pcre_exec(re,NULL, str, strlen(str), offset, 0, ovector, 30) >= 0)){
        int i;
        strncat(&buf[bufpos], &str[strpos], ovector[0] - strpos);
        bufpos += ovector[0] - strpos;
        strpos = ovector[1];
        strncat(&buf[bufpos], repl, strlen(repl));
        bufpos += strlen(repl);
        offset = ovector[1];
        if(!greedy)
        {
			//free(str);
			//str = buf;
			//write(1, str, strlen(str));
            return buf;
		}
    }
    strncat(&buf[bufpos], &str[strpos], strlen(str) - strpos+1);
			//free(str);
			//str = buf;
			
    return buf;
}

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("No pattern\n");
		return -1;
	}
	struct RL* rl = rl_open(0,MAX_LEN);
	int i;
	char** patterns = malloc((argc-1)*sizeof(char*));
	char** repls = malloc((argc-1)*sizeof(char*));
	int* greeds = malloc((argc-1)*sizeof(int));
	char* str;
	for (i = 1; i < argc; i++){
		//get pattern
		strtok(argv[i], "/"); 
		char* pattern_tmp = strtok(NULL, "/");//skip s/
		patterns[i-1] = malloc(strlen(pattern_tmp)+1);
		strcpy(patterns[i-1], pattern_tmp);
		pattern_tmp = strtok(NULL, "/");
		repls[i-1] = malloc(strlen(pattern_tmp)+1);
		strcpy(repls[i-1], pattern_tmp);
		greeds[i-1] = (strtok(NULL, "/") != NULL);
		//printf("%s\n%s\n%d\n", patterns[i-1], repls[i-1], greeds[i-1]);
	}
	//test
	int res = rl_readline(rl, rl->buf, rl->buf_size);
	while (res!=0){
		if (res > 0){
			//do shit and write
			//printf("%s",rl->buf);
			str = malloc((res+1)*sizeof(char));
			strcpy(str,rl->buf);
			//printf("%s+%s\n", str,repls[i]);
			for (i = 0; i < argc-1; i++){
				//if (parse_pattern(str,patterns[i],repls[i],greeds[i]) == NULL){
				//	printf("NO FUCK YOU");
				//}
				str = parse_pattern(str,patterns[i],repls[i],greeds[i]);
			}
			write(1,str,strlen(str));
			write(1,"\n",1);
			free(str);
		} else {
			break;
		}
		res = rl_readline(rl, rl->buf,rl->buf_size);
	} 
	
	for (i = 0; i < argc -1; i++){
		free(patterns[i]);
		free(repls[i]);
	}
	free(patterns);
	free(repls);
	free(greeds);
	return 0;
}
