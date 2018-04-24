#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define KEYSHM1 100
#define KEYSHM2 200
#define KEYSHM3 300
#define KEYSEM 400

void sem_signal(int semid, int val) {
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 1;
    semop(semid, &semaphore, 1);
}

void sem_wait(int semid, int val) {
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = -val;
    semaphore.sem_flg = 1;
    semop(semid, &semaphore, 1);
}

int main(int argc, char **argv) {
    int shmid_nk, shmid_L, shmid_RL;
    int semid;
    int *ptr;
    int n, k, f, i, rl_ind;
    int max_num = INT_MIN;

    if (argc < 3) {
        exit(1);
    }

    n = atoi(argv[1]);
    k = atoi(argv[2]);

    if (n % k != 0) {
        exit(1);
    }

    srand(time(NULL));

    shmid_nk = shmget(KEYSHM1, 2 * sizeof(int), IPC_CREAT|0700);
    ptr = (int *) shmat(shmid_nk, NULL, 0);
    *ptr = n;
    *(ptr + 1) = k;
    shmdt(ptr);

    printf("Array: ");

    shmid_L = shmget(KEYSHM2, n * sizeof(int), IPC_CREAT|0700);
    ptr = (int *) shmat(shmid_L, NULL, 0);
    for (i = 0; i < n; i++) {
        int temp = rand() % 10000;

        *(ptr + i) = temp;

        printf("%d ", temp);
    }
    shmdt(ptr);

    printf("\n");

    shmid_RL = shmget(KEYSHM3, k * sizeof(int), IPC_CREAT|0700);

    semid = semget(KEYSEM, 1, IPC_CREAT|0700);
    semctl(semid, 0, SETVAL, 0);

    for (i = 0; i < k; i++) {
        f = fork();

        if (f == 0) {
            rl_ind = i;
            printf("Child %d created.\n", rl_ind);
            break;
        }
    }

    if (f == 0) {
        int t;

        ptr = (int *) shmat(shmid_nk, NULL, 0);
        t = *ptr / *(ptr + 1);
        shmdt(ptr);

        ptr = (int *) shmat(shmid_L, NULL, 0);
        for (i = 0; i < t; i++) {
            int cur_num = *(ptr + t * rl_ind + i);

            if (cur_num > max_num) {
                max_num = cur_num;
            }
        }
        shmdt(ptr);

        printf("Child %d: The maximum number in the range [%d]-[%d] is %d.\n", rl_ind, t * rl_ind, t * rl_ind + t - 1, max_num);

        ptr = (int *) shmat(shmid_RL, NULL, 0);
        *(ptr + rl_ind) = max_num;
        shmdt(ptr);

        printf("Chlid %d: Wrote %d as maximum number on the shared memory.\n", rl_ind, max_num);

        sem_signal(semid, 1);
    }
    else {
        sem_wait(semid, k);

        ptr = (int *) shmat(shmid_RL, NULL, 0);
        for (i = 0; i < k; i++) {
            int cur_num = *(ptr + i);

            if (cur_num > max_num) {
                max_num = cur_num;
            }
        }
        shmdt(ptr);

        printf("Parent: The maximum number is %d.\n", max_num);

        shmctl(shmid_nk, IPC_RMID, 0);
        shmctl(shmid_L, IPC_RMID, 0);
        shmctl(shmid_RL, IPC_RMID, 0);
        semctl(semid, 0, IPC_RMID, 0);
    }

    exit(0);
}