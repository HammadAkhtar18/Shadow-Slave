// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ShadowSlaveAIController.h"
#include "AI/ShadowSlaveEnemyCharacterBase.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

AShadowSlaveAIController::AShadowSlaveAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsPlayerState = false;

	// Create and configure AI Perception Component
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (SightConfig && PerceptionComp)
	{
		SightConfig->SightRadius = AIConfig.SightRadius;
		SightConfig->LoseSightRadius = AIConfig.LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = AIConfig.PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(AIConfig.SightMaxAge);
		SightConfig->AutoRegisterAsSource = true;

		// Sense all affiliations (enemies, neutrals, friendlies)
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		PerceptionComp->ConfigureSense(*SightConfig);
		PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void AShadowSlaveAIController::BeginPlay()
{
	Super::BeginPlay();

	if (PerceptionComp)
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AShadowSlaveAIController::HandleTargetPerceptionUpdated);
	}
}

void AShadowSlaveAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemyCharacter = Cast<AShadowSlaveEnemyCharacterBase>(InPawn);

	if (InPawn)
	{
		HomeLocation = InPawn->GetActorLocation();
	}

	if (ControlledEnemyCharacter.IsValid())
	{
		// Synchronize controller attack range configuration from enemy character tunables
		AIConfig.AttackRange = ControlledEnemyCharacter->GetAttackRange();
		AIConfig.AttackAcceptanceRadius = ControlledEnemyCharacter->GetAttackAcceptanceRadius();
		AIConfig.AttackRecoveryDuration = ControlledEnemyCharacter->GetAttackRecoveryDuration();

		if (UShadowSlaveCombatComponent* CombatComp = ControlledEnemyCharacter->GetCombatComponent())
		{
			CombatComp->OnCombatStateChanged.AddDynamic(this, &AShadowSlaveAIController::HandleCombatStateChanged);
		}
	}

	SetAIState(EShadowSlaveAIState::Idle);

	if (bShowAIDebug)
	{
		SetAIDebugEnabled(true);
	}
}

void AShadowSlaveAIController::OnUnPossess()
{
	ClearAllStateTimers();

	if (ControlledEnemyCharacter.IsValid())
	{
		if (UShadowSlaveCombatComponent* CombatComp = ControlledEnemyCharacter->GetCombatComponent())
		{
			CombatComp->OnCombatStateChanged.RemoveDynamic(this, &AShadowSlaveAIController::HandleCombatStateChanged);
		}
	}

	ControlledEnemyCharacter.Reset();
	CurrentTarget.Reset();

	Super::OnUnPossess();
}

void AShadowSlaveAIController::SetAIState(EShadowSlaveAIState NewState)
{
	// Death state is permanent and unconditionally overrides all behaviors
	if (CurrentAIState == EShadowSlaveAIState::Dead && NewState != EShadowSlaveAIState::Dead)
	{
		return;
	}

	if (CurrentAIState == NewState)
	{
		return;
	}

	const EShadowSlaveAIState OldState = CurrentAIState;
	ExitState(OldState);
	CurrentAIState = NewState;
	EnterState(CurrentAIState);

	OnAIStateChanged.Broadcast(OldState, NewState);
}

void AShadowSlaveAIController::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget.Get() == NewTarget)
	{
		return;
	}

	AActor* OldTarget = CurrentTarget.Get();
	CurrentTarget = NewTarget;
	OnAITargetChanged.Broadcast(OldTarget, NewTarget);
}

void AShadowSlaveAIController::SetAIConfig(const FShadowSlaveAIConfig& NewConfig)
{
	AIConfig = NewConfig;
	ConfigurePerception(AIConfig.SightRadius, AIConfig.LoseSightRadius, AIConfig.PeripheralVisionAngleDegrees);
}

void AShadowSlaveAIController::ConfigurePerception(float InSightRadius, float InLoseSightRadius, float InPeripheralVisionAngle)
{
	AIConfig.SightRadius = InSightRadius;
	AIConfig.LoseSightRadius = InLoseSightRadius;
	AIConfig.PeripheralVisionAngleDegrees = InPeripheralVisionAngle;

	if (SightConfig && PerceptionComp)
	{
		SightConfig->SightRadius = AIConfig.SightRadius;
		SightConfig->LoseSightRadius = AIConfig.LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = AIConfig.PeripheralVisionAngleDegrees;
		PerceptionComp->ConfigureSense(*SightConfig);
	}
}

void AShadowSlaveAIController::SetAIDebugEnabled(bool bEnabled)
{
	bShowAIDebug = bEnabled;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bShowAIDebug)
	{
		World->GetTimerManager().SetTimer(DebugTimerHandle, this, &AShadowSlaveAIController::DrawAIDebug, 0.1f, true);
	}
	else
	{
		World->GetTimerManager().ClearTimer(DebugTimerHandle);
	}
}

