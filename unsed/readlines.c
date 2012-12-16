#include "readlines.h"
 
#define BUFSIZE 256
 
struct RL* rl_open(int fd, size_t max_size){
        struct RL* rl = malloc(sizeof(struct RL));
        rl->fd = fd;
        rl->buf_size = BUFSIZE;
        rl->buf = malloc(rl->buf_size * sizeof(char));
        rl->max_size = max_size;
        return rl;
}
 
size_t rl_max_size(struct RL *rl) {
        return rl->max_size;
}
 
int rl_close(struct RL *rl){
        if (rl != NULL) {
                if (rl->buf != NULL) {
                        free(rl->buf);
                        rl->buf = NULL;
                }
                close(rl->fd);         
                free(rl);
                rl = NULL;
        }
        return 0;
}
 
int rl_readline(struct RL *rl, char * buf, size_t buf_size){
        memset(buf,0,buf_size);
        size_t max_size = rl_max_size(rl);
        size_t read_to_buf = 0;
        int res = read(rl->fd, buf, sizeof(char));
        if (res == 0){
                return 0;
        }
        read_to_buf += res;
        while (buf[read_to_buf-1] != '\n' && read_to_buf < buf_size && read_to_buf < max_size+1){
                res=read(rl->fd, &buf[read_to_buf], sizeof(char));
                if (res == 0) {
                        return 0;
                }
                read_to_buf += res;
        }
        if (buf[read_to_buf-1] == '\n'){
                return read_to_buf;
        }
        if (read_to_buf == max_size+1) {
                return -3;
        }
        if (read_to_buf == buf_size) {
                //skip this line
                res=read(rl->fd, buf, sizeof(char));
                while (buf[0] != '\n'){
                        res=read(rl->fd, buf, sizeof(char));
                        if (res == 0) {
                                return 0;
                        }
                }
                return -2;
        }
        return -4;
}
