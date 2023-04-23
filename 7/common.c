#include "common.h"
#include <semaphore.h>

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
    buffer->task_count = MAX_TASK_COUNT;
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

extern void execute(task_buffer *buffer, int task_count, int step) {
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


