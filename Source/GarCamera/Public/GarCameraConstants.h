#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GarCameraConstants.generated.h"

UCLASS(Meta = (BlueprintThreadSafe))
class GARCAMERA_API UGarCameraConstants : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Debug

	UFUNCTION(BlueprintPure, Category = "GAR|Camera Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& CameraCurvesDebugDisplayName();

	UFUNCTION(BlueprintPure, Category = "GAR|Camera Constants|Debug", Meta = (ReturnDisplayName = "Display Name"))
	static const FName& CameraTracesDebugDisplayName();
};

inline const FName& UGarCameraConstants::CameraCurvesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.CameraCurves")};
	return Name;
}

inline const FName& UGarCameraConstants::CameraTracesDebugDisplayName()
{
	static const FName Name{TEXTVIEW("GAR.CameraTraces")};
	return Name;
}