void AShadowSlaveAIController::EnterState(EShadowSlaveAIState State)
{
	switch (State)
	{
	case EShadowSlaveAIState::Idle:
		StartIdle();
		break;
	case EShadowSlaveAIState::Investigating:
		StartInvestigating(InvestigationLocation);
		break;
	case EShadowSlaveAIState::Chasing:
		StartChasing(CurrentTarget.Get());
		break;
	case EShadowSlaveAIState::Attacking:
		StartAttacking();
		break;
	case EShadowSlaveAIState::Recovering:
		StartRecovering();
		break;
	case EShadowSlaveAIState::Searching:
		StartSearching(LastKnownTargetLocation);
		break;
	case EShadowSlaveAIState::Returning:
		StartReturning();
		break;
	case EShadowSlaveAIState::Dead:
		HandleDeathState();
		break;
	}
}

void AShadowSlaveAIController::ExitState(EShadowSlaveAIState State)
{
	ClearAllStateTimers();
}

void AShadowSlaveAIController::StartIdle()
{
	ClearAllStateTimers();
	StopMovement();

	if (ControlledEnemyCharacter.IsValid())
	{
		ControlledEnemyCharacter->SetGait(EShadowSlaveGait::Walk);
	}
}

void AShadowSlaveAIController::StartInvestigating(const FVector& TargetLocation)
{
	ClearAllStateTimers();
	InvestigationLocation = TargetLocation;

	if (ControlledEnemyCharacter.IsValid())
	{
		ControlledEnemyCharacter->SetGait(EShadowSlaveGait::Walk);
	}

	MoveToLocation(InvestigationLocation, AIConfig.AttackAcceptanceRadius, true, true, true, false);
}

void AShadowSlaveAIController::StartChasing(AActor* Target)
{
	ClearAllStateTimers();

	if (!IsTargetValidAndAlive(Target))
	{
		SetAIState(EShadowSlaveAIState::Searching);
		return;
	}

	if (ControlledEnemyCharacter.IsValid())
	{
		ControlledEnemyCharacter->SetGait(EShadowSlaveGait::Sprint);
	}

	MoveToActor(Target, AIConfig.AttackAcceptanceRadius, true, true, false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ChaseTimerHandle, this, &AShadowSlaveAIController::OnChaseUpdate, AIConfig.ChaseUpdateInterval, true);
	}
}

void AShadowSlaveAIController::OnChaseUpdate()
{
	if (CurrentAIState != EShadowSlaveAIState::Chasing)
	{
		return;
	}

	AActor* Target = CurrentTarget.Get();
	APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn || !IsTargetValidAndAlive(Target))
	{
		SetCurrentTarget(nullptr);
		SetAIState(EShadowSlaveAIState::Searching);
		return;
	}

	const float DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());

	// If within melee reach, transition to attack
	if (DistanceToTarget <= AIConfig.AttackRange)
	{
		StopMovement();
		FaceTarget(Target);
		SetAIState(EShadowSlaveAIState::Attacking);
		return;
	}

	// Target moved far beyond lose sight threshold
	if (DistanceToTarget > AIConfig.LoseSightRadius * 1.5f)
	{
		LastKnownTargetLocation = Target->GetActorLocation();
		SetCurrentTarget(nullptr);
		SetAIState(EShadowSlaveAIState::Searching);
		return;
	}

	// Maintain dynamic pursuit path
	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		MoveToActor(Target, AIConfig.AttackAcceptanceRadius, true, true, false);
	}
}

void AShadowSlaveAIController::StartAttacking()
{
	ClearAllStateTimers();
	StopMovement();

	AActor* Target = CurrentTarget.Get();
	if (Target)
	{
		FaceTarget(Target);
	}

	if (ControlledEnemyCharacter.IsValid())
	{
		const bool bAttacked = ControlledEnemyCharacter->PerformAttack();
		if (!bAttacked)
		{
			// Attack was unable to execute (e.g. stunned or on cooldown); transition directly to recovery
			SetAIState(EShadowSlaveAIState::Recovering);
		}
	}
	else
	{
		SetAIState(EShadowSlaveAIState::Recovering);
	}
}

void AShadowSlaveAIController::HandleCombatStateChanged(ECombatState OldState, ECombatState NewState)
{
	if (CurrentAIState == EShadowSlaveAIState::Dead)
	{
		return;
	}

	// When melee attack swing completes or enters recovery, advance AI state
	if (CurrentAIState == EShadowSlaveAIState::Attacking)
	{
		if (NewState == ECombatState::Recovering || NewState == ECombatState::Neutral)
		{
			SetAIState(EShadowSlaveAIState::Recovering);
		}
	}
}

