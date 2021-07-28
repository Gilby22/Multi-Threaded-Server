#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#define MAX_THREADS 100
#define MAX_queue_len 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024
#define FILE_SIZE 100000
#define MAX_LOG 100

/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:
typedef struct request_queue {
   int fd;
   char *request;
} request_t;

typedef struct cache_entry {
    int len;
    char *request;
    char *content;
} cache_entry_t;

// Globals for shared resources
request_t req_queue[MAX_queue_len];
int rq_next_in = 0;
int rq_next_out = 0;
int items_in_queue = 0;
int qlen = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_content = PTHREAD_COND_INITIALIZER;
pthread_mutex_t wait_slot = PTHREAD_COND_INITIALIZER;
int log_fd;

// Add a request to the request queue
void addRequest(int fd_, char *file) {
  // Lock mutex
  pthread_mutex_lock(&mtx);
  // Block if buffer is full
  while(items_in_queue == qlen) {
    pthread_cond_wait(&wait_content, &mtx);
  }
  req_queue[rq_next_in].request = (char *) malloc(sizeof(char) * (BUFF_SIZE + 1));
  strcpy(req_queue[rq_next_in].request, file);
  req_queue[rq_next_in].fd = fd_;
  //printf("Stored: %s \n", req_queue[rq_next_in].request);
  // Increment next in, and wrap around at end
  rq_next_in = (rq_next_in + 1) % MAX_queue_len;
  items_in_queue++;
  // Send conditional variable signal
  pthread_cond_signal(&wait_slot);
  // Unlock mutex
  pthread_mutex_unlock(&mtx);
}

// Remove a request from the request queue
request_t removeRequest() {
    // Lock mutex
    pthread_mutex_lock(&mtx);
    // If the queue is empty, wait
    while(rq_next_in == rq_next_out) {
      pthread_cond_wait(&wait_slot, &mtx);
    }
    request_t placeHolder;
    // Set placeholder
    placeHolder.fd = req_queue[rq_next_out].fd;
    placeHolder.request = (char *) malloc(sizeof(char) * BUFF_SIZE);
    // Copy next out queue to placeHolder
    strcpy(placeHolder.request, req_queue[rq_next_out].request);
    //printf("Removed: %s \n", req_queue[rq_next_out].request);
    // Reset the variables?
    req_queue[rq_next_out].fd = 0;
    free(req_queue[rq_next_out].request);
    // Increment rq_next_out
    rq_next_out = (rq_next_out + 1 ) % MAX_queue_len;
    items_in_queue--;
    // Send signal and unlock
    pthread_cond_signal(&wait_content);
    pthread_mutex_unlock(&mtx);
    //printf("Placeholder: %s \n", placeHolder.request);
    return placeHolder;
}

/* ******************** Dynamic Pool Code  [Extra Credit A] **********************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg) {
  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
  }
}
/**********************************************************************************/

