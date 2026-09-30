/*
 * xplane_link.h - finds X-Plane on the network and sends it commands
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef FLIGHTSIM_XPLANE_LINK_H
#define FLIGHTSIM_XPLANE_LINK_H

#include <cstdint>
#include <memory>
#include <string>

class XPlaneUDPClient;

namespace flightsim {

struct SimEndpoint {
	std::string ip;
	uint16_t port = 0;
	std::string name;

	bool operator==(const SimEndpoint & o) const {
		return ip == o.ip && port == o.port;
	}
	bool operator!=(const SimEndpoint & o) const { return !(*this == o); }
};

// Resolves a hostname or dotted quad to a dotted-quad IPv4 address
// (the vendored UDP client only accepts the latter). Throws on failure.
std::string resolveIPv4(const std::string & host);

// Returns true and fills `out` if an X-Plane instance is currently sending
// its multicast beacon. Prefers the master instance when several exist.
// The first call starts the vendored library's background listener.
bool findXPlaneByBeacon(SimEndpoint & out);

// Keeps a UDP client pointed at the current X-Plane instance. With a
// static host the target never changes; otherwise refresh() follows the
// beacon, so the link survives X-Plane restarts and IP changes.
class XPlaneLink {
public:
	// Empty host = beacon discovery.
	XPlaneLink(const std::string & host, uint16_t port);
	~XPlaneLink();

	// Returns true if a target is known. Returns a description of any
	// change through `change` (empty if nothing changed).
	bool refresh(std::string & change);

	bool connected() const { return client != nullptr; }
	const SimEndpoint & target() const { return current; }

	void sendCommand(const std::string & command);

private:
	bool useBeacon;
	SimEndpoint current;
	std::unique_ptr<XPlaneUDPClient> client;
};

}

#endif
