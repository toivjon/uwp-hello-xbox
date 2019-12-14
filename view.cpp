#include "view.h"

using namespace xbox;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;

View::View() : mWindowClosed(false), mWindowVisible(true)
{
	// ...
}

void View::Initialize(CoreApplicationView^ applicationView)
{
	// observe the activation of the main view of the application.
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &View::OnActivated);
	
	// observe the suspension and resume of the application.
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &View::OnSuspending);
	CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &View::OnResuming);
}

void View::SetWindow(CoreWindow^ window)
{
	// observe the events of the main window of the application.
	window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &View::OnSizeChanged);
	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &View::OnVisibilityChanged);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &View::OnClosed);

	// observe changes in the current display.
	DisplayInformation^ displayInformation = DisplayInformation::GetForCurrentView();
	displayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &View::OnDpiChanged);
	displayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &View::OnOrientationChanged);
	displayInformation->DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &View::OnDisplayContentsInvalidated);
}

void View::Load(String^ entryPoint)
{
	// ... scene resource or state restoration logics here
}

void View::Run()
{
	// we arrive here to run the main loop after our core window has been activated.
	while (mWindowClosed) {
		if (mWindowVisible) {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			// TODO update
			// TODO render
		} else {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void View::Uninitialize()
{
	// ... torn while being foreground events here
}

// ============================================================================

void View::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// here we activate the core window so it becomes visible when our application is activated.
	CoreWindow::GetForCurrentThread()->Activate();
}

void View::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{

}

void View::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{

}

void View::OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{

}

void View::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	mWindowVisible = args->Visible;
}

void View::OnClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	mWindowClosed = true;
}

void View::OnDpiChanged(DisplayInformation^ sender, Platform::Object^ args)
{

}

void View::OnOrientationChanged(DisplayInformation^ sender, Platform::Object^ args)
{

}

void View::OnDisplayContentsInvalidated(DisplayInformation^ sender, Platform::Object^ args)
{

}