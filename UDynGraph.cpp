#include "UDynGraph.h"
#include <iostream>
#include <cassert>
#include <algorithm>

UDynGraph::UDynGraph() :
        num_nodes_(0), num_edges_(0) {
}

UDynGraph::~UDynGraph() {
}

void UDynGraph::clear() {
    nodes_.clear();
    node_map_.clear();
    num_nodes_ = 0;
    num_edges_ = 0;
}

bool UDynGraph::add_edge(const int source, const int destination) {
    assert(source != destination);
    assert(source >= 0);
    assert(destination >= 0);

    if (node_map_.find(source) != node_map_.end()) {
        const auto & vec = node_map_.at(source);
        if (std::find(vec.begin(), vec.end(), destination) != vec.end()) {
            return false;
        }
    }

    ++num_edges_;

    if (nodes_.find(source) == nodes_.end()) {
        nodes_.insert(source);
        ++num_nodes_;
    }
    if (nodes_.find(destination) == nodes_.end()) {
        nodes_.insert(destination);
        ++num_nodes_;
    }

    node_map_[destination].push_back(source);
    node_map_[source].push_back(destination);

    return true;
}

bool UDynGraph::remove_edge(const int source, const int destination) {

    if (node_map_.find(source) == node_map_.end()) {
        return false;
    } else {
        const auto & vec = node_map_.at(source);
        if (std::find(vec.begin(), vec.end(), destination) == vec.end()) {
            return false;
        }
    }

    --num_edges_;

    if (node_map_[source].size() == 1) {
        node_map_.erase(source);
        --num_nodes_;
    } else {

        *std::find(node_map_[source].begin(), node_map_[source].end(),
                destination) = node_map_[source][node_map_[source].size() - 1];
        node_map_[source].resize(node_map_[source].size() - 1);
    }

    if (node_map_[destination].size() == 1) {
        node_map_.erase(destination);
        --num_nodes_;
    } else {

        *std::find(node_map_[destination].begin(), node_map_[destination].end(),
                source) = node_map_[destination][node_map_[destination].size()
                - 1];
        node_map_[destination].resize(node_map_[destination].size() - 1);
    }

    return true;
}

int UDynGraph::num_edges() const {
    return num_edges_;
}

int UDynGraph::num_nodes() const {
    return num_nodes_;
}

void UDynGraph::neighbors(const int source, vector<int>* vec) const {
    vec->clear();
    if (node_map_.find(source) != node_map_.end()) {
        const vector<int>& neighbors = node_map_.find(source)->second;
        vec->assign(neighbors.begin(), neighbors.end());
    }
}

void UDynGraph::nodes(vector<int>* vec) const {
    vec->clear();
    vec->assign(nodes_.begin(), nodes_.end());
}

int UDynGraph::degree(const int source) const {
    if (node_map_.find(source) == node_map_.end()) {
        return 0;
    }
    return node_map_.find(source)->second.size();
}

void UDynGraph::edges(vector<pair<int, int> >* vec) const {
    vec->clear();
    for (auto & v_pair : node_map_) {
        int src = v_pair.first;
        for (auto & out_neighbor : v_pair.second) {

            vec->push_back(make_pair(src, out_neighbor));
        }
    }

}
