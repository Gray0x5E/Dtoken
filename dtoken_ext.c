#include <php.h>
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

PHP_FUNCTION(dtoken_build)
{
	RETURN_STRING("Token goes there");
}
