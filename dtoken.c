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


void add_port(mpz_ptr token, short int port)
{
	if (!port)
	{
		// Disabled bit
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 0);
		return;
	}

	mpz_mul_2exp(token, token, PORT_SIZE);
	mpz_add_ui(token, token, port);

	// Enabled bit
	mpz_mul_2exp(token, token, 1);
	mpz_add_ui(token, token, 1);
}

void add_address(
	mpz_ptr token,
	short int enabled,
	short int protocol,
	union { struct in_addr v4; struct in6_addr v6; } *ip
)
{
	if (!enabled)
	{
		mpz_mul_2exp(token, token, 1); // 1 bit for status
		mpz_add_ui(token, token, 0); // 0 for disabled (implicit)
		return;
	}

	if (protocol == AF_INET)
	{
		mpz_mul_2exp(token, token, IPv4_SIZE);
		mpz_add_ui(token, token, ntohl((unsigned long)ip->v4.s_addr));

		// Add protocol bit
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, INET4);
	}
	else // IPv6
	{
		mpz_mul_2exp(token, token, IPv6_SIZE); // 128 bits reserved for method

		// Import IPv6 address into temporary variable
		mpz_t ipv6_addr;
		mpz_init(ipv6_addr);
		mpz_import(ipv6_addr, sizeof(ip->v6.s6_addr), 1, sizeof(ip->v6.s6_addr[0]), 0, 0, ip->v6.s6_addr);

		// Add imported address to token
		mpz_add(token, token, ipv6_addr);

		// Clear temporary variable
		mpz_clear(ipv6_addr);

		// protocol bit
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, INET6);
	}

	// Add enabled bit
	mpz_mul_2exp(token, token, 1);
	mpz_add_ui(token, token, 1);
}

void add_token_data(mpz_ptr token, struct token_data *data)
{
	//mpz_add_ui(token, token, 1);

	// Add page id
	if (!data->page_id)
	{
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 0);
	}
	else
	{
		mpz_mul_2exp(token, token, PAGE_ID_SIZE);
		mpz_add_ui(token, token, data->page_id);
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 1);
	}

	// Add user id
	if (!data->user_id)
	{
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 0);
	}
	else
	{
		mpz_mul_2exp(token, token, USER_ID_SIZE);
		mpz_add_ui(token, token, data->user_id);
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 1);
	}

	// Server
	if (data->server_enabled)
	{
		add_port(token, data->server_port);
	}
	add_address(token, data->server_enabled, data->server_protocol, (void *)&(data->server_ip));

	// LB
	if (data->lb_enabled)
	{
		add_port(token, data->lb_port);
	}
	add_address(token, data->lb_enabled, data->lb_protocol, (void *)&(data->lb_ip));

	// Client
	if (data->client_enabled)
	{
		add_port(token, data->client_port);
	}
	add_address(token, data->client_enabled, data->client_protocol, (void *)&(data->client_ip));

	// Add method
	mpz_mul_2exp(token, token, METHOD_SIZE); // 4 bits reserved for method
	mpz_add_ui(token, token, data->method);

	// Add timestamp
	if (data->time_type == 0)
	{
		// 32 bits for short timestamp
		mpz_mul_2exp(token, token, TIME_S_SIZE);
		// add timestamp
		mpz_add_ui(token, token, data->timestamp);
		// shift 1 bit left and leave bit zero for type seconds
		mpz_mul_2exp(token, token, TIME_TYPE_SIZE);
		// add 0 for type µs
		mpz_add_ui(token, token, 0);
	}
	else
	{
		// 32 bits for short timestamp
		mpz_mul_2exp(token, token, TIME_US_SIZE);
		// add timestamp
		mpz_add_ui(token, token, data->timestamp);
		// shift 1 bit left for time type bit
		mpz_mul_2exp(token, token, TIME_TYPE_SIZE);
		// add 1 for type µs
		mpz_add_ui(token, token, 1);
	}

	// add major version
	mpz_mul_2exp(token, token, VERSION_MAJOR_SIZE);
	mpz_add_ui(token, token, VERSION_MAJOR);

	// add minor version
	mpz_mul_2exp(token, token, VERSION_MINOR_SIZE);
	mpz_add_ui(token, token, VERSION_MINOR);

	// add path version
	mpz_mul_2exp(token, token, VERSION_PATCH_SIZE);
	mpz_add_ui(token, token, VERSION_PATCH);
}

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
	int user_id,
	int page_id)
{
	struct token_data data;

	data.time_type = time_type;
	data.timestamp = timestamp;

	data.method = method;

	data.client_enabled = client_enabled;
	data.client_protocol = client_protocol;
	data.client_port = client_port;

	data.lb_enabled = lb_enabled;
	data.lb_protocol = lb_protocol;
	data.lb_port = lb_port;

	data.server_enabled = server_enabled;
	data.server_protocol = server_protocol;
	data.server_port = server_port;

	data.user_id = user_id;
	data.page_id = page_id;

	// client address
	inet_pton(client_protocol, client_address, (client_protocol == AF_INET) ? (void *)&(data.client_ip.v4) : (void *)&(data.client_ip.v6));

	// lb address
	inet_pton(lb_protocol, lb_address, (lb_protocol == AF_INET) ? (void *)&(data.lb_ip.v4) : (void *)&(data.lb_ip.v6));

	// server address
	inet_pton(server_protocol, server_address, (server_protocol == AF_INET) ? (void *)&(data.server_ip.v4) : (void *)&(data.server_ip.v6));

	data.user_id = user_id;
	data.page_id = page_id;

	mpz_t token;
	mpz_init(token);
	mpz_clear(token);

	add_token_data(token, &data);

	// Convert to and store base 36 value in buffer
	char* token_str = mpz_get_str(NULL, 36, token);
	strcpy(buffer, token_str);
	free(token_str);
	mpz_clear(token);

	return buffer;
}

