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
#define DEFAULT_TASK_COUNT 5
#define MAX_TASK_COUNT 10



void plot() {

    // проггеры выполняют по 3 задачи, затем идёт этап проверки

    // этап выполнения:

    // pi берёт task[cur + 0], pj -- task[cur + 1], pk -- task[cur + 2]
    // время выполнения = rand % MAX_TASK_COUNT (или любая другая const...)
    // здесь синхронизация не нужна, т.к. процессы берут фиксированные индексы
    // потому что пока не завершаться проверки, прогеры не смогут приступить к новым задачам

    /// ! но вообще по условию если один прогер получил корректный вердикт своей проги,
    /// то он берет следующую задачу
    /// т.е. с каждой завершённой задачей увеличивается индекс cur //
    /// вроде достаточно просто увеличивать cur после получения верной задачи
    /// и здесь юзать семафор, чтобы не было обращения к одному ресурсу от нескольких проггеров
    /// после первой тройки задач надо увеличить cur на 2, а потом достаточно просто cur++ // вроде

    // этап проверки:   while все 3 задачи не выполнены правильно -- не совсем верно

    // pi -- первый закончивший задачу, pj -- второй закончивший задачу
    // pk либо передаёт задачу pj, если pi исправляет свою задачу,
    //    либо передаёт задачу pi, если pj проверяет задачу

    // pi блокируется, когда ждёт результата проверки от pj, используя sem_wait

    // pj закидывает в shared-object результат проверки и использует sem_post,
    // чтобы уведомить об окончании проверки

    // если pi получает 0, он какое-то время исправляет задачу (спит), передаёт обратно
    // повторить while pi не получит 1

    // pi сначала проверяет программу от pk, а потом делает исправления в своей

    // переход к следующим 3 задачам

    // while cur != count (MAX_TASK_COUNT default)
}

/////////////////// INIT STRUCTURES AND TOOLS ///////////////////

typedef struct {
    int task[MAX_TASK_COUNT]; // array of tasks; value: -3 -- was not touch by anybody, -2 -- is executing, -1 -- is asking for check, 0 -- incorrect, 1 -- correct and finish
    int cur;                  // current index

    // or use sem field?

    // result of check = buffer->task[cur] ?

    int sender_pid;   // pid of process doing check
    int receiver_pid; // pid of process asking for check

    // int check_pos = -1;

} task_buffer;

// имя области разделяемой памяти
const char *shar_object = "/posix-shar-object";
int buf_id;
task_buffer *buffer;

// имя семафора (мьютекса) для чтения данных из буфера
const char *mutex_sem_name = "/mutex-semaphore";
sem_t *mutex;

void init(task_buffer *buffer) {
    buffer->cur = 0;
    buffer->sender_pid = -1;
    buffer->receiver_pid = -1;
    // buffer->check_pos = -1;

    for (int i = 0; i < MAX_TASK_COUNT; ++i) {
        buffer->task[i] = -3;
    }
}

// Функция, удаляющая все семафоры и разделяемую память
void unlink_all(void) {
    // удаление разделяемой памяти
    if (shm_unlink(shar_object) == -1) {
        perror("shm_unlink");
        exit(-1);
    }
    if(sem_unlink(mutex_sem_name) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore");
        // exit(-1);
    }
}

void handler(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    if (sig == SIGINT) {
        //kill(buffer->receiver_pid, SIGTERM);
        //kill(buffer->sender_pid, SIGTERM);
    }

    printf("KILL\n");
    unlink_all();
    exit (10);
}

/////////////////// COMMON FUNCS FOR PROCESSES ///////////////////

extern void notifyToCheckTask(int r_pid, int task_index) {
    if (buffer->receiver_pid == getpid()) {
        // This process is trying to ask itself to check a task, do nothing
        //return;
    }
    buffer->receiver_pid = r_pid;
    buffer->task[task_index] = -1;
    printf("Process with pid = %d asked another programmer to check task with index = %d\n", getpid(), task_index);
}

