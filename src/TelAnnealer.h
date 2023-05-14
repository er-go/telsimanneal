#pragma once

#include "includes.h"
#include "SimAnneal.h"
#include "Schedule.h"

class TelAnnealer : public SimAnnealer<Schedule> {
public:
	TelAnnealer(int run_id, unique_ptr<cooling::CoolingFn>&& cooler,
					shared_ptr<DirectionDatabase> dirdata,
					bool without_second_rep) :
		SimAnnealer<Schedule> {
			run_id,
			std::make_unique<Schedule>(dirdata, true),
			move(cooler)},
		dirdatabase    {dirdata},
		num_dir        {dirdatabase->get_num_directions_defined()},
		without_second_rep {without_second_rep},
		idx_selecter_1 {1, num_dir-1},
		idx_selecter_2 {1, num_dir-2},
		unif01 {}
		{}

	virtual ~TelAnnealer() = default;
	TelAnnealer(TelAnnealer&) = delete;
	TelAnnealer(TelAnnealer&&) = delete;

	virtual void sample_step(const Schedule& from, Schedule& storage,
			std::mt19937_64& rand) override {
		/* The TelAnnealer class provides an optimizer to perform simulated
		 * annealing. The swaps used, which are here in the sample_step method
		 * specifically, are almost exactly like those reversals used for the
		 * traveling salesman example within the book "Finite Markov Chains and
		 * Algorithmic Applications" by Häggström.  The only difference here is
		 * that in addition to reversing segments of the schedule between
		 * two indices, there is an additional option of switching the
		 * spherical coordinate representation of the Directions; this switch
		 * happens with probability 1/2.
		 */
		size_t i { idx_selecter_1(rand) };
		size_t j { idx_selecter_2(rand) };

		if (i == j) {
			j = num_dir - 1;
		}

		bool switch_rep {};
		if (without_second_rep)
			switch_rep = false;
		else
			switch_rep = (unif01(rand) < 0.5);

		storage.copy_from(from);
		storage.flip_segment(i, j, switch_rep);
	}

	virtual double objective_to_minimize(const Schedule& s) override {
		/* Remember: the SimAnneal class treats LOWER objectives as BETTER. */
		return s.total_distance();
	}

	virtual void write(ostream& o, const Schedule& s) override {
		o << s;
	}

	virtual unique_ptr<Schedule> duplicate(const Schedule& s) override {
		return s.duplicate();
	}

	virtual void copy_from_to(const Schedule& from, Schedule& into) override {
		into.copy_from(from);
	}

	virtual string get_annealing_filename_for_epoch(int run_id, long epoch)
														override {
		string sr { (without_second_rep ? "no-second-rep/" : "" ) };
		return OUTPUT_FOLDER + "run-" + to_string(run_id) +
					"/" + sr + "simanneal-" + to_string(epoch) + ".txt";
	}

	virtual string get_annealing_filename_for_full_log(int run_id) override {
		string sr { (without_second_rep ? "no-second-rep/" : "" ) };
		return OUTPUT_FOLDER + "run-" + to_string(run_id) +
					+ "/" + sr + "simanneal-full-log.txt";
	}

private:
	shared_ptr<DirectionDatabase> dirdatabase;
	size_t num_dir;
	bool without_second_rep;
	std::uniform_int_distribution<size_t> idx_selecter_1, idx_selecter_2;
	std::uniform_real_distribution<double> unif01;
};
