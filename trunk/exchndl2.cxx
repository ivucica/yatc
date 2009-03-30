/* exchndl2.cxx
 *
 * A portable way to load the EXCHNDL.DLL on startup.
 *
 * Jose Fonseca
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class ExceptionHandler
{
	public:

	ExceptionHandler()
	{
		LoadLibrary("exchndl.dll");
	}
};

static ExceptionHandler gExceptionHandler;	//  global instance of class
