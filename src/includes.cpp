#include "includes.h"

#include <filesystem>

string get_annealing_filename_for_epoch(int run_id, long epoch) {
	return OUTPUT_FOLDER + "run-" + to_string(run_id) +
				"/simanneal-" + to_string(epoch) + ".txt";
}

string get_annealing_filename_for_full_log(int run_id) {
	return OUTPUT_FOLDER + "run-" + to_string(run_id) +
				"/simanneal-full-log.txt";
}

string get_input_filename(int run_id) {
	return INPUT_FOLDER + "directions-" + to_string(run_id) + ".txt";
}

ofstream file_writer(string filename) {
	filesystem::path p {filename};
	filesystem::path folder_path {p.parent_path()};
	if (not filesystem::exists(folder_path)) {
		filesystem::create_directories(folder_path);
	}
	ofstream o { filename };
	return o;
}

ifstream file_reader(string filename) {
	filesystem::path p {filename};
	if (not filesystem::exists(filename)) {
		throw std::runtime_error("File named \"" + filename + "\" does not exist.");
	}
	ifstream i { filename };
	return i;
}


/* ************************************************** *
 * This function is used in
 * 	  + Direction.cpp
 * 	  + Main.cpp
 * to match regular expressions.
 *
 * Because matching requires a bit of setup, interactions with
 * the regular expression header are handled below.
 *
 * For reference about matching strings to regex objects, see:
 *
 *     https://en.cppreference.com/w/cpp/regex/regex_match
 *
 *     https://en.cppreference.com/w/cpp/regex/match_results
 *
 * If s successfully matches regex, the regex groups are saved and
 * accessible through match[...].
 *
 * If unsuccessful, this throws an error message.
 */
match_results<string::const_iterator>
wrap_regex_match(const string& s,
				const string& regex,
				const string& error_msg) {

	auto rx = basic_regex(regex);
	match_results<string::const_iterator> match {};
	regex_match(s, match, rx);
	if (match.empty())
		throw std::runtime_error(error_msg);
	return match;
}
