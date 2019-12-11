#include "view_source.h"
#include "view.h"

using namespace xbox;
using namespace Windows::ApplicationModel::Core;

IFrameworkView^ ViewSource::CreateView()
{
	return ref new View();
}