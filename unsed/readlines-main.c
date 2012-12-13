#include "readlines.h"
 //trying to use named pipes for testing
int main(int argc, char *argv[]){
        int fd_out = open("testpipeout",O_WRONLY);
        if (argc < 2){
                printf("No max_size given");
                return 0;
        }
        int max_size = atoi(argv[1]);
        //struct RL* rl = rl_open(1, max_size);
        struct RL* rl = rl_open(open("testpipeto",O_RDONLY),max_size);
        int res = rl_readline(rl, rl->buf, rl->buf_size);
        while (res != 0){
                if (res > 0) {
                        write(fd_out,rl->buf,res);
                }
                res = rl_readline(rl,rl->buf, rl->buf_size);
        }
        close(fd_out);
        rl_close(rl);
        return 0;
}