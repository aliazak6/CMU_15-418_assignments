
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <error.h>
#include <limits.h>
#include <pthread.h>
#include <omp.h>
#include <time.h>
// TODO: You may find this helpful.
//#include <omp.h>

/*
________________________________________________

 FRIENDLY ADVICE FROM YOUR TAs:
________________________________________________

0) Read over the file, familiarizing yourself
with the provided functions. You are free to
change structs and add additional helper 
functions, but should not modify the
argument parsing or printing mechanisms. 

1) Start by developing a sequential solution.
A parallelized approach is useless here if it 
sacrifices correctness for speed.

2) Run the verify.py script to verify the
correctness of your outputs compared to
our reference solution on smaller test cases.
We also encourage you to compare your results
against the reference manually on larger
test cases. 

NOTE: there may be multiple correct paths 
for the same distance file. We are only
looking for you to produce the correct
distance cost.

3) Start with simple parallelization
techniques that do not require large
restructuring of your code (Hint: openMP
may come in handy at this stage). Be mindful
of not accidentally introducing uninetntional
dependencies within parallel sections of code
that will unwantingly serialize code.

4) If you still do not have enough of a 
performance increase, you may need to 
introduce larger structural changes to
your code to help make it more amendable
to parallelism. Attempt small iterative
changes while still ensuring the correctness
of your code.
________________________________________________
*/ 

#define SYSEXPECT(expr) do { if(!(expr)) { perror(__func__); exit(1); } } while(0)
#define error_exit(fmt, ...) do { fprintf(stderr, "%s error: " fmt, __func__, ##__VA_ARGS__); exit(1); } while(0);

typedef int8_t city_t;

int NCORES = -1;  // TODO: this isn't being used anywhere.
int NCITIES = -1; // number of cities in the file.
int *DIST = NULL; // one dimensional array used as a matrix of size (NCITIES * NCITIES).


typedef struct path_struct_t {
	int cost;         // path cost.
	city_t *path;     // order of city visits (you may start from any city).
} path_t;
path_t *bestPath = NULL;


// set DIST[i,j] to value
inline static void set_dist(int i, int j, int value) {
	assert(value > 0);
	int offset = i * NCITIES + j;
	DIST[offset] = value;
	return;
}

// returns value at DIST[i,j]
inline static int get_dist(int i, int j) {
	int offset = i * NCITIES + j;
	return DIST[offset];
}

// prints results
void wsp_print_result() {
	printf("========== Solution ==========\n");
	printf("Cost: %d\n", bestPath->cost);
	printf("Path: ");
	for(int i = 0; i < NCITIES; i++) {
		if(i == NCITIES-1) printf("%d", bestPath->path[i]);
		else printf("%d -> ", bestPath->path[i]);
	}
	putchar('\n');
	putchar('\n');
	return;
}


path_t wsp_recurse(path_t currentPath,int city_count) {
	int skip_flag = 0;
	int cost = 0;
	int new_cost = 0;
	#pragma omp parallel for firstprivate(cost,new_cost,skip_flag)
	for(int i=0; i<NCITIES; i++) { // loop to iterate over different cities
		cost = 0;
		new_cost = 0;
		/* PART WE DECIDE TO ITERATE OR NOT*/
		skip_flag = 0;
		for(int j=0; j<city_count; j++) { // loop to iterate over cities in current path
			if(currentPath.path[j] == i) { // if city is already in path, skip it
				skip_flag = 1;
				break;
			}
		}
		if(skip_flag == 1) { // if city is already in path, skip it
			continue;
		}
		/*********************************/
		/* PART WE ITERATE OVER*/
		new_cost = get_dist(currentPath.path[city_count-1],i);
		cost = currentPath.cost + new_cost; // add cost of current city to path cost
		if(cost > bestPath->cost){
			continue;
		}else{
			currentPath.path[city_count] = i;
			currentPath.cost = cost;
			if(city_count == NCITIES-1) { // if we have reached the end of the path
				if(cost < bestPath->cost) { // if the cost of the path is less than the best path
					bestPath->cost = cost;
					for(int j=0; j<NCITIES; j++) {
						bestPath->path[j] = currentPath.path[j];
					}
				}
				currentPath.cost-= new_cost;			
			}else{
				currentPath = wsp_recurse(currentPath,city_count+1);
			}
		}
		/*********************************/		
	}
	currentPath.cost-=  get_dist(currentPath.path[city_count-1],currentPath.path[ city_count-2]);
	
	return currentPath;
}	

void wsp_start() {

	// TODO: try finding a better path.
	int cityID = 0;
	for(cityID=0; cityID < NCITIES; cityID++) {
		bestPath->path[cityID] = cityID;
		if(cityID>0) bestPath->cost += get_dist(bestPath->path[cityID-1],bestPath->path[cityID]);
	}
	int root,city_count;
	path_t currentPath ;
	currentPath.path = (city_t*)calloc(NCITIES, sizeof(city_t));
	#pragma omp parallel for default(none) private(currentPath,city_count,root) shared(bestPath,NCITIES)
	for(cityID=0; cityID < NCITIES; cityID++) { // loop to iterate over different root nodes
		currentPath.path = (city_t*)calloc(NCITIES, sizeof(city_t));
		root = cityID; // private
		currentPath.path[0] = root; //private
		currentPath.cost = 0; //private
		city_count = 1; // private
		wsp_recurse(currentPath,city_count);
	}

}

int main(int argc, char **argv) {
	if(argc < 4 || strcmp(argv[1], "-p") != 0) error_exit("Expecting two arguments: -p [processor count] and [file name]\n");
	NCORES = atoi(argv[2]);
	if(NCORES < 1) error_exit("Illegal core count: %d\n", NCORES);
	omp_set_num_threads(NCORES);
	char *filename = argv[3];
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) error_exit("Failed to open input file \"%s\"\n", filename);
	int scan_ret;
	scan_ret = fscanf(fp, "%d", &NCITIES);
	if(scan_ret != 1) error_exit("Failed to read city count\n");
	if(NCITIES < 2) {
		error_exit("Illegal city count: %d\n", NCITIES);
	} 
	// Allocate memory and read the matrix
	DIST = (int*)calloc(NCITIES * NCITIES, sizeof(int));
	SYSEXPECT(DIST != NULL);
	for(int i = 1;i < NCITIES;i++) {
		for(int j = 0;j < i;j++) {
			int t;
			scan_ret = fscanf(fp, "%d", &t);
			if(scan_ret != 1) error_exit("Failed to read dist(%d, %d)\n", i, j);
			set_dist(i, j, t);
			set_dist(j, i, t);
		}
	}
	fclose(fp);
	bestPath = (path_t*)malloc(sizeof(path_t));
	bestPath->cost = 0;
	bestPath->path = (city_t*)calloc(NCITIES, sizeof(city_t));
	double before = omp_get_wtime();
	wsp_start();
	double after = omp_get_wtime();
	double delta_ms = (after - before) * 1000.0 + (after - before) / 1000000.0;
	putchar('\n');
	printf("============ Time ============\n");
	printf("Time: %.3f ms (%.3f s)\n", delta_ms, delta_ms / 1000.0);
	wsp_print_result();
	return 0;
}
