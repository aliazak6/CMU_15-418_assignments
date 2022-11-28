
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <error.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <mpi.h>
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
path_t* wsp_recurse(path_t* currentPath,int city_count) {
	int skip_flag = 0;
	int cost = 0;
	int new_cost = 0;
	// MPI GATHER BEST PATH
	// IF MY BEST PATH BETTER 
	for(int i=0; i<NCITIES; i++) { // loop to iterate over different cities
		cost = 0;
		new_cost = 0;
		/* PART WE DECIDE TO ITERATE OR NOT*/
		skip_flag = 0;
		for(int j=0; j<city_count; j++) { // loop to iterate over cities in current path
			if(currentPath->path[j] == i) { // if city is already in path, skip it
				skip_flag = 1;
				break;
			}
		}
		if(skip_flag == 1) { // if city is already in path, skip it
			continue;
		}
		/*********************************/
		/* PART WE ITERATE OVER*/
		new_cost = get_dist(currentPath->path[city_count-1],i);
		cost = currentPath->cost + new_cost; // add cost of current city to path cost
		if(cost > bestPath->cost){
			continue;
		}else{
			currentPath->path[city_count] = i;
			currentPath->cost = cost;
			if(city_count == NCITIES-1) { // if we have reached the end of the path
				if(cost < bestPath->cost) { // if the cost of the path is less than the best path
					bestPath->cost = cost;
					for(int j=0; j<NCITIES; j++) {
						bestPath->path[j] = currentPath->path[j];
					}
				}
				currentPath->cost-= new_cost;			
			}else{
				currentPath = wsp_recurse(currentPath,city_count+1);
			}
		}
		/*********************************/		
	}
	currentPath->cost-=  get_dist(currentPath->path[city_count-1],currentPath->path[ city_count-2]);
	
	return currentPath;
}	

void wsp_start(int process_Rank, int size_Of_Comm) {
	// TODO: try finding a better path.
	int cityID = 0;	
	for(cityID=0; cityID < NCITIES; cityID++) {
		bestPath->path[cityID] = cityID;
		if(cityID>0) bestPath->cost += get_dist(bestPath->path[cityID-1],bestPath->path[cityID]);
	}
	/* MPI INITIALIZATIN AND THREAD INFOS*/
	int root,city_count;
	path_t* currentPath = NULL;
	currentPath = (path_t*)malloc(sizeof(path_t));
	currentPath->path = (city_t *)malloc(NCITIES * sizeof(city_t));
	for(root = process_Rank; root < NCITIES; root+=size_Of_Comm) {
		city_count = 1;
		currentPath->cost = 0;
		currentPath->path[0] = root;
		currentPath = wsp_recurse(currentPath,city_count);
	}	
	free(currentPath->path);
	free(currentPath);
}

int main(int argc, char **argv) {
	int process_Rank, size_Of_Comm;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Comm);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);
	if(argc < 1) error_exit("Expecting one arguments: file name]\n");
	
	// Allocate memory and read the matrix
	char *filename = argv[1];
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) error_exit("Failed to open input file \"%s\"\n", filename);
	int scan_ret;
	scan_ret = fscanf(fp, "%d", &NCITIES);
	if(scan_ret != 1) error_exit("Failed to read city count\n");
	if(NCITIES < 2) {
		error_exit("Illegal city count: %d\n", NCITIES);
	} 
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
	city_t** recvPath = NULL;
	int* recvCost = NULL;
	if(process_Rank == 0) {
		recvCost = (int*)malloc(sizeof(int)*size_Of_Comm);
		recvPath = (city_t**)calloc(NCITIES*size_Of_Comm, sizeof(city_t));
	}
	double before, after;
    before = MPI_Wtime();
	wsp_start(process_Rank,size_Of_Comm);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Gather(&(bestPath->cost),1,MPI_INT,recvCost,1,MPI_INT,0,MPI_COMM_WORLD);
	MPI_Gather(&(bestPath->path),NCITIES,MPI_INT8_T,recvPath,size_Of_Comm*NCITIES,MPI_INT8_T,0,MPI_COMM_WORLD);
	if(process_Rank == 0) {
		for(int i = 0; i < size_Of_Comm; i++) {
			if(recvCost[i] < bestPath->cost) {
				bestPath->cost = recvCost[i];
				for(int j = 0; j < NCITIES; j++) {
					bestPath->path[j] = recvPath[i][j];
				}
			}
		}
	}
	after   = MPI_Wtime();
	if(process_Rank == 0) {
		double delta_ms = (after - before) * 1000.0 + (after - before) / 1000000.0;
		putchar('\n');
		printf("============ Time ============\n");
		printf("Time: %.3f ms (%.3f s)\n", delta_ms, delta_ms / 1000.0);
		wsp_print_result();
	}
	if(process_Rank == 0) {
		free(recvCost);
		free(recvPath);
	}
	free(DIST);
	free(bestPath);
	free(bestPath->path);
	MPI_Finalize();
	return 0;
}
