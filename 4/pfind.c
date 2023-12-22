/*********************************Some Explainations about the code***************************************/
/*
                                                                    id 208685784
				In this assignment we are implementing program that searches a directory tree for
				files whose name matches some search term. The program receives a directory D and a
				search term T, and finds every file in D’s directory tree whose name contains T.
				The program parallelizes its work using threads. Specifically, individual directories
				are searched by different threads.
				in order to gain this purpuse we used two strucs in order to create nodes and FIFO 
				queues. In addition, included <stdatomic.h>, so we can use atomic.int for globals.
				the following funcs are divided into 
				1) queues funcs - to gain the capability to use queues and its structs
				2) threads funcs - for assingment demands. within it, there is important func 
				called "p_search" that implement the flow of the search as described.
				we will check using "access()" and "stat()" the directories given and change the status 
				exit accordingly if not. In addition printing will be printed to the screen in case of error
				using fprintf which is thread-safe
				3) global funcs - to initialize globals and several arrays
				
																										*/
/********************************************************************************************************/

/*********************************************Includes****************************************************/
#include <stdatomic.h>
#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
/*********************************************************************************************************/

/*********************************************Defines*****************************************************/
#define SUCCESS 0
#define TRUE 1
#define FALSE -1
/*********************************************************************************************************/

/*********************************************Structures**********************************************/
typedef struct node_thread {
	struct node_thread *next;
	struct node_thread *prev;
	void *flag;
} node_thread;

typedef struct thread_queue {
	struct node_thread *head;
	struct node_thread *tail;
} thread_queue;

/*********************************************************************************************************/

/*********************************************Fifo Queue funcs********************************************/

thread_queue *queue_initialize() {
	/*in  case of failure return false, and an empty queue with success*/
	thread_queue* queue = (thread_queue*)malloc(sizeof(thread_queue));
	if (queue == NULL) {
		return NULL;
	}
	queue->head = NULL;
	queue->tail = NULL;
	return queue;
}

int queue_is_empty(thread_queue *queue) {
	return queue->head == NULL;
}

void *remove_from_queue(thread_queue *queue) {
	/*takes the value of queue's first elements*/
	node_thread *curr;
	void *value;

	if (queue_is_empty(queue)) {
		return NULL;
	}
	curr = queue->head;
	value = curr->flag;
	queue->head = curr->prev;
	if (queue_is_empty(queue)) {
		queue->tail = NULL;
	}
	return value;
}

int add_to_queue(thread_queue *queue, void *flag) {
	/*adding element to queue and return success/failure*/
	node_thread *curr = (node_thread *)malloc(sizeof(node_thread));
	if (curr == NULL) {
		return FALSE;
	}
	curr->flag = flag;
	curr->next = queue->tail;
	curr->prev = NULL;
	if (queue_is_empty(queue)) {
		queue->head = curr;
	}
	else {
		(queue->tail)->prev = curr;
	}
	queue->tail = curr;
	return SUCCESS;
}

/*********************************************************************************************************/

/*********************************************Globals*****************************************************/
atomic_int waited_threads;
atomic_int current_threads;
atomic_int g_num_of_files;
atomic_int is_finished;
atomic_int num_of_added_threads;
atomic_int status_exit;

mtx_t mutex_p;
mtx_t mutex_c;
mtx_t mutex_s;

thrd_t* arr_of_threads;
cnd_t* arr_of_cnds;
cnd_t state_zero;

thread_queue* q_for_pths;
thread_queue* q_for_cnd;

int g_threads;
char* search_term;

/*********************************************************************************************************/

/*********************************************Threads Routine funcs***************************************/

void is_error() {
	/*raises and error*/
	status_exit = TRUE;
	current_threads--;
}

void thread_wakeup() {
	/*wakes up the following sleeping*/
	long curr = (long)remove_from_queue(q_for_cnd);
	cnd_signal(&arr_of_cnds[curr]);
}

void threads_exit() {
	/* as described, we will wake every threads and exit with success */
	is_finished = TRUE;
	mtx_lock(&mutex_c);
	while (!queue_is_empty(q_for_cnd)) {
		thread_wakeup();
	}
	mtx_unlock(&mutex_c);
	thrd_exit(SUCCESS);
}

