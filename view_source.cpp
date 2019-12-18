#include "view_source.h"
#include "view.h"

using namespace Windows::ApplicationModel::Core;

// ============================================================================
// A factory function to create a view.
//
// CoreApplication will use this function to create a view for the application.
// Note that we need to create a managed class from our view implementation.
// ============================================================================
IFrameworkView^ ViewSource::CreateView()
{
	return ref new View();
}