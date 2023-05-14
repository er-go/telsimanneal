#include "includes.h"

#include <csignal>
#include <thread>

#include "Direction.h"
#include "SimAnneal.h"
#include "TelAnnealer.h"
#include "TelGreedy.h"

/* ************************************************** */

shared_ptr<DirectionDatabase> load_directions(int run_id) {
	ifstream infile { file_reader(get_input_filename(run_id)) };

	constexpr int LEN_RUNID_LABEL = string_view("Run id: ").length();
	infile.ignore(LEN_RUNID_LABEL);
	int run_id_check;
	infile >> run_id_check;
	if (run_id != run_id_check) {
		throw std::runtime_error("Run id was incorrect in an input file.\n"
				"Run id in file name: " + to_string(run_id) + "\n"
				"Run id within input: " + to_string(run_id_check));
	}

	constexpr int LEN_NUMDIRS_LABEL
		= string_view("Num directions: ").length();
	infile.ignore(LEN_NUMDIRS_LABEL);
	int num_directions;
	infile >> num_directions;

	auto dirdata = make_shared<DirectionDatabase>(num_directions);
	for (dir_id_t expected_id { 0 };
			expected_id < num_directions;
			expected_id++) {
		auto d = Direction::read_from(infile);
		dirdata->place_direction(move(d));
	}
	infile.close();
	return dirdata;
}

/* ************************************************** */

int run(int run_id, long run_num_epochs, long vb_every,
		double cool_init, double cool_base, long cool_flat_epochs) {
	shared_ptr<DirectionDatabase> dirdata;
	try {
		dirdata = load_directions(run_id);
	} catch (exception& e) {
		cerr << "ERROR: " << e.what() << endl;
		return -3;
	}

	/* ************************************************** */

	try {
		cout.setf(ios_base::boolalpha);
		cout << "Setup for run id = " << run_id << endl;

		for (int sr {0}; sr < 2; sr++) {
			bool without_second_rep { (sr == 1) };
			cout << "Allowing second rep "
						<< (without_second_rep == false) << endl;

			unique_ptr<cooling::CoolingFn> coolptr {
				new cooling::PiecewiseConstGeomCool
						{ cool_init, cool_base, cool_flat_epochs }
			};
			TelAnnealer telannealer { run_id, move(coolptr), dirdata,
										without_second_rep };
			cout << "Annealing..." << endl;
			telannealer.run(run_num_epochs, vb_every);

			cout << "Trying greedy approach..." << endl;
			TelGreedy telgreedy { run_id, dirdata, without_second_rep };
			double greedy_dist { telgreedy.run_and_save() };
			cout << "Greedy distance: " << greedy_dist << endl;
		}
	} catch (exception& e) {
		cerr << "ERROR: " << e.what() << endl;
		return -4;
	}

	return 0;
}

/*
 * NOTE:  This is a very simple stand-in for the "jthread" class, as
 * is explained here:
 *		https://en.cppreference.com/w/cpp/thread/jthread
 * because the current compiler version does not support it. Also see
 * the discussion of joining threads in the book "C++ Concurrency in
 * Action" by A. Williams.
 */
struct thread_manager {
	template <typename ... args>
	thread_manager(args ... a)
	: t {a...} {}
	thread_manager(thread_manager&& tm)
		: t {forward<thread>(tm.t)} {}
	~thread_manager() {
		if (t.joinable()) {
			t.join();
		}
	}
private:
	thread t;
};

/* ************************************************** */