void AShadowSlaveAIController::StartRecovering()
{
	ClearAllStateTimers();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &AShadowSlaveAIController::OnRecoveryTimerExpired, AIConfig.AttackRecoveryDuration, false);
	}
}

void AShadowSlaveAIController::OnRecoveryTimerExpired()
{
	if (CurrentAIState != EShadowSlaveAIState::Recovering)
	{
		return;
	}

	AActor* Target = CurrentTarget.Get();
	APawn* ControlledPawn = GetPawn();

	if (!ControlledPawn || !IsTargetValidAndAlive(Target))
	{
		SetAIState(EShadowSlaveAIState::Searching);
		return;
	}

	const float DistanceToTarget = FVector::Dist(ControlledPawn->GetActorLocation(), Target->GetActorLocation());

	// If target is still in attack range, execute attack again
	if (DistanceToTarget <= AIConfig.AttackRange)
	{
		FaceTarget(Target);
		SetAIState(EShadowSlaveAIState::Attacking);
	}
	else
	{
		// Target moved away; resume pursuit
		SetAIState(EShadowSlaveAIState::Chasing);
	}
}

void AShadowSlaveAIController::StartSearching(const FVector& Location)
{
	ClearAllStateTimers();
	LastKnownTargetLocation = Location;

	if (ControlledEnemyCharacter.IsValid())
	{
		ControlledEnemyCharacter->SetGait(EShadowSlaveGait::Walk);
	}

	if (!LastKnownTargetLocation.IsZero())
	{
		MoveToLocation(LastKnownTargetLocation, AIConfig.AttackAcceptanceRadius, true, true, true, false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SearchTimerHandle, this, &AShadowSlaveAIController::OnSearchTimerExpired, AIConfig.SearchDuration, false);
	}
}

void AShadowSlaveAIController::OnSearchTimerExpired()
{
	if (CurrentAIState == EShadowSlaveAIState::Searching)
	{
		SetAIState(EShadowSlaveAIState::Returning);
	}
}

void AShadowSlaveAIController::StartReturning()
{
	ClearAllStateTimers();
	SetCurrentTarget(nullptr);

	if (ControlledEnemyCharacter.IsValid())
	{
		ControlledEnemyCharacter->SetGait(EShadowSlaveGait::Walk);
	}

	MoveToLocation(HomeLocation, 50.0f, true, true, true, false);
}

void AShadowSlaveAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (CurrentAIState == EShadowSlaveAIState::Returning)
	{
		if (Result.IsSuccess())
		{
			SetAIState(EShadowSlaveAIState::Idle);
		}
	}
	else if (CurrentAIState == EShadowSlaveAIState::Investigating)
	{
		if (Result.IsSuccess())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(InvestigationTimerHandle, this, &AShadowSlaveAIController::OnInvestigationTimerExpired, AIConfig.InvestigationDuration, false);
			}
		}
		else
		{
			SetAIState(EShadowSlaveAIState::Returning);
		}
	}
	else if (CurrentAIState == EShadowSlaveAIState::Chasing)
	{
		if (Result.IsSuccess() && CurrentTarget.IsValid() && GetPawn())
		{
			const float Dist = FVector::Dist(GetPawn()->GetActorLocation(), CurrentTarget->GetActorLocation());
			if (Dist <= AIConfig.AttackRange)
			{
				SetAIState(EShadowSlaveAIState::Attacking);
			}
		}
	}
}

void AShadowSlaveAIController::OnInvestigationTimerExpired()
{
	if (CurrentAIState == EShadowSlaveAIState::Investigating)
	{
		SetAIState(EShadowSlaveAIState::Returning);
	}
}

void AShadowSlaveAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (CurrentAIState == EShadowSlaveAIState::Dead || !Actor || Actor == GetPawn())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		if (IsTargetValidAndAlive(Actor))
		{
			// Acquire target if currently idle, searching, investigating, or returning
			if (CurrentAIState == EShadowSlaveAIState::Idle ||
				CurrentAIState == EShadowSlaveAIState::Investigating ||
				CurrentAIState == EShadowSlaveAIState::Searching ||
				CurrentAIState == EShadowSlaveAIState::Returning)
			{
				SetCurrentTarget(Actor);
				LastKnownTargetLocation = Stimulus.StimulusLocation;
				SetAIState(EShadowSlaveAIState::Chasing);
			}
		}
	}
	else
	{
		// Lost line of sight with active target
		if (Actor == CurrentTarget.Get())
		{
			LastKnownTargetLocation = Stimulus.StimulusLocation;

			if (CurrentAIState == EShadowSlaveAIState::Chasing)
			{
				SetAIState(EShadowSlaveAIState::Searching);
			}
		}
	}
}

