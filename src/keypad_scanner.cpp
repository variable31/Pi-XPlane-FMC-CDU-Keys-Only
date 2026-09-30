/*
 * keypad_scanner.cpp - debounced key matrix scanner
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 * Scanning approach from Pi-XPlane-FMC-CDU, Copyright (c) 2017-2019 Shahada Abubakar.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "keypad_scanner.h"

namespace flightsim {

KeypadScanner::KeypadScanner(GpioMatrix & _matrix,
		std::chrono::milliseconds _debounce) :
		matrix(_matrix), debounce(_debounce) {
}

KeypadScanner::Reading KeypadScanner::sample() {
	Reading r;
	for (size_t col = 0; col < matrix.columnCount(); col++) {
		matrix.selectColumn((int) col);
		uint64_t active = matrix.readActiveRows();
		for (size_t row = 0; active && row < matrix.rowCount(); row++) {
			if (active & (1ULL << row)) {
				if (r.kind != Reading::Kind::None) {
					r.kind = Reading::Kind::Multiple;
					r.row = r.col = 0;
					matrix.selectColumn(-1);
					return r;
				}
				r.kind = Reading::Kind::Single;
				r.row = (int) row;
				r.col = (int) col;
			}
		}
	}
	matrix.selectColumn(-1);
	return r;
}

std::vector<KeyEvent> KeypadScanner::poll(Clock::time_point now) {
	std::vector<KeyEvent> events;

	Reading r = sample();
	if (r != candidate) {
		candidate = r;
		candidateSince = now;
		return events;
	}
	if (candidate == accepted || now - candidateSince < debounce) {
		return events;
	}
	accepted = candidate;

	switch (accepted.kind) {
	case Reading::Kind::None:
		if (holding) {
			events.push_back( { KeyEvent::Type::Release, heldRow + 1, heldCol + 1 });
			holding = false;
		}
		break;

	case Reading::Kind::Single:
		if (holding && heldRow == accepted.row && heldCol == accepted.col) {
			break;	// back to the same key after a chord
		}
		if (holding) {
			events.push_back( { KeyEvent::Type::Release, heldRow + 1, heldCol + 1 });
		}
		holding = true;
		heldRow = accepted.row;
		heldCol = accepted.col;
		events.push_back( { KeyEvent::Type::Press, heldRow + 1, heldCol + 1 });
		break;

	case Reading::Kind::Multiple:
		events.push_back( { KeyEvent::Type::MultipleIgnored, 0, 0 });
		break;
	}
	return events;
}

}
