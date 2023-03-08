PHP_ARG_ENABLE(dtoken, Whether to enable the Dtoken extension, [ --enable-dtoken Enable Dtoken])

if test "$DTOKEN" != "no"; then
	PHP_NEW_EXTENSION(dtoken, dtoken_ext.c dtoken.c, $ext_shared,, -O3)
fi
