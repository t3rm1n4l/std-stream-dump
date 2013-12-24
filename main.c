#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libgen.h>

#define CAP_IN "/tmp/input.data"
#define CAP_OUT "/tmp/output.data"

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    int in, out;
    pid_t pid;
    int ret;
    int pipe_stdin[2], pipe_stdout[2];
    int stdin_fd, stdout_fd;

    pipe(pipe_stdin);
    pipe(pipe_stdout);

    stdin_fd = open(CAP_IN, O_WRONLY | O_CREAT, 0644);
    if (stdin_fd < 0) {
        perror("Input cap file");
    }

    stdout_fd = open(CAP_OUT, O_WRONLY | O_CREAT, 0644);
    if (stdout_fd < 0) {
        perror("Output cap file");
    }

    pid = fork();

    if (pid == 0) {
        close(0);
        close(1);
        dup(pipe_stdin[0]);
        dup(pipe_stdout[1]);
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);

        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        if (execvp(basename(argv[0]), argv) < 0) {
            perror("Execution failed");
        }
    } else {
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        set_nonblock(0);
        set_nonblock(pipe_stdout[0]);
        while (1) {
            ret = read(0, &in, 1);
            if (ret == 1) {
                write(pipe_stdin[1], &in, 1);
                write(stdin_fd, &in, 1);
            }

            ret = read(pipe_stdout[0], &out, 1);
            if (ret == 1) {
                write(1, &out, 1);
                write(stdout_fd, &out, 1);
            }

            if (!ret) {
                break;
            }
        }
    }

    close(stdin_fd);
    close(stdout_fd);
}
