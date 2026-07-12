#include "GarModule.h"

#if WITH_EDITOR
#include "MessageLogModule.h"
#endif

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#endif

#include "HAL/IConsoleManager.h"
#include "Utility/GarLog.h"

IMPLEMENT_MODULE(FGarModule, GAR)

#define LOCTEXT_NAMESPACE "GarModule"

void FGarModule::StartupModule()
{
	FDefaultModuleImpl::StartupModule();

	// p.EnableDynamicPerBodyFilterHacks is ECVF_ReadOnly but must be 1 for
	// bHACK_DisableCollisionResponse to take effect in BuildBodyFilterData().
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("p.EnableDynamicPerBodyFilterHacks")))
	{
		CVar->SetFlags((EConsoleVariableFlags)(CVar->GetFlags() & ~ECVF_ReadOnly));
		CVar->Set(1, ECVF_SetByCode);
		CVar->SetFlags((EConsoleVariableFlags)(CVar->GetFlags() | ECVF_ReadOnly));
	}

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
	Command->Command = FString{TEXTVIEW("ShowDebug AbilitySystem")};
	Command->Desc = FString{TEXTVIEW("Displays Gameplay Ability System Info.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("Stat GAR")};
	Command->Desc = FString{TEXTVIEW("Displays GAR performance statistics.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug GAR.Curves")};
	Command->Desc = FString{TEXTVIEW("Displays animation curves.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug GAR.Shapes")};
	Command->Desc = FString{TEXTVIEW("Displays debug shapes.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug GAR.Traces")};
	Command->Desc = FString{TEXTVIEW("Displays animation traces.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug GAR.Traversal")};
	Command->Desc = FString{TEXTVIEW("Displays traversal traces.")};
	Command->Color = CommandColor;

	Command = &AutoCompleteCommands.AddDefaulted_GetRef();
	Command->Command = FString{TEXTVIEW("ShowDebug GAR.PhysicalAnimation")};
	Command->Desc = FString{TEXTVIEW("Displays Physical Animation Info.")};
	Command->Color = CommandColor;
}
#endif

#undef LOCTEXT_NAMESPACE
