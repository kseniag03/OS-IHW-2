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

    // define tasks count from command line arguments

    int task_count = DEFAULT_TASK_COUNT < MAX_TASK_COUNT ? DEFAULT_TASK_COUNT : MAX_TASK_COUNT;
    if (argc > 1) {
        int new_count = atoi(argv[1]);
        if (new_count > MAX_TASK_COUNT) {
            printf("Введённый размер превышает максимальный размер массива. Размер не поменяется\n");
        } else if (new_count == 1) {
            printf("Введённый размер противоречит условию обмена задачами. Размер не поменяется\n");
        } else {
            if (new_count <= 0) {
                printf("ура, быстрое завершение\n");
            } 
            task_count = new_count;
            printf("Задано число задач: %d\n", task_count);
        }
    }
    
    printf("Parent process with pid = %d\n starts working", getpid());

    // define system v shared memory segment


    shmid = shmget(SHM_ID, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(-1);
    }
    printf("Shared memory segment ID = %d\n", shmid);

    buffer = shmat(shmid, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat");
        exit(-1);
    }
    printf("Shared memory attached at address %p\n", buffer);

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    printf("buffer got access, it has %d tasks\n", task_count);
    
    init(buffer);
    
    // init semaphore
    
    key_t key = ftok(".", 's');
    
    if ((mutex = semget(SHM_ID, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        perror("semget: Can not create mutex semaphore");
        exit(-1);
    }
    if ((mutex_sender = semget(SHM_ID + 1, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        perror("semget: Can not create mutex-s semaphore");
        exit(-1);
    }
    if ((mutex_receiver = semget(SHM_ID + 2, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        perror("semget: Can not create mutex-r semaphore");
        exit(-1);
    }
    semctl(mutex, 0, SETVAL, 1);
    semctl(mutex_sender, 0, SETVAL, 1);
    semctl(mutex_receiver, 0, SETVAL, 1);

    execute(buffer, task_count, 0);

    unlink_all();

    printf("Process 1 went to finish\n");

    exit(0);
}
