#include <arpa/inet.h>
#include <php.h>
#include "main/SAPI.h"
#include "ext/standard/info.h"
#include "dtoken.h"

PHP_FUNCTION(dtoken_build);

zend_function_entry dtoken_functions[] =
{
	PHP_FE(dtoken_build, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry dtoken_module_entry =
{
	STANDARD_MODULE_HEADER,
	"dtoken",
	dtoken_functions,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	VERSION,
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(dtoken)

int is_ipv4_address(char* str)
{
	struct in_addr addr;
	int result = inet_pton(AF_INET, str, &addr);
	return result == 1;
}

int is_ipv6_address(char* str)
{
	struct in6_addr addr;
	int result = inet_pton(AF_INET6, str, &addr);
	return result == 1;
}

int is_valid_ip_address(const char *ip_str)
{

	if (ip_str == NULL)
	{
		return 0;
	}

	struct in_addr ipv4;
	struct in6_addr ipv6;

	if (inet_pton(AF_INET, ip_str, &ipv4) == 1)
	{
		// Valid IPv4 address
		return 1;
	}
	else if (inet_pton(AF_INET6, ip_str, &ipv6) == 1)
	{
		// Valid IPv6 address
		return 1;
	}
	else
	{
		// Invalid address
		return 0;
	}
}

char* get_token(
	int _method,
	short int _precision,
	long int _timestamp,
	char* _address,
	char* _balancer,
	char* _server,
	int _id1,
	int _id2
)
{
	// Request timestamp
	_Bool time_type = (_Bool)(_precision != 0 ? 1 : 0); // default to second precision
	long int timestamp;
	if (_timestamp)
	{
		timestamp = _timestamp;
	}
	else
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if (time_type == 1)
		{
			timestamp = (tv.tv_sec + (tv.tv_usec / 1000000.0)) * 1000000;
		}
		else
		{
			timestamp = tv.tv_sec;
		}
	}

	// HTTP method
	int method = 0;
	if (_method)
	{
		method = _method;
	}
	else
	{
		const char* request_method = SG(request_info).request_method;

			 if (strcmp(request_method, "GET") == 0)	 { method = GET;	 }
		else if (strcmp(request_method, "POST") == 0)	{ method = POST;	}
		else if (strcmp(request_method, "PUT") == 0)	 { method = PUT;	 }
		else if (strcmp(request_method, "DELETE") == 0)  { method = DELETE;  }
		else if (strcmp(request_method, "HEAD") == 0)	{ method = HEAD;	}
		else if (strcmp(request_method, "CONNECT") == 0) { method = CONNECT; }
		else if (strcmp(request_method, "OPTIONS") == 0) { method = OPTIONS; }
		else if (strcmp(request_method, "TRACE") == 0)   { method = TRACE;   }
		else if (strcmp(request_method, "PATCH") == 0)   { method = PATCH;   }
	}

	short int client_enabled = 0;
	short int client_protocol = AF_INET;
	char* client_address = "";
	short int client_port = 0;

	short int lb_enabled = 0;
	short int lb_protocol = AF_INET;
	char* lb_address = "";
	short int lb_port = 0;

	short int server_enabled = 0;
	short int server_protocol = AF_INET;
	char* server_address = "";
	short int server_port = 0;

	zval *server_vars = &PG(http_globals)[TRACK_VARS_SERVER];

	if (_address)
	{
		if (is_ipv4_address(_address))
		{
			client_enabled = 1;
			client_protocol = AF_INET;
			client_address = _address;
		}
		else if (is_ipv6_address(_address))
		{
			client_enabled = 1;
			client_protocol = AF_INET6;
			client_address = _address;
		}
		else
		{
			client_enabled = 0;
			client_protocol = AF_INET;
			client_address = "";
		}
	}
	else
	{
		zval *client_addr;
		if (server_vars && (client_addr = zend_hash_str_find(Z_ARRVAL_P(server_vars), ZEND_STRL("REMOTE_ADDR"))) != NULL)
		{
			client_enabled = 1;
			client_protocol = is_ipv4_address(Z_STRVAL_P(client_addr)) == 1 ? AF_INET : AF_INET6;
			client_address = Z_STRVAL_P(client_addr);
		}
	}

	if (_balancer)
	{
		if (is_ipv4_address(_balancer))
		{
			lb_enabled = 1;
			lb_protocol = AF_INET;
			lb_address = _balancer;
		}
		else if (is_ipv6_address(_balancer))
		{
			lb_enabled = 1;
			lb_protocol = AF_INET6;
			lb_address = _balancer;
		}
		else
		{
			lb_enabled = 0;
			lb_protocol = AF_INET;
			lb_address = "";
		}
	}
	else
	{
		zval *lb_addr;
		if (server_vars && (lb_addr = zend_hash_str_find(Z_ARRVAL_P(server_vars), ZEND_STRL("HTTP_X_TS_LB"))) != NULL)
		{
			lb_enabled = 1;
			lb_protocol = is_ipv4_address(Z_STRVAL_P(lb_addr)) == 1 ? AF_INET : AF_INET6;
			lb_address = Z_STRVAL_P(lb_addr);
		}
	}

	if (_server)
	{
		if (is_ipv4_address(_server))
		{
			server_enabled = 1;
			server_protocol = AF_INET;
			server_address = _server;
		}
		else if (is_ipv6_address(_server))
		{
			server_enabled = 1;
			server_protocol = AF_INET6;
			server_address = _server;
		}
		else
		{
			server_enabled = 0;
			server_protocol = AF_INET;
			server_address = "";
		}
	}
	else
	{
		zval *server_addr;
		if (server_vars && (server_addr = zend_hash_str_find(Z_ARRVAL_P(server_vars), ZEND_STRL("SERVER_ADDR"))) != NULL)
		{
			server_enabled = 1;
			server_protocol = is_ipv4_address(Z_STRVAL_P(server_addr)) == 1 ? AF_INET : AF_INET6;
			server_address = Z_STRVAL_P(server_addr);
		}
	}

	int id1 = (_id1 != 0 ? _id1 : 0);
	int id2 = (_id2 != 0 ? _id2 : 0);

	php_printf(
		"time_type: %d\n"
		"timestamp: %ld\n"
		"method: %d\n"
		"client_enabled: %d\n"
		"client_protocol: %s\n"
		"client_address: %s\n"
		"client_port: %s\n"
		"lb_enabled: %d\n"
		"lb_protocol: %s\n"
		"lb_address: %s\n"
		"lb_port: %s\n"
		"server_enabled: %d\n"
		"server_protocol: %s\n"
		"server_address: %s\n"
		"server_port: %s\n"
		"id1: %d\n"
		"id2: %d\n",
		(time_type ? 1 : 0),
		timestamp,
		method,
		(client_enabled ? 1 : 0),
		(client_protocol == AF_INET ? "IPv4" : "IPv6"),
		client_address,
		client_port,
		(lb_enabled ? 1 : 0),
		(lb_protocol == AF_INET ? "IPv4" : "IPv6"),
		lb_address,
		lb_port,
		(server_enabled ? 1 : 0),
		(server_protocol == AF_INET ? "IPv4" : "IPv6"),
		server_address,
		server_port,
		id1,
		id2
	);

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

	char token_buffer[buffer_size];

	char* token = build(
		token_buffer,
		method,
		time_type,
		timestamp,
		client_enabled,
		client_protocol,
		client_address,
		client_port,
		lb_enabled,
		lb_protocol,
		lb_address,
		lb_port,
		server_enabled,
		server_protocol,
		server_address,
		server_port,
		id1,
		id2
	);

	return token;

}


PHP_FUNCTION(dtoken_build)
{
	zend_long method = 0;
	zend_long precision = 0;
	zend_long timestamp = 0;
	char* address = NULL;
	char* balancer = NULL;
	char* server = NULL;
	zend_long id1 = 0;
	zend_long id2 = 0;

	zend_bool method_null;
	zend_bool precision_null;
	zend_bool timestamp_null;
	zend_bool address_null;
	zend_bool balancer_null;
	zend_bool server_null;
	zend_bool id1_null;
	zend_bool id2_null;

	size_t address_lennn;
	size_t balancer_lennn;
	size_t server_lennn;

	ZEND_PARSE_PARAMETERS_START(0, 8)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(method, method_null)
		Z_PARAM_LONG_OR_NULL(precision, precision_null)
		Z_PARAM_LONG_OR_NULL(timestamp, timestamp_null)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING_EX(address, address_lennn, 1, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING_EX(balancer, balancer_lennn, 1, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING_EX(server, server_lennn, 1, 1)
		Z_PARAM_LONG_OR_NULL(id1, id1_null)
		Z_PARAM_LONG_OR_NULL(id2, id2_null)
	ZEND_PARSE_PARAMETERS_END();

	if (method < 0 || method > 9)
	{
		method = 0;
		php_error(E_WARNING, "$method has to be an integer from 1 to 9");
	}

	if (precision != 0 && precision != 1)
	{
		precision = 0;
		php_error(E_WARNING, "$precision has to be 0 or 1");
	}

	if (address != NULL && !is_valid_ip_address(address))
	{
		address = NULL;
		address_lennn = 0;
		php_error(E_WARNING, "$address is not a valid IPv4 or IPv6 address");
	}

	if (balancer != NULL && !is_valid_ip_address(balancer))
	{
		balancer = NULL;
		balancer_lennn = 0;
		php_error(E_WARNING, "$balancer is not a valid IPv4 or IPv6 address");
	}

	if (server != NULL && !is_valid_ip_address(server))
	{
		server = NULL;
		server_lennn = 0;
		php_error(E_WARNING, "$server is not a valid IPv4 or IPv6 address");
	}

	if (id1 < 0 || id1 > (2 << ID1_SIZE) - 1)
	{
		id1 = 0;
		php_error(E_WARNING, "$id1 has to be an integer between 0 and %d", (2 << ID1_SIZE) - 1);
	}

	if (id2 < 0 || id2 > (2 << ID2_SIZE) - 1)
	{
		id2 = 0;
		php_error(E_WARNING, "$id1 has to be an integer between 0 and %d", (2 << ID2_SIZE) - 1);
	}

	RETURN_STRING(get_token(method, precision, timestamp, address, balancer, server, id1, id2));
}
