/*
 * gpio_chardev.cpp - GpioMatrix on the Linux GPIO character device (uAPI v2)
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "gpio_chardev.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/gpio.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

namespace flightsim {

namespace {

const char * CONSUMER = "fmc-cdu-keys";

// Time for a column change to propagate through the switch and pull-up
// before the rows are sampled.
const useconds_t SETTLE_US = 10;

uint64_t allBits(size_t n) {
	return n >= 64 ? ~0ULL : ((1ULL << n) - 1);
}

std::string describeLines(const std::vector<unsigned> & lines) {
	std::string s;
	for (unsigned l : lines) {
		s += (s.empty() ? "" : " ") + std::to_string(l);
	}
	return s;
}

int requestLines(int chipFd, const std::string & chipPath,
		const std::vector<unsigned> & lines, uint64_t flags,
		bool setOutputsHigh, const char * role) {
	struct gpio_v2_line_request req;
	memset(&req, 0, sizeof(req));
	for (size_t i = 0; i < lines.size(); i++) {
		req.offsets[i] = lines[i];
	}
	req.num_lines = (uint32_t) lines.size();
	strncpy(req.consumer, CONSUMER, sizeof(req.consumer) - 1);
	req.config.flags = flags;

	if (setOutputsHigh) {
		// Start with every column released (high / floating).
		req.config.num_attrs = 1;
		req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
		req.config.attrs[0].attr.values = allBits(lines.size());
		req.config.attrs[0].mask = allBits(lines.size());
	}

	if (ioctl(chipFd, GPIO_V2_GET_LINE_IOCTL, &req) < 0) {
		int err = errno;
		std::string msg = "cannot claim " + std::string(role) + " GPIO lines ["
				+ describeLines(lines) + "] on " + chipPath + ": "
				+ strerror(err);
		if (err == EBUSY) {
			msg += ". Something else is using these pins. Either the background "
					"service is already running (stop it with 'sudo systemctl stop "
					"flight-simulator-keys'), or, on a Pi, GPIO 14/15 are the serial "
					"console and GPIO 2/3 are I2C: disable them with 'sudo "
					"raspi-config' (Interface Options) and reboot.";
		} else if (err == EINVAL) {
			msg += ". Check the pin numbers are BCM numbers valid for this chip.";
		}
		throw std::runtime_error(msg);
	}
	return req.fd;
}

}

ChardevGpioMatrix::ChardevGpioMatrix(const std::string & chipPath,
		const std::vector<unsigned> & columns,
		const std::vector<unsigned> & rows) :
		nColumns(columns.size()), nRows(rows.size()) {

	if (columns.size() > GPIO_V2_LINES_MAX || rows.size() > GPIO_V2_LINES_MAX) {
		throw std::runtime_error("too many GPIO lines in one request (max "
				+ std::to_string(GPIO_V2_LINES_MAX) + ")");
	}

	chipFd = open(chipPath.c_str(), O_RDWR | O_CLOEXEC);
	if (chipFd < 0) {
		int err = errno;
		std::string msg = "cannot open " + chipPath + ": " + strerror(err);
		if (err == EACCES) {
			msg += ". Run as root or add the user to the 'gpio' group.";
		}
		throw std::runtime_error(msg);
	}

	try {
		columnFd = requestLines(chipFd, chipPath, columns,
				GPIO_V2_LINE_FLAG_OUTPUT | GPIO_V2_LINE_FLAG_OPEN_DRAIN, true,
				"column");
		rowFd = requestLines(chipFd, chipPath, rows,
				GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_BIAS_PULL_UP, false,
				"row");
	} catch (...) {
		if (columnFd >= 0) {
			close(columnFd);
		}
		close(chipFd);
		throw;
	}
}

ChardevGpioMatrix::~ChardevGpioMatrix() {
	// Closing the request fds returns the lines to the kernel.
	if (rowFd >= 0) {
		close(rowFd);
	}
	if (columnFd >= 0) {
		try {
			selectColumn(-1);
		} catch (const std::exception &) {
			// closing the fd below releases the lines anyway
		}
		close(columnFd);
	}
	if (chipFd >= 0) {
		close(chipFd);
	}
}

void ChardevGpioMatrix::selectColumn(int col) {
	struct gpio_v2_line_values vals;
	memset(&vals, 0, sizeof(vals));
	vals.mask = allBits(nColumns);
	vals.bits = allBits(nColumns);
	if (col >= 0) {
		vals.bits &= ~(1ULL << col);
	}
	if (ioctl(columnFd, GPIO_V2_LINE_SET_VALUES_IOCTL, &vals) < 0) {
		throw std::runtime_error(
				std::string("setting column GPIO values failed: ")
						+ strerror(errno));
	}
	usleep(SETTLE_US);
}

uint64_t ChardevGpioMatrix::readActiveRows() {
	struct gpio_v2_line_values vals;
	memset(&vals, 0, sizeof(vals));
	vals.mask = allBits(nRows);
	if (ioctl(rowFd, GPIO_V2_LINE_GET_VALUES_IOCTL, &vals) < 0) {
		throw std::runtime_error(
				std::string("reading row GPIO values failed: ")
						+ strerror(errno));
	}
	// Pull-ups hold idle rows high; a pressed key pulls its row low.
	return ~vals.bits & allBits(nRows);
}

}
