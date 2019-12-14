#include "view_source.h"

using namespace Windows::ApplicationModel::Core;
using namespace xbox;

// ============================================================================
// The entry point of the application.
//
// Note that this function is annotated with [Platform::MTAThread] attribute.
// This puts application to use multithreaded apartment model for COM interop.
// ============================================================================
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto viewSource = ref new ViewSource();
	CoreApplication::Run(viewSource);
	return 0;
}