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

/**
 * Represents the data included in a Dtoken request token
 *
 * @struct token_data
 *
 * @param short int time_type The format used for the timestamp: 0 = seconds, 1 = microseconds
 * @param long int timestamp The timestamp of the request, in either seconds or microseconds
 * @param int method The HTTP method used for the request
 * @param short int client_enabled Whether client information is included in the token
 * @param short int client_protocol The client address protocol (0 = IPv4, 1 = IPv6)
 * @param union { struct in_addr v4; struct in6_addr v6; } client_ip The client IP address
 * @param short int client_port The port the client is speaking from
 * @param short int lb_enabled Whether load balancer information is included in the token
 * @param short int lb_protocol The load balancer address protocol (0 = IPv4, 1 = IPv6)
 * @param union { struct in_addr v4; struct in6_addr v6; } lb_ip The load balancer IP address
 * @param short int lb_port The port the load balancer is speaking from
 * @param short int server_enabled Whether web server information is included in the token
 * @param short int server_protocol The server address protocol (0 = IPv4, 1 = IPv6)
 * @param union { struct in_addr v4; struct in6_addr v6; } server_ip The web server IP address
 * @param short int server_port The port connected to on the web server
 * @param int id1 Generic id (e.g. user id)
 * @param int id2 Generic id (e.g. page id)
 */
struct token_data
{
	short int time_type;
	long int timestamp;
	int method;
	short int client_enabled;
	short int client_protocol;
	union { struct in_addr v4; struct in6_addr v6; } client_ip;
	short int client_port;
	short int lb_enabled;
	short int lb_protocol;
	union { struct in_addr v4; struct in6_addr v6; } lb_ip;
	short int lb_port;
	short int server_enabled;
	short int server_protocol;
	union { struct in_addr v4; struct in6_addr v6; } server_ip;
	short int server_port;
	int id1;
	int id2;
};

/**
 * Adds a port number to the given token
 *
 * @param mpz_ptr token The token to add the port number to
 * @param short int port The port number to add
 */
void add_port(mpz_ptr token, short int port);

/**
 * Adds an address to the given token
 *
 * @param mpz_ptr token The token to add the address to
 * @param short int enabled Whether the address is enabled or not
 * @param short int protocol The protocol used by the address (IPv4 or IPv6)
 * @param void* ip The IP address to add
 */
void add_address(
	mpz_ptr token,
	short int enabled,
	short int protocol,
	void* ip
);

/**
 * Adds token data to the given token
 *
 * @param mpz_ptr token The token to add the data to
 * @param struct token_data data The data to add
 */
void add_token_data(mpz_ptr token, struct token_data *data);

 /**
 * Builds a request token using the given parameters
 *
 * @param char* buffer The buffer to store the token in
 * @param int method The HTTP method used for the request (see macros for mapping)
 * @param _Bool time_type The type of timestamp used (second (0) or microsecond (1) precision)
 * @param long int timestamp The timestamp of the request
 * @param _Bool client_enabled Whether the client address is enabled or not
 * @param short int client_protocol The protocol used by the client address (IPv4 or IPv6)
 * @param char* client_address The IP address of the client
 * @param short int client_port The port number used by the client
 * @param _Bool lb_enabled Whether the load balancer address is enabled or not
 * @param short int lb_protocol The protocol used by the load balancer address (IPv4 or IPv6)
 * @param char* lb_address The IP address of the load balancer
 * @param short int lb_port The port number used by the load balancer
 * @param _Bool server_enabled Whether the server address is enabled or not
 * @param short int server_protocol The protocol used by the server address (IPv4 or IPv6)
 * @param char* server_address The IP address of the server
 * @param short int server_port The port number used by the server
 * @param int id1 The first generic id value to include in the token
 * @param int id2 The second generic id value to include in the token
 *
 * @return char* The generated request token as base 36
 */
char* build(
	char* buffer,
	int method,
	_Bool time_type,
	long int timestamp,
	_Bool client_enabled,
	short int client_protocol,
	char* client_address,
	short int client_port,
	_Bool lb_enabled,
	short int lb_protocol,
	char* lb_address,
	short int lb_port,
	_Bool server_enabled,
	short int server_protocol,
	char* server_address,
	short int server_port,
	int id1,
	int id2);

#endif /* DTOKEN_H */
