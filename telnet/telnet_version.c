#include "telnet_version.h"

#define Q(str)    #str
#define QSTR(str) Q(str)

static const char* version_string = QSTR( VERS_STRING );

const char* telnet_version() {return version_string; }
