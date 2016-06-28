/*
 * GraphScheduler.cpp
 *
 *  Created on: Oct 22, 2014
 *      Author: aepasto
 */

#include "GraphScheduler.h"
#include <cstring>
#include <cassert>
#include <iostream>
#include <functional>
using namespace std;

GraphScheduler::~GraphScheduler() {
	this->file_stream_.close();
}

GraphScheduler::GraphScheduler(const string& file_name, bool store_time) {

	store_time_ = store_time;
	file_stream_.open(file_name.c_str(), ios_base::in);
	retrieve_next_chunk();
	assert(this->has_next());
	add_count = 0;
	remove_count = 0;
}

void GraphScheduler::retrieve_next_chunk() {

	std::string delimiter = " ";
	string line;
	vector<string> tokens;

	EdgeUpdate next_edge;
	EdgeUpdateNoTime next_edge_no_time;

	int read = 0;

	while (read++ < CHUNK_SIZE && getline(file_stream_, line)) {
		tokens.clear();
		//cout <<" READING "<<line<< endl;
		size_t pos = 0;
		std::string token;
		while (pos != line.npos) {
			pos = line.find(delimiter);
			token = line.substr(0, pos);
			tokens.push_back(token);
			line.erase(0, pos + delimiter.length());
		}

		assert(tokens.size() >= 3 && tokens.size() <= 4);

		int start_rest_tokens = 0;
		if(tokens.size() == 4){ // plus/minus u v time
			if (tokens[0][0] == '+') {
				next_edge.is_add = true;
			} else if (tokens[0][0] == '-') {
				next_edge.is_add = false;
			} else {
				assert(false);
			}
			start_rest_tokens = 1;
		} else if (tokens.size() == 3){ // no sign assume +
			next_edge.is_add = true;
			start_rest_tokens = 0;
		} else {
			assert(false);
		}

		next_edge.node_u = atoi(tokens[start_rest_tokens].c_str());
		next_edge.node_v = atoi(tokens[start_rest_tokens+1].c_str());
		if (store_time_){
			next_edge.time = atoi(tokens[start_rest_tokens+2].c_str());
		}else{
			next_edge.time = 0;
		}
		assert(next_edge.node_u >= 0);
		assert(next_edge.node_v >= 0);
		assert(next_edge.time >= 0);
		//cerr <<"EDGE: "<<next_edge.node_u<< " " << next_edge.node_v<< " " << next_edge.time<< endl;


		if(next_edge.node_u == next_edge.node_v){
			//cerr <<"ERR: Loop"<<endl;
			continue;
		}

		if (store_time_){
			edge_queue_.push(next_edge);
		} else {
			next_edge_no_time.is_add = next_edge.is_add;
			next_edge_no_time.node_u = next_edge.node_u;
			next_edge_no_time.node_v = next_edge.node_v;
			edge_queue_no_time_.push(next_edge_no_time);
		}
	}
}

EdgeUpdate GraphScheduler::next_update() {


	EdgeUpdate edge_queue;
	if (store_time_){
		assert(!edge_queue_.empty());
		if (edge_queue_.size() == 1){
			retrieve_next_chunk();
		}

		edge_queue = edge_queue_.front();
		edge_queue_.pop();
	} else {
		assert(!edge_queue_no_time_.empty());
		if (edge_queue_no_time_.size() == 1){
			retrieve_next_chunk();
		}


		EdgeUpdateNoTime no_time = edge_queue_no_time_.front();
		edge_queue_no_time_.pop();
		edge_queue.is_add = no_time.is_add;
		edge_queue.node_u = no_time.node_u;
		edge_queue.node_v = no_time.node_v;
		edge_queue.time = 0;
	}

	if (edge_queue.is_add) {
		++add_count;
	} else {
		++remove_count;
	}
	/*if (edge_queue.is_add && add_count % 100000 == 99999) {
		cerr << "ADD #: " << add_count + 1 << endl;
		cerr.flush();
	} else if (!edge_queue.is_add && remove_count % 100000 == 99999) {
		cerr << "REM #: " << remove_count + 1 << endl;
		cerr.flush();
	}*/
	return edge_queue;
}
