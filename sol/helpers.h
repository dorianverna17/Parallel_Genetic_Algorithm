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
typedef struct _individual {
	int fitness;
	int *chromosomes;
    int chromosome_length;
	int index;
} individual;

// struct that is being passed as argument to the thread functin
typedef struct _generation_info {
    int index;
    int capacity;
    int *nr_generations;
    int *nr_objects;
    individual *current_generation;
    individual *next_generation;
    sack_object *objects;
} generation_info;

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

void compute_fitness_function(const sack_object *objects, individual *generation,
                                int nr_objects, int capacity)
{
	int weight;
	int profit;

	for (int i = 0; i < nr_objects; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

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

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

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

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

void run_parallel_algorithm(generation_info *gen_info)
{
    for (int k = 0; k < gen_info->nr_generations; k++) {
        // TODO - compute fitness function parallel
    }
}

void run_genetic_algorithm(const sack_object *objects, int nr_objects, int nr_gen, int capacity, int nr_threads)
{
    int count, cursor;
	individual *current_generation = (individual*) calloc(nr_objects, sizeof(individual));
	individual *next_generation = (individual*) calloc(nr_objects, sizeof(individual));
	individual *tmp = NULL;

	// set initial generation (composed of object_count individuals with a single item in the sack)
	for (int i = 0; i < nr_objects; ++i) {
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

    // declaring the threads that are going to be used in my algorithm
    pthread_t threads[nr_threads];

    int err;
    generation_info *info;
    int *nr_gen_aux, nr_objects_aux;
    
    nr_gen_aux = &nr_gen;
    nr_objects_aux = &nr_objects;

    for (int i = 0; i < nr_threads; i++) {
        info = malloc(sizeof(generation_info));
        info->index = i;
        info->nr_generations = nr_gen_aux;
        info->nr_objects = nr_objects_aux;
        info->current_generation = current_generation;
        info->next_generation = next_generation;
        info->objects = objects;
        info->capacity = capacity;

        err = pthread_create(&threads[i], NULL, run_parallel_algorithm, info);
        if (err) {
            printf("Error when creating the thread\n");
            exit(-1);
        }
    }

    // joining the threads when the algorithm is completed
    for (int i = 0; i < nr_threads; i++) {
		err = pthread_join(threads[i], NULL);

		if (err) {
	  		printf("Eroare la asteptarea thread-ului %ld\n", i);
	  		exit(-1);
		}
  	}

    // free resources for old generation
	free_generation(current_generation);
	free_generation(next_generation);

	// free resources
	free(current_generation);
	free(next_generation);

    pthread_exit(NULL);
}

#endif
