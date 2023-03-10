# Dtoken PHP Extension

Dtoken is a PHP extension written in C that generates a unique request token containing various information about the request, including time and basic client/server details.

## Requirements

- PHP development files
- PHP 8.1 or later (to use the extension)
- Build tools
- C compiler

## Installation

1. Clone the repository: `git clone git@github.com:Gray0x5E/Dtoken.git`
2. Enter the repository directory: `cd Dtoken`
3. Compile the extension: `phpize && ./configure && make`
4. Install the extension: `sudo make install`
5. Add `extension=dtoken.so` to your PHP configuration file (e.g. php.ini)

## Bit field diagram

The data is packed as such:

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Patch |      Minor      | Major |T|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          Timestamp                          ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Mtd  |C|c|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                        Client Address                       ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |L|l|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                     Load Balancer Address                   ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |W|w|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      Web Server Address                     ...
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |I|           ID1             |i|            ID2                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

* **Patch**: 4 bits to store the patch version of Dtoken.
* **Minor**: 8 bits to store the minor version of Dtoken.
* **Major**: 4 bits to store the major version of Dtoken.
* **T**: 1 bit to store the type of timestamp (*s: 0, µ: 1*).
* **Timestamp**: 32 bits or 52 bits, depending on the value of *T*.
* **Mtd**: 4 bits to store HTTP method (*GET: 1, POST: 2, PUT: 3, DELETE: 4, HEAD: 5, CONNECT: 6, OPTIONS: 7, TRACE: 8, PATCH: 9*).
* **C**: 1 bit to indicate if client address is included.
* **C**: 1 bit to indicate if client address is included. To include set to 1. If value is 0 the next two units (*c* and *Client Address*) are not included.
* **c**: 1 bit to indicate type of client address (*IPv4: 0, IPv6: 1*).
* **Client Address**: 32 bits or 128 bits to store client IP address, dependant on value of *c*.
* **L**: 1 bit to indicate if load balancer address is included. To include set to 1. If value is 0 the next two units (*l* and *Load Balancer*) are not included.
* **l**: 1 bit to indicate type of load balancer address (*IPv4: 0, IPv6: 1*).
* **Load Balancer Address**: 32 bits or 128 bits to store load balancer IP address, dependant on value of *l*.
* **W**: 1 bit to indicate if web server address is included.
* **w**: 1 bit to indicate type of web server address (*IPv4: 0, IPv6: 1*).
* **Web Server Address**: 32 bits or 128 bits to store web server IP address, dependant on value of *w*.
* **I**: 1 bit to indicate if ID1 is included.
* **ID1**: 23 bits to store value of ID1 if *I* is 1.
* **i**: 1 bit to indicate if ID1 is included.
* **ID2**: 23 bits to store value of ID1 if *i* is 1.

## Extension usage

### Description

All parameters are optional. If not set, the value will be automatically set, if possible.

```php
dtoken_build(
	int $method = 0,
	int $precision = 0,
	float|int $timestamp = null,
	string $address = null,
	string $balancer = null,
	string $server = null,
	int $id1 = null,
	int $id2 = null
): string
```

### Parameters

**method**: Integer representing an HTTP method. Values are: `GET: 1, POST: 2, PUT: 3, DELETE: 4, HEAD: 5, CONNECT: 6, OPTIONS: 7, TRACE: 8, PATCH: 9`

**precision**: Precision of timestamp. Possible values are `0` and `1`, with 0 being second precision, and 1 being microsecond precision.

**timestamp**: Timestamp of the request, in seconds (if precision set to 0) or microseconds (if precision set to 1).

**address**: IP address, and optional port, of the client that made the request. Format: `IP:[PORT]`. IP address can be IPv4 or IPv6.

**balancer**: IP address, and optional port, of the load balancer that handled the request. Format: `IP:[PORT]`. IP address can be IPv4 or IPv6.

**server**: IP address, and optional port, of the web server that handled the request. Format: `IP:[PORT]`. IP address can be IPv4 or IPv6.

**id1**: Optional integer value that can represent any useful value up to 8.388.607.

**id2**: Optional integer value that can represent any useful value up to 32.767.

### Example

```php
<?php
$token = dtoken_build();
var_export($token);
```
The above will output:
```
'2rl87iiq92vmb500'
```
