/*
 * check.h - minimal test assertions (no test framework dependency)
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors. GPL-3.0-or-later.
 */

#ifndef FLIGHTSIM_TEST_CHECK_H
#define FLIGHTSIM_TEST_CHECK_H

#include <iostream>

static int failures = 0;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond \
					<< std::endl; \
			failures++; \
		} \
	} while (0)

#define CHECK_THROWS(expr, fragment) \
	do { \
		bool threw = false; \
		try { \
			expr; \
		} catch (const std::exception & e) { \
			threw = true; \
			if (std::string(e.what()).find(fragment) == std::string::npos) { \
				std::cerr << __FILE__ << ":" << __LINE__ << ": wrong error: " \
						<< e.what() << " (expected \"" << fragment << "\")" \
						<< std::endl; \
				failures++; \
			} \
		} \
		if (!threw) { \
			std::cerr << __FILE__ << ":" << __LINE__ << ": expected throw: " #expr \
					<< std::endl; \
			failures++; \
		} \
	} while (0)

#define TEST_MAIN_END() \
	do { \
		if (failures) { \
			std::cerr << failures << " check(s) failed" << std::endl; \
			return 1; \
		} \
		std::cout << "all checks passed" << std::endl; \
		return 0; \
	} while (0)

#endif
