/*
 * host_file.h - publishes the discovered X-Plane address for other programs
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_HOST_FILE_H
#define FLIGHTSIM_HOST_FILE_H

#include <string>

namespace flightsim {

// Keeps a one-line file holding the current X-Plane IP, so the optional
// CDU display (flight-simulator-display) can find X-Plane without its own
// configuration. Writes are atomic (temp file + rename), so readers never
// see a partial line. Failures are reported once and never stop the keys.
class HostFile {
public:
	explicit HostFile(std::string path);
	~HostFile();	// removes the file: X-Plane is no longer known

	// Publishes `ip`, or removes the file when `ip` is empty.
	void set(const std::string & ip);

private:
	std::string path;
	std::string current;
	bool warned = false;
};

}

#endif
