#pragma once

#include <OAIdl.h>

#include "FiccDerivatives_DLL.h"

FICCDERIVATIVES_ENTRY int __stdcall VBA_Call( BSTR data );
FICCDERIVATIVES_ENTRY int __stdcall VBA_CallByFile( BSTR file_name, bool useProxy, bool calcGreek, bool showDetail );