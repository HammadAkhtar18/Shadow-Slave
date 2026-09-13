// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ShadowSlaveHitWindowNotifyState.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotifyState_ShadowSlaveHitWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UShadowSlaveCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UShadowSlaveCombatComponent>())
		{
			CombatComp->OpenHitWindow();
		}
	}
}

void UAnimNotifyState_ShadowSlaveHitWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UShadowSlaveCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UShadowSlaveCombatComponent>())
		{
			CombatComp->PerformMeleeTrace();
		}
	}
}

void UAnimNotifyState_ShadowSlaveHitWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (UShadowSlaveCombatComponent* CombatComp = MeshComp->GetOwner()->FindComponentByClass<UShadowSlaveCombatComponent>())
		{
			CombatComp->CloseHitWindow();
		}
	}
}
