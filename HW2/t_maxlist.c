#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/sem.h>

#define KEYSEM 400

int semid;
int n, k;
int *L;
int *RL;

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

void* child_op(void *arg) {
    int i;
    int max_num = INT_MIN;
    int ind = *((int *) arg);
    int t = n / k;

    for (i = 0; i < t; i++) {
        int cur_num = *(L + t * ind + i);

        if (cur_num > max_num) {
            max_num = cur_num;
        }
    }

    printf("Child %d: The maximum number in the range [%d]-[%d] is %d.\n", ind, t * ind, t * ind + t - 1, max_num);

    *(RL + ind) = max_num;

    printf("Chlid %d: Wrote %d as maximum number on the shared memory.\n", ind, max_num);

    sem_signal(semid, 1);
}

int main(int argc, char **argv) {
    int i;
    int max_num = INT_MIN;
    int *thread_inds;
    pthread_t *thread_arr;

    if (argc < 3) {
        exit(1);
    }

    n = atoi(argv[1]);
    k = atoi(argv[2]);
    thread_arr = (pthread_t *) malloc(k * sizeof(pthread_t));
    thread_inds = (int *) malloc(k * sizeof(int));

    if (n % k != 0) {
        exit(1);
    }

    L = (int *) malloc(n * sizeof(int));
    RL = (int *) malloc(k * sizeof(int));

    printf("Array: ");

    for (i = 0; i < n; i++) {
        int temp = rand() % 10000;

        *(L + i) = temp;

        printf("%d ", temp);
    }

    printf("\n");

    semid = semget(KEYSEM, 1, IPC_CREAT|0700);
    semctl(semid, 0, SETVAL, 0);

    for (i = 0; i < k; i++) {
        *(thread_inds + i) = i;

        pthread_create(thread_arr + i, NULL, child_op, (void *) (thread_inds + i));

        printf("Child %d created.\n", i);
    }

    sem_wait(semid, k);

    for (i = 0; i < k; i++) {
        int cur_num = *(RL + i);

        if (cur_num > max_num) {
            max_num = cur_num;
        }
    }

    printf("Parent: The maximum number is %d.\n", max_num);

    semctl(semid, 0, IPC_RMID, 0);
    free(thread_inds);
    free(thread_arr);
    free(L);
    free(RL);
    exit(0);
}