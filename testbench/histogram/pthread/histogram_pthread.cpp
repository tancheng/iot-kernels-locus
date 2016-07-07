/* Lab 1: Histrogram generation 
 * compile as follows: gcc -o histogram histogram.c -std=c99 -lpthread -lm
 * Histogramerific!
 * reset && gcc -g -o histogramerific.wow histogram.c -std=c99 -lpthread -lm && ./histogramerific.wow 500000000
 */
 
#define _XOPEN_SOURCE 600 //FO BARRIERS
 
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <unistd.h>
 
#define DUMP

#ifdef DUMP
#include <m5op.h>
#endif

#ifndef DUMP
#include <pthread.h>
#endif

using namespace std;

typedef int data_t;

void run_test(int);
void compute_using_pthreads(data_t *, int, int);
 
int HISTOGRAM_SIZE;
int NUM_THREADS;
int DATA_LEN;
 
//Global thread list
pthread_t *thread;

//Global list of individual thread local workspaces
int ** local_histograms;
//Also keep generated input data as global
data_t * input_data;
//Also make the variable for returning global 
//since 'compute' functions do not return values
int * histogram_using_pthreads;

pthread_barrier_t barr;
pthread_mutex_t lock_max;
int interval[16];
pthread_mutex_t lock_min;
int test_max = 0;
int test_min = 0;
 
float dmin = 10000;
float dmax = -1;

/**
 * Get the command line arguments and change global variables.
 * We currently send 20 elements each for four cores
 * We can have 10 buckets for now
 */
