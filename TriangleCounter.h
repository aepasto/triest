
#ifndef TRIANGLE_COUNTER_H_
#define TRIANGLE_COUNTER_H_

#include "GraphScheduler.h"
#include "UDynGraph.h"


// hashing pairs
namespace std {
	template <> struct hash<std::pair<int, int>> {
  	inline size_t operator()(const std::pair<int, int> &v) const {
    	std::hash<int> int_hasher;
    	return int_hasher(v.first) ^ int_hasher(v.second);
  	}
	};
}

using namespace std;


// For each edge update:
// 1) call new_update
// 2) if the edge need to be added the sample call add_edge
// 		or if need to be removed remove_edge
// NEW_UPDATE ASSUMES: all edges added are not present (in the original graph) and all removed are present (in the original graph)
// remove_edge assumes that the edge was in the sample.


// Not crucial can be eliminated used only for speedup
#define MAX_NUM_NODES 50000000

unsigned long long edge_to_id(const int u,
		const int v);


class TriangleCounter {
public:
	TriangleCounter(bool local);
	virtual ~TriangleCounter();

	void clear();

	// Add or remove edge from sample (no other operation executed)
	bool add_edge_sample(const int u, const int v);
	bool remove_edge_sample(const int u, const int v);
	// Increments the counters of seen edges
	void new_update(const EdgeUpdate& update);
	// Increase or decrease the triangles (notice the sample is not affected)
	void add_triangles(const int u, const int v, double weight);
	void remove_triangles(const int u, const int v, double weight);

	unsigned long long int edges_present_original() const;

	inline unsigned long long int triangles() const{
		return triangles_;
	}
	inline double triangles_weight() const{
		return triangles_weight_;
	}
	inline unsigned long long int triangles_local(int n) const{
		if (triangles_local_map_.find(n)==triangles_local_map_.end()){
			return 0;
		} else {
			return triangles_local_map_.at(n);
		}
	}
	inline double triangles_weight_local(int n) const{
		if (triangles_weight_local_map_.find(n)==triangles_weight_local_map_.end()){
			return 0.0;
		} else {
			return triangles_weight_local_map_.at(n);
		}
	}

	inline int size_sample() const {
		return graph_.num_edges();
	}
	inline bool is_local() const {
		return local_;
	}
	void add_edge_weight(const int u, const int v, const double w){
		edge_weight_[make_pair(u,v)] = w;
		edge_weight_[make_pair(v,u)] = w;
	}

	void remove_edge_weight(const int u, const int v) {
		edge_weight_.erase(make_pair(u,v));
		edge_weight_.erase(make_pair(v,u));
	}

	void get_nodes(vector<int>*nodes_v){
		nodes_v->clear();
		graph_.nodes(nodes_v);
	}

private:
	bool local_;

	int common_neighbors(const int u, const int v) const;
	UDynGraph graph_;

	unordered_set<unsigned long long> set_edge_ids_;//used for fast lookup of x,y edge

	unsigned long long int triangles_;
	double triangles_weight_;

	unordered_map<int, unsigned long long> triangles_local_map_;
	unordered_map<int, double> triangles_weight_local_map_;


	unsigned long long int edges_present_original_; // count of current edges in
	//the original graph (not in the sampled graph).

	// used only for add/rem reservoir
	unordered_map<pair<int,int>, double> edge_weight_;


};

#endif /* TRIANGLE_COUNTER_H_ */
