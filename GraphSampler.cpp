#include "GraphSampler.h"
#include <random>
#include <cassert>
#include <iostream>
#include <cmath>
#include <boost/math/distributions/hypergeometric.hpp>
#include <boost/math/distributions/hypergeometric.hpp>
#include <boost/math/policies/policy.hpp>


using namespace std;

GraphSampler::GraphSampler(TriangleCounter* counter) : counter_(counter) {
	if (counter_){
		counter_->clear();
	}
}

GraphSampler::~GraphSampler(){}

FixedPSampler::FixedPSampler(double p, bool use_sample_and_hold, TriangleCounter* counter)
	: GraphSampler(counter), p_(p), use_sample_and_hold_(use_sample_and_hold){
}

FixedPSampler::~FixedPSampler(){}

void FixedPSampler::exec_operation(const EdgeUpdate& update){

	counter_->new_update(update);

	if (update.is_add){
		double u_rand = (double)rand() / ((double)RAND_MAX+1.0);

		if(use_sample_and_hold_){ // always count first
			counter_->add_triangles(update.node_u, update.node_v, 1.0); //Weight not used
		}

		if (u_rand < p_){
			// Undirected graph
			if(!use_sample_and_hold_){ // count only if sampled
				counter_->add_triangles(update.node_u, update.node_v, 1.0); //Weight not used
			}
			counter_->add_edge_sample(update.node_u, update.node_v);

		}
	} else { // Remove are always executed (if not present no effect)
		assert(!use_sample_and_hold_); // Not supported.
		bool succeed = counter_->remove_edge_sample(update.node_u, update.node_v);
		// Here !use_sample_and_hold_
		if (succeed){ // it was present
			counter_->remove_triangles(update.node_u, update.node_v, 1.0); //Weight not used
		}
	}
}

double FixedPSampler::get_triangle_est(){
	if (!use_sample_and_hold_){
		return (double)counter_->triangles()*pow(1.0/p_,3);
	} else {
		return (double)counter_->triangles()*pow(1.0/p_,2);
	}
}


double FixedPSampler::get_triangle_est_local(int node){
	assert(counter_->is_local());
	if (!use_sample_and_hold_){
		return (double)counter_->triangles_local(node)*pow(1.0/p_,3);
	} else {
		return (double)counter_->triangles_local(node)*pow(1.0/p_,2);
	}
}

//RESERVOIR SAMPLER

ReservoirSampler::ReservoirSampler(size_t reservoir_size, bool use_sample_and_hold, TriangleCounter* counter)
	: GraphSampler(counter), reservoir_size_(reservoir_size), use_sample_and_hold_(use_sample_and_hold){
		reservoir_.reserve(reservoir_size);
}

ReservoirSampler::~ReservoirSampler(){}


void ReservoirSampler::add_reservoir(const pair<int,int> edge){
	//cout<<"ADD RES"<<edge.first<<" "<<edge.second<<endl;
	assert(reservoir_map_.find(edge) == reservoir_map_.end());
	reservoir_.push_back(edge);
	reservoir_map_.insert(make_pair(edge, reservoir_.size()-1));
	counter_->add_edge_sample(edge.first, edge.second);
	if (!use_sample_and_hold_){
		counter_->add_triangles(edge.first, edge.second, 1.0); //Weight not used
	}
}

void ReservoirSampler::delete_reservoir(const pair<int,int> edge){
	if (reservoir_map_.find(edge) == reservoir_map_.end()){
		return;
	}
	int pos = reservoir_map_.at(edge);
	reservoir_map_.erase(edge);
	if (pos < reservoir_.size() -1){ // not the last item
		pair<int, int> last_edge = reservoir_.back();
		reservoir_[pos] = last_edge;
		reservoir_map_[last_edge] = pos;
	}
	reservoir_.pop_back();
	bool succ = counter_->remove_edge_sample(edge.first, edge.second);
	if (!use_sample_and_hold_){
		counter_->remove_triangles(edge.first, edge.second, 1.0); //Weight not used
	}

	assert(succ);
	assert(reservoir_.size()<=reservoir_size_);
	assert(reservoir_.size()==reservoir_map_.size());
}

