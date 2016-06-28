#include "GraphScheduler.h"
#include "GraphSampler.h"
#include "TriangleCounter.h"
#include "UDynGraph.h"
#include "Stats.h"

#include <iostream>
#include <cassert>
#include <cstring>

using namespace std;

int main(int argc, char** argv) {

	if (argc <= 6) {
		cerr
				<< "ERROR Requires FIRST 5 parameters. RunCounting only_add (1=yes,0=no); random_seed (int); stats_every_num_updates (int); graph-udates.txt;\n" <<
				"THEN: Type of Sampler (R for reservoir, F for fix-p, RH resevoir sample and hold, FH fix-p sample and hold, P for pinar algo or V for paVan algorithm)"<<
				" THEN IF reservoir: size reservoir (int) "<<
				" ELSE IF fixed-p: p (double) "<< endl;
		exit(1);
	}

	bool only_add = atoi(argv[1])==1;
	assert(atoi(argv[1])<=1 && atoi(argv[1])>=0);
	int random_seed = atoi(argv[2]);
	assert(random_seed>=0);
	srand(random_seed);

	int stats_freq = atoi(argv[3]);
	assert(stats_freq >0);
	string file_name(argv[4]);

	bool is_reservoir = strcmp(argv[5], "R") == 0 || strcmp(argv[5], "RH") == 0;
	bool is_fix_p  = strcmp(argv[5], "F") == 0 || strcmp(argv[5], "FH") == 0;
	bool use_sample_and_hold = strcmp(argv[5], "RH") == 0 || strcmp(argv[5], "FH") == 0;
	bool is_pinar = strcmp(argv[5], "P") == 0;
	bool is_pavan = strcmp(argv[5], "V") == 0;

	assert(only_add || !use_sample_and_hold); //can't use sample and hold with deletion

	double p = -1;
	int size_reservoir = -1;
	if (is_reservoir){
		size_reservoir = atoi(argv[6]);
	} else if (is_fix_p){
		p = atof(argv[6]);
	} else if (is_pinar){
		size_reservoir = atoi(argv[6]);
	} else if (is_pavan){
		size_reservoir = atoi(argv[6]);
	} else{
		cerr<<argv[5]<<" not supported yet."<<endl;
		assert(false);
	}

  GraphScheduler scheduler(file_name, false /* not storing time*/);
  TriangleCounter counter(false /*no local count*/);
	GraphSampler* sampler;

	if(is_reservoir && only_add) {
		sampler = new ReservoirSampler(size_reservoir, use_sample_and_hold, &counter);
	} else if(is_reservoir && !only_add) {
		sampler = new ReservoirAddRemSampler(size_reservoir, &counter);
	} else if(is_fix_p) {
		sampler = new FixedPSampler(p, use_sample_and_hold, &counter);
	} else if (is_pinar){
		sampler = new PinarSampler(size_reservoir, size_reservoir); // USE SAME SIZE FOR BOTH RESERVOIR
	} else if (is_pavan){
		sampler = new PavanSampler(size_reservoir); 
	} else {
		assert(false);
	}

	Stats stats(stats_freq);

	while (scheduler.has_next()) {
		EdgeUpdate update = scheduler.next_update();
		if(only_add && !update.is_add){
			break; // ENDS at the first remove
		}

		sampler->exec_operation(update);

		//cout << "OP: "<<update.is_add<<" "<<update.node_u<<" "<<update.node_v <<endl;

		// This is the crude number of triangles in the sample (not the unbiased est.) Use Sampler->get_triangles_est() for the unbiased estimator.
		unsigned long long int triangles = counter.triangles();
		double triangles_est = sampler->get_triangle_est();

		stats.exec_op(update.is_add, triangles, triangles_est,
			counter.size_sample(), update.time);
		//if(++count_op%stats_freq==0){
		//	cout << count_op << " "<<triangles<< " " <<sampler->get_triangle_est()<<endl;
		//}
	}
	stats.end_op();

	delete sampler;
  return 0;
}
