#include "view.h"

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;

// ============================================================================
// Initializes the application view.
//
// This function is called when the application is launched by the runtime. We
// may perform all kinds of initialization logics here before actual execution.
// ============================================================================
void View::Initialize(CoreApplicationView^ applicationView)
{
	// observe the activation of the main view of the application.
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &View::OnActivated);

	// create a renderer for the application.
	mRenderer = ref new Renderer();
}

// ============================================================================
// Assigns the window for the application.
//
// This function is called after the Initialize function has been called. Here
// we have opportunity to set up window and display event handling operations.
// ============================================================================
void View::SetWindow(CoreWindow^ window)
{
	// observe the events of the main window of the application.
	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &View::OnVisibilityChanged);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &View::OnClosed);
}

// ============================================================================
// Loads view related resources.
//
// This function is called by the runtime when the application should load all
// initial scene resources or to restore a previously saved application state.
// ============================================================================
void View::Load(String^ entryPoint)
{
	// ... nothing
}

// ============================================================================
// Run the application view.
//
// This function is called when the application is set to running state. This
// function typically contains the main loop for the game application.
// ============================================================================
void View::Run()
{
	while (!mWindowClosed) {
		auto window = CoreWindow::GetForCurrentThread();
		if (mWindowVisible) {
			window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			mRenderer->Render();
		} else {
			window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

// ============================================================================
// Release and dispose reserved resources.
//
// Note that this function is not always called when application is exited. It 
// seems that this function is not commonly used but just kept as a stump.
// ============================================================================
void View::Uninitialize()
{
	// ... nothing
}

// ============================================================================
// Handle the application activation event.
//
// This function is called when the application is activated. Here we can show
// our window so it appears on the screen and we can start our renderings.
// ============================================================================
void View::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow^ window = CoreWindow::GetForCurrentThread();
	window->Activate();
	mRenderer->SetWindow(window);
}

// ============================================================================
// Listener for events when window visibility is changed.
//
// Runtime calls this function when the target window visibility is changed. So
// here we can store the value such that we can suspend our rendering and such.
// ============================================================================
void View::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	mWindowVisible = args->Visible;
}

// ============================================================================
// Listener for events when window is being closed.
//
// Runtime calls this function when the t arget window is being closed. Here we
// can store a flag about the event so we can e.g. also close our application.
// ============================================================================
void View::OnClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	mWindowClosed = true;
}