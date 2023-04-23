#include <ctype.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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
const char *shar_object = "/posix-shar-object";
int buf_id;
task_buffer *buffer;

// имя семафора (мьютекса) для чтения данных из буфера
const char *mutex_sem_name = "/mutex-semaphore";
sem_t *mutex;

// имя семафора (мьютекса) для идентификации проверяющего
const char *mutex_sem_sender_name = "/mutex-semaphore-sender";
sem_t *mutex_sender;

// имя семафора (мьютекса) для идентификации ждущего проверки
const char *mutex_sem_receiver_name = "/mutex-semaphore-receiver";
sem_t *mutex_receiver;

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
    // удаление разделяемой памяти
    if (shm_unlink(shar_object) == -1) {
        perror("shm_unlink");
        exit(-1);
    }
    if (sem_unlink(mutex_sem_name) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore");
    }
    if (sem_unlink(mutex_sem_sender_name) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore");
    }
    if (sem_unlink(mutex_sem_receiver_name) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore");
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
    sem_wait(mutex_receiver);
    buffer->task[task_index] = -1;
    for (int i = 0; i < MAX_TASK_COUNT; ++i) {
        if (buffer->check_pull[i].second == -1) {
            buffer->check_pull[i].first = r_pid;
            buffer->check_pull[i].second = task_index;
            break;
        }
    }
    sem_post(mutex_receiver);
}

extern int getProgramForCheck(int s_pid, int count) {
    sem_wait(mutex_sender);
    for (int i = 0; i < MAX_TASK_COUNT; ++i) {
        if (buffer->check_pull[i].second != -1 && buffer->check_pull[i].first != s_pid) {
            int result = rand() % 2;
            buffer->task[buffer->check_pull[i].second] = result;
            buffer->check_pull[i].second = -1;
	    buffer->sender_pid = s_pid;
            sem_post(mutex_sender);
            return result;
        }
    }
    sem_post(mutex_sender);
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
        sem_wait(mutex); // кто первый успел, того и задача...
        while (buffer->task[cur] != -3) ++cur;       // take next not expliting task
        if (cur < task_count) {
            printf("Process with pid = %d moved to next task with index = %d\n", getpid(), cur);
        }
        sem_post(mutex); // можно снова обращаться к пуллу задач
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

    // define posix shared memory segment

    // Формирование объекта разделяемой памяти для общего доступа к кольцевому буферу
    if ((buf_id = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) == -1) {
        perror("shm_open");
        exit(-1);
    } else {
        printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
    }
    // Задание размера объекта памяти
    if (ftruncate(buf_id, sizeof(task_buffer)) == -1) {
        perror("ftruncate");
        exit(-1);
    } else {
        printf("Memory size set and = %lu\n", sizeof(task_buffer));
    }
    // получить доступ к памяти
    buffer = mmap(0, sizeof(task_buffer), PROT_WRITE | PROT_READ, MAP_SHARED, buf_id, 0);
    if (buffer == (task_buffer *)-1) {
        perror("writer: mmap");
        exit(-1);
    }
    printf("mmap checkout\n");
    printf("buffer got access, it has %d tasks\n", task_count);

    init(buffer);

    // init semaphore
    if ((mutex = sem_open(mutex_sem_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create mutex semaphore");
        exit(-1);
    };
    if ((mutex_sender = sem_open(mutex_sem_sender_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create mutex-s semaphore");
        exit(-1);
    };
    if ((mutex_receiver = sem_open(mutex_sem_receiver_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create mutex-r semaphore");
        exit(-1);
    };

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

    printf("Parent process went to finish\n");

    exit(0);
}