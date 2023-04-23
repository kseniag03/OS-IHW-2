#include "common.h"
#include <unistd.h>

void handler(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
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
    
    printf("Process with pid = %d\n starts working", getpid());

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
    
    execute(buffer, task_count, 1);

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