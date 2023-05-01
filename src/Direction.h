#pragma once

#include "includes.h"

using dir_id_t = unsigned int;

struct Direction {
public:
	Direction(dir_id_t id, double theta, double phi);
	~Direction()             = default;
	Direction(Direction& d)  = default;
	Direction(Direction&& d) = default;

	double get_theta() const;
	double get_phi() const;
	void set_theta(double theta_new);
	void set_phi(double phi_new);

	bool is_other();
	void switch_rep();

	double dist_to(const Direction& d2) const;
	static double dist_between(const Direction& d1, const Direction& d2);
	static Direction read_from(istream& s);
	static Direction read_from(string s);

	const dir_id_t id;
private:
	double theta, theta_o, phi, phi_o;
};

struct DirectionDatabase {
public:
	DirectionDatabase(size_t num_dirs_reserved=0) {
		reserve_directions(num_dirs_reserved);
	}
	~DirectionDatabase() = default;

	// Not copyable, but movable:
	DirectionDatabase(DirectionDatabase&) = delete;
	DirectionDatabase& operator=(DirectionDatabase&) = delete;

	DirectionDatabase(DirectionDatabase&&) = default;
	DirectionDatabase& operator=(DirectionDatabase&&) = default;

	void reserve_directions(size_t n);
	bool place_direction(Direction&& dptr);

	size_t get_num_directions_defined() const;
	const Direction& get_direction(dir_id_t id, bool other_rep) const;

	bool is_id_already_defined(dir_id_t look_for_id) const;

private:
	/* NOTE:
	 *
	 * A design decision is that the DIRECTION... vectors must be sorted
	 * by direction IDs, and these IDs must be incremental (0, 1, 2, ...).
	 * This ensures we can quickly look up a direction by its id, and
	 * means each schedule need only store a list of direction IDs, not the
	 * full Direction classes.  (The latter would require longer to copy
	 * and reorder.)
	 */

	vector<Direction> DIRECTION_PRIME {};
	vector<Direction> DIRECTION_OTHER {};
};

ostream& operator<<(ostream& s, const Direction& d);
