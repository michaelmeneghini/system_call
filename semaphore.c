#include <sys/sem.h>

#include "semaphore.h"
#include "err_exit.c"

key_t sem_key = 1111;
key_t shm_key = 2222;


void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        err_exit("semop failed");
}