void ReservoirSampler::exec_operation(const EdgeUpdate& update){
	assert(update.is_add); //only add supported
	assert(update.node_u != update.node_v);
	int max_n = (int)max(update.node_u, update.node_v);
	int min_n = (int)min(update.node_u, update.node_v);

	pair<int,int> edge = make_pair(min_n, max_n);

	counter_->new_update(update);

	//if(update.is_add){
	assert(reservoir_map_.find(edge) == reservoir_map_.end());

	// Prob of sampling at this step if using sample and hold
  double p = 0;
  unsigned long long int t = counter_->edges_present_original();

  if (t >= 2 && use_sample_and_hold_){
    p = ((double)reservoir_size_/t)*((double)(reservoir_size_-1)/(t-1));
    p = min(p, 1.0);
  }


	if (use_sample_and_hold_){ // Always count and rever remove.
		counter_->add_triangles(update.node_u, update.node_v, (double)1.0/p);
	}


	// Enough space
	if (reservoir_.size() < reservoir_size_){
		add_reservoir(edge);
	} else {
		double u_rand = (double)rand() / ((double)RAND_MAX+1.0);
		double thres = ((double)reservoir_size_)/counter_->edges_present_original();
		if (u_rand < thres){
			// Doing the exchange
			int rand_pos = rand()%reservoir_size_;
			const pair<int,int>& to_remove = reservoir_[rand_pos];
			delete_reservoir(to_remove);
			add_reservoir(edge);
		}
	}
	//} else { // is remove
	//	delete_reservoir(edge);
	//}
}

 double ReservoirSampler::get_triangle_est(){
	if (!use_sample_and_hold_){

    double p = 0;
    unsigned long long int t = counter_->edges_present_original();

    if (t >= 3){
      p = ((double)reservoir_size_/t)*((double)(reservoir_size_-1)/(t-1))
          *((double)(reservoir_size_-2)/(t-2));
      p = min(p, 1.0);
      return (double)counter_->triangles()/p;

    } else {
      return 0; // no triangles possible;
    }


	} else {
		return counter_->triangles_weight();
	}
}


 double ReservoirSampler::get_triangle_est_local(int node){
	assert(counter_->is_local());

	if (!use_sample_and_hold_){
    unsigned long long int t = counter_->edges_present_original();
    if (t >= 3){
      double p = ((double)reservoir_size_/t)*((double)(reservoir_size_-1)/(t-1))
          *((double)(reservoir_size_-2)/(t-2));
      p = min(p, 1.0);
      return (double)counter_->triangles_local(node)/p;

    } else {
      return 0; // no triangles possible;
    }
	} else {
		return counter_->triangles_weight_local(node);
	}
}


// ****************************************
// Reservoir add & remove



ReservoirAddRemSampler::ReservoirAddRemSampler(size_t reservoir_size, TriangleCounter* counter)
	: GraphSampler(counter), reservoir_size_(reservoir_size), d_i_(0), d_o_(0){
		reservoir_.reserve(reservoir_size);
}

ReservoirAddRemSampler::~ReservoirAddRemSampler(){}


void ReservoirAddRemSampler::add_reservoir(const pair<int,int> edge){
	//cout<<"ADD RES"<<edge.first<<" "<<edge.second<<endl;
  int before_size = reservoir_.size();

	assert(reservoir_map_.find(edge) == reservoir_map_.end());
	reservoir_.push_back(edge);
	reservoir_map_.insert(make_pair(edge, reservoir_.size()-1));
	bool succ = counter_->add_edge_sample(edge.first, edge.second);

  assert (succ);
  assert(reservoir_.size()<=reservoir_size_);
  assert(reservoir_.size()==reservoir_map_.size());
  assert (reservoir_map_.find(edge) != reservoir_map_.end());
  assert(reservoir_.size()== before_size +1);
}