extern int getCheckResult(int task_index) {
    if (buffer->sender_pid == getpid()) {
        // This process is trying to check its own task, return -1 to indicate it hasn't been checked yet
        printf("Process with pid = %d hasn't got the check result yet...\n", getpid());
        return -1;
    }
    
    //printf("Process with pid = %d asked another programmer to check current task\n", getpid());
    buffer->sender_pid = getpid();
    printf("Programmer with pid = %d has checked the task with index = %d\n", buffer->sender_pid, task_index);
    
    // sem_wait and sem_post are used in void extern func
    if (0)
        return -1; // write here some cond using semaphores...
        
    srand(time(NULL));
    int result = rand() % 2;
    buffer->task[task_index] = result;
    if (result == 0) {
        printf("Process with pid = %d have mistakes to fix\n", getpid());
    }
    return buffer->task[task_index];
}

extern void getProgramForCheck(int s_pid, int task_index) {
    while (buffer->task[task_index] != -1) {    // task for check is not found
	    printf("Process with pid = %d is waiting for task checking...\n", getpid());
        //sleep(1);
    }
    printf("Process with pid = %d started checking task %d from other programmer\n", getpid(), task_index);
    
    // use sem_wait on process from which we get task for checking
    srand(time(NULL));
    sleep(rand() % MAX_TASK_COUNT);
    
    // use sem_post on cur process
    printf("Process with pid = %d finished checking task %d from other programmer\n", getpid(), task_index);
}

void execute(task_buffer *buffer, int task_count, int step) {
    // maybe transfer to separate func

    // use there semaphores to correct init of beginning cur positions

    srand(time(NULL));
    int cur = buffer->cur + step;

    while (cur < task_count) {
        // execute current task
        printf("Process with pid = %d started task at index = %d\n", getpid(), cur);
        buffer->task[cur] = -2;
        sleep(rand() % MAX_TASK_COUNT); // maybe change const
        printf("Process with pid = %d finished task at index = %d\n", getpid(), cur);

        // define other processes that there is a new task to check
        notifyToCheckTask(getpid(), cur);

        // transfer it to p2 or p3
        // maybe call some extern method to get check result
        // before calling CheckResult p2 or p3 must call another extern method to change

        int result;
        while (1) {
            // p1 can check program from p3 or p2 throw extern method
            // use semaphores to sync

            getProgramForCheck(getpid(), cur);

            // after finishing checking other program

            result = getCheckResult(cur);
            if (result == 1) { // checked, correct
                printf("Process with pid = %d got great result fastly\n", getpid());
                break; // go to another task;
            }
            
            while (result != 1) {       // incorrect or not checked
                printf("Process with pid = %d is fixing task %d\n", getpid(), cur);
                sleep(rand() % MAX_TASK_COUNT);
                result = getCheckResult(cur);
            }
            printf("Process with pid = %d finally fixed program\n", getpid());
            break;
        }
        // for all p1, p2, p3 this code is similar, so we can use extern method to launch executing
        
        // how to take task that are not executing by p2 or p3 at the moment?       !!!!!!!!!!!!!!!!!!!!
        // maybe use semaphores...
        //++cur;
    
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

    printf("Parent process with pid = %d\n starts working", getpid());

    // define tasks count from command line arguments

    int task_count = DEFAULT_TASK_COUNT;
    if (argc > 1) {
        task_count = atoi(argv[1]);
        printf("Задано число задач: %d\n", task_count);
    }

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
    // init();
    // Создание или открытие мьютекса для доступа к буферу (доступ открыт)
    if ((mutex = sem_open(mutex_sem_name, O_CREAT, 0666, 1)) == 0) {
        perror("sem_open: Can not create mutex semaphore");
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
    
    if(sem_close(mutex) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };

    unlink_all();

    printf("Parent process went to finish\n");

    exit(0);
}
