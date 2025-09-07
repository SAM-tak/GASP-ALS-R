#include "GarModule.h"

#if WITH_EDITOR
#include "MessageLogModule.h"
#endif

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#endif

#include "Utility/GarLog.h"

IMPLEMENT_MODULE(FGarModule, GAR)

#define LOCTEXT_NAMESPACE "GarModule"

void FGarModule::StartupModule()
{
	FDefaultModuleImpl::StartupModule();

#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.AddRaw(this, &FGarModule::Console_OnRegisterAutoCompleteEntries);
#endif

#if WITH_EDITOR
	auto& MessageLog{FModuleManager::LoadModuleChecked<FMessageLogModule>(FName{TEXTVIEW("MessageLog")})};

	FMessageLogInitializationOptions Options;
	Options.bShowFilters = true;
	Options.bAllowClear = true;
	Options.bDiscardDuplicates = true;

	MessageLog.RegisterLogListing(GarLog::MessageLogName, LOCTEXT("MessageLogLabel", "GAR"), Options);
#endif
}

void FGarModule::ShutdownModule()
{
#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.RemoveAll(this);
#endif

	FDefaultModuleImpl::ShutdownModule();
}

#if ALLOW_CONSOLE
// ReSharper disable once CppMemberFunctionMayBeStatic
void FGarModule::Console_OnRegisterAutoCompleteEntries(TArray<FAutoCompleteCommand>& AutoCompleteCommands)
{
	const auto CommandColor{GetDefault<UConsoleSettings>()->AutoCompleteCommandColor};

	auto* Command{&AutoCompleteCommands.AddDefaulted_GetRef()};
	Command->Command = FString{TEXTVIEW("Stat Gar")};
	Command->Desc = FString{TEXTVIEW("Displays GAR performance statistics.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.Curves")};
	Command->Desc = FString{TEXTVIEW("Displays animation curves.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.State")};
	Command->Desc = FString{TEXTVIEW("Displays character state.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.Shapes")};
	Command->Desc = FString{TEXTVIEW("Displays debug shapes.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.Traces")};
	Command->Desc = FString{TEXTVIEW("Displays animation traces.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.Mantling")};
	Command->Desc = FString{TEXTVIEW("Displays mantling traces.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug Gar.PhysicalAnimation")};
	Command->Desc = FString{TEXTVIEW("Displays Physical Animation Info.")};
	Command->Color = CommandColor;
}
#endif

#undef LOCTEXT_NAMESPACE
