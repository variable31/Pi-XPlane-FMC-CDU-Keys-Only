/*
 * config.cpp - keys.conf parser for Pi-XPlane-FMC-CDU-Keys-Only
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 * Based on Pi-XPlane-FMC-CDU-Keys-Only, Copyright (c) 2017-2019 Shahada Abubakar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "config.h"

#include <fstream>
#include <set>
#include <sstream>

namespace flightsim {

namespace {

const unsigned MAX_LINES = 64;	// row state is read into a 64-bit mask

std::string trim(const std::string & s) {
	const char * ws = " \t\r\n";
	size_t b = s.find_first_not_of(ws);
	if (b == std::string::npos) {
		return "";
	}
	size_t e = s.find_last_not_of(ws);
	return s.substr(b, e - b + 1);
}

// Comments start with ';' or '#' anywhere on the line. X-Plane command
// names never contain either character.
std::string stripComment(const std::string & s) {
	size_t pos = s.find_first_of(";#");
	return pos == std::string::npos ? s : s.substr(0, pos);
}

[[noreturn]] void fail(const std::string & where, const std::string & msg) {
	throw ConfigError(where + ": " + msg);
}

long parseInt(const std::string & text, const std::string & where,
		long min, long max) {
	size_t used = 0;
	long v = 0;
	try {
		v = std::stol(text, &used, 10);
	} catch (const std::exception &) {
		fail(where, "expected a number, got \"" + text + "\"");
	}
	if (used != text.size()) {
		fail(where, "expected a number, got \"" + text + "\"");
	}
	if (v < min || v > max) {
		fail(where, "value " + text + " out of range " + std::to_string(min)
				+ ".." + std::to_string(max));
	}
	return v;
}

std::vector<unsigned> parsePinList(const std::string & text,
		const std::string & where) {
	std::vector<unsigned> pins;
	std::istringstream in(text);
	std::string tok;
	while (in >> tok) {
		pins.push_back((unsigned) parseInt(tok, where, 0, 1023));
	}
	return pins;
}

void validate(const Config & c, const std::string & source) {
	if (c.columns.empty()) {
		fail(source, "[gpio] columns must list at least one pin");
	}
	if (c.rows.empty()) {
		fail(source, "[gpio] rows must list at least one pin");
	}
	if (c.columns.size() > MAX_LINES || c.rows.size() > MAX_LINES) {
		fail(source, "at most " + std::to_string(MAX_LINES)
				+ " rows and columns are supported");
	}
	std::set<unsigned> seen;
	for (unsigned p : c.columns) {
		if (!seen.insert(p).second) {
			fail(source, "GPIO " + std::to_string(p) + " is listed twice");
		}
	}
	for (unsigned p : c.rows) {
		if (!seen.insert(p).second) {
			fail(source, "GPIO " + std::to_string(p) + " is listed twice");
		}
	}
	for (const auto & k : c.keys) {
		if (k.first.first > (int) c.rows.size()
				|| k.first.second > (int) c.columns.size()) {
			fail(source, "key " + std::to_string(k.first.first) + ","
					+ std::to_string(k.first.second)
					+ " is outside the configured matrix ("
					+ std::to_string(c.rows.size()) + " rows x "
					+ std::to_string(c.columns.size()) + " columns)");
		}
	}
}

}

Config parseConfig(std::istream & in, const std::string & sourceName) {
	Config c;
	std::string section;
	std::string raw;
	int lineNo = 0;

	while (std::getline(in, raw)) {
		lineNo++;
		const std::string where = sourceName + ":" + std::to_string(lineNo);
		std::string line = trim(stripComment(raw));
		if (line.empty()) {
			continue;
		}

		if (line.front() == '[') {
			if (line.back() != ']') {
				fail(where, "malformed section header");
			}
			section = trim(line.substr(1, line.size() - 2));
			if (section != "gpio" && section != "xplane" && section != "keys") {
				fail(where, "unknown section [" + section + "]");
			}
			continue;
		}

		size_t eq = line.find('=');
		if (eq == std::string::npos) {
			fail(where, "expected key = value");
		}
		std::string key = trim(line.substr(0, eq));
		std::string value = trim(line.substr(eq + 1));

		if (section.empty()) {
			fail(where, "setting outside of a section");
		} else if (section == "gpio") {
			if (key == "chip") {
				c.chip = value;
			} else if (key == "columns") {
				c.columns = parsePinList(value, where);
			} else if (key == "rows") {
				c.rows = parsePinList(value, where);
			} else if (key == "debounce_ms") {
				c.debounceMs = (int) parseInt(value, where, 1, 1000);
			} else {
				fail(where, "unknown [gpio] setting \"" + key + "\"");
			}
		} else if (section == "xplane") {
			if (key == "host") {
				c.host = value;
			} else if (key == "port") {
				c.port = (uint16_t) parseInt(value, where, 1, 65535);
			} else {
				fail(where, "unknown [xplane] setting \"" + key + "\"");
			}
		} else {
			// [keys]  row,col = command
			size_t comma = key.find(',');
			if (comma == std::string::npos) {
				fail(where, "key position must be row,col");
			}
			int row = (int) parseInt(trim(key.substr(0, comma)), where, 1,
					MAX_LINES);
			int col = (int) parseInt(trim(key.substr(comma + 1)), where, 1,
					MAX_LINES);
			if (value.empty()) {
				fail(where, "missing X-Plane command");
			}
			if (value.find_first_of(" \t") != std::string::npos) {
				fail(where, "X-Plane command must not contain spaces");
			}
			if (!c.keys.emplace(std::make_pair(row, col), value).second) {
				fail(where, "key " + key + " is defined twice");
			}
		}
	}

	validate(c, sourceName);
	return c;
}

Config loadConfig(const std::string & path) {
	std::ifstream in(path);
	if (!in) {
		throw ConfigError(path + ": cannot open config file");
	}
	return parseConfig(in, path);
}

}
