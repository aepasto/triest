#include "GraphScheduler.h"
#include "GraphSampler.h"
#include "TriangleCounter.h"
#include "UDynGraph.h"
#include "Stats.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <cmath>

using namespace std;

struct Result {
	double pearson = 0.0;
	double mean_eps_err = 0.0; //As defined in mascot
	double top_triangle_exact = 0.0;
	double top_triangle_est = 0.0;
};

Result local_err(TriangleCounter& gt_counter, GraphSampler& gt_sampler, TriangleCounter& est_counter, GraphSampler& est_sampler){
	Result res;
	res.top_triangle_exact = 0.0;
	res.top_triangle_est = 0.0;

	vector<int> nodes;
	gt_counter.get_nodes(&nodes);

	double gt_avg = 0.0;
	double est_avg = 0.0;

	for (const auto & n: nodes){
		gt_avg+=gt_sampler.get_triangle_est_local(n);
		est_avg+=est_sampler.get_triangle_est_local(n);
	}
	gt_avg/=nodes.size();
	est_avg/=nodes.size();

	double cov = 0.0;
	double sum_dev_gt = 0.0;
	double sum_dev_est = 0.0;

	double mean_eps_err = 0.0;
	for (const auto & n: nodes){
		double gt_ =gt_sampler.get_triangle_est_local(n);
		double est_ =est_sampler.get_triangle_est_local(n);
		if (gt_>res.top_triangle_exact){
			res.top_triangle_exact = gt_;
		}
		if (est_>res.top_triangle_est){
			res.top_triangle_est = est_;
		}
		mean_eps_err+= abs(gt_-est_)/(gt_+1);

		cov += (gt_-gt_avg)*(est_-est_avg);
		sum_dev_gt += (gt_-gt_avg)*(gt_-gt_avg);
		sum_dev_est += (est_-est_avg)*(est_-est_avg);
	}
	if(sum_dev_est==0){
		res.pearson =0;//In this case it is not well defined
	} else {
		res.pearson = cov/(sqrt(sum_dev_gt)*sqrt(sum_dev_est));
	}
	res.mean_eps_err = mean_eps_err/nodes.size();
	return res;
}


int main(int argc, char** argv) {

	if (argc <= 6) {
		cerr
				<< "ERROR Requires FIRST 5 parameters. RunCounting only_add (1=yes,0=no); random_seed (int); Check_Error_every_number_steps (int); graph-udates.txt;\n" <<
				"THEN: Type of Sampler (R for reservoir, F for fix-p, RH resevoir sample and hold, FH fix-p sample and hold)"<<
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

	assert(only_add || !use_sample_and_hold); //can't use sample and hold with deletion

	double p = -1;
	int size_reservoir = -1;
	if (is_reservoir){
		size_reservoir = atoi(argv[6]);
	} else if (is_fix_p){
		p = atof(argv[6]);
	} else {
		cerr<<argv[5]<<" not supported yet."<<endl;
		assert(false);
	}

  GraphScheduler scheduler(file_name, false /* not storing time*/);
  TriangleCounter counter(true /*use local count*/);
	TriangleCounter counter_exact(true /*use local count*/);
	FixedPSampler sampler_exact(1.0, false, &counter_exact);

	GraphSampler* sampler;

	if(is_reservoir && only_add) {
		sampler = new ReservoirSampler(size_reservoir, use_sample_and_hold, &counter);
	} else if(is_reservoir && !only_add) {
		sampler = new ReservoirAddRemSampler(size_reservoir, &counter);
	} else if(is_fix_p) {
		sampler = new FixedPSampler(p, use_sample_and_hold, &counter);
	} else{
		assert(false);
	}

//	Statsstats(stats_freq);
	unsigned long long count_op = 0;


	while (scheduler.has_next()) {
		EdgeUpdate update = scheduler.next_update();
		if(only_add && !update.is_add){
			break; // ENDS at the first remove
		}

		sampler->exec_operation(update);

		sampler_exact.exec_operation(update);

		unsigned long long  triangles_exact = counter_exact.triangles();
		if(++count_op%stats_freq==0){
			if(triangles_exact == 0){
				continue; // No error possibile !
			}
			Result res = local_err(counter_exact,sampler_exact,counter,*sampler);
			cout << count_op << "\t"<<counter.size_sample()<<"\t"<<triangles_exact<< "\t" <<res.top_triangle_exact<<"\t"<<res.top_triangle_est<<"\t" <<res.pearson<<"\t"<<res.mean_eps_err<<endl;
		}
	}
	//stats.end_op();

	delete sampler;
  return 0;
}