int main()
{
	// Request timestamp
	_Bool time_type = 0; // 0 = s, 1 = µs
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long int timestamp = tv.tv_sec;

	// HTTP method
	int method = GET;

	// Information about the client connection
	_Bool client_enabled = 0;
	int client_protocol = AF_INET;
	char client_address[INET6_ADDRSTRLEN] = "";
	int client_port = 0;

	// Information about load balancer connection
	_Bool lb_enabled = 0;
	short int lb_protocol = AF_INET;
	char lb_address[INET6_ADDRSTRLEN] = "";
	short int lb_port = 0;

	// Information about web server connection
	_Bool server_enabled = 0;
	short int server_protocol = AF_INET;
	char server_address[INET6_ADDRSTRLEN] = "";
	short int server_port = 0;

	// Id of user requesting page
	int user_id = 0;

	// Id of page being requested
	int page_id = 0;

	char input[45];
	char* message;

	// Set time precision
	message = "Enter time precision (s/us) [s]";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "s") == 0 || strcmp(input, "") == 0)
		{
			time_type = TIME_S;
			timestamp = tv.tv_sec;
			break;
		}
		else if (strcmp(input, "us") == 0)
		{
			time_type = TIME_US;
			timestamp = (tv.tv_sec + (tv.tv_usec / 1000000.0)) * 1000000;
			break;
		}

		printf("Invalid option.\n%s: ", message);
	}

	// Set HTTP method
	message = "Enter HTTP method (GET, POST, PUT, etc.) [GET]";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "") == 0)
		{
			break;
		}
		else if (strcmp(input, "GET") == 0)
		{
			method = GET;
			break;
		}
		else if (strcmp(input, "POST") == 0)
		{
			method = POST;
			break;
		}
		else if (strcmp(input, "PUT") == 0)
		{
			method = PUT;
			break;
		}
		else if (strcmp(input, "DELETE") == 0)
		{
			method = DELETE;
			break;
		}
		else if (strcmp(input, "HEAD") == 0)
		{
			method = HEAD;
			break;
		}
		else if (strcmp(input, "CONNECT") == 0)
		{
			method = CONNECT;
			break;
		}
		else if (strcmp(input, "OPTIONS") == 0)
		{
			method = OPTIONS;
			break;
		}
		else if (strcmp(input, "TRACE") == 0)
		{
			method = TRACE;
			break;
		}
		else if (strcmp(input, "PATCH") == 0)
		{
			method = PATCH;
			break;
		}
		else
		{
			printf("Invalid option.\n%s: ", message);
		}
	}

	// Add client data
	message = "Enter client IP address (leave empty for none)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "") == 0)
		{
			break;
		}

		struct sockaddr_in sa;

		// For IP validation
		int result;

		result = inet_pton(AF_INET, input, &(sa.sin_addr));

		// Valid IPv4
		if (result == 1)
		{
			client_enabled = 1;
			client_protocol = AF_INET;
			strcpy(client_address, input);
			break;
		}

		result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

		// Valid IPv6
		if (result == 1)
		{
			client_enabled = 1;
			client_protocol = AF_INET6;
			strcpy(client_address, input);
			break;
		}

		printf("Invalid address.\n%s: ", message);
	}

	if (client_enabled)
	{
		message = "Enter client port (leave empty for none)";
		printf("%s: ", message);
		while (fgets(input, sizeof(input), stdin))
		{
			input[strcspn(input, "\n")] = '\0';

			// User pressed Enter, i.e. no client port
			if (strcmp(input, "") == 0)
			{
				break;
			}

			int entered_port = atoi(input);
			if (entered_port > 0 && entered_port <= 65535)
			{
				client_port = entered_port;
				break;
			}

			printf("Invalid port.\n%s: ", message);
		}
	}

	// Add load balancer data
	message = "Enter load balancer IP address (leave empty for none)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "") == 0)
		{
			break;
		}

		struct sockaddr_in sa;

		// For IP validation
		int result;

		result = inet_pton(AF_INET, input, &(sa.sin_addr));

		// Valid IPv4
		if (result == 1)
		{
			lb_enabled = 1;
			lb_protocol = AF_INET;
			strcpy(lb_address, input);
			break;
		}

		result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

		// Valid IPv6
		if (result == 1)
		{
			lb_enabled = 1;
			lb_protocol = AF_INET6;
			strcpy(lb_address, input);
			break;
		}

		printf("Invalid address.\n%s: ", message);
	}

	if (lb_enabled)
	{
		message = "Enter load balancer port (leave empty for none)";
		printf("%s: ", message);
		while (fgets(input, sizeof(input), stdin))
		{
			input[strcspn(input, "\n")] = '\0';

			// User pressed Enter, i.e. no load balancer port
			if (strcmp(input, "") == 0)
			{
				break;
			}

			int entered_port = atoi(input);
			if (entered_port > 0 && entered_port <= 65535)
			{
				lb_port = entered_port;
				break;
			}

			printf("Invalid port.\n%s: ", message);
		}
	}

	// Add server data
	message = "Enter server IP address (leave empty for none)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "") == 0)
		{
			break;
		}

		struct sockaddr_in sa;

		// For IP validation
		int result;

		result = inet_pton(AF_INET, input, &(sa.sin_addr));

		// Valid IPv4
		if (result == 1)
		{
			server_enabled = 1;
			server_protocol = AF_INET;
			strcpy(server_address, input);
			break;
		}

		result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

		// Valid IPv6
		if (result == 1)
		{
			server_enabled = 1;
			server_protocol = AF_INET6;
			strcpy(server_address, input);
			break;
		}

		printf("Invalid address.\n%s: ", message);
	}

	if (server_enabled)
	{
		message = "Enter server port (leave empty for none)";
		printf("%s: ", message);
		while (fgets(input, sizeof(input), stdin))
		{
			input[strcspn(input, "\n")] = '\0';

			// User pressed Enter, i.e. no client port
			if (strcmp(input, "") == 0)
			{
				break;
			}

			int entered_port = atoi(input);
			if (entered_port > 0 && entered_port <= 65535)
			{
				server_port = entered_port;
				break;
			}

			printf("Invalid port.\n%s: ", message);
		}
	}

	// Set user id
	message = "Enter user id (leave empty for none)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "") == 0)
		{
			break;
		}

		int entered_id = atoi(input);
		if (entered_id > 0 && entered_id <= INT_MAX)
		{
			user_id = entered_id;
			break;
		}

		printf("Invalid option.\n%s: ", message);
	}

	// Set page id
	message = "Enter page id (leave empty for none)";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "") == 0)
		{
			break;
		}

		int entered_id = atoi(input);
		if (entered_id > 0 && entered_id <= INT_MAX)
		{
			page_id = entered_id;
			break;
		}

		printf("Invalid option.\n%s: ", message);
	}

	printf("\n");

	// Output token data
	//---------------------------------------------------------

	if (time_type == TIME_S)
	{
		// Output timestamp in seconds
		//-----------------------------------------------------
		printf("Timestamp:     %ld\n", timestamp);
	}
	else
	{
		// Output timestamp with microseconds
		//-----------------------------------------------------
		printf("Timestamp:     %.6f\n", timestamp / 1000000.0);
	}

	if (client_enabled)
	{
		// Output client information
		//-----------------------------------------------------
		printf("Client:        %s", client_address);

		if (client_port)
		{
			printf(":%d", client_port);
		}

		printf("\n");
	}

	if (lb_enabled)
	{
		// Output load balancer information
		//-----------------------------------------------------
		printf("Load balancer: %s", lb_address);

		if (lb_port)
		{
			printf(":%d", lb_port);
		}

		printf("\n");
	}

	if (server_enabled)
	{
		// Output server information
		//-----------------------------------------------------
		printf("Server:        %s", server_address);

		if (server_port)
		{
			printf(":%d", server_port);
		}

		printf("\n");
	}

	if (user_id)
	{
		// Output user id
		//-----------------------------------------------------
		printf("User id:       %d\n", user_id);
	}

	if (page_id)
	{
		// Output page id
		//-----------------------------------------------------
		printf("Page id:       %d\n", page_id);
	}

	// Build and output token
	//-----------------------------------------------------

	int buffer_size =
		VERSION_PATCH_SIZE +
		VERSION_MINOR_SIZE +
		VERSION_MAJOR_SIZE +
		TIME_TYPE_SIZE +
		TIME_US_SIZE +
		METHOD_SIZE +
		USER_ID_SIZE +
		PAGE_ID_SIZE +
		((2 + PORT_SIZE) * 3) + // x number of addresses + extra bits for meta
		((2 + IPv6_SIZE) * 3) + // x number of addresses + extra bits for meta
		100; // Adding some extra bits for good measure

	char token_buffer[buffer_size];

	char* token = build(token_buffer, method, time_type, timestamp, client_enabled, client_protocol, client_address, client_port, lb_enabled, lb_protocol, lb_address, lb_port, server_enabled, server_protocol, server_address, server_port, user_id, page_id);

	printf("\nToken: %s\n", token);
}
