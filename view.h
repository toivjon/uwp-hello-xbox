#pragma once

namespace xbox
{
	ref class View sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();
	};
}
