// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "InputActionValue.h"
#include "ShadowSlavePlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UShadowSlaveInventoryComponent;
class UShadowSlaveMemoryComponent;
class UShadowSlaveProgressionComponent;
class UShadowSlaveAspectComponent;
class UShadowSlaveEchoComponent;
class UShadowSlaveInteractionComponent;

/**
 * Player-controlled character class for Shadow Slave (Sunless foundation).
 * Manages collision-aware lagged third-person camera setup, camera-relative movement,
 * and Enhanced Input routing to locomotion and modular combat systems.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlavePlayerCharacter : public AShadowSlaveCharacterBase
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Modular inventory component managing player slots, stacking, and capacity */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveInventoryComponent> InventoryComponent;

	/** Modular memory component managing player Memories, manifestation, and equipment */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Memories", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveMemoryComponent> MemoryComponent;

	/** Modular progression component managing character rank, soul cores, and advancement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Progression", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveProgressionComponent> ProgressionComponent;

	/** Modular aspect component managing aspect identity, abilities, and flaw */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Aspects", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveAspectComponent> AspectComponent;

	/** Modular echo component managing player Echoes, summoning, and lifecycle */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Echoes", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveEchoComponent> EchoComponent;

	/** Modular interaction component managing detection, prompts, and world interactions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveInteractionComponent> InteractionComponent;

	/* --- Camera Tunables --- */

	/** Default camera boom length in cm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	float DefaultTargetArmLength = 380.0f;

	/** Camera offset relative to spring arm socket (over-the-shoulder framing) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	FVector CameraSocketOffset = FVector(0.0f, 40.0f, 20.0f);

	/** Target offset for focusing character upper body */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	FVector CameraTargetOffset = FVector(0.0f, 0.0f, 40.0f);

	/** Enable smooth positional camera lag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	bool bEnableCameraLag = true;

	/** Speed of camera position lag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableCameraLag"))
	float CameraLagSpeed = 10.0f;

	/** Enable smooth rotational camera lag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true"))
	bool bEnableCameraRotationLag = true;

	/** Speed of camera rotation lag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableCameraRotationLag"))
	float CameraRotationLagSpeed = 12.0f;

	/* --- Enhanced Input Assets --- */

	/** MappingContext for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	/** Light Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	/** Heavy Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	/** Dodge Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

public:
	AShadowSlavePlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_Controller() override;

	virtual void InitializeAttributes() override;

	/** Camera-relative movement input handler */
	void Move(const FInputActionValue& Value);

	/** Mouse / thumbstick look input handler */
	void Look(const FInputActionValue& Value);

	/** Jump press handler */
	void JumpStarted();

	/** Jump release handler */
	void JumpStopped();

	/** Sprint press handler */
	void SprintStarted();

	/** Sprint release handler */
	void SprintStopped();

	/** Light attack input handler */
	void LightAttack();

	/** Heavy attack input handler */
	void HeavyAttack();

	/** Interact input handler */
	void Interact();

	/** Dodge input handler */
	void Dodge();

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns InventoryComponent subobject **/
	FORCEINLINE UShadowSlaveInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Returns MemoryComponent subobject **/
	FORCEINLINE UShadowSlaveMemoryComponent* GetMemoryComponent() const { return MemoryComponent; }

	/** Returns ProgressionComponent subobject **/
	FORCEINLINE UShadowSlaveProgressionComponent* GetProgressionComponent() const { return ProgressionComponent; }

	/** Returns AspectComponent subobject **/
	FORCEINLINE UShadowSlaveAspectComponent* GetAspectComponent() const { return AspectComponent; }

	/** Returns EchoComponent subobject **/
	FORCEINLINE UShadowSlaveEchoComponent* GetEchoComponent() const { return EchoComponent; }

	/** Returns InteractionComponent subobject **/
	FORCEINLINE UShadowSlaveInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

private:
	/** Registers player character and controller with authoritative GameplaySubsystem */
	void NotifyGameplaySubsystemContext();

	/** Cleans up player context from GameplaySubsystem on unpossess or destruction */
	void ClearGameplaySubsystemContext();
};
