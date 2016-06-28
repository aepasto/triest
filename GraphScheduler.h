/*
 * GraphScheduler.h
 *
 *  Created on: Oct 22, 2014
 *      Author: aepasto
 */

#ifndef GRAPHSCHEDULER_H_
#define GRAPHSCHEDULER_H_

#include <algorithm>
#include <vector>
#include <queue>
#include <fstream>
using namespace std;

//Number of lines read each time
#define CHUNK_SIZE 10000

enum Update {
	ADD, REM
};

typedef struct EdgeUpdate {
	int node_u;
	int node_v;
	int time;
	bool is_add;
} EdgeUpdate;

typedef struct EdgeUpdateNoTime {
	int node_u;
	int node_v;
	bool is_add;
} EdgeUpdateNoTime;




class GraphScheduler {
public:
	GraphScheduler(const string& file_name, bool store_time_);
	virtual ~GraphScheduler();

	EdgeUpdate next_update();
	inline bool has_next() {
		if (store_time_) {
			return !edge_queue_.empty();
		} else {
			return !edge_queue_no_time_.empty();
		}
	}

private:
	bool store_time_;
	int add_count;
	int remove_count;
	void retrieve_next_chunk();
	ifstream file_stream_;
	queue<EdgeUpdate> edge_queue_;
	queue<EdgeUpdateNoTime> edge_queue_no_time_;
};

#endif /* GRAPHSCHEDULER_H_ */
