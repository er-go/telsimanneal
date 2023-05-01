#include "includes.h"
#include "Schedule.h"

Schedule::Schedule(shared_ptr<DirectionDatabase> dirdata, bool do_setup) :
	dirdata { dirdata },
	num_dir { dirdata->get_num_directions_defined() },
	schd_dir_id_t(num_dir),
	schd_dir_othr(num_dir, false) {
	if (do_setup) {
		for (size_t j {0}; j<num_dir; j++) {
			/* Note: by convention, Direction IDs are the indices 0, 1, 2, ...,
			 * through num_dir-1.  This is verified upon setup within
			 * Direction.cpp, in place_direction(...). */
			schd_dir_id_t[j] = j;
		}
	}
}

Schedule::Schedule(vector<dir_id_t>&& sdidt,
				 vector<bool>&& sdothr,
				 shared_ptr<DirectionDatabase> dirdata) :
		dirdata { dirdata },
		num_dir { dirdata->get_num_directions_defined() },
		schd_dir_id_t {move(sdidt)},
		schd_dir_othr {move(sdothr)} {}

unique_ptr<Schedule> Schedule::duplicate() const {
	auto dupl = make_unique<Schedule>(dirdata, false);
	dupl->copy_from(*this);
	return dupl;
}

void Schedule::copy_from(const Schedule& source) {
	if (source.num_dir != this->num_dir) {
		throw std::runtime_error(
				"Asked to copy a schedule with "
				+ to_string(source.num_dir)
				+ " Directions into a schedule with "
				+ to_string(this->num_dir)
				+ " Directions.");
	}
	std::copy(source.schd_dir_id_t.begin(), source.schd_dir_id_t.end(),
				this->schd_dir_id_t.begin());
	std::copy(source.schd_dir_othr.begin(), source.schd_dir_othr.end(),
				this->schd_dir_othr.begin());
}

double Schedule::total_distance() const {
	auto i1 = this->begin();
	auto the_end = this->end();
	if (i1 == the_end)
		return 0;
	/* The schedule has entries (as it should) which we can now step
	 * through in pairs.  The iterators i1, i2 will be off by one at
	 * all times.  For instance:
	 *                                i1  i2
	 * (Index within schedule) 0  1   2   3   4 ...
	 *                                ^   ^
	 * The call (*i1).dist_to(*i2) computes the distance between this
	 * pair of directions.
	 */
	double total_dist {0};
	auto i2 = this->begin();
	i2++;
	while (i2 != the_end) {
		total_dist += (*i1).dist_to(*i2);
		i1++, i2++;
	}
	return total_dist;
}

ScheduleIterator Schedule::begin() const {
	return ScheduleIterator {this};
}
ScheduleIterator Schedule::end() const {
	ScheduleIterator s {this};
	s += this->num_dir;
	return s;
}

void Schedule::flip_segment(size_t i, size_t j, bool switch_rep) {
	if (i > j) {
		flip_segment(j, i, switch_rep);
		return;
	} else if (i < 0 or j >= this->num_dir) {
		throw std::out_of_range(
				"Asked to flip range within schedule between indices "
				+ to_string(i) + " and " + to_string(j)
				+ " (inclusive) but there are only "
				+ to_string(num_dir) + " Directions.");
	} else if (i == 0) {
		throw std::runtime_error(
				"Rescheduling the Direction at index i=0 is not permitted.");
	} else {
		{
			// First, reverse the direction ids.
			auto ptr_start_i = schd_dir_id_t.begin() + i;
			auto ptr_end_j   = schd_dir_id_t.begin() + j;
			std::reverse(ptr_start_i, ptr_end_j);
		}
		{
			// Now reverse the direction primary/other rep indicators.
			auto ptr_start_i = schd_dir_othr.begin() + i;
			auto ptr_end_j   = schd_dir_othr.begin() + j;
			std::reverse(ptr_start_i, ptr_end_j);
		}
		// Now, if asked to flip the representations, do so.
		if (switch_rep) {
			for (size_t idx { i }; idx <= j; idx++)
				this->schd_dir_othr[idx] = not (this->schd_dir_othr[idx]);
		}
	}
}

ostream& operator<<(ostream& o, const Schedule& sched) {
	/* Simply print all directions as ordered in the schedule, one per line. */
	for (auto& d : sched) {
		o << d << '\n';
	}
	return o;
}

/* ************************************************** */

ScheduleIterator::ScheduleIterator(const Schedule* s) :
	current_idx {0},
	my_schedule {s} {}

bool ScheduleIterator::operator!=(ScheduleIterator& other) {
	/* Simply determine if the iterators point to different indices
	 * or if they are for different schedules.
	 */
	return (this->current_idx != other.current_idx
			or this->my_schedule != other.my_schedule);
}

bool ScheduleIterator::operator==(ScheduleIterator& other) {
	return not (*this != other);
}

ScheduleIterator& ScheduleIterator::operator+=(int j) {
	current_idx += j;
	return *this;
}

ScheduleIterator ScheduleIterator::operator++(int __) {
	ScheduleIterator prev_iter { *this };  // Copy construction
	current_idx += 1;
	return prev_iter;
}

ScheduleIterator& ScheduleIterator::operator++() {
	current_idx += 1;
	return *this;
}

const Direction& ScheduleIterator::operator*() {
	if (current_idx < 0 or current_idx >= my_schedule->num_dir) {
		throw std::out_of_range("Iterator is out of range: "
					+ to_string(current_idx) + " out of "
					+ to_string(my_schedule->num_dir));
	}

	dir_id_t id       {my_schedule->schd_dir_id_t[current_idx]};
	bool     is_other {my_schedule->schd_dir_othr[current_idx]};
	return my_schedule->dirdata->get_direction(id, is_other);
}
