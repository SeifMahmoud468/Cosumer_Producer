#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include<unistd.h>
#include <string>
#include <string.h>
#include <random>
#include <cstdio>
#include <ctime>
#include <queue>

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

void getTime(char *str)
{
    std::time_t rawtime;
    std::tm* timeinfo;
    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);
    std::strftime(str,80,"%Y/%m/%d %H:%M:%S",timeinfo);
}

int getID(char *str)
{
    for(int i=0; i<11; i++)
    {
        if(strcasecmp(str,commodity_names[i])==0){
            return i;
        }
    }
    return -1;
}

double avg(queue<double> q){
    int sum=0, i=0;
    while(i!=5){
        sum+=q.front();
        q.push(q.front());
        q.pop();
        i++;
    }
    return sum/4;
}

int main()
{
    char name[20];
    int bufferSize;
    long sleeptime;
    double mean, variance;
    cin >> name;
    cin >> mean;
    cin >> variance;
    cin >> sleeptime;
    cin >> bufferSize;

    //Queue for average calculation
    queue<double> average;
    average.push(0.00);

    //Normal distribution
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(mean,variance);

    //Shared Memory Part
    int shmid = shmget(SHM_KEY,sizeof(MyData)*bufferSize,0666| IPC_EXCL);
    MyData *data = (MyData*) shmat(shmid,(void*)0,0);

    //Semaphore
    int semid;
    semid = semget(SEM_KEY, 3, 0666 | IPC_EXCL);
    struct sembuf IsEmpty = {0,-1,0};
    struct sembuf IsFull = {1,1,0};
    struct sembuf Mutuax = {2,-1,0};


    int num=0;
    char str[80];
    int id = getID(name);
    if(id<0){
        perror("Not valid name");
        exit(1);
    }

    double prv_price=0.00,value;

    while(true)
    {
        value = distribution(generator);
        getTime(str);
        printf("\n%s %s: generating a new value %0.2f\n",str,commodity_names[id],value);
        
        if (semop(semid, &IsEmpty, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        getTime(str);
        Mutuax.sem_op = -1;
        printf("%s %s: trying to get mutex on shared buffer\n",str,commodity_names[id]);
        if (semop(semid, &Mutuax, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        // Critical Section
        while(data[num].id !=-1){
            num=(num+1)%bufferSize;
        }
        data[num].id = id;
        data[num].previous_price = prv_price;
        data[num].price = value;
        prv_price = data[num].price;
        average.push(data[num].price);
        if(average.size() == 6){
            average.pop(); 
        }
        data[num].avrag = avg(average);



        Mutuax.sem_op = 1;
        if (semop(semid, &Mutuax, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        getTime(str);
        printf("%s %s: placing %0.2f on shared buffer\n",str,commodity_names[id],value);
        if (semop(semid, &IsFull, 1) == -1)
        {
            perror("semop");
            exit(1);
        }
        getTime(str);
        printf("%s %s: sleeping for %ld ms\n",str,commodity_names[id],sleeptime);
        num=(num+1)%bufferSize;
        sleep(sleeptime/1000);
    }
    shmdt(data);
    return 0;
}

