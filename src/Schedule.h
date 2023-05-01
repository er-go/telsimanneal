#pragma once

#include "includes.h"
#include "Direction.h"

class ScheduleIterator;

struct Schedule {
	/* A schedule iterator will run through the current schedule and
	 * provide a pointer to each direction, as appropriate. It must
	 * therefore be able to access the current schedule.  We want this
	 * to happen fast without needing any getter function calls, so
	 * we provide direct access to the arrays.
	 *
	 * To explain the "friend class" declaration, refer to:
	 *     https://en.cppreference.com/w/cpp/language/friend
	 */
	friend class ScheduleIterator;
public:
	Schedule(shared_ptr<DirectionDatabase> dirdata=nullptr, bool do_setup=true);
	Schedule(vector<dir_id_t>&& schd_dir_id_t,
			 vector<bool>&& schd_dir_othr,
			 shared_ptr<DirectionDatabase> dirdata=nullptr);
	~Schedule() = default;

	Schedule(const Schedule& other) = default;
	Schedule(Schedule&&) = default;

	/* Note that copy assignment should have non-void return type.  See
	 *     https://en.cppreference.com/w/cpp/language/copy_assignment
	 */
	Schedule& operator=(const Schedule& other) { this->copy_from(other); return *this; }
	Schedule& operator=(Schedule&&) = default;

	unique_ptr<Schedule> duplicate() const;
	void copy_from(const Schedule& source);
	double total_distance()  const;
	ScheduleIterator begin() const;
	ScheduleIterator end()   const;

	void flip_segment(size_t i, size_t j, bool switch_rep_rep);

	size_t get_num_dir() const {
		return num_dir;
	}
private:
	shared_ptr<DirectionDatabase> dirdata;
	size_t num_dir;
	vector<dir_id_t> schd_dir_id_t;
	vector<bool>     schd_dir_othr;
};

class ScheduleIterator {
public:
	ScheduleIterator(const Schedule* s);
	~ScheduleIterator() = default;
	ScheduleIterator(ScheduleIterator&)  = default;
	ScheduleIterator(ScheduleIterator&&) = default;

	bool operator!=(ScheduleIterator& other);
	bool operator==(ScheduleIterator& other);
	/* NOTE:
	 * To explain why there are two ++ definitions, see the following
     * two discussions on StackOverflow:
	 *
	 *     https://stackoverflow.com/questions/50233463/trying-to-overload-operator-for-my-date-class-still-getting-the-error-canno
	 *
	 *     https://stackoverflow.com/questions/4659162/int-argument-in-operator
	 *
	 * The takeaway is that ++ and ++(int) are different and represent
     * prefix and postfix.
	 */
	ScheduleIterator& operator++();
	ScheduleIterator operator++(int __);

	ScheduleIterator& operator+=(int j);
	const Direction& operator*();
private:
	int current_idx;
	const Schedule* my_schedule;
};

ostream& operator<<(ostream& o, const Schedule& sched);