int task_waiting(long id_of_thread) {
	/* this function checks whether the threads are sleeping
	return success/failure properly  */
	if (current_threads - 1 == waited_threads && queue_is_empty(q_for_pths)) {
		mtx_unlock(&mutex_p);
		threads_exit();
	}
	waited_threads++;
	mtx_lock(&mutex_c);
	if (add_to_queue(q_for_cnd, (void *)id_of_thread) < 0) {
		mtx_unlock(&mutex_c);
		fprintf(stderr, "Error in task_waiting: add_to_queue failed\n");
		is_error();
		return FALSE;
	}
	mtx_unlock(&mutex_c);
	cnd_wait(&arr_of_cnds[id_of_thread], &mutex_p);
	waited_threads--;
	return SUCCESS;
}

char *p_updation(char *path, char *file_name) {
	/*path updation with given path and filename*/
	char *curr_p = (char *)malloc(PATH_MAX * sizeof(char));
	if (curr_p == NULL)
	{
		return NULL;
	}
	/* create new_path */
	strcpy(curr_p, path);
    strcat(curr_p, "/");
    strcat(curr_p, file_name);
    return curr_p;
}

int p_search(char *path, long id_of_thread) {
	/*the flow of search as described in the assignment and above.
     if an error occurs - we will not exit but when we will eventually exit
     with status 1 as described. if not so - with status 0*/
	struct dirent *dir_ent;
	DIR *dir;
	struct stat buf;
	char *file_name;
	char *curr_p;
	int is_dir = 0;
    dir = opendir(path);

	/*dir open*/
    /*access is used as described in the assignment as syscall and raises properly*/
	if (dir == NULL   || ((access(path, R_OK))) ||((access(path, X_OK)))) {
		if (errno != EACCES) {
			fprintf(stderr, "Error in p_search: opendir failed on path %s\n", path);
			is_error();
			return FALSE;
		}
		else {
			printf("Directory %s: Permission denied.\n", path);
			return SUCCESS;
		}
	}

	while ((dir_ent = readdir(dir)) != NULL) {
		/*search*/
		file_name = dir_ent->d_name;
		if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
			/*ignore as described in readdir()*/
            		continue;
        	}
		curr_p = p_updation(path, file_name);       
		if (curr_p == NULL) {
			fprintf(stderr, "Error in p_updation: malloc failed\n");
            /*decrease and status 1*/
			is_error();		
		}
		if (stat(curr_p, &buf) < 0) {
			/*checks with stat*/
			if (errno != ENOENT) {
			closedir(dir);
            fprintf(stderr, "Error in p_search: stat failed\n");
			}
			else {
			is_dir = 0;
            /*symbolic link edge case- status exit will set to 1*/
            status_exit = 1;
			}
		}
		else {
			is_dir = S_ISDIR(buf.st_mode);
		}
		if (is_dir)
		{ 
			/* add to q_for_pths */
			mtx_lock(&mutex_p);
			if (add_to_queue(q_for_pths, curr_p) < 0)
			{
				closedir(dir);
				mtx_unlock(&mutex_p);
				fprintf(stderr, "Error in p_search: add_to_queue failed\n");
                /*decrease and status 1*/
				is_error();
			}
			mtx_unlock(&mutex_p);
			/* wakeup*/
			mtx_lock(&mutex_c);
			if (!queue_is_empty(q_for_cnd))
			{
				thread_wakeup();
			}
			mtx_unlock(&mutex_c);
		}
		
		/* the given entry is file */
		else if (strstr(file_name, search_term) != NULL)
		{/* contains search_term */
			printf("%s\n", curr_p);
			g_num_of_files++;
		}
	}
	closedir(dir);
    /*status exit returned as if changed/not during the flow*/
	return status_exit;
}


int functionality_of_threads(void *id_of_thread) {
	/* returns success or failure */
	char *curr_path;
	long id = (long)id_of_thread;
	mtx_lock(&mutex_s);
	num_of_added_threads++;
	cnd_wait(&state_zero, &mutex_s);
	mtx_unlock(&mutex_s);
	while (TRUE) {
		mtx_lock(&mutex_p);
		while (queue_is_empty(q_for_pths)) {
			if (is_finished == 1) {
				mtx_unlock(&mutex_p);
				thrd_exit(SUCCESS);
			}
			if (task_waiting(id) < 0) {
				return FALSE;
			}
		}
		curr_path = (char *)remove_from_queue(q_for_pths);
        if (access(curr_path , R_OK)){
            /*first access flag*/
            if (access(curr_path, X_OK)){
                /*second access flag*/
            }
        }
		mtx_unlock(&mutex_p);
		if (p_search(curr_path, id) < 0) {
			return FALSE;
		}
	}
	return SUCCESS; 
}

