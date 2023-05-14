#pragma once

#include "includes.h"

namespace cooling {

	class CoolingFn {
	public:
		CoolingFn()            = default;
		virtual ~CoolingFn()   = default;
		CoolingFn(CoolingFn&)  = default;
		CoolingFn(CoolingFn&&) = default;
		virtual double coolingfn(long epoch) = 0;
		string descr;
	};

	class GeomCool : public CoolingFn {
	public:
		GeomCool(double init_scale, double base)
			: init_scale {init_scale},
			  base {base}
			{
				ostringstream s {this->descr};
				s << "Geometric cooling schedule:\n"
						<< init_scale << " * (" << base << "^epoch)";
				descr = s.str();
			}
		~GeomCool() = default;
		GeomCool(GeomCool&)  = default;
		GeomCool(GeomCool&&) = default;
		double coolingfn(long epoch) override {
			return init_scale * pow(base, epoch);
		}
	private:
		double init_scale, base;
	};

	class PiecewiseConstGeomCool : public CoolingFn {
	public:
		PiecewiseConstGeomCool(double init_scale, double base, long epochs_flat)
		: init_scale { init_scale }, base { base },
		  epochs_flat { epochs_flat } {
			ostringstream s { this->descr };
			s << "Piecewise constant geometric cooling schedule:\n"
					<< init_scale << " * " << base
					<< "^(epoch / " << epochs_flat << ")";
			descr = s.str();
		}
		~PiecewiseConstGeomCool() = default;
		PiecewiseConstGeomCool(PiecewiseConstGeomCool&)  = default;
		PiecewiseConstGeomCool(PiecewiseConstGeomCool&&) = default;
		double coolingfn(long epoch) override {
			return init_scale * pow(base, epoch / epochs_flat);
		}
	private:
		double init_scale, base;
		long epochs_flat;
	};

}

int get_rand_seed(int run_id) {
	/* This is an easy system to manage different seeds for different
	 * run IDs, but it is not necessarily a good system.
	 */
	return run_id;
}

using nanos = std::chrono::nanoseconds;

template<typename T>
class SimAnnealer {
/* This is a base class for a simulated annealing optimizer.
 * For reference, see the book "Finite Markov Chains and Algorithmic
 * Applications" by O. Häggström.
 */
public:
	SimAnnealer(int run_id, unique_ptr<T>&& start_state,
			unique_ptr<cooling::CoolingFn>&& cooler) :
		run_id {run_id},
		coolfn {move(cooler)},
		state_curr {move(start_state)},
		state_best {}, // DO NOT call virtual method duplicate here. Call it in run() below.
		obj_curr  {},
		obj_best  {},
		time_curr {},
		time_best {},
		annealer_random_generator {} {
			/* NOTE: You cannot construct an mt instance with a non-constant
			 * or perhaps static integer using its constructor.  However,
			 * the URL below indicates that there is a way to reset the
			 * starting state, the seed() method.
			 *     https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine
			 */
			annealer_random_generator.seed(get_rand_seed(run_id));
		}

	virtual ~SimAnnealer() = default;
	SimAnnealer(SimAnnealer&)  = delete;
	SimAnnealer(SimAnnealer&&) = delete;

	/* Below are the pure virtual methods for derived classes to implement.
	 * Briefly:
	 *
	 * - objective_to_minimize(t) should compute the objective function
	 *   for the state t. It is important to remember that a LOWER objective
	 *   is BETTER, hence the name.
	 *
	 * - sample_step(s1, s2, rand) should use the given random generator
	 *   to sample a valid step from state s1 to one of its neighbors.  The
	 *   state s2 should be used to store the sampled neighbor.
	 *
	 * - write(ostr, t) should nicely write the state t into ostr.
	 *
	 * - duplicate(t) should return a duplicate of t, a fully independent copy.
	 *
	 * - copy_from_to(s1, s2) should copy all information about the state
	 *   from s1 into s2.
	 */
	virtual double objective_to_minimize(const T& t) = 0;
	virtual void sample_step(const T& from, T& storage,
								std::mt19937_64& random_generator) = 0;
	virtual void write(ostream& ostr, const T& t) = 0;
	virtual unique_ptr<T> duplicate(const T& t) = 0;
	virtual void copy_from_to(const T& from, T& into) = 0;

	/* ************************************************** */

	virtual void save_best_state(string filename, bool current_also=false) final {
		ofstream o { file_writer(filename) };
		o.setf(ios_base::fixed);
		o << setprecision(10);
		o << "Run id: " << run_id
		  << "\nCurrent objective: " << obj_curr
		  << "\nBest objective: " << obj_best
		  << "\n" << SEPARATOR
		  << "\nCurrent epoch: " << time_curr.epoch
		  << "\nTime running (ns): " << time_curr.wall_time_ns.count()
		  << "\nCooling method description:\n"
		  << coolfn->descr
		  << "\n" << SEPARATOR
		  << "\nRandom start: "
		  << get_rand_seed(run_id) << "\n"
		  << SEPARATOR;
		if (current_also) {
			o << "\nCurrent State:\n";
			write(o, *state_curr);
			o << SEPARATOR;
		}
		o << "\nBest state from epoch: "
		  << time_best.epoch
		  << "\nBest found after time (ns): "
		  << time_best.wall_time_ns.count()
		  << "\nBest State:\n";
		write(o, *state_best);
		o << SEPARATOR;
		o << endl;
		o.close();
	}

