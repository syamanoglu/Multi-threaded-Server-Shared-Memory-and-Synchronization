#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

#define MAX_LEN 1024
#define MAX_NUM_OF_CLIENTS 10
#define BUF_SIZE 101
#define MAX_NUM_OF_KEYWORD 128
#define SEMAPHORE_REQUEST_SUFFIX "rsf"
#define SEMAPHORE_CLIENT_SUFFIX "csf"
#define SEMAPHORE_RESULT_SUFFIX "resf"

void sig_handler(int);

sem_t *resQSems[MAX_NUM_OF_CLIENTS];

struct region{
	int queue_state[MAX_NUM_OF_CLIENTS + 1];
	int reqIn, reqOut;
	int resIn[MAX_NUM_OF_CLIENTS], resOut[MAX_NUM_OF_CLIENTS];
	int resArr[MAX_NUM_OF_CLIENTS][BUF_SIZE];
	char reqArr[MAX_NUM_OF_CLIENTS][MAX_NUM_OF_KEYWORD];
	int reqQueArr[MAX_NUM_OF_CLIENTS];
};

struct thread_arg{
	int resQIndex;
	char keyword[MAX_NUM_OF_KEYWORD];
};

/* For timing experiments
struct timeval tv;
*/
FILE *file;
void *ptr;
char *inputfilename;
void *runner(void *param)

{
	struct thread_arg *arg = (struct thread_arg *)param;
	file = fopen(inputfilename,"r");
	//printf("Thread created: \"%s\" - %d\n", arg->keyword, arg->resQIndex);

	struct region *shmregion = (struct region *) ptr;

	//Send to result queue
	char linebuff[MAX_LEN];
	int linecount = 0;
	while(fgets(linebuff, MAX_LEN,file )){
		linecount++;
		if(strstr(linebuff, arg->keyword))
		{
			//printf("Included %s\n", linebuff);
			//Buffer is full
			while(shmregion->resIn[arg->resQIndex] == ((shmregion->resOut[arg->resQIndex] - 1) + BUF_SIZE) % BUF_SIZE );
			//TODO: Use semaphore
			sem_wait(resQSems[arg->resQIndex]);
			shmregion->resArr[arg->resQIndex][shmregion->resIn[arg->resQIndex]] = linecount;			
			shmregion->resIn[arg->resQIndex] = (shmregion->resIn[arg->resQIndex] + 1) % BUF_SIZE;
			sem_post(resQSems[arg->resQIndex]);
		}

	}
	//buffer is full
	while(shmregion->resIn[arg->resQIndex] == ((shmregion->resOut[arg->resQIndex] - 1) + BUF_SIZE) % BUF_SIZE );
	//TODO: Use semaphore
	sem_wait(resQSems[arg->resQIndex]);
	shmregion->resArr[arg->resQIndex][shmregion->resIn[arg->resQIndex]] = -1;	
	shmregion->resIn[arg->resQIndex] = (shmregion->resIn[arg->resQIndex] + 1) % BUF_SIZE;
	sem_post(resQSems[arg->resQIndex]);

	pthread_exit(0);

}



int finish = 0;

