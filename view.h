#pragma once

#include "renderer.h"

// ============================================================================
// An object that presents the view for the application.
//
// Application (view)s are created with the view source factory instances. View
// presents an object that forms the user interface for the application.
// ============================================================================
ref class View : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();
protected:
	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
	void OnClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
private:
	bool		mWindowClosed;
	bool		mWindowVisible;
	Renderer^	mRenderer;
};
