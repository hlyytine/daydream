#include <ddcommon.h>

void unsetenv(const char *name)
{
	setenv(name, "", 1);
}
