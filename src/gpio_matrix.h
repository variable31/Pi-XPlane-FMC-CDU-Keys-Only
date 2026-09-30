/*
 * gpio_matrix.h - hardware abstraction for a scanned key matrix
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_GPIO_MATRIX_H
#define FLIGHTSIM_GPIO_MATRIX_H

#include <cstddef>
#include <cstdint>

namespace flightsim {

// A key matrix where one column at a time is pulled low and the rows are
// read back. A pressed key connects its column to its row, pulling it low.
class GpioMatrix {
public:
	virtual ~GpioMatrix() = default;

	virtual size_t rowCount() const = 0;
	virtual size_t columnCount() const = 0;

	// Drive column `col` (0-based) low and release all others.
	// A negative value releases every column.
	virtual void selectColumn(int col) = 0;

	// Bit i set = row i (0-based) is being pulled low, i.e. a key in the
	// selected column and that row is pressed.
	virtual uint64_t readActiveRows() = 0;
};

}

#endif
