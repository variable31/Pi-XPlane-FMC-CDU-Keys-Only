/*
 * gpio_fake.h - in-memory GpioMatrix for tests and --fake-keys wiring-free runs
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_GPIO_FAKE_H
#define FLIGHTSIM_GPIO_FAKE_H

#include "gpio_matrix.h"

#include <set>
#include <utility>

namespace flightsim {

// Keys are 1-based (row, col), matching keys.conf.
class FakeGpioMatrix : public GpioMatrix {
public:
	FakeGpioMatrix(size_t rows, size_t columns) :
			nRows(rows), nColumns(columns) {
	}

	size_t rowCount() const override { return nRows; }
	size_t columnCount() const override { return nColumns; }

	void selectColumn(int col) override { selected = col; }

	uint64_t readActiveRows() override {
		uint64_t bits = 0;
		if (selected < 0) {
			return 0;
		}
		for (const auto & k : down) {
			if (k.second - 1 == selected) {
				bits |= 1ULL << (k.first - 1);
			}
		}
		return bits;
	}

	void press(int row, int col) { down.insert( { row, col }); }
	void release(int row, int col) { down.erase( { row, col }); }
	void releaseAll() { down.clear(); }

private:
	size_t nRows;
	size_t nColumns;
	int selected = -1;
	std::set<std::pair<int, int>> down;
};

}

#endif
