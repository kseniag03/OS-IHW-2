#  ИДЗ №2 #
## Markdown report <br> ##

### 1. Ганина Ксения Андреевна 212 (тг для вопросов: @kgnn47) <br> ###
### 2. Вариант-21 <br> ###

![image](https://user-images.githubusercontent.com/114473740/233373017-42e56507-db24-47fd-a033-ff46b3e36a59.png)
![image](https://user-images.githubusercontent.com/114473740/232320682-3f8504cb-5b03-4b67-869a-c32d4fa825c1.png)
![image](https://user-images.githubusercontent.com/114473740/232320698-cfeb096e-79c6-46d7-9731-20ce879d40da.png)
________________________

### 3. Содержание отчёта и нюансы <br> ###

Каждый следующий раздел содержит сценарий решаемой задачи, нюансы к коду и ссылку на соответствующий код <br>
Разделы 4-6: 1 родитель-супервизор запускает 3 детей-программистов <br>
Разделы 7-8: 3 независимых процесса-программиста <br>


псевдокод прогера

в качестве аргумента командной строки -- кол-во задач для выполнения
// ака размер буфера задач

```cpp
while 1
	get task from pull of tasks (from buffer in shared memory)
	if task_buffer is empty: break
	// буфер можно представить в виде интового счётчика, который уменьшается по мере выполнения задач

	startProgram() - working on its task
	when finishes -- finishProgram = 1
	when gets other program for check -- getForCheck = 1
	when gets its program after checking -- getChecked = 1 and isCorrect = 0 or 1

	while (finishProgram && !getForCheck && !getChecked) sleep(1);

	// isCorrect = rand() % 2, что обеспечит многократные проверки и коррекции

```	

как расставлять семафоры

получение текущего индекса внутри процесса идёт поочерёдно
выбирается первая свободная задача
затем текущий процесс меняет флаг выбранного индекса isExecuting[cur] = -1

потом можно запускать процессы

в буфер записывать в пулл индексы задач
каждый прогер будет эти индексы по указателям перебирать
при этом передавать индексы задач для проверок
три указателя: старт1, старт2, старт3

________________________

### 4. именованные POSIX семафоры <br> ###

#### 4.1. Сценарий задачи <br> ####

// WANTED

#### 4.2. Код с комментариями <br> ####


```cpp
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
#define DEFAULT_TASK_COUNT 10
#define MAX_TASK_COUNT 100

typedef struct { // get info for current task = info-arr[index-of-cur-task]
    int isExecuting[MAX_TASK_COUNT]; // 0 - free, -1 - busy, 1 - finished
    int isChecking[MAX_TASK_COUNT]; // 0 - need check, -1 - is cheking, 1 - checked // task cannot be checked until it is finished
    int isCorrect[MAX_TASK_COUNT]; // 0 - not correct, 1 - correct // task cannot be defined (in)correct until it is checked
} task_buffer;

// имя области разделяемой памяти
const char* shar_object = "/posix-shar-object";
int buf_id;        // дескриптор объекта памяти
task_buffer *buffer;    // указатель на разделямую память, хранящую буфер

// имя семафора (мьютекса) для критической секции,
// обеспечивающей доступ к буферу
const char *mutex_sem_name = "/mutex-semaphore";

sem_t *mutex;   // указатель на семафор

// Функция осуществляющая при запуске общие манипуляции с памятью и семафорами
// для децентрализованно подключаемых процессов читателей и писателей.
void init(void) {
    // Создание или открытие мьютекса для доступа к буферу (доступ открыт)
    if ( (mutex = sem_open(mutex_sem_name, O_CREAT, 0666, 1)) == 0 ) {
        perror("sem_open: Can not create mutex semaphore");
        exit(-1);
    };
}

// Функция закрывающая семафоры
void close_common_semaphores(void) {
    if(sem_close(mutex) == -1) {
        perror("sem_close: Incorrect close of mutex semaphore");
        exit(-1);
    };
}

// Функция, удаляющая все семафоры и разделяемую память
void unlink_all(void) {
    if(sem_unlink(mutex_sem_name) == -1) {
        perror("sem_unlink: Incorrect unlink of mutex semaphore");
        exit(-1);
    };
    // удаление разделяемой памяти
    if(shm_unlink(shar_object) == -1) {
        perror("shm_unlink");
        exit(-1);
    }
}

////////////////////////////////////////////

bool all_tasks_correct(task_buffer *buffer, int count) {
    for (int i = 0; i < count; ++i) {
        if (buffer->isExecuting[i] == 1 
            && buffer->isChecking[i] == 1 
            && buffer->isCorrect[i] == 1) {
            // cur task is ok
        } else {
            return false;
        }
    }
    return true;
}

// programmers funcs

void execute(task_buffer *buffer, int count) {
    srand(time(NULL));
    int cur = 0;
    while (1) {
        
        // критическая секция, конкуренция между программистами
        if (sem_wait(mutex) == -1) {
            perror("sem_wait: Incorrect wait of mutex");
            exit(-1);
        };
        
        printf("Programmer with pid = %d looks at current task with index = %d\n", getpid(), cur);
        if (buffer->isExecuting[cur] == 0) {
            // current task is free
            buffer->isExecuting[cur] = -1; // now is busy
            
            
            // Выход из критической секции
            if (sem_post(mutex) == -1) {
                perror("sem_post: Incorrect post of mutex semaphore");
                exit(-1);
            };
            
            sleep(rand() % 10); // do task some time
            
            // критическая секция, конкуренция между программистами
            if (sem_wait(mutex) == -1) {
                perror("sem_wait: Incorrect wait of mutex");
                exit(-1);
            };
            
            
            buffer->isExecuting[cur] = 1; // now is finished
        } else if (buffer->isExecuting[cur] == 1) {
            // current task is finished
            if (buffer->isChecking[cur] == 0) {
                // current task is to be checked
                buffer->isChecking[cur] = -1; // now is cheking
                
                
                // Выход из критической секции
                if (sem_post(mutex) == -1) {
                    perror("sem_post: Incorrect post of mutex semaphore");
                    exit(-1);
                };
                
                sleep(rand() % 10); // do check some time
                
                //критическая секция, конкуренция между программистами
                if (sem_wait(mutex) == -1) {
                    perror("sem_wait: Incorrect wait of mutex");
                    exit(-1);
                };
                
                
                buffer->isChecking[cur] = 1; // now is checked
                buffer->isCorrect[cur] = rand() % 2; // randomly define if it is correct or not
            } else if (buffer->isChecking[cur] == 1) {
                if (buffer->isCorrect[cur] == 0) {
                    // task was incorrect, it need rewriting, so it need to be executed again
                    buffer->isExecuting[cur] = 0;
                } else {
                    // task was correct, it is ok
                }
                ++cur;
                cur = cur % count; // round buffer
            } else {
                // current task check is in process... so go to next to check...
                ++cur;
                cur = cur % count; // round buffer
            }
        } else {
            // current task is in process... so skip
            ++cur;
            cur = cur % count; // round buffer
        }
        if (all_tasks_correct(buffer, count)) { // critical section starts
            printf("All tasks're completed!\n");
            break;
        } // critical section ends
    }
}

////////////////////////////////////////////

int main(int argc, char *argv[]) {
    
    printf("Parent process with pid = %d\n starts working", getpid());
    
    // define tasks count from command line arguments
    
    int task_count = DEFAULT_TASK_COUNT;
    if (argc > 1) {
        task_count = atoi(argv[1]);
        printf("Задано число задач: %d\n", task_count);
    }
    
    // define posix shared memory segment
    
    // Формирование объекта разделяемой памяти для общего доступа к кольцевому буферу
    if ( (buf_id = shm_open(shar_object, O_CREAT|O_RDWR, 0666)) == -1 ) {
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
        printf("Memory size set and = %lu\n", sizeof (task_buffer));
    }
    //получить доступ к памяти
    buffer = mmap(0, sizeof (task_buffer), PROT_WRITE|PROT_READ, MAP_SHARED, buf_id, 0);
    if (buffer == (task_buffer*)-1 ) {
        perror("writer: mmap");
        exit(-1);
    }
    printf("mmap checkout\n");
    printf("buffer got access, it has %d tasks\n", task_count);
    
    // init semaphore
    
    init();

    // define child processes
    
    pid_t pid_first, pid_second, pid_third;
    int status;
    
    pid_first = fork();
    if (pid_first == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_first == 0) {
        printf("Child process #1 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());
        execute(buffer, task_count);
        exit(0);
    }

    pid_second = fork();
    if (pid_second == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_second == 0) {
        printf("Child process #2 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());
        execute(buffer, task_count);
        exit(0);
    }

    pid_third = fork();
    if (pid_third == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid_third == 0) {
        printf("Child process #3 with id: %d with parent id: %d\n", (int)getpid(), (int)getppid());
        execute(buffer, task_count);
        exit(0);
    }

    // Ожидаем завершения дочерних процессов, чтобы родитель у всех был одинаковый
    waitpid(pid_first, &status, 0);
    waitpid(pid_second, &status, 0);
    waitpid(pid_third, &status, 0);
    
    close_common_semaphores();
    
    unlink_all();
    exit(0);
}

```


// WANTED

[Задание на 4 балла](https://github.com/kseniag03/OS-IHW-1/blob/master/programs/4/4-task.c) <br>
________________________

### 5. неименованные POSIX семафоры <br> ###

#### 5.1. Сценарий задачи <br> ####

// WANTED

#### 5.2. Код с комментариями <br> ####

// WANTED

[Задание на 5 баллов](https://github.com/kseniag03/OS-IHW-1/blob/master/programs/5/5-task.c) <br>
________________________

### 6. семафоры в стандарте UNIX SYSTEM V <br> ###

#### 6.1. Сценарий задачи <br> ####

// WANTED

#### 6.2. Код с комментариями <br> ####

// WANTED

[Задание на 6 баллов](https://github.com/kseniag03/OS-IHW-1/blob/master/programs/6/6-task.c) <br>
________________________

### 7. стандарт POSIX <br> ###

#### 7.1. Сценарий задачи <br> ####

// WANTED

#### 7.2. Код с комментариями <br> ####

// WANTED

[Задание на 7 баллов](https://github.com/kseniag03/OS-IHW-1/blob/master/programs/7/7-task.c) <br>
________________________

### 8. стандарт UNIX SYSTEM V <br> ###

#### 8.1. Сценарий задачи <br> ####

// WANTED

#### 8.2. Код с комментариями <br> ####

// WANTED

[Задание на 8 баллов, 1й процесс](https://github.com/kseniag03/OS-IHW-1/blob/master/programs/8/8-task-1.c) <br>
[Задание на 8 баллов, 2й процесс](https://github.com/kseniag03/OS-IHW-1/blob/master/programs/8/8-task-2.c) <br>