int main(int argc, char** argv) {
	std::signal(SIGINT, [] (int signal) {
			cerr << "Interrupted. Exiting." << endl;
			throw runtime_error("Program interrupted.");
		});

	if (argc < 8) {
		cerr << "Wrong number of command line arguments provided."
				" Must specify, in order:\n"
				"  1. number of threads to use\n"
				"  2. number of simulated annealing epochs\n"
				"  3. frequency of status updates printed to std::cout\n"
				"  4. cooling initial scale (strictly positive) \n"
				"  5. cooling exponential base (strictly between 0 and 1) \n"
				"  6. flat cooling exponential scale (strictly positive)\n"
				"  7. run id, or a list of ids"
				<< endl;
		return -1;
	}
	int NUM_THREADS;
	long run_num_epochs;
	long vb_every;

	double cool_init;
	double cool_base;
	long cool_flat_epochs;

	vector<int> run_id_list {};

	try {
		vector<string> params {};
		for (int i {1}; i < argc; i++) {
			/* Do nothing with parameter i=0, which is the program name.
			 * For every other parameter, allow underscores so numbers are easy
			 * to read (e.g., 1_000_000 instead of 1000000).  These need to be
			 * removed.  After removing them, however, remember to erase
			 * the extra space at the end!  (Refer to the discussion on
			 * pp. 604-605 in Lospinoso's "C++ Crash Course".)
			 */
			string param { argv[i] };
			auto s_orig_end { param.end() };
			auto s_new_end { std::remove(param.begin(), s_orig_end, '_') };
			param.erase(s_new_end, s_orig_end);

			const string param_error_message =
				"Command line argument of the wrong format.\n"
				"  + Parameter 1–3 and 6 must be positive integers.\n"
				"  + Parameters 4–5 should be decimals.\n"
				"  + Parameter 7 and on must be a list of run id integers.\n"
				"Note: Parameters may include underscores '_' if desired.\n"
				"Provided parameter " + to_string(i) + " as \"" + param + "\"";
			string regex { i == 4 or i == 5 ?
							"(0|([1-9][0-9]*))(.[0-9]*)?"
						  :	"0|([1-9][0-9]*)"
					};
			auto match = wrap_regex_match(param, regex, param_error_message);

			/* NOTE: that easy_match returned without throwing an exception
			 * ensures the parameter actually had the correct form. */
			params.push_back(param);
		}
		NUM_THREADS    = { stoi(params[0]) };
		run_num_epochs = { stol(params[1]) };
		vb_every       = { stol(params[2]) };

		cool_init        = { stod(params[3]) };
		cool_base        = { stod(params[4]) };
		cool_flat_epochs = { stol(params[5]) };

		for (size_t idx {6}; idx < params.size(); idx++) {
			run_id_list.push_back(stoi(params[idx]));
		}

		// Do some routine input verification.
		if (NUM_THREADS <= 0) {
			throw runtime_error("Provided number of threads "
					+ to_string(NUM_THREADS) + ", however "
					"this value must be strictly positive.");
		} else if (auto hc {std::thread::hardware_concurrency()};
					hc > 0 and NUM_THREADS >= hc) {
			cerr << "Warning: Specified "
					<< NUM_THREADS
					<< " threads, but runtime says only "
					<< hc
					<< " might be available."
					<< endl;
		} else if (run_num_epochs <= 0) {
			throw runtime_error("Provided number of epochs "
					+ to_string(run_num_epochs) + ", however "
					"this value must be strictly positive.");
		} else if (vb_every <= 0) {
			throw runtime_error("Provided verbose frequency "
					+ to_string(vb_every) + ", however "
					"this value must be strictly positive.");
		} else if (cool_init <= 0.0) {
			throw runtime_error("Provided cooling initial scale "
					+ to_string(cool_init) + ", however "
					"this value must be strictly positive.");
		} else if (cool_base <= 0.0 or cool_base >= 1.0) {
			throw runtime_error("Provided cooling exponential base "
					+ to_string(cool_base) + ", however "
					"this value must be strictly between 0.0 and 1.0.");
		} else if (cool_flat_epochs <= 0) {
			throw runtime_error("Provided cooling flat epochs "
					+ to_string(cool_flat_epochs) + ", however "
					"this value must be strictly positive.");
		}
	} catch (exception& e) {
		cerr << "ERROR: " << e.what() << endl;
		return -2;
	};

	/* Figure out how many run ids will be computed on
	 * each thread.
	 */
	auto num_ids {run_id_list.size()};
	if (auto remainder { num_ids % NUM_THREADS };
				remainder > 0) {
		num_ids += (NUM_THREADS - remainder);
	}
	size_t ids_per_thread { num_ids / NUM_THREADS };
	try {
		vector<thread_manager> all_threads;
		all_threads.reserve(NUM_THREADS);

		for (size_t i {0}; i < NUM_THREADS; i++) {
			auto start {ids_per_thread *  i };
			auto end   { i == NUM_THREADS - 1 ?
							run_id_list.size()
						 :	ids_per_thread * (i+1) };

			all_threads.push_back(thread_manager { [=]() {
				for (size_t idx {start}; idx < end; idx++) {
					auto run_id {run_id_list[idx]};
					run(run_id, run_num_epochs, vb_every,
							cool_init, cool_base, cool_flat_epochs);
				}
			} });
		}
	} catch (exception& e) {
		cerr << e.what() << endl;
	}
	return 0;
}
