/*
 * dtoken.c — Unique request token.
 *
 * Copyright (C) 2023 ghax.org
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * This file contains the implementation of Dtoken: a unique request token that
 * contains various information about a web request. The token includes details
 * such as the time, client and server information, and optional generic ids,
 * and can be used for logging and debugging web requests.
 *
 * This file contains the definition of Dtoken.
 */

#ifndef DTOKEN_H
#define DTOKEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <math.h>
#include <gmp.h>

#define STR(x) #x
#define CONCAT(a, b, c) STR(a) "." STR(b) "." STR(c)

/* Version info for this application */
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0
#define VERSION CONCAT(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

/* Time types */
#define TIME_S 0
#define TIME_US 1

/* HTTP methods */
#define GET 1
#define POST 2
#define PUT 3
#define DELETE 4
#define HEAD 5
#define CONNECT 6
#define OPTIONS 7
#define TRACE 8
#define PATCH 9

/* Bit sizes of the various token segments */
#define VERSION_PATCH_SIZE 4
#define VERSION_MINOR_SIZE 8
#define VERSION_MAJOR_SIZE 4
#define TIME_TYPE_SIZE 1
#define TIME_S_SIZE 32
#define TIME_US_SIZE 52
#define METHOD_SIZE 4
#define ID1_SIZE 23
#define ID2_SIZE 15
#define PORT_SIZE 16
#define IPv4_SIZE 32
#define IPv6_SIZE 128

#define INET4 0 /* bit to store for AF_INET  */
#define INET6 1 /* bit to store for AF_INET6 */

// This struct represents the Dtoken data that will be encoded
struct token_data
{
	short int time_type; // Format used for the timestamp: 0 = seconds, 1 = µs
	long int timestamp; // The timestamp of the request, in either seconds or µs

	int method; // Use predefined macro constants (GET, POST, PUT, etc.)

	short int client_enabled; // Whether client information is included in token
	short int client_protocol; // Client address protocol (0 = IPv4, 1 = IPv6)
	union { struct in_addr v4; struct in6_addr v6; } client_ip;
	short int client_port; // Port client is speaking from

	short int lb_enabled; // Whether load balancer information is included in token
	short int lb_protocol; // LB address protocol (0 = IPv4, 1 = IPv6)
	union { struct in_addr v4; struct in6_addr v6; } lb_ip;
	short int lb_port; // Port load balancer is speaking from

	short int server_enabled; // Whether web server information is included in token
	short int server_protocol; // Server address protocol (0 = IPv4, 1 = IPv6)
	union { struct in_addr v4; struct in6_addr v6; } server_ip;
	short int server_port; // Port connected to on the web server

	int id1; // Generic id: for instance id of the user that requested the page
	int id2; // Generic id: for instance the id of the page that was resuted
};

#endif /* DTOKEN_H */