void AShadowSlaveAIController::HandleDamagedBy(AActor* Attacker, const FVector& HitLocation)
{
	if (CurrentAIState == EShadowSlaveAIState::Dead)
	{
		return;
	}

	// If attacker is valid and alive, engage immediately
	if (Attacker && IsTargetValidAndAlive(Attacker))
	{
		if (CurrentAIState != EShadowSlaveAIState::Chasing && CurrentAIState != EShadowSlaveAIState::Attacking)
		{
			SetCurrentTarget(Attacker);
			SetAIState(EShadowSlaveAIState::Chasing);
		}
	}
	else if (!HitLocation.IsZero())
	{
		// Unknown attacker; investigate the disturbance origin
		if (CurrentAIState == EShadowSlaveAIState::Idle ||
			CurrentAIState == EShadowSlaveAIState::Returning ||
			CurrentAIState == EShadowSlaveAIState::Searching)
		{
			InvestigationLocation = HitLocation;
			SetAIState(EShadowSlaveAIState::Investigating);
		}
	}
}

void AShadowSlaveAIController::OnPawnDeath()
{
	SetAIState(EShadowSlaveAIState::Dead);
}

void AShadowSlaveAIController::HandleDeathState()
{
	ClearAllStateTimers();
	StopMovement();
	SetCurrentTarget(nullptr);

	if (PerceptionComp)
	{
		PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(this, &AShadowSlaveAIController::HandleTargetPerceptionUpdated);
	}
}

bool AShadowSlaveAIController::IsTargetValidAndAlive(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	if (Target->GetClass()->ImplementsInterface(UShadowSlaveDamageableInterface::StaticClass()))
	{
		return IShadowSlaveDamageableInterface::Execute_IsAlive(Target);
	}

	return true;
}

void AShadowSlaveAIController::FaceTarget(AActor* Target)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !Target)
	{
		return;
	}

	const FVector Direction = (Target->GetActorLocation() - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		const FRotator TargetRot = Direction.Rotation();
		ControlledPawn->SetActorRotation(FRotator(0.0f, TargetRot.Yaw, 0.0f));
	}
}

void AShadowSlaveAIController::ClearAllStateTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChaseTimerHandle);
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(InvestigationTimerHandle);
		World->GetTimerManager().ClearTimer(SearchTimerHandle);
	}
}

void AShadowSlaveAIController::DrawAIDebug()
{
	APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector TextLocation = PawnLocation + FVector(0.0f, 0.0f, 110.0f);

	FString TargetName = CurrentTarget.IsValid() ? CurrentTarget->GetName() : TEXT("None");
	float DistanceToTarget = CurrentTarget.IsValid() ? FVector::Dist(PawnLocation, CurrentTarget->GetActorLocation()) : 0.0f;

	FString DebugText = FString::Printf(
		TEXT("[AI: %s]\nState: %s\nTarget: %s (Dist: %.0f)\nHome: %s"),
		*ControlledPawn->GetName(),
		LexToString(CurrentAIState),
		*TargetName,
		DistanceToTarget,
		*HomeLocation.ToCompactString()
	);

	FColor StateColor = FColor::White;
	switch (CurrentAIState)
	{
	case EShadowSlaveAIState::Idle:          StateColor = FColor::Cyan; break;
	case EShadowSlaveAIState::Investigating: StateColor = FColor::Yellow; break;
	case EShadowSlaveAIState::Chasing:       StateColor = FColor::Orange; break;
	case EShadowSlaveAIState::Attacking:     StateColor = FColor::Red; break;
	case EShadowSlaveAIState::Recovering:    StateColor = FColor::Magenta; break;
	case EShadowSlaveAIState::Searching:     StateColor = FColor::Blue; break;
	case EShadowSlaveAIState::Returning:     StateColor = FColor::Emerald; break;
	case EShadowSlaveAIState::Dead:          StateColor = FColor::Black; break;
	}

	DrawDebugString(World, TextLocation, DebugText, nullptr, StateColor, 0.1f, true, 1.1f);
	DrawDebugSphere(World, PawnLocation, AIConfig.AttackRange, 16, StateColor, false, 0.1f);

	if (CurrentTarget.IsValid())
	{
		DrawDebugLine(World, PawnLocation, CurrentTarget->GetActorLocation(), FColor::Red, false, 0.1f, 0, 2.0f);
	}

	if (!HomeLocation.IsZero())
	{
		DrawDebugSphere(World, HomeLocation, 40.0f, 8, FColor::Cyan, false, 0.1f);
	}
}
