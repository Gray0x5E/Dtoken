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
 * such as the time, client and server information, and user and page IDs, and
 * can be used for logging and debugging web requests.
 *
 * Dtoken is implemented as a PHP extension, making it easy to integrate into
 * web applications. By generating a unique token for each request, Dtoken can
 * help developers track down issues and debug problems with their web
 * applications.
 *
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define STR(x) #x
#define CONCAT(a, b, c) STR(a) "." STR(b) "." STR(c)

/* Version info for this application */
#define VERSION_MAJOR 7
#define VERSION_MINOR 8
#define VERSION_PATCH 9
#define VERSION CONCAT(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

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
#define USER_ID_SIZE 24
#define PAGE_ID_SIZE 16
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

	int user_id; // The id of the user that requested the page
	int page_id; // The id of the page that was resuted
};

int main()
{
	// 0 = s, 1 = µs
	_Bool time_type = 0;

	// Information about the client connection
	_Bool client_enabled = 0;
	int client_protocol = AF_INET6;
	char* client_address = "";
	int client_port = 0;

	_Bool lb_enabled = 0;
	short int lb_protocol = AF_INET;
	char* lb_address = "";
	short int lb_port = 0;

	_Bool server_enabled = 0;
	short int server_protocol = AF_INET;
	char* server_address = "";
	short int server_port = 0;

	int user_id = 0;
	int page_id = 0;

	char input[45];
	char* message;

	// Set time precision
	message = "Enter time precision (s/us)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0'; // remove newline character
		if (strcmp(input, "s") == 0)
		{
			time_type = 0;
			break;
		}
		else if (strcmp(input, "us") == 0)
		{
			time_type = 1;
			break;
		}
		printf("Invalid option.\n%s: ", message);
	}

	// Determine if we want to add client data
	message = "Add client information to token (yes/no)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0'; // remove newline character
		if (strcmp(input, "no") == 0)
		{
			client_enabled = 0;
			break;
		}
		else if (strcmp(input, "yes") == 0)
		{
			client_enabled = 1;
			break;
		}
		printf("Invalid option.\n%s: ", message);
	}

	// Add client data
	if (client_enabled)
	{
		message = "Enter client IP address";
		printf("%s: ", message);
		while (fgets(input, sizeof(input), stdin))
		{
			input[strcspn(input, "\n")] = '\0'; // remove newline character

			struct sockaddr_in sa;

			// For IP validation
			int result;

			result = inet_pton(AF_INET, input, &(sa.sin_addr));

			// Valid IPv4
			if (result == 1)
			{
				client_protocol = AF_INET;
				client_address = input;
				break;
			}

			result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

			// Valid IPv6
			if (result == 1)
			{
				client_protocol = AF_INET6;
				client_address = input;
				break;
			}

			printf("Invalid address.\n%s: ", message);
		}
	}

	printf("\n");
	printf("Timestamp: %s\n", time_type == 0 ? "seconds" : "seconds with µs");

	if (client_enabled)
	{
		printf("Client:\n");
		printf("    protocol: %s\n", client_protocol == AF_INET ? "IPv4" : "IPv6");
		printf("    address: %s\n", client_address);
		if (client_port)
		{
			printf("    port: %d\n", client_port);
		}
	}

	if (lb_enabled)
	{
		printf("LB:\n");
		printf("    protocol: %s\n", lb_protocol == AF_INET ? "IPv4" : "IPv6");
		printf("    address: %s\n", lb_address);
		if (lb_port)
		{
			printf("    port: %d\n", lb_port);
		}
	}

	if (server_enabled)
	{
		printf("Server:\n");
		printf("    protocol: %s\n", server_protocol == AF_INET ? "IPv4" : "IPv6");
		printf("    address: %s\n", server_address);
		if (server_port)
		{
			printf("    server_port: %d\n", lb_port);
		}
	}

	if (user_id)
	{
		printf("User id: %d\n", user_id);
	}

	if (page_id)
	{
		printf("User id: %d\n", page_id);
	}
}
