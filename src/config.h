/*
 * config.h - keys.conf parser for Pi-XPlane-FMC-CDU-Keys-Only
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 * Based on Pi-XPlane-FMC-CDU-Keys-Only, Copyright (c) 2017-2019 Shahada Abubakar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_CONFIG_H
#define FLIGHTSIM_CONFIG_H

#include <cstdint>
#include <istream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace flightsim {

class ConfigError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

struct Config {
	std::string chip = "/dev/gpiochip0";
	std::vector<unsigned> columns;	// BCM line offsets, driven low one at a time
	std::vector<unsigned> rows;		// BCM line offsets, read with pull-up
	int debounceMs = 20;

	std::string host;				// empty = discover via X-Plane beacon
	uint16_t port = 49000;

	// (row, col), both 1-based -> X-Plane command
	std::map<std::pair<int, int>, std::string> keys;
};

// Parses INI-style config text. Throws ConfigError naming source:line on
// any error, and validates the result as a whole before returning.
Config parseConfig(std::istream & in, const std::string & sourceName);

Config loadConfig(const std::string & path);

}

#endif
