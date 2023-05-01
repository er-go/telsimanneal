#pragma once

#include "includes.h"
#include "SimAnneal.h"
#include "Schedule.h"

class TelAnnealer : public SimAnnealer<Schedule> {
public:
	TelAnnealer(int run_id, unique_ptr<cooling::CoolingFn>&& cooler,
					shared_ptr<DirectionDatabase> dirdata) :
		SimAnnealer<Schedule> {
			run_id,
			std::make_unique<Schedule>(dirdata, true),
			move(cooler)},
		dirdatabase    {dirdata},
		num_dir        {dirdatabase->get_num_directions_defined()},
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
		 * specifically, are almost exactly like those steps used for the
		 * traveling salesman example within the book "Finite Markov Chains and
		 * Algorithmic Applications" by Häggström.  The main difference is
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

		storage.copy_from(from);
		bool switch_rep { unif01(rand) < 0.5 };
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

private:
	shared_ptr<DirectionDatabase> dirdatabase;
	size_t num_dir;
	std::uniform_int_distribution<size_t> idx_selecter_1, idx_selecter_2;
	std::uniform_real_distribution<double> unif01;
};
