#include "view.h"

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;

void View::Initialize(CoreApplicationView^ applicationView)
{
	// observe the activation of the main view of the application.
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &View::OnActivated);

	// initialize our D3D12 renderer instance.
	mRenderer = ref new Renderer();
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
	// ... nothing to do
}

void View::Run()
{
	// we arrive here to run the main loop after our core window has been activated.
	while (!mWindowClosed) {
		if (mWindowVisible) {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			mRenderer->Render();
		} else {
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void View::Uninitialize()
{
	// ... nothing to do
}

void View::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// here we activate the core window so it becomes visible when our application is activated.
	CoreWindow::GetForCurrentThread()->Activate();
	mRenderer->SetWindow(CoreWindow::GetForCurrentThread());
}

void View::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	mWindowVisible = args->Visible;
}

void View::OnClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	mWindowClosed = true;
}

void View::OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	mRenderer->SetWindow(sender);
	// notify rendering devices and application about the window size change.
	// GetDeviceResources()->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	// mApplication->OnWindowSizeChanged();
}

void View::OnDpiChanged(DisplayInformation^ sender, Platform::Object^ args)
{
	// notify rendering devices and application about the DPI change.
	// GetDeviceResources()->SetDpi(sender->LogicalDpi);
	// mApplication->OnWindowSizeChanged();
}

void View::OnOrientationChanged(DisplayInformation^ sender, Platform::Object^ args)
{
	// notify rendering devices and application about the orientation change.
	// GetDeviceResources()->SetCurrentOrientation(sender->CurrentOrientation);
	// mApplication->OnWindowSizeChanged();
}

void View::OnDisplayContentsInvalidated(DisplayInformation^ sender, Platform::Object^ args)
{
	// ensure that rendering devices will (re)validate themselves.
	// GetDeviceResources()->ValidateDevice();
}