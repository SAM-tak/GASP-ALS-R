#pragma once

#include "Components/PawnComponent.h"
#include "GarCharacterComponent.generated.h"

class AGarCharacter;

UCLASS(Abstract)
class GAR_API UGarCharacterComponent : public UPawnComponent
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "GarCharacterComponent|State")
	TWeakObjectPtr<AGarCharacter> Character;

protected:
	virtual void OnRegister() override;

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|CharacterComponent")
	void OnPossessed(AController* NewController);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|CharacterComponent")
	void OnUnPossessed(AController* PreviousController);

	UFUNCTION(BlueprintNativeEvent, Category = "GAR|CharacterComponent")
	void OnOwnerTick(float DeltaTime);
};
