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

ИЛИ

намутить 3 пулла через разделяемую память
в одном пулле будут храниться индексы задач для выполнения
в другом индексы задач для проверки
в третьем массив задач
а надо ли делать структуру задач?



________________________

### 4. именованные POSIX семафоры <br> ###

#### 4.1. Сценарий задачи <br> ####

// WANTED

#### 4.2. Код с комментариями <br> ####

NEW

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

/*
typedef struct { // get info for current task = info-arr[index-of-cur-task]
    int isExecuting[MAX_TASK_COUNT]; // 0 - free, -1 - busy, 1 - finished
    int isChecking[MAX_TASK_COUNT]; // 0 - need check, -1 - is cheking, 1 - checked // task cannot be checked until it is finished
    int isCorrect[MAX_TASK_COUNT]; // 0 - not correct, 1 - correct // task cannot be defined (in)correct until it is checked
} task_buffer;

typedef struct {
    int index[MAX_TASK_COUNT];

    int start;
    int end;
    //
} execution;

typedef struct {
    int index[MAX_TASK_COUNT];

    int start;
    int end;

    int i12;    // 1-й передаёт 2-у
    int i13;    // 1-й передаёт 3-у

    int i21;    // 2-й передаёт 1-у
    int i23;    // 2-й передаёт 2-у

    int i31;    // 3-й передаёт 1-у
    int i32;    // 3-й передаёт 2-у

    //
} check;
*/

void plot()
{

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

typedef struct
{
    int task[MAX_TASK_COUNT]; // array of tasks
    int cur;                  // current index
} task_buffer;

// имя области разделяемой памяти
const char *shar_object = "/posix-shar-object";
int buf_id;
task_buffer *buffer;

extern int getCheckResult()
{
    if (0)
        return -1; // write here some cond using semaphores...
    srand(time(NULL));
    return rand() % 2;
}

int main(int argc, char *argv[])
{

    // p1

    printf("Programmer #1 process with pid = %d\n starts working", getpid());

    // get count from arguments

    int task_count = DEFAULT_TASK_COUNT;
    if (argc > 1)
    {
        task_count = atoi(argv[1]);
        printf("Tasks' count was defined: %d\n", task_count);
    }

    // define shared-memory-object

    if ((buf_id = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) == -1)
    {
        perror("shm_open");
        exit(-1);
    }
    else
    {
        printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
    }

    if (ftruncate(buf_id, sizeof(task_buffer)) == -1)
    {
        perror("ftruncate");
        exit(-1);
    }
    else
    {
        printf("Memory size set and = %lu\n", sizeof(task_buffer));
    }

    buffer = mmap(0, sizeof(task_buffer), PROT_WRITE | PROT_READ, MAP_SHARED, buf_id, 0);
    if (buffer == (task_buffer *)-1)
    {
        perror("writer: mmap");
        exit(-1);
    }
    printf("mmap checkout\n");
    printf("buffer got access, it has %d tasks\n", task_count);

    // execute current task

    srand(time(NULL));
    int cur = buffer->cur;
    sleep(rand() % MAX_TASK_COUNT); // maybe change const

    // transfer it to p2 or p3
    // maybe call some extern method to get check result
    // before calling CheckResult p2 or p3 must call another extern method to change

    int result;
    while (1)
    {
        // p1 can check program from p3 or p2
        result = getCheckResult();
        if (result == -1)
        { // not checked now
            sleep(1);
        }
        else if (result == 1)
        {          // checked, correct
            break; // go to another task;
        }
        else
        { // checked, incorrect
            while (result != 1)
            {
                sleep(rand() % MAX_TASK_COUNT);
                result = getCheckResult();
            }
        }
    }
    // for all p1, p2, p3 this code is similar, so we can use extern method to launch executing

    // how to take task that are not executing by p2 or p3 at the moment?
    // maybe use semaphores...

    ++cur;
}

/*

const char* tasks_for_execution_pull = "/posix-execution-pull";
const char* tasks_for_check_pull = "/posix-check-pull";
//const char* shar_object = "/posix-shar-object"; // а надо ли...

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
*/
```

OLD

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



