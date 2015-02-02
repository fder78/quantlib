
#include <OAIdl.h>

#include <boost/python.hpp>
#include <pyCall.h>

using namespace boost::python;

#if defined(QL_ENABLE_SESSIONS)
namespace QuantLib {
	Integer sessionId() { return 0; }
	void* mutex = NULL;
	void* createMutex()	{ return NULL; }
	void __Lock::AchieveLock() {}
	void __Lock::ReleaseLock() {}
}
#endif

BOOST_PYTHON_MODULE(PythonModule)
{
	class_<pyCall>("QL")
		.def(init<std::wstring>())
		.def("calc", &pyCall::calc)
		;
}