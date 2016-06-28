
#ifndef STATS_H_
#define STATS_H_

#include <vector>
#include <chrono>

using namespace std;

typedef struct Stat {
	unsigned long operation_num;
	unsigned int add_op_in_window;
	unsigned int rem_op_in_window;
	double micros;

	double last_triangles_est;
	unsigned long long last_triangles_count;
	unsigned int last_size_sample;

	double micros_per_op;
	unsigned int last_timestamp;

	unsigned int op_count_total;
} Stat;

class Stats {
public:
	explicit Stats(const int stat_window_size) :
			stat_window_size_(stat_window_size), op_count_(0) {

		add_count_window_ = remove_count_window_ = 0;
		last_triangles_est_ = last_triangles_count_ = last_size_sample_ = 0;
		last_op_count_ = 0;
	}
	virtual ~Stats();

	void exec_op(bool is_add, unsigned long long last_triangles_count, double last_triangles_est, unsigned int last_size_sample, unsigned int timestamp);
	void end_op();

	const vector<Stat> stats() {
		return stats_;
	}

private:
	void reset_window();

	const unsigned int stat_window_size_;

	unsigned int add_count_window_;
	unsigned int remove_count_window_;
	unsigned int last_size_sample_;
	
	double last_triangles_est_;
	unsigned long long last_triangles_count_;

	unsigned int last_op_count_;
	unsigned int last_timestamp_;

	std::chrono::system_clock::time_point last_time_;

	unsigned int op_count_;
	vector<Stat> stats_;
};

#endif /* STATS_H_ */