void threads_initialize() {
	/*this function initializes the globals for arr_of_threads*/
    long index = 0;
	arr_of_threads = (thrd_t *)malloc(g_threads * sizeof(thrd_t));
	if (arr_of_threads == NULL) {
		fprintf(stderr, "Error in main: error in malloc\n");
		exit(TRUE);
	}
	while (index < g_threads)
    {
		thrd_create(&arr_of_threads[index], functionality_of_threads, (void *)index);
        ++index;
	}
	/* wakeup all threads after created */
	while (TRUE) {
		mtx_lock(&mutex_s);
		if (num_of_added_threads == g_threads) {
			cnd_broadcast(&state_zero);
			mtx_unlock(&mutex_s);
			break;
		}
		mtx_unlock(&mutex_s);
	}
}

void initialize_arr_of_cnds() {
	/*this function initializes the globals for arr_of_cnds*/
	int index = 0;
	arr_of_cnds = (cnd_t *)malloc(g_threads * sizeof(cnd_t));
	if (arr_of_cnds == NULL) {
		fprintf(stderr, "Error in main: error in malloc\n");
		exit(TRUE);
	}
	while(index< g_threads)
	{
		cnd_init(&arr_of_cnds[index]);
		index++;
	}
}

void initialize_globals(char* p_root) {
	/*this function initializes the globals*/
	num_of_added_threads = 0;
	waited_threads = 0;
	current_threads = g_threads;
	g_num_of_files = 0;
	is_finished = 0;
	status_exit = 0;

	mtx_init(&mutex_p, mtx_plain);
	mtx_init(&mutex_c, mtx_plain);
	mtx_init(&mutex_s, mtx_plain);

	q_for_pths = queue_initialize();
	if (q_for_pths == NULL) {
		fprintf(stderr, "Error in main: queue_initialize failed\n");
		exit(TRUE);
	}
	if (add_to_queue(q_for_pths, p_root) < 0) {
		fprintf(stderr, "Error in main: add_to_queue failed\n");
		exit(TRUE);
	}
	q_for_cnd = queue_initialize();
	if (q_for_cnd == NULL) {
		fprintf(stderr, "Error in main: queue_initialize failed\n");
		exit(1);
	}

	initialize_arr_of_cnds();
	threads_initialize();
}


/*********************************************************************************************************/

/*************************************************Main****************************************************/
int main(int argc, char *argv[]) {
	/*main func*/
	char *p_root;
	DIR *dir;
	int index = 0;

	if (argc != 4) {
		fprintf(stderr, "Error in main: incorrect number of arguments\n");
		exit(TRUE);
	}
	
	p_root = argv[1];
	search_term = argv[2];
	g_threads = atoi(argv[3]);
	
    if (access(p_root , R_OK)){
        /*first access flag*/
        if (access(p_root, X_OK)){
            /*second access flag*/
        dir = opendir(p_root);
	if (dir == NULL) {
		fprintf(stderr, "Error in main: root directory can't be searched\n");
		exit(TRUE);
	}
	closedir(dir);
    }
    }
	initialize_globals(p_root);
	while (index < g_threads)
	{
		/*waiting for all to finish*/
    	if (thrd_join(arr_of_threads[index], NULL) == thrd_error) {
    			fprintf(stderr, "Error in main: thrd_join failed\n");
			exit(TRUE);
		}
		index++;
	}
	
	mtx_destroy(&mutex_p);
	mtx_destroy(&mutex_c);
	mtx_destroy(&mutex_s);
	cnd_destroy(&state_zero);
	for (int i = 0; i < g_threads; ++i) {
        /*cnd destroy g_threads*/
		cnd_destroy(&arr_of_cnds[i]);
	}
	printf("Done searching, found %d files\n", g_num_of_files);
	exit(status_exit);
}
/*********************************************************************************************************/
