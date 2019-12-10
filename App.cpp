#include "pch.h"

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	std::cout << "hello xbox!" << std::endl;
	return 0;
}