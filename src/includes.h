#pragma once

#include <iostream>
#include <fstream>
#include <sstream>

#include <memory>

#include <vector>
#include <algorithm>

#include <random>
#include <cmath>

#include <chrono>
#include <regex>

using namespace std;

/* ************************************************** *
 * Determines where the program looks for the main input directions
 * and where the computed schedules should written.  The functions
 * below are used within SimAnneal.h in the run(...) method.
 */
static const string INPUT_FOLDER ("./input/");
static const string OUTPUT_FOLDER ("./output/");

/* Location to save the schedules while annealing: */
string get_annealing_filename_for_epoch(int run_id, long epoch);

/* Location to save the log about annealing improvements and
 * the required compute times: */
string get_annealing_filename_for_full_log(int run_id);

/* Location of each input file. */
string get_input_filename(int run_id);

ofstream file_writer(string filename);
ifstream file_reader(string filename);

/* ************************************************** *
 * Try to match s to the regex, and if so return the match object through which
 * regex groups are accessible via match[...].  Otherwise, throw an error
 * message.
 */
match_results<string::const_iterator>
wrap_regex_match(const string& s,
				const string& regex,
				const string& error_msg);

/* ************************************************** */

constexpr static double PI = 3.1415926535898;
constexpr static double TWO_PI = 2 * PI;
constexpr static string_view SEPARATOR
		{ "--------------------------------------------------" };
