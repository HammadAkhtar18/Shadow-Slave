// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ShadowSlaveHitWindowNotifyState.generated.h"

/**
 * Animation Notify State used in combat Montages to mark the active hit detection window.
 * Calls OpenHitWindow on begin, sweeps during tick, and CloseHitWindow on end.
 */
UCLASS(meta = (DisplayName = "Shadow Slave Melee Hit Window"))
class SHADOWSLAVE_API UAnimNotifyState_ShadowSlaveHitWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
