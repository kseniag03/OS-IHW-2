#include "common.h"
#include <unistd.h>

void handler(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    if (sig == SIGINT) {
        //kill(buffer->sender_pid, SIGTERM);
    }
    printf("KILL\n");
    unlink_all();
    exit (10);
}

/////////////////// MAIN ///////////////////

int main(int argc, char *argv[]) {
    
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    printf("Process with pid = %d\n starts working", getpid());
    
    execute(buffer, buffer->task_count, 1);

    // close_common_semaphores();
    if (sem_close(mutex) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };
    if (sem_close(mutex_sender) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };
    if (sem_close(mutex_receiver) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };

    unlink_all();

    printf("Process 2 went to finish\n");

    exit(0);
}