#include "GarCameraModule.h"

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#endif

IMPLEMENT_MODULE(FGarCameraModule, GARCamera)

void FGarCameraModule::StartupModule()
{
	FDefaultModuleImpl::StartupModule();

#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.AddRaw(this, &FGarCameraModule::Console_OnRegisterAutoCompleteEntries);
#endif
}

void FGarCameraModule::ShutdownModule()
{
#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.RemoveAll(this);
#endif

	FDefaultModuleImpl::ShutdownModule();
}

#if ALLOW_CONSOLE
// ReSharper disable once CppMemberFunctionMayBeStatic
void FGarCameraModule::Console_OnRegisterAutoCompleteEntries(TArray<FAutoCompleteCommand>& AutoCompleteCommands)
{
	const auto CommandColor{GetDefault<UConsoleSettings>()->AutoCompleteCommandColor};

	auto* Command{&AutoCompleteCommands.AddDefaulted_GetRef()};
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.CameraCurves")};
	Command->Desc = FString{TEXTVIEW("Displays camera animation curves.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.CameraTraces")};
	Command->Desc = FString{TEXTVIEW("Displays camera traces.")};
	Command->Color = CommandColor;
}
#endif
