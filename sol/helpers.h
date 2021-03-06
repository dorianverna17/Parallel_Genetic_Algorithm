#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

// structure for the object to be put in the sack
typedef struct _sack_object {
    int weight;
    int profit;
} sack_object;

// structure for an individual in the genetic algorithm
// the chromosomes are an array corresponding to each sack
// object, in order, where 1 means that the object is in
// the sack, 0 that it is not

// I added the count member so that I can keep track of the
// non-zero chromosomes
typedef struct _individual {
	int fitness;
	int *chromosomes;
    int chromosome_length;
	int index;
	int count;
} individual;

// struct that is being passed as argument to the thread functin
typedef struct _generation_info {
    int index;
    int capacity;
	int nr_threads;
	int square_length;
    int *nr_generations;
    int *nr_objects;
	individual *current_generation;
	individual *next_generation;
	individual *prev_generation;
    sack_object *objects;
	pthread_barrier_t *barrier;
	pthread_t *threads;
} generation_info;

// structure passed as argument to
// the thread for sorting
typedef struct _info {
	int id;
	int count;
	pthread_barrier_t *barrier;
	individual **v;
	individual **v_prev;
	int square_length;
	int actual_length;
	int nr_threads;
} info;


// the read input function
// similar to the one given in the skel
int read_input(sack_object **objects, int *nr_objects, int *capacity,
                int *nr_gen, int *nr_threads, int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", nr_objects, capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*nr_objects % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects;
    tmp_objects = (sack_object *) calloc(*nr_objects, sizeof(sack_object));

	for (int i = 0; i < *nr_objects; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*nr_gen = (int) strtol(argv[2], NULL, 10);
	
	if (*nr_gen == 0) {
		free(tmp_objects);
		return 0;
	}

    *nr_threads = (int) strtol(argv[3], NULL, 10);
	
	if (*nr_threads == 0) {
		free(tmp_objects);
		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void print_objects(const sack_object *objects, int nr_objects)
{
	for (int i = 0; i < nr_objects; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

// mutate bit string 1 function - as implemented in the
// skel given by the APD team
void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

// mutate bit string 2 function - as implemented in the
// skel given by the APD team
void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

// crossover function - as implemented in the
// skel given by the APD team
void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

// copy individual function as implemented in the skel received
// from the APD team
void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

// free generation function as implemented in the skel received
// from the APD team
void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

// the compute fitness function but i parallelized it
// I added a count member in the individual structure
// to keep track of the non-zero chromosomes in the individual
void compute_fitness_function_parallel(const sack_object *objects, individual *generation,
	int nr_objects, int sack_capacity, int id_thread, int nr_threads) {
	int weight, profit;
	int start, end;
	int count;

	// here I set the start and the end of the vector
	start = id_thread * (double) nr_objects / nr_threads;
	if ((id_thread + 1) * (double) nr_objects / nr_threads > nr_objects)
		end = nr_objects;
	else
		end = (id_thread + 1) * (double) nr_objects / nr_threads;

	for (int i = start; i < end; i++) {
		weight = 0;
		profit = 0;
		count = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
				count++;
			}
		}
		generation[i].count = count;
		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

// function that merges the intervals from mergesort
// it is similar to the one implemented in the laboratory
void merge_intervals(individual *source, int start, int mid, int end, individual *destination) {
	int iA = start;
	int iB = mid;
	int i;
	for (i = start; i < end; i++) {
		// here I implement the merging
		// the conditions are taken from the compare function
		// that was given as parameter to the qsort function
		// in the skel received from the APD team
		if (end == iB || (iA < mid && source[iB].fitness < source[iA].fitness)) {
			memcpy(&destination[i], &source[iA], sizeof(individual));
			iA++;
		} else if (end == iB || (iA < mid && source[iB].fitness == source[iA].fitness)) {
			if (source[iA].count < source[iB].count) {
				memcpy(&destination[i], &source[iA], sizeof(individual));
				iA++;
			} else if (source[iA].count == source[iB].count) {
				if (source[iB].index - source[iA].index) {
					memcpy(&destination[i], &source[iA], sizeof(individual));
					iA++;	
				} else {
					memcpy(&destination[i], &source[iB], sizeof(individual));
					iB++;		
				}
			} else {
				memcpy(&destination[i], &source[iB], sizeof(individual));
				iB++;
			}
		} else {
			memcpy(&destination[i], &source[iB], sizeof(individual));
			iB++;
		}
	}
}


void mergesort_parallel(info *info_ms) {
	// getting the information nedeed from the structure
	int thread_id = info_ms->id;
	int actual_length = info_ms->actual_length;
	int square_length = info_ms->square_length;

	individual **vNew = info_ms->v_prev;
	individual **v = info_ms->v;
	int P = info_ms->nr_threads;

	int start_index, end_index;
	int start_local, end_local;
	int width;

	// declaring an auxiliary vector for the swap between
	// the vectors
	individual **aux = malloc(actual_length * sizeof(individual));

	pthread_barrier_t *barrier = info_ms->barrier;
	
	// we advance with the width of the vectors
	// that we are merging
	pthread_barrier_wait(barrier);
	// here are the steps performed by the mergesort
	// I gradually increase the width of the intervals which are merged
	// this part is similar to the one at the laboratory
	for (width = 1; width < actual_length; width = 2 * width) {
		// here I compute the starting and ending indexes 
		start_index = thread_id * (double) square_length / P;
		end_index = (thread_id + 1) * (double) square_length / P;
		// the the local index is greater than the squared length of
		// the generation to be sorted, hten the end index (local)
		// takes the value of this squared length
		start_local = (start_index / (2 * width)) * (2 * width);
		if (square_length > (end_index / (2 * width)) * (2 * width)) {
			end_local = (end_index / (2 * width)) * (2 * width);
		} else {
			end_local = square_length;
		}

		// here I iterate over the indexes which are specific to the thread
		// with the id thread_id
		for (int i = start_local; i < end_local; i = i + 2 * width) {
			// I have 3 cases in which I call the function that merges the intervals
			if (i + 2 * width > actual_length && i + width > actual_length) {
				// This is the case in which the size of the two intervals that
				// need to be merged ar actually overflowing, the size of the vector
				// overall is smaller. The position of the mid is also greater than
				// the actual size. This is why I set the mid and the end to be the
				// actual size of the vector 
				merge_intervals(*v, i, actual_length, actual_length, *vNew);
			} else if (i + 2 * width > actual_length && i + width <= actual_length) {
				// The second case is just like the case above, but only the size of the
				// second interval is overflowing, the index of the mid is ok
				merge_intervals(*v, i, i + width, actual_length, *vNew);
			} else {
				// here is the default case that has been treated in the laboratory as well
				merge_intervals(*v, i, i + width, i + 2 * width, *vNew);
			}
		}

		// these barrier wait calls are for assuring that
		// all threads have finished execution of a part of code
		// needed by all of them later
		pthread_barrier_wait(barrier);
 
		// here I interchange the vectors so that I
		// get the result in v
		*aux = *v;
		*v = *vNew;
		*vNew = *aux;

		pthread_barrier_wait(barrier);
	}
}

void run_parallel_algorithm(generation_info *gen_info)
{
	// we take the id of the thread and the number of the threads
	int nr_threads = gen_info->nr_threads;
	int id = gen_info->index;

	// get the number of objects
	int nr_objects = *gen_info->nr_objects;
	int nr_generations = *gen_info->nr_generations;

	// get the objects and sack capacity
	int sack_capacity = gen_info->capacity;
	sack_object *objects = gen_info->objects;

	// compute the start and end index using the id of the thread
	int end;
	int start = id * (double) nr_objects / nr_threads;
	if ((id + 1) * (double) nr_objects / nr_threads > nr_objects)
		end = nr_objects;
	else
		end = (id + 1) * (double) nr_objects / nr_threads;

	// get the barrier
	pthread_barrier_t *barrier = gen_info->barrier;
	// get the generations
	individual *current_generation = gen_info->current_generation;
	individual *next_generation = gen_info->next_generation;

	// init the current generation and the next generation
	for (int i = start; i < end; i++) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(nr_objects, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = nr_objects;
		
		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(nr_objects, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = nr_objects;
	}
	pthread_barrier_wait(barrier);

	int count, cursor;
	individual *tmp = NULL;	

	int start_sel, end_sel;
	int start_mut1, end_mut1;
	int start_mut2, end_mut2;

	// compute start and end indexes for the children selection
	int count1 = nr_objects * 3 / 10;
	start_sel = id * (double) count1 / nr_threads;
	if ((id + 1) * (double) count1 / nr_threads > count1)
		end_sel = count1;
	else
		end_sel = (id + 1) * (double) count1 / nr_threads;

	int count2 = nr_objects * 2 / 10;
 	// compute the start and the end for this computation
	// depending on the value of count
	start_mut1 = id * (double) count2 / nr_threads;
	if ((id + 1) * (double) count2 / nr_threads > count2)
		end_mut1 = count2;
	else
		end_mut1 = (id + 1) * (double) count2 / nr_threads;

	start_mut2 = start_mut1 + count2;
	end_mut2 = end_mut1 + count2;

	// create the structure used as argument for the sort function
	// (it helps each thread with the data accessible)
	info *info_ms;
	info_ms = malloc(sizeof(info));
	info_ms->id = id;
	info_ms->count = nr_objects;
	info_ms->barrier = barrier;
	info_ms->square_length = gen_info->square_length;
	info_ms->actual_length = nr_objects;
	info_ms->nr_threads = nr_threads;

	individual *prev_generation = gen_info->prev_generation;
	
	info_ms->v = &current_generation;
	info_ms->v_prev = &prev_generation;

	// iterate to construct the generations
	for (int k = 0; k < nr_generations; k++) {
		cursor = 0;

		// compute the fitness
		compute_fitness_function_parallel(objects, current_generation, nr_objects, sack_capacity, id, nr_threads);
		
		// perform the sort
		pthread_barrier_wait(barrier);
		mergesort_parallel(info_ms);
		pthread_barrier_wait(barrier);
		
	 	// keep first 30% children (elite children selection)
		for (int i = start_sel; i < end_sel; i++) {
	 		copy_individual(current_generation + i, next_generation + i);
		}
	 	cursor = count1;
		pthread_barrier_wait(barrier);

		// mutate first 20% children with the first version of bit string mutation
		for (int i = start_mut1; i < end_mut1; i++) {
	 		copy_individual(current_generation + i, next_generation + cursor + i);
	 		mutate_bit_string_1(next_generation + cursor + i, k);
		}
	 	cursor += count2;
		pthread_barrier_wait(barrier);

		// mutate next 20% children with the second version of bit string mutation
		for (int i = start_mut2; i < end_mut2; i++) {
			copy_individual(current_generation + i, next_generation + cursor + i - count2);
 			mutate_bit_string_2(next_generation + cursor + i - count2, k);
		}
		cursor += count2;
		pthread_barrier_wait(barrier);
		
	 	// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = nr_objects * 3 / 10;
		if (count % 2 == 1 && id == 0) {
 			copy_individual(current_generation + nr_objects - 1, next_generation + cursor + count - 1);
			count--;
		}

		// now I perform the crossover
		pthread_barrier_wait(barrier);		
	 	for (int i = start; i < end; i += 2) {
			if (i < count)
			{
	 			crossover(current_generation + i, next_generation + cursor + i, k);
			} else {
				break;
			}
		}
		pthread_barrier_wait(barrier);

		// switch to new generation
		tmp = current_generation;
		current_generation = next_generation;
		next_generation = tmp;

		for (int i = start; i < end; ++i) {
			current_generation[i].index = i;
		}
		pthread_barrier_wait(barrier);

		// print the fitness
		if (id == 0) {
			if (k % 5 == 0) {
				print_best_fitness(current_generation);
			}
		}
    }

	pthread_barrier_wait(barrier);
	compute_fitness_function_parallel(objects, current_generation, nr_objects, sack_capacity, id, nr_threads);
	// here I sort one last time and then I print the final result
	// (the best firness)
	pthread_barrier_wait(barrier);
	mergesort_parallel(info_ms);
	pthread_barrier_wait(barrier);
	if (id == 0)
		print_best_fitness(current_generation);
}

void run_genetic_algorithm(sack_object *objects, int nr_objects, int nr_gen, int capacity, int nr_threads)
{
    // declaring the threads that are going to be used in my algorithm
    pthread_t threads[nr_threads];

    int err;
    int *nr_gen_aux, *nr_objects_aux;
    
    nr_gen_aux = &nr_gen;
    nr_objects_aux = &nr_objects;

	//create the barrier
	pthread_barrier_t barrier;
	int err_barrier = pthread_barrier_init(&barrier, NULL, nr_threads);
	if (err_barrier) {
		printf("Eroare la initializarea barierei\n");
		exit(-1);
	}

	individual *current_generation, *next_generation;
	current_generation = (individual*) calloc(nr_objects, sizeof(individual));
	next_generation = (individual*) calloc(nr_objects, sizeof(individual));

	// let's change N now
	int res_pow = 1, square_length;
	int count_pow = 0;
	while (res_pow < nr_objects) {
		count_pow++;
		res_pow = pow(2, count_pow);
	}
	square_length = res_pow;

	individual *prev_generation = malloc(nr_objects * sizeof(individual));

	// declare the array of structures passed as arguments to the parallel function
	generation_info info[nr_threads];
	// create the threads and the structure that is
	// passed to each one of these threads
    for (int i = 0; i < nr_threads; i++) {
        info[i].index = i;
        info[i].nr_generations = nr_gen_aux;
        info[i].nr_objects = nr_objects_aux;
		info[i].square_length = square_length;
        info[i].objects = objects;
        info[i].capacity = capacity;
		info[i].nr_threads = nr_threads;
		info[i].barrier = &barrier;
		info[i].threads = threads;
		info[i].current_generation = current_generation;
		info[i].next_generation = next_generation;
		info[i].prev_generation = prev_generation;
    }

	for (int i = 0; i < nr_threads; i++) {
		err = pthread_create(&threads[i], NULL, (void *)run_parallel_algorithm, &info[i]);
        if (err) {
            printf("Error when creating the thread\n");
            exit(-1);
        }
	}

	// joining the threads when the algorithm is completed
    for (int i = 0; i < nr_threads; i++) {
		int err = pthread_join(threads[i], NULL);

		if (err) {
	  		printf("Eroare la asteptarea thread-ului %d\n", i);
	  		exit(-1);
		}
  	}

	pthread_barrier_destroy(&barrier);

	// free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// free resources
	free(current_generation);
	free(next_generation);

    pthread_exit(NULL);
}

#endif