int main(int argc, char** argv)
{

	/* For timing experiments
	int count = 0;
	long long int begin, end;
	*/
	signal(SIGINT, sig_handler);
    //Server
    char *shm_name = argv[1];
    inputfilename = argv[2];
    char *sem_name = argv[3];

    //printf("%s %s %s\n", shm_name, inputfilename, sem_name);

    

    //Create shm
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR );
    if(fd == -1)
    {
        printf("Error occured shm_open: %s", strerror(errno));
    }

    //Set size of shm
    if(ftruncate(fd, sizeof(struct region)) == -1)
    {
        printf("Error occured ftruncate: %s", strerror(errno));
    }

    //Get pointer of shm region
    ptr = mmap(NULL, sizeof(struct region), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED)
    {
        printf("Error occured mmap: %s", strerror(errno));
    }
    //Get struct pointer
    struct region *shmregion = (struct region *) ptr;
    char semName[140];

    
    for(int i = 0; i < MAX_NUM_OF_CLIENTS; i++)
    {
    	shmregion->queue_state[i] = 0;

    	//Create semaphore for result queues
    	sprintf(semName, "%s%s%d", sem_name, SEMAPHORE_RESULT_SUFFIX,i);
		resQSems[i] = sem_open(semName, O_CREAT | O_EXCL, S_IWUSR | S_IRUSR, 10);
		//printf("Sem state: %s", strerror(errno));
		sem_init(resQSems[i], 1, 1);
		//printf("Sem state2: %s", strerror(errno));

    }
    for(int i = 0; i < BUF_SIZE; i++)
    {
    	shmregion->resIn[i] = 0;
    	shmregion->resOut[i] = 0;
    	
    }

	shmregion->queue_state[MAX_NUM_OF_CLIENTS] = -1;
	shmregion->reqIn = 0;
	shmregion->reqOut = 0;
    //shmregion->a = 17;
     
    sprintf(semName, "%s%s", sem_name, SEMAPHORE_CLIENT_SUFFIX);

    //Create semaphore for clients
    sem_t *semClient = sem_open(semName, O_CREAT | O_EXCL, S_IWUSR | S_IRUSR, 10);
	//printf("Sem state: %s", strerror(errno));
    sem_init(semClient, 1, 1);
	//printf("Sem state2: %s", strerror(errno));


	sprintf(semName, "%s%s", sem_name, SEMAPHORE_REQUEST_SUFFIX);
	sem_t *semRequest = sem_open(semName, O_CREAT | O_EXCL, S_IWUSR | S_IRUSR, 10);
	//printf("Sem state req: %s", strerror(errno));
    sem_init(semRequest, 1, 1);
	//printf("Sem state2 req: %s", strerror(errno));



    //printf("Waiting for request\n");
    while(finish == 0)
    {
    	while(shmregion->reqIn == shmregion->reqOut && finish == 0);
    	if(finish == 1)
    		break;
    	//printf("GOT request\n");
    	sem_wait(semRequest); 
    	
    	/*for timing Experiments
    	if( count == 0) {
			gettimeofday(&tv, NULL);
			begin = ((long long int) tv.tv_sec) * 1000000ll + (long long int) tv.tv_usec;
		}  
		count++;
	*/ 	
    	int curIndex = shmregion->reqOut;
    	shmregion->reqOut = (shmregion->reqOut + 1) % MAX_NUM_OF_CLIENTS;
    	sem_post(semRequest);
    	
    	//Create a thread for process
    	//char *reqWord = shmregion->reqArr[curIndex];  

    	//printf("Request detail: \"%s\" %d\n", shmregion->reqArr[curIndex], curIndex);

		struct thread_arg *tArg = (struct thread_arg *) malloc(sizeof(struct thread_arg));
    	strcpy(tArg->keyword, shmregion->reqArr[curIndex]);    	
    	tArg->resQIndex = shmregion->reqQueArr[curIndex];// shmregion->reqQueArr[curIndex];

    	//, shmregion->reqQueArr[curIndex]);
    	
    	pthread_t tid;
    	pthread_attr_t attr;

    	pthread_attr_init(&attr);    	
    	
    	
    	pthread_create(&tid, &attr, runner, tArg);
    	
    	pthread_join(tid, NULL);
	
	/* Timing Experiments	
	if( count == 10) {
		gettimeofday(&tv, NULL);
		end = ((long long int) tv.tv_sec) * 1000000ll + (long long int) tv.tv_usec;
		printf("Time elapsed: %d\n", (int)(end-begin));
		count = 0;
	}
	*/
    	 
    	// = (curIndex + 1) / BUF_SIZE;
    	//printf("Word: %s\n", reqWord);
    	
    	

 

    }
    //Unlink shm
    shm_unlink(shm_name);
    sprintf(semName, "%s%s", sem_name, SEMAPHORE_REQUEST_SUFFIX);
    sem_unlink(semName);
    sprintf(semName, "%s%s", sem_name, SEMAPHORE_CLIENT_SUFFIX);
    sem_unlink(semName);
    for(int i = 0; i < MAX_NUM_OF_CLIENTS; i++)
    {
    	//Unlink semaphores for result queues
    	sprintf(semName, "%s%s%d", sem_name, SEMAPHORE_RESULT_SUFFIX,i);
    	sem_unlink(semName);
    }

}


void sig_handler(int sig)
{	
	//printf("SIGINT CALLED\n");
	finish = 1;	
	
}
