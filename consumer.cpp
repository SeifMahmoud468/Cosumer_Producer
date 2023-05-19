#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include<unistd.h>
#include <string>
#include <random>

#define SHM_KEY 0x12345
#define SEM_KEY 0x54321

//names of the Commodity
char commodity_names[11][20] = {"ALUMINIUM","COPPER","COTTON","CRUDEOIL","GOLD","LEAD","MENTHAOIL","NATURALGAS","NICKEL","SILVER","ZINC"};

using namespace std;
typedef struct
{
    int id;
    double price;
    double previous_price;
    double avrag;
} MyData;

union semun
{
    int val;               /* used for SETVAL only */
    struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
    ushort *array;         /* used for GETALL and SETALL */
};

int printing(double prices[],double previous_prices[], double avrag[])
{
    printf("\e[1;1H\e[2J");
    printf("+--------------------------------------+\n");
    printf("| Currency      |  Price  |  AvgPrice  |\n");
    printf("+--------------------------------------+\n");
    for(int i=0; i<11; i++)
    {
        printf("| %s",commodity_names[i]);
        printf("\033[%d;17H",i+4);
        if(prices[i]==0){
            printf("| \033[;36m %0.2f \033[0m",prices[i]);
            printf("\033[%d;27H",i+4);
            printf("|  \033[;36m %0.2f \033[0m",avrag[i]);
            printf("\033[%d;40H",i+4);
        }
        else if(previous_prices[i] > prices[i]){
            printf("| \033[;31m %0.2f↓\033[0m",prices[i]);
            printf("\033[%d;27H",i+4);
            printf("|  \033[;31m %0.2f↓\033[0m",avrag[i]);
            printf("\033[%d;40H",i+4);
        }
        else if(prices[i] > previous_prices[i]){
            printf("| \033[;32m %0.2f↑\033[0m",prices[i]);
            printf("\033[%d;27H",i+4);
            printf("|  \033[;32m %0.2f↑\033[0m",avrag[i]);
            printf("\033[%d;40H",i+4);
        }
        printf("|\n");
    }
    printf("+--------------------------------------+\n");
    return 0;
}

int main()
{
    double prices[11]= {0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00};
    double previous_prices[11] = {0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00};
    double avrags[11] = {0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00,0.00};

    int bufferSize;
    cin >> bufferSize;

    //Shared Memory Part
    int shmid = shmget(SHM_KEY,sizeof(MyData)*bufferSize,0666|IPC_CREAT);
    if(shmid<0){
        perror("SharedMemory");
        exit(1);
    }
    MyData *data = (MyData*) shmat(shmid,(void*)0,0);

    //Empty the field
    for(int i=0; i<bufferSize; i++){
        data[i].id = -1;
    }

    //Semaphore
    int semid;
    union semun arg;
    semid = semget(SEM_KEY, 3, 0666 | IPC_CREAT);
    struct sembuf IsEmpty = {0,1,0};
    struct sembuf IsFull = {1,-1,0};
    struct sembuf Mutuax = {2,-1,0};
    arg.val = bufferSize;
    if (semctl(semid, 0, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(1);
    }
    arg.val = 0;
    if (semctl(semid, 1, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(1);
    }
    arg.val = 1;
    if (semctl(semid, 2, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(1);
    }
    
    int number=0;
    while(true)
    {
        if (semop(semid, &IsFull, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        Mutuax.sem_op = -1;
        if (semop(semid, &Mutuax, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        // Critical Section
        MyData exp = data[number];
        previous_prices[exp.id] = exp.previous_price;
        prices[exp.id]= exp.price;
        avrags[exp.id] = exp.avrag;
        printing(prices,previous_prices,avrags);
        data[number].id=-1;

        Mutuax.sem_op = 1;
        if (semop(semid, &Mutuax, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        if (semop(semid, &IsEmpty, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        number=(number+1)%bufferSize;
    }
    shmdt(data);
    return 0;
}
