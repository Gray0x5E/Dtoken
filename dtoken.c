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

#define STR(x) #x
#define CONCAT(a, b, c) STR(a) "." STR(b) "." STR(c)

/* Version info for this application */
#define VERSION_MAJOR 7
#define VERSION_MINOR 8
#define VERSION_PATCH 9
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

int main()
{
	// Request timestamp
	_Bool time_type = 0; // 0 = s, 1 = µs
	long int timestamp = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);

	// HTTP method
	int method = GET;

	// Information about the client connection
	_Bool client_enabled = 0;
	int client_protocol = AF_INET6;
	char client_address[INET6_ADDRSTRLEN] = "";
	int client_port = 0;

	_Bool lb_enabled = 0;
	short int lb_protocol = AF_INET;
	char lb_address[INET6_ADDRSTRLEN] = "";
	short int lb_port = 0;

	_Bool server_enabled = 0;
	short int server_protocol = AF_INET;
	char server_address[INET6_ADDRSTRLEN] = "";
	short int server_port = 0;

	int user_id = 0;
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

	// Determine if we want to add client data
	message = "Add client information to token (yes/no) [no]";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "no") == 0 || strcmp(input, "") == 0)
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
			input[strcspn(input, "\n")] = '\0';

			struct sockaddr_in sa;

			// For IP validation
			int result;

			result = inet_pton(AF_INET, input, &(sa.sin_addr));

			// Valid IPv4
			if (result == 1)
			{
				client_protocol = AF_INET;
				strcpy(client_address, input);
				break;
			}

			result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

			// Valid IPv6
			if (result == 1)
			{
				client_protocol = AF_INET6;
				strcpy(client_address, input);
				break;
			}

			printf("Invalid address.\n%s: ", message);
		}

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

	// Determine if we want to add load balancer data
	message = "Add load balancer information to token (yes/no) [no]";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "no") == 0 || strcmp(input, "") == 0)
		{
			lb_enabled = 0;
			break;
		}
		else if (strcmp(input, "yes") == 0)
		{
			lb_enabled = 1;
			break;
		}

		printf("Invalid option.\n%s: ", message);
	}

	// Add load balancer data
	if (lb_enabled)
	{
		message = "Enter load balancer IP address";
		printf("%s: ", message);
		while (fgets(input, sizeof(input), stdin))
		{
			input[strcspn(input, "\n")] = '\0';

			struct sockaddr_in sa;

			// For IP validation
			int result;

			result = inet_pton(AF_INET, input, &(sa.sin_addr));

			// Valid IPv4
			if (result == 1)
			{
				lb_protocol = AF_INET;
				strcpy(lb_address, input);
				break;
			}

			result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

			// Valid IPv6
			if (result == 1)
			{
				lb_protocol = AF_INET6;
				strcpy(lb_address, input);
				break;
			}

			printf("Invalid address.\n%s: ", message);
		}

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

	// Determine if we want to add server data
	message = "Add web server information to token (yes/no) [no]";
	printf("%s: ", message);
	while (fgets(input, sizeof(input), stdin))
	{
		input[strcspn(input, "\n")] = '\0';

		if (strcmp(input, "no") == 0 || strcmp(input, "") == 0)
		{
			server_enabled = 0;
			break;
		}
		else if (strcmp(input, "yes") == 0)
		{
			server_enabled = 1;
			break;
		}

		printf("Invalid option.\n%s: ", message);
	}

	// Add server data
	if (server_enabled)
	{
		message = "Enter server IP address";
		printf("%s: ", message);
		while (fgets(input, sizeof(input), stdin))
		{
			input[strcspn(input, "\n")] = '\0';

			struct sockaddr_in sa;

			// For IP validation
			int result;

			result = inet_pton(AF_INET, input, &(sa.sin_addr));

			// Valid IPv4
			if (result == 1)
			{
				server_protocol = AF_INET;
				strcpy(server_address, input);
				break;
			}

			result = inet_pton(AF_INET6, input, &(((struct sockaddr_in6 *)&sa)->sin6_addr));

			// Valid IPv6
			if (result == 1)
			{
				server_protocol = AF_INET6;
				strcpy(server_address, input);
				break;
			}

			printf("Invalid address.\n%s: ", message);
		}

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
		printf("Timestamp: %ld\n", timestamp);
	}
	else
	{
		// Output timestamp with microseconds
		//-----------------------------------------------------
		printf("Timestamp: %.6f\n", timestamp / 1000000.0);
	}

	if (client_enabled)
	{
		// Output client information
		//-----------------------------------------------------
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
		// Output load balancer information
		//-----------------------------------------------------
		printf("Load balancer:\n");
		printf("    protocol: %s\n", lb_protocol == AF_INET ? "IPv4" : "IPv6");
		printf("    address: %s\n", lb_address);

		if (lb_port)
		{
			printf("    port: %d\n", lb_port);
		}
	}

	if (server_enabled)
	{
		// Output server information
		//-----------------------------------------------------
		printf("Server:\n");
		printf("    protocol: %s\n", server_protocol == AF_INET ? "IPv4" : "IPv6");
		printf("    address: %s\n", server_address);

		if (server_port)
		{
			printf("    server_port: %d\n", server_port);
		}
	}

	if (user_id)
	{
		// Output user id
		//-----------------------------------------------------
		printf("User id: %d\n", user_id);
	}

	if (page_id)
	{
		// Output page id
		//-----------------------------------------------------
		printf("Page id: %d\n", page_id);
	}
}
