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

int main()
{
	// 0 = s, 1 = µs
	_Bool time_type = 0;

	// Set time precision
	char input[45];
	printf("Enter time precision (s/us): ");
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
		printf("Enter time precision (s/us): ");
	}

	printf("Displaying timestamp %s\n", time_type == 0 ? "as seconds" : "including µs");
}
