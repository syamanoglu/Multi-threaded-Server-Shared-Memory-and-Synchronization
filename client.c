#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>


#define MAX_LEN 1024
#define MAX_NUM_OF_CLIENTS 10
#define BUF_SIZE 101
#define MAX_NUM_OF_KEYWORD 128

#define SEMAPHORE_REQUEST_SUFFIX "rsf"
#define SEMAPHORE_CLIENT_SUFFIX "csf"
#define SEMAPHORE_RESULT_SUFFIX "resf"


struct region{
	int queue_state[MAX_NUM_OF_CLIENTS + 1];
	int reqIn, reqOut;
	int resIn[MAX_NUM_OF_CLIENTS], resOut[MAX_NUM_OF_CLIENTS];
	int resArr[MAX_NUM_OF_CLIENTS][BUF_SIZE];
	char reqArr[MAX_NUM_OF_CLIENTS][MAX_NUM_OF_KEYWORD];
	int reqQueArr[MAX_NUM_OF_CLIENTS];
	/*
	struct result_queue result_queues[MAX_NUM_OF_CLIENTS];
	struct request_queue request_queue;
	*/
};


void* ptr;	
int main(int argc, char** argv)
{

    void *ptr;
    char *shm_name = argv[1];
    char *keyword = argv[2];
    char *sem_name = argv[3];


	int myResultQueueIndex = -1;
	int serverAvailable = 1;
    //printf("%s %s %s\n", shm_name, keyword, sem_name);
    int fd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR );
    if(fd == -1)
    {
        printf("Error occured client shm_open: %s", strerror(errno));
    }

    char semName[140];
    sprintf(semName, "%s%s", sem_name, SEMAPHORE_CLIENT_SUFFIX);

    sem_t *semClient = sem_open(semName, O_CREAT, S_IWUSR | S_IRUSR);
	sprintf(semName, "%s%s", sem_name, SEMAPHORE_REQUEST_SUFFIX);    
    sem_t *semRequest = sem_open(semName, O_CREAT, S_IWUSR | S_IRUSR);
    ptr = mmap(NULL, sizeof(struct region), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct region *shmregion = (struct region *) ptr;

    
	
    
	
    //printf(" %s\n", shmregion);
    
    //Allocate a result queue
    
	//Semaphore locked
	//printf("Sem error client 1: %s\n", strerror(errno) );
    //("Semahphore locked for keyword: %s\n", keyword);

    sem_wait(semClient);
    int i = 0;
    int isAvailable = shmregion->queue_state[i];
    while(isAvailable != -1)
    {
    	if(isAvailable == 0)
    	{
    		shmregion->queue_state[i] = 1;
    		myResultQueueIndex = i;
    		break;
    	}
    	//printf("Is av: %d\n", isAvailable);
    	i++;
    	isAvailable = shmregion->queue_state[i];
    }

    sem_post(semClient);
    if(myResultQueueIndex == -1)
    {
    	printf("too many clients started\n");
    	serverAvailable = 0;
//    	exit(0);
    }
    sprintf(semName, "%s%s%d", sem_name, SEMAPHORE_RESULT_SUFFIX,myResultQueueIndex);
	sem_t * resQSem = sem_open(semName, O_CREAT, S_IWUSR | S_IRUSR);
    if(serverAvailable == 1)
    {
	    //printf("My key: %s my index : %d\n", keyword, myResultQueueIndex);


	    
	    //Make a request to server via shared memory
	    sem_wait(semRequest);
	    int curIndex = shmregion->reqIn;	
	    strcpy(shmregion->reqArr[curIndex], keyword);
	    shmregion->reqQueArr[curIndex] = myResultQueueIndex;
	    shmregion->reqIn = (shmregion->reqIn + 1) % MAX_NUM_OF_CLIENTS;// = (curIndex + 1) /BUF_SIZE;
	    //printf("Client %s made request %d\n", shmregion->reqArr[curIndex], shmregion->reqIn );
	   	sem_post(semRequest);

	   	//Wait for the result
	   	int line;
		while(1)
		{
			//When buffer is empty

			while(shmregion->resIn[myResultQueueIndex] == shmregion->resOut[myResultQueueIndex]);	
			line = shmregion->resArr[myResultQueueIndex][shmregion->resOut[myResultQueueIndex]];
			sem_wait(resQSem); 
			shmregion->resOut[myResultQueueIndex] = (shmregion->resOut[myResultQueueIndex] + 1) % BUF_SIZE;
			sem_post(resQSem);
			if(line == -1)
				break;
			printf("%d\n", line);
			
		}

		sem_wait(semClient);
		shmregion->queue_state[myResultQueueIndex] = 0;
		sem_post(semClient);
	}

	
	
}