/* ************************ Cache Code [Extra Credit B] **************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  /// return the index if the request is present in the cache
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memory when adding or replacing cache entries
}

// clear the memory allocated to the cache
void deleteCache(){
  // De-allocate/free the cache memory
}

// Function to initialize the cache
void initCache(){
  // Allocating memory and initializing the cache array
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char * mybuf) {
  // Should return the content type based on the file type in the request
  // (See Section 5 in Project description for more details)
  // Strstr returns NULL pointer if the substring is not found in mybuf
  if(strstr(mybuf, ".html") != NULL) {
    return "text/html";
  }
  else if(strstr(mybuf, ".jpg") != NULL) {
    return "image/jpeg";
  }
  else if(strstr(mybuf, ".gif") != NULL) {
    return "image/gif";
  }
  else if(strstr(mybuf, ".txt")) {
    return "text/plain";
  }
  else {
    return "error";
  }
}

// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
int readFromDisk(request_t queue_entry, char *buf) {
    // Open and read the contents of file given the request
    int bytes = 0;
    char fileName[BUFF_SIZE];
    strcpy(fileName, ".");
    strcat(fileName, queue_entry.request);
    queue_entry.fd = open(fileName, O_RDONLY, 0777);
    if (queue_entry.fd < 0) {
			printf("ERROR: Cannot open the file %s \n", fileName);
      return -1;
	  }
    // Seek to end of file to get size
    bytes = lseek(queue_entry.fd, 0, SEEK_END);
    //printf("Bytes: %d \n", bytes);
    // Seek back to start of file
    lseek(queue_entry.fd, 0, SEEK_SET);
    // Read entire file in with read
    read(queue_entry.fd, buf, bytes);
    // close the file and return
    close(queue_entry.fd);
    return bytes;
}

/**
	@brief Function to carefully log each request to a file called "web_server_log" and also to the terminal
	@param threadID - an integer from 0 to num_workers -1 indicating thread ID of request handling worker
	@param fd       - the file descriptor given to you by accept_connection() for this request
	@param reqNum   - total number of requests a specific worker thread has handled so far, including current request
	@param request  - the filename buffer filled in by the get request function
	@param bytes    - the number of bytes returned by a successful request
	@param error    - the error string returned by return error if an error occurred
**/
void requestLog(int threadID, int fd, int reqNum, char *request, int bytes, char *error) {
  char lBracket[2] = "[";
  char rBracket[2] = "]";
  //char *logBuf = (char *) malloc(sizeof(char) * MAX_LOG);
  char logBuf[MAX_LOG] = "";
  char dummyInt[10];
  int endOf = 0;
  //memset(logBuf, '\0', MAX_LOG);
  // [ThreadID]
  strcpy(logBuf, lBracket); // 1
  sprintf(dummyInt, "%d", threadID);
  strcat(logBuf, dummyInt);
  strcat(logBuf, rBracket);
  // [Request num]
  strcat(logBuf, lBracket);
  sprintf(dummyInt, "%d", reqNum);
  strcat(logBuf, dummyInt);
  strcat(logBuf, rBracket);
  // [fd]
  strcat(logBuf, lBracket);
  sprintf(dummyInt, "%d", fd);
  strcat(logBuf, dummyInt);
  strcat(logBuf, rBracket);
  // [File]
  strcat(logBuf, lBracket);
  strcat(logBuf, request);
  strcat(logBuf, rBracket);
  // [error]
  if(bytes == 0) {
    strcat(logBuf, lBracket);
    strcat(logBuf, error);
    strcat(logBuf, rBracket);
  }
  // [bytes]
  else {
    strcat(logBuf, lBracket);
    sprintf(dummyInt, "%d", bytes);
    strcat(logBuf, dummyInt);
    strcat(logBuf, rBracket);
  }
  strcat(logBuf, "\n");
  strcat(logBuf, "\0");
  for(int i = 0; i < sizeof(logBuf); i++) {
    if(logBuf[i] == '\0') {
      endOf = i;
      break;
    }
  }
  printf("%s \n", logBuf);
  write(log_fd, logBuf, endOf);
}

/**********************************************************************************/

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
  int input = (int) arg;
  int fd;
  char *file = (char *) malloc(sizeof(char) * BUFF_SIZE);

  while (1) {

    // Accept client connection
    fd = accept_connection();
    // Ignore request if negative return value
    if(fd >= 0) {
      // Get request from the client
      if(get_request(fd, file) < 0) {
        printf("Failed to get request\n");
      }
      // If the request succeeds, add request to the queue
      else {
        addRequest(fd, file);
      }
    }
   }
   return NULL;
}



/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  int input = (int) arg;
  request_t entry;
  char fileType[20];
  int numbytes = 0;
  char *buf = (char *) malloc(sizeof(char) * FILE_SIZE);
  int reqNum = 0;
  char errorBuf[50];

  while (1) {
    // Get the request from the queue
    entry = removeRequest();
    reqNum++;
    strcpy(fileType, getContentType(entry.request));
    // Get the data from the disk or the cache (extra credit B)
    numbytes = readFromDisk(entry, buf);

    // Return error for bad content type
    if(strcmp(fileType, "error") == 0) {
      strcpy(errorBuf, "Error: bad file type");
      if(return_error(entry.fd, errorBuf) == 0) {
        printf("Error recieved: %s \n", errorBuf);
        // Log error
        requestLog(input, reqNum, entry.fd, entry.request, 0, errorBuf);
      }
    }
    // Return error if the file failed to open
    if(numbytes == -1) {
      strcpy(errorBuf, "Error: file failed to open or doesn't exist");
      if(return_error(entry.fd, errorBuf) == 0) {
        printf("Error recieved: %s \n", errorBuf);
        // Log error
        requestLog(input, reqNum, entry.fd, entry.request, 0, errorBuf);
      }
    }
    // return the result
    if(return_result(entry.fd, fileType, buf, numbytes) == 0) {
      // Log a success
      requestLog(input, reqNum, entry.fd, entry.request, numbytes, "");
    }
  }
  free(buf);
  return NULL;
}

