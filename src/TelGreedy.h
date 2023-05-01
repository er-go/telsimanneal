#pragma once

#include "includes.h"

#include <unordered_set>

#include "Schedule.h"

using nanos = std::chrono::nanoseconds;
string get_save_filename(int run_id) {
	return OUTPUT_FOLDER + "run-" + to_string(run_id) +
				"/greedy-solution.txt";
}

class TelGreedy {
public:
	TelGreedy(int run_id, shared_ptr<DirectionDatabase> dirdata) :
		run_id {run_id},
		dirdatabase    {dirdata},
		num_dir        {dirdatabase->get_num_directions_defined()},
		sch {nullptr}
		{}
	~TelGreedy() = default;
	TelGreedy(TelGreedy&) = delete;
	TelGreedy(TelGreedy&&) = delete;

	TelGreedy& operator=(TelGreedy&) = delete;
	TelGreedy& operator=(TelGreedy&&) = delete;

	double run_and_save() {
		vector<dir_id_t> s_ids {};
		vector<bool>     s_othr {};
		s_ids.reserve(num_dir);
		s_othr.reserve(num_dir);

		unordered_set<dir_id_t> unvisited {};
		for (dir_id_t id {1};
				id < dirdatabase->get_num_directions_defined();
				id++) {
			unvisited.insert(id);
		}

		/* By convention, the starting location must be
		 * id #0 using the standard representation.
		 */
		const Direction* current_dir { & ( dirdatabase->get_direction(0, false) ) };

		auto start { chrono::high_resolution_clock::now() };
		s_ids.push_back(0);
		s_othr.push_back(false);
		while (not unvisited.empty()) {
			dir_id_t best_id   { 1 }; // will be reset
			bool     best_othr { false };
			double	 best_dist { numeric_limits<double>::max() } ;

			for (auto uv : unvisited) {
				for (int othr {0}; othr < 2; othr++) {
					bool use_othr { (othr == 1) };
					double potential_dist { current_dir->dist_to(
								dirdatabase->get_direction(uv, use_othr)) };
					if (potential_dist < best_dist) {
						best_id = uv;
						best_othr = use_othr;
						best_dist = potential_dist;
					}
				}
			}
			s_ids.push_back(best_id);
			s_othr.push_back(best_othr);
			unvisited.erase(best_id);
			current_dir = & ( dirdatabase->get_direction(best_id, best_othr) );
		}
		auto stop { chrono::high_resolution_clock::now() };
		time_running = stop - start;
		sch = make_unique<Schedule>(move(s_ids), move(s_othr), dirdatabase);
		save(get_save_filename(run_id));
		return sch->total_distance();
	}

private:
	int run_id;
	shared_ptr<DirectionDatabase> dirdatabase;
	size_t num_dir;
	unique_ptr<Schedule> sch;
	nanos time_running;

	void save(string filename) {
		ofstream o { file_writer(filename)};
		o.setf(ios_base::fixed);
		o << setprecision(10);
		o << "Run id: " << run_id
		  << "\nObjective: " << sch->total_distance()
		  << "\nTime running (ns): " << time_running.count()
		  << "\n" << SEPARATOR
		  << "\nGreedy solution:\n"
		  << *sch
		  << SEPARATOR
		  << endl;
		o.close();
	}
};
