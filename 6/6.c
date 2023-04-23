#include <ctype.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHM_ID  0x777     // ключ разделяемой памяти
#define SHM_SIZE sizeof(task_buffer)

#define PROGERS_COUNT 3
#define DEFAULT_TASK_COUNT 7
#define MAX_TASK_COUNT 10

/////////////////// INIT STRUCTURES AND TOOLS ///////////////////

typedef struct {
    int first;
    int second;
} pair;


typedef struct {
    int task[MAX_TASK_COUNT]; // array of tasks; value: -3 -- was not touch by anybody, -2 -- is executing, -1 -- is asking for check, 0 -- incorrect, 1 -- correct and finish
    int cur;                  // current index
    pair check_pull[MAX_TASK_COUNT]; // check pull (first - pid, second - index)
    int sender_pid;   // pid of process doing check ака проверяющий

} task_buffer;

// имя области разделяемой памяти
const char *shar_object = "/sysv-shr-object";
int shmid;
task_buffer *buffer;

// имя семафора (мьютекса) для чтения данных из буфера
const char *mutex_sem_name = "/mutex-semaphore";
int mutex;

// имя семафора (мьютекса) для идентификации проверяющего
const char *mutex_sem_sender_name = "/mutex-semaphore-sender";
int mutex_sender;

// имя семафора (мьютекса) для идентификации ждущего проверки
const char *mutex_sem_receiver_name = "/mutex-semaphore-receiver";
int mutex_receiver;

void init(task_buffer *buffer) {
    buffer->cur = 0;
    buffer->sender_pid = -1;
    for (int i = 0; i < MAX_TASK_COUNT; ++i) {
        buffer->task[i] = -3;
        buffer->check_pull[i].first = -1;
        buffer->check_pull[i].second = -1;
    }
}

// Функция, удаляющая все семафоры и разделяемую память
void unlink_all(void) {
    shmdt(buffer);
    shmctl(shmid, IPC_RMID, NULL); // delete the shared memory segment
    if (semctl(mutex, 0, IPC_RMID, 0) == -1) {
        perror("semctl:");
    }
    if (semctl(mutex_sender, 0, IPC_RMID, 0) == -1) {
        perror("semctl: ");
    }
    if (semctl(mutex_receiver, 0, IPC_RMID, 0) == -1) {
        perror("semctl:");
    }
}

void handler(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }
    printf("KILL\n");
    unlink_all();
    exit (10);
}

/////////////////// COMMON FUNCS FOR PROCESSES ///////////////////

extern void notifyToCheckTask(int r_pid, int task_index) {
    struct sembuf buf = {1, -1, 0};
    semop(mutex_receiver, &buf, 1);
    buffer->task[task_index] = -1;
    for (int i = 0; i < MAX_TASK_COUNT; ++i) {
        if (buffer->check_pull[i].second == -1) {
            buffer->check_pull[i].first = r_pid;
            buffer->check_pull[i].second = task_index;
            break;
        }
    }
    buf.sem_op = 1;
    semop(mutex_receiver, &buf, 1);
}

extern int getProgramForCheck(int s_pid, int count) {
    struct sembuf buf = {1, -1, 0};
    semop(mutex_sender, &buf, 1);
    for (int i = 0; i < MAX_TASK_COUNT; ++i) {
        if (buffer->check_pull[i].second != -1 && buffer->check_pull[i].first != s_pid) {
            int result = rand() % 2;
            buffer->task[buffer->check_pull[i].second] = result;
            buffer->check_pull[i].second = -1;
	        buffer->sender_pid = s_pid;
            buf.sem_op = 1;
            semop(mutex_receiver, &buf, 1);
            return result;
        }
    }
    buf.sem_op = 1;
    semop(mutex_receiver, &buf, 1);
    return -1;
}

void execute(task_buffer *buffer, int task_count, int step) {
    srand(time(NULL));
    int cur = buffer->cur + step;

    while (cur < task_count) {
        // execute current task
        printf("Process with pid = %d started task at index = %d\n", getpid(), cur);
        buffer->task[cur] = -2;
        sleep(rand() % MAX_TASK_COUNT);
        printf("Process with pid = %d finished task at index = %d\n", getpid(), cur);

        notifyToCheckTask(getpid(), cur);

        int result = getProgramForCheck(getpid(), task_count);
        while (1) {
            if (result == 1) { // checked, correct
                printf("!!!!!!!!!!!!!   Process with pid = %d got great result fastly   !!!!!!!!!!!!!!\n", getpid());
                break; // go to another task;
            }
            while (result != 1) {       // incorrect or not checked
                if (result == 0) {
                    printf("Process with pid = %d is fixing task %d\n", getpid(), cur);
                }
                notifyToCheckTask(getpid(), cur);
                if (buffer->sender_pid != -1) {
                    result = getProgramForCheck(buffer->sender_pid, task_count);
                } else {
                    result = buffer->task[cur];
                }
            }
            printf("!!!!!!!!!!!!!   Process with pid = %d finally fixed program     !!!!!!!!!!!!!\n", getpid());
            break;
        }
        struct sembuf buf = {1, -1, 0};
        semop(mutex, &buf, 1); // кто первый успел, того и задача...
        while (buffer->task[cur] != -3) ++cur;       // take next not expliting task
        if (cur < task_count) {
            printf("Process with pid = %d moved to next task with index = %d\n", getpid(), cur);
        }
        buf.sem_op = 1; 
        semop(mutex_receiver, &buf, 1);  // можно снова обращаться к пуллу задач
    }
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

    // define child processes
    
    pid_t pid_first, pid_second, pid_third;
    int status;

    pid_first = fork();
    if (pid_first == -1) {
        perror("fork");
        exit(1);
    } else if (pid_first == 0) {
        printf("Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());
        execute(buffer, task_count, 0);
        printf("Child process #1 went to finish\n");
        exit(0);
    }

    pid_second = fork();
    if (pid_second == -1) {
        perror("fork");
        exit(1);
    } else if (pid_second == 0) {
        printf("Child process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());
        execute(buffer, task_count, 1);
        printf("Child process #2 went to finish\n");
        exit(0);
    }

    pid_third = fork();
    if (pid_third == -1) {
        perror("fork");
        exit(1);
    } else if (pid_third == 0) {
        printf("Child process #3 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());
        execute(buffer, task_count, 2);
        printf("Child process #3 went to finish\n");
        exit(0);
    }

    // Ожидаем завершения дочерних процессов, чтобы родитель у всех был одинаковый
    waitpid(pid_first, &status, 0);
    waitpid(pid_second, &status, 0);
    waitpid(pid_third, &status, 0);

    unlink_all();

    printf("Parent process went to finish\n");

    exit(0);
}