/**********************************************************************************/
// Function to gracefully terminate the server when it gets a SIGINT (^C) signal.
static void handleInterrupt(int signo) {
	// Print the number of pending requests in the request queue
	printf("the number of pending requests in the request queue: %d \n", items_in_queue);
	// close logfile
    close(log_fd);
    exit(1);
}

int main(int argc, char **argv) {
  int port;
  char path[BUFF_SIZE];
  int num_dispatcher;
  int num_workers;
  bool dynamic_flag;
  int cache_entries;

  struct sigaction act;

  pthread_t worker_threads[MAX_THREADS];
  pthread_t dispatcher_threads[MAX_THREADS];

  // Error check on number of arguments
  if(argc != 8){
    printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
    return -1;
  }

  // Get the input args
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_workers = atoi(argv[4]);
  dynamic_flag = atoi(argv[5]);
  qlen = atoi(argv[6]);
  cache_entries = atoi(argv[7]);

  // Perform error checks on the input arguments
  if(port > 65535 || port < 1025) {
    printf("Input Error: port must be between 1025 - 65535\n");
    exit(-1);
  }

  if(sizeof(argv[2]) > BUFF_SIZE) {
    printf("Input Error: path can not exceed 1024 characters\n");
    exit(-1);
  }

  if(num_dispatcher > MAX_THREADS) {
    printf("Input Error: dispatchers exceeds max of 100\n");
    exit(-1);
  }

  if(num_workers > MAX_THREADS) {
    printf("Input Error: workers exceeds max of 100\n");
    exit(-1);
  }

  if(qlen > MAX_queue_len) {
    printf("Input Error: max queue size of 100\n");
    exit(-1);
  }

  if(cache_entries > MAX_CE) {
    printf("Input Error: max cache size of 100\n");
    exit(-1);
  }

  char *LOG_FILE = "web_server_log.txt";
  // Open log file
  log_fd = open(LOG_FILE, O_CREAT | O_WRONLY, 0777);
	if (log_fd < 0) {
			printf("ERROR: Cannot open the file %s\n", LOG_FILE);
			exit(-1);
	} 

  // Change SIGINT action for grace termination
  act.sa_handler = handleInterrupt;       
	act.sa_flags = 0;
	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) {
		perror("Failed to set SIGINT handler");
		exit(-1);
	}
  // Change the current working directory to server root directory
  // !!! Needs to use input path
  if(chdir(path) < 0) {
    perror("Failed to change directory\n");
    exit(-1);
  }
  // Initialize cache (extra credit B)

  // Start the server
  init(port);
  // Create dispatcher and worker threads (all threads should be detachable)
  for(int i = 0; i < num_dispatcher; i++) {
    // Create dispatcher thread
    if(pthread_create(&dispatcher_threads[i], NULL, dispatch, (void *)i) < 0) {
      printf("Error: failed to create dispatcher thread %d \n", i);
    }
    // Detatch thread
    if(pthread_detach(dispatcher_threads[i]) < 0) {
      printf("Error: failed to detach dispatcher thread %d \n", i);
    }
  }

  for(int i = 0; i < num_workers; i++) {
    // Create worker thread
    if(pthread_create(&worker_threads[i], NULL, worker, (void *)i) < 0) {
      printf("Error: failed to create worker thread %d \n", i);
    }
    // Detatch thread
    if(pthread_detach(worker_threads[i]) < 0) {
      printf("Error: failed to detach worker thread %d \n", i);
    }
  }
  // Create dynamic pool manager thread (extra credit A)
    // Remove cache (extra credit B)
  pthread_exit(NULL);

  return 0;
}
