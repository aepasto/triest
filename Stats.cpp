
#include "Stats.h"
#include <cstring>
#include <ctime>
#include <cassert>
#include <chrono>
#include <iostream>


using namespace std;

Stats::~Stats() {
}

#define SEPARATOR "\t"

void Stats::reset_window() {
	Stat new_stat;

	auto now = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
			now - last_time_);
	new_stat.micros = elapsed.count();
	last_time_ = now;

	new_stat.operation_num = op_count_ - last_op_count_;
	new_stat.add_op_in_window = add_count_window_;
	new_stat.rem_op_in_window = remove_count_window_;
	new_stat.last_triangles_est = last_triangles_est_;
	new_stat.last_triangles_count = last_triangles_count_;
	new_stat.op_count_total = op_count_;
	assert(new_stat.operation_num > 0);
	new_stat.micros_per_op = new_stat.micros / new_stat.operation_num;
	new_stat.last_timestamp = last_timestamp_;
	new_stat.last_size_sample = last_size_sample_;

	add_count_window_ = 0;
	remove_count_window_ = 0;
	last_op_count_ = op_count_;

	stats_.push_back(new_stat);

	cout << new_stat.op_count_total << SEPARATOR << new_stat.last_timestamp << SEPARATOR
			<< new_stat.last_triangles_count << SEPARATOR << new_stat.last_triangles_est << SEPARATOR
			<< new_stat.last_size_sample << SEPARATOR << new_stat.operation_num
			<< SEPARATOR << new_stat.micros << SEPARATOR << new_stat.micros_per_op << SEPARATOR
			<< new_stat.add_op_in_window << SEPARATOR << new_stat.rem_op_in_window
			<< endl;

	now = std::chrono::system_clock::now();
	last_time_ = now;
}

void Stats::end_op() {
	if (op_count_ != last_op_count_) {
		reset_window();
	}
}

void Stats::exec_op(bool is_add, unsigned long long last_triangles_count, double last_triangles_est, unsigned int last_size_sample, unsigned int last_timestamp) {
	if (op_count_ == 0) {
		last_time_ = std::chrono::system_clock::now();
		cout << "op_count_total" << SEPARATOR << "last_timestamp" << SEPARATOR
				<< "last_triangles_count" << SEPARATOR << "last_triangles_est" << SEPARATOR
				<< "last_size_sample"<< SEPARATOR << "operation_num" << SEPARATOR
				<< "micros_total" << SEPARATOR << "micros_per_op" << SEPARATOR
				<< "add_op_in_window" << SEPARATOR << "rem_op_in_window" << endl;
	}

	if (is_add) {
		++add_count_window_;
	} else {
		++remove_count_window_;
	}

	++op_count_;

	last_triangles_count_ = last_triangles_count;
	last_triangles_est_ = last_triangles_est;
	last_size_sample_ = last_size_sample;

	if (is_add){
		last_timestamp_ = last_timestamp;
	}

	if (op_count_ % stat_window_size_ == 0) {
		reset_window();
	}
}
