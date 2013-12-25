#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libgen.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

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
    char buf[1024];

    sprintf(buf, "%s/%s", base, next);
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
    char *buf;
    int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];
    int stdin_fd, stdout_fd, stderr_fd;
    char *prog, *wdir, *stdinlog, *stdoutlog,  *stderrlog;
    char *binpathenv, *pathenv;

    binpathenv = getenv("BIN_PATH");
    if (binpathenv) {
        pathenv = getenv("PATH");
        if (pathenv) {
            buf = calloc(strlen(binpathenv) + strlen(pathenv) + 2, 1);
            sprintf(buf, "%s:%s", binpathenv, pathenv);
            setenv("PATH", buf, 1);
        }
    }

    prog = basename(argv[0]);
    wdir = workdir(prog);
    if (mkdir(wdir, 0755) < 0) {
        perror("Failed creating directory");
        exit(1);
    }

    stdinlog = join(wdir, "stdin.data");
    stdoutlog = join(wdir, "stdout.data");
    stderrlog = join(wdir, "stderr.data");

    pipe(pipe_stdin);
    pipe(pipe_stdout);
    pipe(pipe_stderr);

    stdin_fd = open(stdinlog, O_WRONLY | O_CREAT, 0644);
    if (stdin_fd < 0) {
        perror("Stdin cap file");
        exit(1);
    }

    stdout_fd = open(stdoutlog, O_WRONLY | O_CREAT, 0644);
    if (stdout_fd < 0) {
        perror("Stdout cap file");
        exit(1);
    }

    stderr_fd = open(stderrlog, O_WRONLY | O_CREAT, 0644);
    if (stderr_fd < 0) {
        perror("Stderr cap file");
        exit(1);
    }

    pid = fork();

    if (pid == 0) {
        close(0);
        close(1);
        close(2);

        dup(pipe_stdin[0]);
        dup(pipe_stdout[1]);
        dup(pipe_stderr[1]);
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);

        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        if (execvp(prog, argv) < 0) {
            perror("Execution failed");
        }
    } else {
        close(pipe_stdin[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        set_nonblock(0);
        set_nonblock(pipe_stdout[0]);
        set_nonblock(pipe_stderr[0]);
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

            ret = read(pipe_stderr[0], &out, 1);
            if (ret == 1) {
                write(2, &out, 1);
                write(stderr_fd, &out, 1);
            }

            if (!ret) {
                break;
            }
        }
    }

    waitpid(-1, &ret, 0);
    close(stdin_fd);
    close(stdout_fd);

    exit(WEXITSTATUS(ret));
}
