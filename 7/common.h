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

typedef struct {
    int first;
    int second;
} pair;


typedef struct {
    int task[MAX_TASK_COUNT]; // array of tasks; value: -3 -- was not touch by anybody, -2 -- is executing, -1 -- is asking for check, 0 -- incorrect, 1 -- correct and finish
    int cur;                  // current index
    pair check_pull[MAX_TASK_COUNT]; // check pull (first - pid, second - index)
    int sender_pid;   // pid of process doing check ака проверяющий
    int task_count;
} task_buffer;

// имя области разделяемой памяти
extern const char* shar_object;
extern int buf_id;        // дескриптор объекта памяти
extern task_buffer *buffer;    // указатель на разделямую память, хранящую буфер

// имя семафора (мьютекса) для критической секции,
// обеспечивающей доступ к буферу
extern const char *mutex_sem_name;
extern sem_t *mutex;   // указатель на семафор читателей

// имя семафора (мьютекса) для идентификации проверяющего
extern const char *mutex_sem_sender_name;
extern sem_t *mutex_sender;

// имя семафора (мьютекса) для идентификации ждущего проверки
extern const char *mutex_sem_receiver_name;
extern sem_t *mutex_receiver;

// для инициализации начальных значений задач
void init(task_buffer *buffer);

// Функция, удаляющая все семафоры и разделяемую память
void unlink_all(void);

extern void notifyToCheckTask(int r_pid, int task_index);

extern int getProgramForCheck(int s_pid, int count);

extern void execute(task_buffer *buffer, int task_count, int step);