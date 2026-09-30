/*
 * gpio_chardev.h - GpioMatrix on the Linux GPIO character device (uAPI v2)
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_GPIO_CHARDEV_H
#define FLIGHTSIM_GPIO_CHARDEV_H

#include "gpio_matrix.h"

#include <string>
#include <vector>

namespace flightsim {

// Talks to the kernel directly through /dev/gpiochipN ioctls, so there is
// no wiringPi or libgpiod dependency (libgpiod's API differs between
// Raspberry Pi OS Bookworm and Trixie; the kernel ABI does not).
//
// Columns are open-drain outputs: an idle column floats instead of driving
// high, so pressing two keys in one row cannot short two outputs together.
// Rows are inputs with the internal pull-up enabled.
class ChardevGpioMatrix : public GpioMatrix {
public:
	// Throws std::runtime_error with a user-facing hint on failure.
	ChardevGpioMatrix(const std::string & chipPath,
			const std::vector<unsigned> & columns,
			const std::vector<unsigned> & rows);
	~ChardevGpioMatrix() override;

	ChardevGpioMatrix(const ChardevGpioMatrix &) = delete;
	ChardevGpioMatrix & operator=(const ChardevGpioMatrix &) = delete;

	size_t rowCount() const override { return nRows; }
	size_t columnCount() const override { return nColumns; }

	void selectColumn(int col) override;
	uint64_t readActiveRows() override;

private:
	int chipFd = -1;
	int columnFd = -1;
	int rowFd = -1;
	size_t nColumns;
	size_t nRows;
};

}

#endif
