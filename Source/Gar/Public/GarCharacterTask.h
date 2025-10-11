#pragma once

#include "CoreMinimal.h"
#include "GarCharacterTask.generated.h"

class UWorld;
class AGarCharacter;

UCLASS(Abstract, Blueprintable, BlueprintType, AutoExpandCategories = ("Settings"))
class GAR_API UGarCharacterTask : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	uint8 bEnableInputBinding : 1{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<AGarCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bActive : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bInputBinded : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	uint8 bEpilogRunningCurrently : 1{false};

public:
	virtual bool IsActive() const { return bActive; }

	virtual void OnRegister();

	virtual void Begin();

	virtual void Tick(float DeltaTime);

	virtual void OnPossessed(AController* NewController);

	virtual void OnUnPossessed(AController* PreviousController);

	virtual void End();

	virtual void Cancel();

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "GAR|CharacterTask")
	bool IsEpilogRunning() const;

	bool HasFinished() const
	{
		return !bActive && !bEpilogRunningCurrently;
	}

	virtual UWorld* GetWorld() const override final;

protected:
	virtual void OnEnd(bool bWasCancelled);

	virtual void OnFinished();

	void BindInput(UInputComponent* InputComponent);

	void UnbindInput(UInputComponent* InputComponent);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|CharacterTask", DisplayName = "OnBegin", Meta = (ScriptName = "OnBegin"))
	void K2_OnBegin();

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|CharacterTask", DisplayName = "OnTick", Meta = (ScriptName = "OnTick"))
	void K2_OnTick(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|CharacterTask", DisplayName = "OnEnd", Meta = (ScriptName = "OnEnd"))
	void K2_OnEnd(bool bWasCancelled);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAR|CharacterTask", DisplayName = "OnFinished", Meta = (ScriptName = "OnFinished"))
	void K2_OnFinished();
};
