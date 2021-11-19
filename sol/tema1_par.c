#include "helpers.h"

int main(int argc, char* argv[]) {
	
	// declare the array of objects
	sack_object *objects = NULL;

	// declare the number of objects and the capacity of the sack
	int capacity = 0;
	int nr_objects = 0;
	// declare the number of threads to use
	int nr_threads;
	// declare the number of generations
	int nr_gen;
	
	// read input from the file
	int err;
	err = read_input(&objects, &nr_objects, &capacity,
						&nr_gen, &nr_threads, argc, argv);
	if (err == 0) {
		return 0;
	}

	// print_objects(objects, nr_objects);
	// printf("%d %d %d %d\n", nr_objects, capacity, nr_gen, nr_threads);

	// run the genetic algorithm
	run_genetic_algorithm(objects, nr_objects, nr_gen, capacity, nr_threads);

	// free the memory
	free(objects);

	return 0;
}