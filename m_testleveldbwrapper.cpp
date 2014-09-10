#include "leveldbwrapper.h"

int main()
{
	boost::filesystem::path path("/tmp/test");
	CLevelDBWrapper(path, 100);
}
