#include "softfloat_version.h"

#define Q(str)    #str
#define QSTR(str) Q(str)

static const char* version_string = QSTR( VERS_STRING );

const char* softfloat_version() {return version_string; }
