#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    mknod("/tmp/testpipeto",'p',0);
    char *test;
    test = malloc(5*sizeof(char));
    int fd_out = open("/tmp/testpipeto",O_WRONLY);
    int fd_int = open("/tmp/testpipeout",O_RDONLY);
    while (1) {
        write(fd_out,"TEST\n",5);
        read(fd_in,test,5);
        printf("%s",test);
    }
    free(test);
    return 0;
}