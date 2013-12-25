#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libgen.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#define BASE_DIR "/tmp"

char *workdir(const char *prog) {
    size_t len;
    char buf[1024];
    char *out;
    uint64_t rand = time(NULL);

    len = sprintf(buf, "%s/%s.%"PRIu64, BASE_DIR, prog, rand);
    buf[len] = '\0';
    return strdup(buf);
}

char *join(const char *base, const char *next) {
    size_t len;
    char buf[1024];

    len = sprintf(buf, "%s/%s", base, next);
    buf[len] = '\0';
    return strdup(buf);
}

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
    char *prog, *wdir, *stdinlog, *stdoutlog;

    prog = basename(argv[0]);
    wdir = workdir(prog);
    if (mkdir(wdir, 744) < 0) {
        perror("Failed creating directory");
        exit(1);
    }

    stdinlog = join(wdir, "stdin.data");
    stdoutlog = join(wdir, "stdout.data");

    pipe(pipe_stdin);
    pipe(pipe_stdout);

    stdin_fd = open(stdinlog, O_WRONLY | O_CREAT, 0644);
    if (stdin_fd < 0) {
        perror("Input cap file");
        exit(1);
    }

    stdout_fd = open(stdoutlog, O_WRONLY | O_CREAT, 0644);
    if (stdout_fd < 0) {
        perror("Output cap file");
        exit(1);
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
        if (execvp(prog, argv) < 0) {
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
