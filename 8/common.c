#include "common.h"
#include <semaphore.h>

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
    buffer->task_count = MAX_TASK_COUNT;
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