/*
 * xplane_link.cpp - finds X-Plane on the network and sends it commands
 *
 * Copyright (c) 2026 Pi-XPlane-FMC-CDU-Keys-Only contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "xplane_link.h"

#include <XPlaneBeaconListener.h>
#include <XPlaneUDPClient.h>

#include <arpa/inet.h>
#include <cstring>
#include <list>
#include <netdb.h>
#include <stdexcept>

namespace flightsim {

namespace {

// Beacon "role" field, from X-Plane's "Exchanging Data with X-Plane" spec.
const int32_t ROLE_MASTER = 1;

}

std::string resolveIPv4(const std::string & host) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	struct addrinfo * res = nullptr;
	int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res);
	if (rc != 0 || res == nullptr) {
		throw std::runtime_error("cannot resolve X-Plane host \"" + host + "\": "
				+ gai_strerror(rc));
	}
	char buf[INET_ADDRSTRLEN];
	const struct sockaddr_in * sin = (const struct sockaddr_in *) res->ai_addr;
	inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
	freeaddrinfo(res);
	return buf;
}

bool findXPlaneByBeacon(SimEndpoint & out) {
	// Poll the listener's mutex-protected cache rather than registering a
	// callback: the library adds callbacks without locking while its
	// listener thread may already be iterating them.
	std::list<XPlaneBeaconListener::XPlaneServer> servers;
	XPlaneBeaconListener::getInstance()->get(servers);
	if (servers.empty()) {
		return false;
	}
	const XPlaneBeaconListener::XPlaneServer * pick = &servers.front();
	for (const auto & s : servers) {
		if (s.role == ROLE_MASTER) {
			pick = &s;
			break;
		}
	}
	out.ip = pick->host;
	out.port = pick->receivePort;
	out.name = pick->name;
	return true;
}

XPlaneLink::XPlaneLink(const std::string & host, uint16_t port) :
		useBeacon(host.empty()) {
	if (!useBeacon) {
		current.ip = resolveIPv4(host);
		current.port = port;
		current.name = host;
	}
}

XPlaneLink::~XPlaneLink() = default;

bool XPlaneLink::refresh(std::string & change) {
	change.clear();

	if (!useBeacon) {
		if (!client) {
			client.reset(new XPlaneUDPClient(current.ip, current.port, nullptr,
					nullptr));
			change = "sending to " + current.name + " (" + current.ip + ":"
					+ std::to_string(current.port) + ")";
		}
		return true;
	}

	SimEndpoint found;
	if (!findXPlaneByBeacon(found)) {
		if (client) {
			client.reset();
			change = "lost X-Plane beacon from " + current.ip
					+ "; waiting for it to return";
			current = SimEndpoint();
		}
		return false;
	}
	if (!client || found != current) {
		current = found;
		client.reset(new XPlaneUDPClient(current.ip, current.port, nullptr,
				nullptr));
		change = "found X-Plane \"" + current.name + "\" at " + current.ip + ":"
				+ std::to_string(current.port);
	}
	return true;
}

void XPlaneLink::sendCommand(const std::string & command) {
	if (client) {
		client->sendCommand(command);
	}
}

}
