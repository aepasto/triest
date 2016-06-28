/*
 * GraphScheduler.h
 *
 *  Created on: Oct 22, 2014
 *      Author: aepasto
 */

#ifndef GRAPHSAMPLER_H_
#define GRAPHSAMPLER_H_

#include "GraphScheduler.h"
#include "UDynGraph.h"
#include "TriangleCounter.h"

#include <unordered_set>


using namespace std;

class GraphSampler {
public:
	GraphSampler(TriangleCounter* counter);
	virtual ~GraphSampler();

public:
	// Given the update (i.e. add or remove edge) execute it in the underlying graph according to the sampling
	virtual void exec_operation(const EdgeUpdate& update) = 0;
	virtual double get_triangle_est() = 0;
	virtual double get_triangle_est_local(int n) = 0;

	TriangleCounter* counter_; // The underlying graph used to execute the operations need to be allocated/deallocated by the callee
};

class FixedPSampler: public GraphSampler {
public:
	FixedPSampler(double p, bool use_sample_and_hold, TriangleCounter* counter);
	virtual ~FixedPSampler();

	void exec_operation(const EdgeUpdate& update);
	double get_triangle_est();
	double get_triangle_est_local(int n);

private:
	double p_;
	bool use_sample_and_hold_;
};

// ONLY ADDITIONS
class ReservoirSampler: public GraphSampler {
public:
	ReservoirSampler(size_t reservoir_size, bool use_sample_and_hold, TriangleCounter* counter);
	virtual ~ReservoirSampler();

	void exec_operation(const EdgeUpdate& update);
	double get_triangle_est();
	double get_triangle_est_local(int n);

private:
	void add_reservoir(const pair<int,int> edge);
	void delete_reservoir(const pair<int,int> edge);

	bool use_sample_and_hold_;

	unsigned long long reservoir_size_;
	vector<pair<int,int>> reservoir_;
	unordered_map<pair<int,int>, int> reservoir_map_;
};


// Addition and deletion
class ReservoirAddRemSampler: public GraphSampler {
public:
	ReservoirAddRemSampler(size_t reservoir_size, TriangleCounter* counter);
	virtual ~ReservoirAddRemSampler();

	void exec_operation(const EdgeUpdate& update);
	double get_triangle_est();
	double get_triangle_est_local(int n);

private:
	void add_reservoir(const pair<int,int> edge);
	void delete_reservoir(const pair<int,int> edge);

  double prob_sampling_triangle() const;


  unsigned long long d_i_; //counter used by the algorithm
  unsigned long long d_o_; //counter used by the algorithm
	unsigned long long reservoir_size_;

	vector<pair<int,int>> reservoir_;
	unordered_map<pair<int,int>, int> reservoir_map_;
  unordered_set<pair<int,int>> all_edges_; //used only for debug not stored!
};

// Ali pinar "A space efficient streaming algorithm for estimating transitivity"
// TKDD paper.
class PinarSampler: public GraphSampler {
public:
	PinarSampler(size_t edge_res_size, size_t wedge_res_size);
	virtual ~PinarSampler();

	void exec_operation(const EdgeUpdate& update);
	double get_triangle_est();
	double get_triangle_est_local(int n){
		return 0;
		// Not implemented by this algorithm
	}

private:
	//void add_reservoir(const pair<int,int> edge);
	//void delete_reservoir(const pair<int,int> edge);

	unsigned long long t_;
	unsigned long long tot_wedges_;
	double fraction_closed_;


	unsigned long long edge_res_size_;
	unsigned long long wedge_res_size_;
	vector<pair<int,int>> edge_reservoir_;
	vector<pair<int, pair<int,int>>> wedge_reservoir_; // wedge represented as (u, (x,y)) for u-x u-y
	vector<bool> wedge_closed_;

};

// Pavan VLDB 2013 paper  "Counting and sampling triangles from stream"

class PavanEstimator {
public:
	pair<int, int> e1;
	pair<int, int> e2;
	int c;
	bool is_triangle;
	int t;

	PavanEstimator(){
		e1.first = e1.second = -1;
		e2.first = e2.second = -1;
		c = 0;
		t = 0;
		is_triangle = false;
	}

	void add_edge(int u, int v){
		int min_u = int(min(u,v));
		int max_u = int(max(u,v));
		u = min_u;
		v = max_u;

		t++;
		double u_rand = (double)rand() / ((double)RAND_MAX+1.0);
		if (u_rand<= 1.0/t){
			e1.first = u;
			e1.second = v;
			e2.first = e2.second = -1;
			c = 0;
			is_triangle = false;
		} else {
			if (e1.first == u || e1.second == u || e1.first == v || e1.second == v){
				c++;
				double u_rand2 = (double)rand() / ((double)RAND_MAX+1.0);
				if (u_rand2<= 1.0/c){
					e2.first = u;
					e2.second = v;
					is_triangle = false;
				} else {
					// Checks u,v forms a triangle with e1 and e2
					if (((e1.first == u || e1.second == u) && (e2.first == v || e2.second == v)) ||
								((e1.first == v || e1.second == v) && (e2.first == u || e2.second == u))){
						is_triangle = true;
					}
				}
			}
		}
	}
};

class PavanSampler: public GraphSampler {
public:
	PavanSampler(size_t est_number) : GraphSampler(NULL){
		t_ = 0;
		estimators.resize(est_number);
	}
	virtual ~PavanSampler(){};

	void exec_operation(const EdgeUpdate& update){
		t_ += 1;
		for (auto& est: estimators){
			est.add_edge(update.node_u, update.node_v);
		}
	}
	double get_triangle_est(){
		double sum = 0;
		for (const auto& est: estimators){
			if (est.is_triangle){
				sum+=est.c*t_;
			}
		}
		return 1.0*sum/estimators.size();
	}
	double get_triangle_est_local(int n){
		return 0;
		// Not implemented by this algorithm
	}

private:
	//void add_reservoir(const pair<int,int> edge);
	//void delete_reservoir(const pair<int,int> edge);

	unsigned long long t_;
	vector<PavanEstimator> estimators;
};



#endif /* GRAPHSAMPLER_H_ */