void ReservoirAddRemSampler::delete_reservoir(const pair<int,int> edge){
  //cout<<"REM RES"<<edge.first<<" "<<edge.second<<endl;

  assert (reservoir_map_.find(edge) != reservoir_map_.end());
  int before_size = reservoir_.size();

	int pos = reservoir_map_.at(edge);
	reservoir_map_.erase(edge);
	if (pos < reservoir_.size() -1){ // not the last item
		pair<int, int> last_edge = reservoir_.back();
		reservoir_[pos] = last_edge;
		reservoir_map_[last_edge] = pos;
	}
	reservoir_.pop_back();
	bool succ = counter_->remove_edge_sample(edge.first, edge.second);
	counter_->remove_triangles(edge.first, edge.second, 1.0); //Weight not used

	assert(succ);
	assert(reservoir_.size()<=reservoir_size_);
	assert(reservoir_.size()==reservoir_map_.size());
  assert (reservoir_map_.find(edge) == reservoir_map_.end());
  assert(reservoir_.size()== before_size -1);
}

void ReservoirAddRemSampler::exec_operation(const EdgeUpdate& update){
	assert(update.node_u != update.node_v);
	int max_n = (int)max(update.node_u, update.node_v);
	int min_n = (int)min(update.node_u, update.node_v);

	pair<int,int> edge = make_pair(min_n, max_n);

	counter_->new_update(update);

	if(update.is_add){
		assert(reservoir_map_.find(edge) == reservoir_map_.end());

    if (d_o_ + d_i_ > 0) { // case d_o + d_i > 0
      double u_rand = (double)rand() / ((double)RAND_MAX+1.0);
			double thres = ((double)d_i_)/(d_i_+d_o_);
      if (u_rand < thres){ //with pro d_i / (d_i + d_o)
        d_i_ --;
        assert(reservoir_.size() < reservoir_size_);

        add_reservoir(edge);
        counter_->add_triangles(edge.first, edge.second, 1.0); //Weight not used
      } else {
        d_o_ --;
      }
    } else if (reservoir_.size() < reservoir_size_){ // enough space and d_i + d_o = 0

			add_reservoir(edge);
      counter_->add_triangles(edge.first, edge.second, 1.0); //Weight not used

		} else { // reservoid full and d_i + d_o = 0
      assert (counter_->edges_present_original()>reservoir_size_);

			double u_rand = (double)rand() / ((double)RAND_MAX+1.0);
			double thres = ((double)reservoir_size_)/(counter_->edges_present_original());
			if (u_rand < thres){
				// Doing the exchange
				int rand_pos = rand()%reservoir_size_;
				pair<int,int> to_remove = reservoir_[rand_pos];

        size_t before_size = reservoir_.size();

				delete_reservoir(to_remove);
				add_reservoir(edge);

        assert(reservoir_.size()<=reservoir_size_);
        assert(reservoir_.size()==reservoir_map_.size());
        assert (reservoir_map_.find(to_remove) == reservoir_map_.end());
        assert (reservoir_map_.find(edge) != reservoir_map_.end());
        assert(reservoir_.size()== before_size);


				counter_->add_triangles(edge.first, edge.second, 1.0); //Weight not used
			}
		}
	} else { // is remove
    assert(counter_->edges_present_original()>=0);

		if (reservoir_map_.find(edge) != reservoir_map_.end()){//Was present
      d_i_ ++; // deletion in sample

			delete_reservoir(edge);
		} else { // the edge deleted was not in the reservoir
      d_o_ ++; // deletion in sample

    }
	}
}

double ReservoirAddRemSampler::prob_sampling_triangle() const{
  if (counter_->edges_present_original() < 3 || reservoir_.size() < 3) {
    return 0;
  }
  unsigned long long int s = counter_->edges_present_original();

  //cout<<"SIZES: "<<reservoir_.size()<< " " <<s<<endl;
  assert ((unsigned long long int)reservoir_.size()<=s);

  double mt = reservoir_.size();

  double p = 0;

  if (reservoir_.size() == s) {
    p = 1;
  } else {
    p = (mt/s)*((mt-1)/(s-1))*((mt-2)/(s-2));
  }

  assert (p<=1);

  unsigned long long int n = min((unsigned long long int)reservoir_size_, s+d_i_+d_o_);

  boost::math::hypergeometric_distribution<double> hyper(s, n, d_i_+d_o_+s);
  double kt = 0;

  for (int i = 0; i<=2 ; i++){
    if (i >= max(0ull, (unsigned long long int)(n) +  - d_i_-d_o_) && i <= min((unsigned long long int)(n), s)){
      kt += boost::math::pdf<double>(hyper, i);
    }
  }

  return p*(1-kt);
}


