
#include "TriangleCounter.h"
#include <cassert>
#include <iostream>

using namespace std;

unsigned long long edge_to_id(const int u,
		const int v)		  {
	assert(MAX_NUM_NODES > u);
	assert(MAX_NUM_NODES > v);
	assert(u!=v);

	int n_u = (u<v ? u : v);
	int n_v = (u<v ? v : u);

	unsigned long long ret = static_cast<unsigned long long>(MAX_NUM_NODES)
			* static_cast<unsigned long long>(n_u)
			+ static_cast<unsigned long long>(n_v);
	assert(ret);
	return ret;
}


TriangleCounter::TriangleCounter(bool local) : local_(local), triangles_(0), edges_present_original_(0), triangles_weight_(0.0) {
}

TriangleCounter::~TriangleCounter() {
}


bool TriangleCounter::add_edge_sample(const int u, const int v){
	assert(u!=v);

	set_edge_ids_.insert(edge_to_id(u,v));
	bool succeed = graph_.add_edge(u,v);
	//if (! succeed){
	//	cerr<<"NOT SUCCED ADD: "<<u<< " "<<v<<endl;
	//}
	//assert(succeed); // edge update need to be distinct
  return succeed;
}

bool TriangleCounter::remove_edge_sample(const int u, const int v){
	assert(u!=v);

	set_edge_ids_.erase(edge_to_id(u,v));

	return graph_.remove_edge(u,v);
}

void TriangleCounter::add_triangles(const int u, const int v, double weight){
	assert(u!=v);
	int min_deg_n = (graph_.degree(u) <= graph_.degree(v) ? u : v);
	int max_deg_n = (graph_.degree(u) <= graph_.degree(v) ? v : u);

	vector<int> min_neighbors;
	graph_.neighbors(min_deg_n, & min_neighbors);

	for(const auto& n : min_neighbors){
		if(n!= max_deg_n){
			if(set_edge_ids_.find(edge_to_id(n, max_deg_n)) != set_edge_ids_.end()){
				double weight_to_use = 0.0;

				if(edge_weight_.empty()){ // easy case used by most algorithms
					weight_to_use = weight;
				} else { // each triangle is weighted by the product of the weights of the edges
					assert(weight = 1.0); //not used in this case
					assert(edge_weight_[make_pair(u,v)]>0);
					assert(edge_weight_[make_pair(u,n)]>0);
					assert(edge_weight_[make_pair(v,n)]>0);
					weight_to_use = edge_weight_.at(make_pair(u,v))*edge_weight_.at(make_pair(n,v))*edge_weight_.at(make_pair(u,n));
				}
				triangles_weight_ += weight_to_use;
				triangles_ += 1;

				if(local_){
					triangles_local_map_[u]++;
					triangles_local_map_[v]++;
					triangles_local_map_[n]++;
					triangles_weight_local_map_[u]+=weight_to_use;
					triangles_weight_local_map_[v]+=weight_to_use;
					triangles_weight_local_map_[n]+=weight_to_use;
				}
			}
		}
	}
}
void TriangleCounter::remove_triangles(const int u, const int v, double weight){
	assert(u!=v);
	int min_deg_n = (graph_.degree(u) <= graph_.degree(v) ? u : v);
	int max_deg_n = (graph_.degree(u) <= graph_.degree(v) ? v : u);

	vector<int> min_neighbors;
	graph_.neighbors(min_deg_n, & min_neighbors);

	for(const auto& n : min_neighbors){
		if(n!= max_deg_n){
			if(set_edge_ids_.find(edge_to_id(n, max_deg_n)) != set_edge_ids_.end()){
				double weight_to_use = 0.0;

				if(edge_weight_.empty()){ // easy case used by most algorithms
					weight_to_use = weight;
				} else { // each triangle is weighted by the product of the weights of the edges
					assert(weight = 1.0); //not used in this case
					assert(edge_weight_[make_pair(u,v)]>0);
					assert(edge_weight_[make_pair(u,n)]>0);
					assert(edge_weight_[make_pair(v,n)]>0);
					weight_to_use = edge_weight_.at(make_pair(u,v))*edge_weight_.at(make_pair(n,v))*edge_weight_.at(make_pair(u,n));
				}
				triangles_weight_ -= weight_to_use;
				triangles_ -= 1;

				if(local_){
					triangles_local_map_[u]--;
					triangles_local_map_[v]--;
					triangles_local_map_[n]--;
					triangles_weight_local_map_[u]-=weight_to_use;
					triangles_weight_local_map_[v]-=weight_to_use;
					triangles_weight_local_map_[n]-=weight_to_use;
					// To avoid numerical error
					triangles_weight_local_map_[u]= max(triangles_weight_local_map_[u], 0.0);
					triangles_weight_local_map_[v]= max(triangles_weight_local_map_[v], 0.0);
					triangles_weight_local_map_[n]= max(triangles_weight_local_map_[n], 0.0);
				}
			}
		}
	}
}


void TriangleCounter::new_update(const EdgeUpdate& update){
	if(update.is_add){
		edges_present_original_++;
	} else {
		edges_present_original_--;
	}
}

void TriangleCounter::clear() {
	triangles_ = edges_present_original_ = triangles_weight_ = 0;
	graph_.clear();
	set_edge_ids_.clear();
	edge_weight_.clear();
}

unsigned long long int TriangleCounter::edges_present_original() const {
	return edges_present_original_;
}

int TriangleCounter::common_neighbors(const int u, const int v) const {
	int min_deg_n = (graph_.degree(u) <= graph_.degree(v) ? u : v);
	int max_deg_n = (graph_.degree(u) <= graph_.degree(v) ? v : u);

	vector<int> min_neighbors;
	graph_.neighbors(min_deg_n, & min_neighbors);

	//unordered_set<int> min_set(min_neighbors.begin(), min_neighbors.end());
  //vector<int> max_neighbors;
	//graph_.neighbors(max_deg_n, & max_neighbors);
	//int ret = 0;
	//for(const auto& n : max_neighbors){
	//	if(min_set.find(n) != min_set.end()){
	//		ret++;
	//	}
	//}

	int ret = 0;
	for(const auto& n : min_neighbors){
		if(n!= max_deg_n){
			if(set_edge_ids_.find(edge_to_id(n, max_deg_n)) != set_edge_ids_.end()){
				ret++;
			}
		}
	}


	return ret;
}
