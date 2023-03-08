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
 * Dtoken is implemented as a PHP extension, making it easy to integrate into
 * web applications. By generating a unique token for each request, Dtoken can
 * help developers track down issues and debug problems with their web
 * applications.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <math.h>
#include <gmp.h>
#include "dtoken.h"

/**
 * Add input port to the given token
 *
 * @param mpz_t* token The token to add the port to.
 * @param short int port The port to add.
 *
 * @return void
 */
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

/**
 * Add input address to the given token
 *
 * @param mpz_t* token The token to add the address to.
 * @param short int enabled Whether the address is enabled or not.
 * @param short int protocol The protocol used by the address (AF_INET or AF_INET6).
 * @param union { struct in_addr v4; struct in6_addr v6; }* ip The IP address to add, represented as a struct in_addr or struct in6_addr depending on the protocol.
 *
 * @return void
 */
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

/**
 * Add token data to the given token
 *
 * @param mpz_t* token The token to add the data to.
 * @param struct token_data* data The token data to add.
 *
 * @return void
 */
void add_token_data(mpz_ptr token, struct token_data *data)
{
	// Add second generic id
	if (!data->id2)
	{
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 0);
	}
	else
	{
		mpz_mul_2exp(token, token, ID2_SIZE);
		mpz_add_ui(token, token, data->id2);
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 1);
	}

	// Add first generic id
	if (!data->id1)
	{
		mpz_mul_2exp(token, token, 1);
		mpz_add_ui(token, token, 0);
	}
	else
	{
		mpz_mul_2exp(token, token, ID1_SIZE);
		mpz_add_ui(token, token, data->id1);
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

/**
 * Builds a token using the given data and returns it as a base 36 encoded string
 *
 * @param char* buffer The buffer to use for storing the token string.
 * @param int method The method used to generate the token.
 * @param _Bool time_type The precision of the timestamp (0 for seconds, 1 for microseconds).
 * @param long int timestamp The timestamp to add to the token.
 * @param _Bool client_enabled Whether client information should be included in the token.
 * @param short int client_protocol The protocol used by the client address (AF_INET or AF_INET6).
 * @param char* client_address The client IP address to include in the token.
 * @param short int client_port The client port to include in the token.
 * @param _Bool lb_enabled Whether load balancer information should be included in the token.
 * @param short int lb_protocol The protocol used by the load balancer address (AF_INET or AF_INET6).
 * @param char * lb_address The load balancer IP address to include in the token.
 * @param short int lb_port The load balancer port to include in the token.
 * @param _Bool server_enabled Whether server information should be included in the token.
 * @param short int server_protocol The protocol used by the server address (AF_INET or AF_INET6).
 * @param char* server_address The server IP address to include in the token.
 * @param short int server_port The server port to include in the token.
 * @param int id1 Some generic associated id to store in the token.
 * @param int id2 Another generic associated id to store in the token.
 *
 * @return char* The built token as a string.
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
	int id2)
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

	data.id1 = id1;
	data.id2 = id2;

	// client address
	inet_pton(client_protocol, client_address, (client_protocol == AF_INET) ? (void *)&(data.client_ip.v4) : (void *)&(data.client_ip.v6));

	// lb address
	inet_pton(lb_protocol, lb_address, (lb_protocol == AF_INET) ? (void *)&(data.lb_ip.v4) : (void *)&(data.lb_ip.v6));

	// server address
	inet_pton(server_protocol, server_address, (server_protocol == AF_INET) ? (void *)&(data.server_ip.v4) : (void *)&(data.server_ip.v6));

	data.id1 = id1;
	data.id2 = id2;

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

/*
 * Command line tool for generating tokens using the dtoken extension
 *
 * @return int Returns 0 on success, or 1 on failure.
 */
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

	// Generic id 1
	int id1 = 0;

	// Generic id 2
	int id2 = 0;

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
		else if (strcmp(input, "GET") == 0)     { method = GET;     break; }
		else if (strcmp(input, "POST") == 0)    { method = POST;    break; }
		else if (strcmp(input, "PUT") == 0)     { method = PUT;     break; }
		else if (strcmp(input, "DELETE") == 0)  { method = DELETE;  break; }
		else if (strcmp(input, "HEAD") == 0)    { method = HEAD;    break; }
		else if (strcmp(input, "CONNECT") == 0) { method = CONNECT; break; }
		else if (strcmp(input, "OPTIONS") == 0) { method = OPTIONS; break; }
		else if (strcmp(input, "TRACE") == 0)   { method = TRACE;   break; }
		else if (strcmp(input, "PATCH") == 0)   { method = PATCH;   break; }
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

	// Set generic id 1
	message = "Enter generic id 1 (leave empty for none)";
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
			id1 = entered_id;
			break;
		}

		printf("Invalid option.\n%s: ", message);
	}

	// Set generic id 2
	message = "Enter generic id 2 (leave empty for none)";
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
			id2 = entered_id;
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

	if (id1)
	{
		// Output generic id 1
		//-----------------------------------------------------
		printf("Generic id 1:       %d\n", id1);
	}

	if (id2)
	{
		// Output generic id 2
		//-----------------------------------------------------
		printf("Generic id 2:       %d\n", id2);
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
		ID1_SIZE +
		ID2_SIZE +
		((2 + PORT_SIZE) * 3) + // x number of addresses + extra bits for meta
		((2 + IPv6_SIZE) * 3) + // x number of addresses + extra bits for meta
		100; // Adding some extra bits for good measure

	char token_buffer[(int)ceil(buffer_size / 8)];

	char* token = build(token_buffer, method, time_type, timestamp, client_enabled, client_protocol, client_address, client_port, lb_enabled, lb_protocol, lb_address, lb_port, server_enabled, server_protocol, server_address, server_port, id1, id2);

	printf("\nToken: %s\n", token);
}