double ReservoirAddRemSampler::get_triangle_est(){

  double prob = prob_sampling_triangle();
	return counter_->triangles() / prob;
}

double ReservoirAddRemSampler::get_triangle_est_local(int node){
	assert(counter_->is_local());

  double prob = prob_sampling_triangle();
	return counter_->triangles_local(node) / prob;
}


// ALI PINAR Paper

PinarSampler::PinarSampler(size_t edge_res_size, size_t wedge_res_size)
	: GraphSampler(NULL/* no need of TriangleCounter*/), t_(0), tot_wedges_(0), fraction_closed_(0.0), edge_res_size_(edge_res_size), wedge_res_size_(wedge_res_size){
		edge_reservoir_.resize(edge_res_size);
		wedge_reservoir_.resize(wedge_res_size);
		wedge_closed_.resize(wedge_res_size);
}

PinarSampler::~PinarSampler(){}


double PinarSampler::get_triangle_est(){
	return fraction_closed_*t_*t_/
		(static_cast<double>(edge_res_size_)*(edge_res_size_-1))*tot_wedges_;
}


void PinarSampler::exec_operation(const EdgeUpdate& update){
	assert(update.is_add); //only add supported
	assert(update.node_u != update.node_v);
	int max_n = (int)max(update.node_u, update.node_v);
	int min_n = (int)min(update.node_u, update.node_v);

	t_+=1;

	pair<int,int> edge = make_pair(min_n, max_n);

	long long int closed = 0;

	// Check if closing wedges
	for (int i = 0; i<wedge_reservoir_.size(); i++){
		const pair<int,pair<int,int>> &wedge = wedge_reservoir_[i];

		if ((wedge.second.first == max_n && wedge.second.second == min_n)
				||(wedge.second.first == min_n && wedge.second.second == max_n)
		){
				wedge_closed_[i] = true;

		}
		closed += (wedge_closed_[i] ? 1 : 0);
	}



	// Update edge reservoir
	bool updated = false;
	for (int i = 0; i<edge_reservoir_.size(); i++){
		double u_rand = (double)rand() / ((double)RAND_MAX+1.0);
		if (u_rand <= 1.0/t_){
			edge_reservoir_[i] = edge;
			updated = true;
		}
	}

	if(updated){
		// count total num of wedges
		UDynGraph sub_graph;
		for (const auto& e: edge_reservoir_){
			sub_graph.add_edge(e.first, e.second);
		}
		vector<int> nodes;
		sub_graph.nodes(&nodes);

		tot_wedges_ = 0;
		for (const auto& n:nodes){
			int deg = sub_graph.degree(n);
			tot_wedges_+= (deg*(deg-1))/2;
		}

		// get new wedges with this edge...
		vector<pair<int,pair<int,int>>> new_wedges;
		vector<int> neighbors_min;
		sub_graph.neighbors(min_n, &neighbors_min);
		for (const auto & neighbor: neighbors_min){
			// wedge min_n-neighbor, min_n-max_n
			if (neighbor != max_n){
				new_wedges.push_back(make_pair(min_n, make_pair(neighbor, max_n)));
			}
		}
		vector<int> neighbors_max;
		sub_graph.neighbors(max_n, &neighbors_max);
		for (const auto & neighbor: neighbors_max){
			// wedge max_n-neighbor, max_n-min_n
			if (neighbor != min_n){
				new_wedges.push_back(make_pair(max_n, make_pair(neighbor, min_n)));
			}
		}

		for (int i = 0; i<wedge_reservoir_.size(); i++){
			double u_rand = (double)rand() / ((double)RAND_MAX+1.0);
			if (u_rand <= 1.0*new_wedges.size()/tot_wedges_){
				wedge_reservoir_[i] = new_wedges[rand()%new_wedges.size()];
				closed -= (wedge_closed_[i] ? 1 : 0);
				wedge_closed_[i] = false; // This make absolutely no sense but it is done in ali pinar paper So I implemented it as stated. ****
				closed += (wedge_closed_[i] ? 1 : 0);
			}
		}

		fraction_closed_ = 1.0*closed/ wedge_reservoir_.size();
	}

}
