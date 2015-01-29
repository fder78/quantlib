#pragma once

#ifdef FICCDERIVATIVES_DLL_EXPORT
// DLL library project uses this
#define FICCDERIVATIVES_ENTRY __declspec(dllexport)
#else
#ifdef FICCDERIVATIVES_DLL_IMPORT
// client of DLL uses this
#define FICCDERIVATIVES_ENTRY __declspec(dllimport)  
#else
// static library project uses this
#define FICCDERIVATIVES_ENTRY
#endif
#endif

