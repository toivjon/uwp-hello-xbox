#pragma once

// ============================================================================
// A factory which creates views for the application.
//
// CoreApplication uses view factory to build a view for the application. Note
// that our implementation is defined as a managed class with the ref keyword.
// ============================================================================
ref class ViewSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