void get_options(int argc, char **argv)
{
    int op;
    DATA_LEN = 2000;
    HISTOGRAM_SIZE = 10;
    NUM_THREADS = 16;
    /*
        while ((op = getopt(argc, argv, "hn:b:t:")) != -1) {
                switch (op) {
                        case 'n':
                                DATA_LEN = atoi(optarg);
                                break;
                        case 'b':
                                HISTOGRAM_SIZE = atoi(optarg);
                                break;
			case 't':
				NUM_THREADS = atoi(optarg);
                        case 'h':
                        default:
                                break;
                }
        }
    */
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int 
main( int argc, char** argv) 
{
	get_options(argc, argv);

	thread = (pthread_t*)malloc(sizeof(pthread_t) * NUM_THREADS);
	local_histograms = (int**)malloc(sizeof(int*) * NUM_THREADS);
	histogram_using_pthreads = (int *)malloc(sizeof(int) * HISTOGRAM_SIZE); // Space to store histogram generated by the CPU

	//printf("done initializing. calling run\n");

	run_test(DATA_LEN * NUM_THREADS);

	free(thread);
	free(local_histograms);
	free(histogram_using_pthreads);

	return 0;
}
 
////////////////////////////////////////////////////////////////////////////////
//! Generate the histogram on Single Threaded CPU and Then PTHREADS and Check for Correctness
////////////////////////////////////////////////////////////////////////////////
void run_test(int num_elements) 
{
	int diff;
	int i; 
 
	// Allocate memory for the input data
	int size = sizeof(data_t) * num_elements;
	input_data = (data_t *)malloc(size); //Removed 'int *'

	//printf("done allocating input data\n");
 
	// Compute the histogram using pthreads. The result histogram should be stored on the histogram_using_pthreads array
	compute_using_pthreads(input_data, num_elements, HISTOGRAM_SIZE);
  
	// cleanup memory
//	free(input_data);
}


void get_data(data_t *local_min, data_t *local_max, int id)
{
	//printf("Inside get data id %d\n", id);
	int i;
	data_t temp;
        char filename[256] = {"input_histogram/input_"};
        int base_fname_length = 22;
        char rank_str[20];
        sprintf(rank_str, "%d", id);
        for(i = 0; rank_str[i] != '\0'; i++)
	{
		//printf("inside loop iter %d\n", i);
                filename[base_fname_length + i] = rank_str[i];
	}
	//printf("after for loop base %d i %d\n", base_fname_length, i);
        filename[base_fname_length + i] = '\0';

	//printf("fname %s\n", filename);

	//printf("reading from file\n");

        FILE *fp = fopen(filename, "r");
	int len = DATA_LEN;
	int count = 0;
	
	for(count = 0; count < DATA_LEN; count++)
        {
                fscanf(fp, "%d", &temp);
                if (temp < *local_min)
                        *local_min = temp;
                else if (temp > *local_max)
                        *local_max = temp;

		//printf("temp = %f local_min %f local_max %f\n", temp, *local_min, *local_max);

		input_data[ DATA_LEN*id + count] = temp;
        }

        fclose(fp);
        //printf("Filename of ID %d is %s max %f min %f\n",id, filename, *local_max, *local_min);
}
 
void * thread_function(void * thread_id)
{
	//Get id, gives warning but whatevski
	int *id = (int*)thread_id;
    int i;

 	//Allocate a local histogram to record results in
	int * my_local_histogram_not_yours_man = (int*)malloc(sizeof(int) * HISTOGRAM_SIZE);

	//printf("before accessing loc_hist\n"); 
	//Put this info in the global table
	local_histograms[*id] = my_local_histogram_not_yours_man;
 
	//printf("after accessing loc_hist\n");
	//Initialize that localgram yo!
	for(i = 0; i < HISTOGRAM_SIZE; i++)
	{
		//printf("acc id %d\n", i);
		my_local_histogram_not_yours_man[i] = 0;
	}

	//printf("after accessing my_loc_hist\n");
	data_t local_max = -1, local_min = 10000;
	get_data(&local_min, &local_max, *id);

    //printf("\nhistogram thread %d finish init...\n", *id);

	pthread_barrier_wait(&barr);

    if(*id == 0)
    {
#ifdef DUMP
    m5_dump_stats(0, 0);
    m5_reset_stats(0, 0);
#endif
    }
//while(test_max==1);
//test_max = 1;
	pthread_mutex_lock(&lock_max);
	if(dmax < local_max)
		dmax = local_max;
	pthread_mutex_unlock(&lock_max);
//test_max = 0;

//while(test_min==1);
//test_min = 1;
	pthread_mutex_lock(&lock_min);
	if(dmin > local_min)
		dmin = local_min;
	pthread_mutex_unlock(&lock_min);
//test_min = 0;
	//printf("unlock id=%d\n", *id);
//	if(*id == 0)
//		printf("global max = %f global min = %f\n", dmax, dmin);

 
	//Do my histogram work
	//Size of my chunk is determined by:
	//number of threads
	//size of data
	//thread id
	//DIVVVYY THAT ARRAAY UUUPP
	int chunk_size = DATA_LEN;
	int start_bound = (*id) * chunk_size;
	int end_bound = start_bound + chunk_size - 1;
	////TAKE CARE OF THAT END CASE
	if((*id)==NUM_THREADS-1)
	{
		end_bound = DATA_LEN*NUM_THREADS-1;
	}
 
    int delt = (dmax - dmin);
    int r;
//	pthread_barrier_wait(&barr);

	////Actual WORK
	////Bin the elements

	for(i = start_bound; i <= end_bound; i++)
	{	//Curly Braces for President!
		//input_data[i] = floorf((HISTOGRAM_SIZE - 1) * (rand()/(float)RAND_MAX));
        r = HISTOGRAM_SIZE * (input_data[i] - dmin) / delt;
        //int b = (int)floor(r);
        if (r == HISTOGRAM_SIZE)
            r--;
        if (r >= 0 && r < HISTOGRAM_SIZE)
            my_local_histogram_not_yours_man[r]++;
        else
        {
            printf("RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRROR r %d\n", r);
            exit(0);
        }
	} 
    
	pthread_barrier_wait(&barr);    
}
 
// Write the function to compute the histogram using pthreads
void compute_using_pthreads(data_t *input_data, int num_elements, int histogram_size)
{
	pthread_barrier_init(&barr, NULL, NUM_THREADS);
	pthread_mutex_init(&lock_min, NULL);
	pthread_mutex_init(&lock_max, NULL);

    int tid[NUM_THREADS];

	//Start threads
	int thread_id, i;
	for(thread_id=0; thread_id < NUM_THREADS-1; thread_id++)
	{
        tid[thread_id] = thread_id;
		pthread_create( &thread[thread_id], NULL, thread_function, (void*)(tid+thread_id));
	}

    tid[NUM_THREADS-1] = NUM_THREADS-1;
    thread_function((void*) (tid+NUM_THREADS-1));

	for(thread_id=0; thread_id < NUM_THREADS; thread_id++)
	{
		//pthread_join(thread[thread_id], NULL);

		//Since that thread just finished, merge it's data into mine.
		//Look up threads storage location
		int * curr_threads_histogram = local_histograms[thread_id];

		//Add the results of the other histogram to my own
		for(i = 0; i < HISTOGRAM_SIZE; i++)
			histogram_using_pthreads[i] += curr_threads_histogram[i];
	}

#ifdef DUMP
    m5_dump_stats(0, 0);
    m5_reset_stats(0, 0);
#endif

	for(i =0; i < HISTOGRAM_SIZE; i++)
		printf("%d ",histogram_using_pthreads[i]);
	printf("\n");
    
}