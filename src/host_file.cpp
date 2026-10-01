/*
 * host_file.cpp - publishes the discovered X-Plane address for other programs
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "host_file.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <utility>

namespace flightsim {

HostFile::HostFile(std::string _path) :
		path(std::move(_path)) {
	// Never leave an address from a previous run behind.
	std::remove(path.c_str());
}

HostFile::~HostFile() {
	if (!path.empty()) {
		std::remove(path.c_str());
	}
}

void HostFile::set(const std::string & ip) {
	if (path.empty() || ip == current) {
		return;
	}
	current = ip;

	if (ip.empty()) {
		std::remove(path.c_str());
		return;
	}

	const std::string tmp = path + ".tmp";
	{
		std::ofstream out(tmp, std::ios::trunc);
		if (out) {
			out << ip << '\n';
		}
		if (!out) {
			if (!warned) {
				std::cerr << "note: cannot write " << tmp << ": "
						<< strerror(errno)
						<< " (only needed by the optional CDU display)"
						<< std::endl;
				warned = true;
			}
			return;
		}
	}
	if (std::rename(tmp.c_str(), path.c_str()) != 0 && !warned) {
		std::cerr << "note: cannot create " << path << ": " << strerror(errno)
				<< std::endl;
		warned = true;
	}
}

}