	/* ************************************************** *
	 * Explanation of some parameters for the run(...) method:
	 *
	 * - num_epochs is self-explanatory; it is how many epochs to run.
	 *
	 * - verbose_every says how often to write information
	 *   to std::cout.
	 *
	 * - SAVE_TOLERANCE is how much the objective needs to decrease
	 *   before the state is saved once more.
	 */

	virtual void run(
		unsigned long num_epochs,
		unsigned long verbose_every=50,
		const double SAVE_TOLERANCE=0.1
	) final {

		cout.setf(ios_base::scientific);
		cout << setprecision(10);

		/* ----------------------------------------
		 * Set up the full annealing log.
		 */

		string filename { get_annealing_filename_for_full_log(run_id) };
		ofstream full_log { file_writer(filename) };
		full_log << setprecision(10);
		full_log << "Run id: " << run_id
				<< "\nBest objective remained constant between epochs listed below."
				<< "\n(Current objective may have changed, however.)"
				<< "\nEpoch, Current Objective, Best Objective, Wall Time (ns)\n";

		state_best = state_curr->duplicate();
		obj_curr = objective_to_minimize(*state_curr);
		obj_best = obj_curr;

		/* Assign some memory for several different mid-calculation objects,
		 * rather than reassigning memory at every iteration of the loop.
		 *
		 * The values of save_and_log and vb will be decided anew at each epoch
		 * to determine what output there is:
		 *
		 * - should_vb   says whether to write to cout; and
		 * - should_save says whether to call save_state()
		 * - should_log  says whether to write into the log, full_log.
		 */
		bool should_vb {false}, should_save {false}, should_log {false};
		std::uniform_real_distribution<double> unif {};

		auto state_storage { state_curr->duplicate() };
		double obj_storage {};
		double obj_prev_saved  { 10 * max(obj_curr, 1.0)};
		double obj_prev_logged { 10 * max(obj_curr, 1.0)};

		/* Start the clock and GO! */
		using clock = std::chrono::high_resolution_clock;
		auto start { clock::now() };
		auto temp  { start };

		for (unsigned long epochs_remaining {num_epochs};
				epochs_remaining > 0;
				epochs_remaining--) {
			time_curr.epoch += 1;
			this->sample_step(*state_curr, *state_storage, annealer_random_generator);
			obj_storage = this->objective_to_minimize(*state_storage);

			/* The next if-else pair decides whether or not the chain
			 * will move during this epoch.  In the "if" block,
			 * the chain automatically moves without computing any
			 * probabilities.  This is an immediate consequence of the
			 * form of the probabilities---see the comment within the
             * else block.
			 */
			if (obj_storage < obj_curr) {
				// Change the current state and update the objective.
				swap(state_curr, state_storage);
				obj_curr   = obj_storage;

				if (obj_curr < obj_best) {
					/* Because the objective went down, we must check
					 * whether it has beaten the best objective so far.
					 */
					time_best.epoch = time_curr.epoch;
					temp = clock::now();
					time_curr.wall_time_ns += temp - start;
					time_best.wall_time_ns  = time_curr.wall_time_ns;
					start = temp;
					copy_from_to(*state_curr, *state_best);
					obj_best = obj_curr;
				}
			} else {
				/* Here is the main appearance of the exponent related to
				 * the Boltzman distribution.  The probability of switching
				 * states is the exponent of the value below.  See Häggström's
				 * book "Finite Markov Chains and Algorithmic Applications"
                 * for details.
				 *
				 * Because of the if statement above, the exponent here, called
				 * log_move_prob, is always negative.
				 */
				double log_move_prob {
					(obj_curr - obj_storage)
						/ coolfn->coolingfn(time_curr.epoch) };
				if (unif(annealer_random_generator) < std::exp(log_move_prob)) {
					// Change the current state and update the objective.
					swap(state_curr, state_storage);
					obj_curr = obj_storage;
				}
			}

			/* ----------------------------------------
			 * Determine which logs to write.
			 */
			should_log = (
					obj_best < obj_prev_logged
					or epochs_remaining == 1
					or epochs_remaining == num_epochs
				);
			should_save = (
					obj_best < obj_prev_saved - SAVE_TOLERANCE
					or epochs_remaining == 1
					or epochs_remaining == num_epochs
				);
			should_vb = (
					verbose_every > 0
					and time_curr.epoch % verbose_every == 0
					);

			if (should_log or should_save) {
				/* Need to update the clock. */
				auto stop = clock::now();
				time_curr.wall_time_ns += (stop - start);
				start = stop;

				if (should_save) {
					save_best_state(get_annealing_filename_for_epoch(run_id, time_curr.epoch));
					obj_prev_saved = obj_best;
				}

				if (should_log) {
					full_log << time_curr.epoch << ", "
							<< obj_curr << ", "
							<< obj_best << ", "
							<< time_curr.wall_time_ns.count()
							<< "\n";
					obj_prev_logged = obj_best;
				}
			}

			if (should_vb) {
				cout << "Epoch " << time_curr.epoch
						<< ".  Temperature = "
						<< coolfn->coolingfn(time_curr.epoch)
						<< ", Objective = "
						<< obj_curr << " (curr) and "
						<< obj_best << " (best)" << endl;
			}
		}
		full_log.close();
	}

private:
	struct RunningTimeStore {
		long epoch {};
		nanos wall_time_ns {};
	};

	const int run_id;
	unique_ptr<cooling::CoolingFn> coolfn;
	unique_ptr<T> state_curr;
	unique_ptr<T> state_best;
	double obj_curr;
	double obj_best;
	RunningTimeStore time_curr;
	RunningTimeStore time_best;
	std::mt19937_64 annealer_random_generator;
};
