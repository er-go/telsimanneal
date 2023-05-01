#include "Direction.h"

Direction::Direction(dir_id_t id, double theta, double phi) :
	id {id},
	theta   {},
	theta_o {},
	phi     {},
	phi_o   {} {
		set_theta(theta);
		set_phi(phi);
	}

double Direction::dist_to(const Direction& d2) const {
	return Direction::dist_between(*this, d2);
}

double Direction::dist_between(const Direction& d1, const Direction& d2) {
	return std::max(
			std::abs(d1.phi - d2.phi),
			std::min(abs(d1.theta - d2.theta),
					std::min(
						abs(d1.theta - d2.theta + TWO_PI),
						abs(d1.theta - d2.theta - TWO_PI))
					)
	);
}

double Direction::get_theta() const {
	return theta;
}

void Direction::set_theta(double theta_new) {
	this->theta = theta_new;
	theta_o = theta_new + PI;
	if (theta_o > 2 * PI)
		theta_o = theta_new - PI;
}

double Direction::get_phi() const {
	return phi;
}

void Direction::set_phi(double phi_new) {
	this->phi = phi_new;
	phi_o = -phi_new;
}

bool Direction::is_other() {
	return (phi < 0);
}

void Direction::switch_rep() {
	std::swap(theta, theta_o);
	std::swap(phi,   phi_o);
}

/* ************************************************** */

Direction Direction::read_from(istream& s) {
	std::stringstream datastream {};
	char last {};
	while (s.peek() != 'D') {
		s.ignore();
	}
	do {
		s >> last;
		datastream << last;
	} while (last != ')');

	std::string data;
	datastream >> data;

	return Direction::read_from(data);
}

Direction Direction::read_from(string s) {
	// First, remove all spaces, tabs, newlines, etc.
	char to_remove[] { ' ', '\t', '\n' };
	for (auto c : to_remove) {
		auto s_orig_end { s.end() };
		auto s_new_end { std::remove(s.begin(), s_orig_end, c) };
		s.erase(s_new_end, s_orig_end);
	}

	const string regex =
		"Direction\\("
		"id=(\\d*),"
		"theta=(\\d+.\\d*),"
		"phi=(\\d+.\\d*)\\)";

	const string error_msg =
			"Regex matching error.\n"
			"Tried to read a direction from:\n    \""
			+ s + "\"";

	auto match = wrap_regex_match(s, regex, error_msg);
	dir_id_t id;
	double theta, phi;
	try {
		id = stoi(match[1]);
	} catch (...) {
		throw std::runtime_error("Direction id must be an int");
	}
	try {
		theta = stod(match[2]);
	} catch (...) {
		throw std::runtime_error("Direction theta must be a decimal number.");
	}
	try {
		phi = stod(match[3]);
	} catch (...) {
		throw std::runtime_error("Direction phi must be a decimal number.");
	}
	return Direction { id, theta, phi };
}

/* ************************************************** */

bool DirectionDatabase::is_id_already_defined(dir_id_t look_for_id) const {
	auto ids_equal = [look_for_id] (const Direction& d) {
		return d.id == look_for_id;
	};
	return std::none_of(
			DIRECTION_PRIME.cbegin(),
			DIRECTION_PRIME.cend(),
			ids_equal
	);
}

bool DirectionDatabase::place_direction(Direction&& d) {
	auto expect_id { get_num_directions_defined() };
	if (d.id != expect_id) {
		throw std::runtime_error(
				"Tried to add a direction with ID " + to_string(d.id)
				+ " but expected ID " + to_string(expect_id)
				+ ". Every id must be incremental (0, 1, 2, ...) and"
						" added in order.");
	}
	DIRECTION_PRIME.push_back(move(d));
	Direction otherrep { DIRECTION_PRIME[expect_id] }; // copy on purpose
	otherrep.switch_rep();
	DIRECTION_OTHER.push_back(move(otherrep));
	return true;
}


size_t DirectionDatabase::get_num_directions_defined() const {
	return DIRECTION_PRIME.size();
}

const Direction& DirectionDatabase::get_direction(dir_id_t id, bool other_rep) const {
	return (other_rep ?
				DIRECTION_OTHER.at(id)
				: DIRECTION_PRIME.at(id));
}

void DirectionDatabase::reserve_directions(size_t n) {
	DIRECTION_PRIME.reserve(n);
	DIRECTION_OTHER.reserve(n);
}

/* ************************************************** */

ostream& operator<<(ostream& s, const Direction& d) {
	return s << "Direction(id=" << d.id
			<< ", theta=" << d.get_theta()
			<< ", phi=" << d.get_phi() << ")";
}
