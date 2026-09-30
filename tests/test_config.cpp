/*
 * test_config.cpp - keys.conf parser tests
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors. GPL-3.0-or-later.
 */

#include "check.h"
#include "config.h"

#include <sstream>

using namespace flightsim;

namespace {

Config parse(const std::string & text) {
	std::istringstream in(text);
	return parseConfig(in, "test.conf");
}

const std::string GPIO = "[gpio]\ncolumns = 5 6\nrows = 20 21 26\n";

}

int main() {
	// The shipped default is the Zibo 737 (captain CDU), same 69 positions
	// as the original Pi-XPlane-FMC-CDU Zibo mapping.
	{
		Config c = loadConfig(SOURCE_DIR "/config/keys.conf");
		CHECK(c.chip == "/dev/gpiochip0");
		CHECK((c.columns == std::vector<unsigned> { 14, 15, 18, 23, 24, 25, 8, 7, 16 }));
		CHECK((c.rows == std::vector<unsigned> { 21, 2, 3, 4, 17, 27, 22, 10 }));
		CHECK(c.debounceMs == 20);
		CHECK(c.host.empty());
		CHECK(c.port == 49000);
		CHECK(c.keys.size() == 69);
		CHECK(c.keys.at( { 1, 1 }) == "laminar/B738/button/fmc1_1L");
		CHECK(c.keys.at( { 7, 3 }) == "laminar/B738/button/fmc1_exec");
		CHECK(c.keys.at( { 3, 3 }) == "laminar/B738/button/fmc1_legs");
		CHECK(c.keys.at( { 5, 2 }) == "laminar/B738/button/fmc1_init_ref");
		CHECK(c.keys.at( { 6, 7 }) == "laminar/B738/button/fmc1_SP");
		CHECK(c.keys.at( { 5, 9 }) == "laminar/B738/button/fmc1_3");
		CHECK(c.keys.count( { 8, 9 }) == 0);
		for (const auto & k : c.keys) {
			CHECK(k.second.rfind("laminar/B738/button/fmc1_", 0) == 0);
		}
	}

	// The X-Plane default 737 keymap keeps the original 2019 mapping, and
	// both keymaps cover exactly the same key positions and wiring.
	{
		Config z = loadConfig(SOURCE_DIR "/config/keys.conf");
		Config c = loadConfig(SOURCE_DIR "/config/keys-xplane-default.conf");
		CHECK(c.keys.size() == 69);
		CHECK(c.keys.at( { 1, 1 }) == "sim/FMS/ls_1l");
		CHECK(c.keys.at( { 7, 3 }) == "sim/FMS/exec");
		CHECK(c.keys.at( { 2, 3 }) == "sim/fms_direct");
		CHECK(c.keys.at( { 5, 9 }) == "sim/FMS/key_3");
		CHECK(c.columns == z.columns);
		CHECK(c.rows == z.rows);
		bool samePositions = c.keys.size() == z.keys.size();
		for (const auto & k : c.keys) {
			samePositions = samePositions && z.keys.count(k.first) == 1;
		}
		CHECK(samePositions);
	}

	// Comments, blank lines, whitespace, inline comments, xplane section.
	{
		Config c = parse("# header\n\n" + GPIO
				+ "  debounce_ms = 35  ; slower switches\n"
				"[xplane]\nhost = 192.168.1.50\nport = 49010\n"
				"[keys]\n 1 , 2 = sim/lights/landing_lights_toggle # comment\n"
				"3,1=sim/none/none\n");
		CHECK(c.debounceMs == 35);
		CHECK(c.host == "192.168.1.50");
		CHECK(c.port == 49010);
		CHECK(c.keys.size() == 2);
		CHECK(c.keys.at( { 1, 2 }) == "sim/lights/landing_lights_toggle");
		CHECK(c.keys.at( { 3, 1 }) == "sim/none/none");
	}

	// Errors carry file:line and a useful message.
	CHECK_THROWS(parse("[bogus]\n"), "test.conf:1: unknown section [bogus]");
	CHECK_THROWS(parse("columns = 1\n"), "setting outside of a section");
	CHECK_THROWS(parse(GPIO + "speed = 3\n"), "unknown [gpio] setting");
	CHECK_THROWS(parse(GPIO + "debounce_ms = fast\n"), "expected a number");
	CHECK_THROWS(parse(GPIO + "debounce_ms = 0\n"), "out of range");
	CHECK_THROWS(parse(GPIO + "[keys]\n1,1 = a\n1,1 = b\n"), "test.conf:6: key 1,1 is defined twice");
	CHECK_THROWS(parse(GPIO + "[keys]\n4,1 = a\n"), "outside the configured matrix");
	CHECK_THROWS(parse(GPIO + "[keys]\n1,3 = a\n"), "outside the configured matrix");
	CHECK_THROWS(parse(GPIO + "[keys]\n1 = a\n"), "row,col");
	CHECK_THROWS(parse(GPIO + "[keys]\n1,1 =\n"), "missing X-Plane command");
	CHECK_THROWS(parse(GPIO + "[keys]\n1,1 = sim/a sim/b\n"), "must not contain spaces");
	CHECK_THROWS(parse("[gpio]\ncolumns = 5 6\nrows = 6 7\n"), "GPIO 6 is listed twice");
	CHECK_THROWS(parse("[gpio]\ncolumns = 5 6\n"), "rows must list");
	CHECK_THROWS(parse("[gpio\n"), "malformed section header");
	CHECK_THROWS(parse(GPIO + "[xplane]\nport = 70000\n"), "out of range");
	CHECK_THROWS(loadConfig("/nonexistent/keys.conf"), "cannot open config file");

	TEST_MAIN_END();
}
