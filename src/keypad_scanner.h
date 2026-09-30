/*
 * keypad_scanner.h - debounced key matrix scanner
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 * Scanning approach from Pi-XPlane-FMC-CDU, Copyright (c) 2017-2019 Shahada Abubakar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_KEYPAD_SCANNER_H
#define FLIGHTSIM_KEYPAD_SCANNER_H

#include "gpio_matrix.h"

#include <chrono>
#include <vector>

namespace flightsim {

struct KeyEvent {
	enum class Type {
		Press,
		Release,
		MultipleIgnored	// more than one key down; nothing is sent
	};
	Type type;
	int row;	// 1-based; 0 for MultipleIgnored
	int col;	// 1-based; 0 for MultipleIgnored

	bool operator==(const KeyEvent & o) const {
		return type == o.type && row == o.row && col == o.col;
	}
};

// Scans the whole matrix on each poll() and reports debounced changes.
// A reading must stay identical for the debounce interval before it is
// acted on, which filters contact bounce on both press and release.
// Only single-key presses produce a Press; chords are reported as
// MultipleIgnored (without diodes a matrix cannot tell chords from ghosts).
class KeypadScanner {
public:
	using Clock = std::chrono::steady_clock;

	KeypadScanner(GpioMatrix & matrix, std::chrono::milliseconds debounce);

	std::vector<KeyEvent> poll(Clock::time_point now);

private:
	struct Reading {
		enum class Kind { None, Single, Multiple } kind = Kind::None;
		int row = 0;	// 0-based, valid for Single
		int col = 0;

		bool operator==(const Reading & o) const {
			return kind == o.kind && row == o.row && col == o.col;
		}
		bool operator!=(const Reading & o) const { return !(*this == o); }
	};

	Reading sample();

	GpioMatrix & matrix;
	std::chrono::milliseconds debounce;

	Reading candidate;					// latest raw reading
	Clock::time_point candidateSince;
	Reading accepted;					// last reading that survived debounce
	bool holding = false;				// a single key press is active
	int heldRow = 0;
	int heldCol = 0;
};

}

#endif
