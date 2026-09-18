// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ShadowSlaveGameplaySubsystem.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Dialogue/ShadowSlaveDialogueDefinition.h"
#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "World/ShadowSlaveWorldStateComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Subsystems/SubsystemCollection.h"
#include "ShadowSlave.h"

UShadowSlaveGameplaySubsystem::UShadowSlaveGameplaySubsystem()
	: CurrentFlowState(EShadowSlaveGameplayFlowState::None)
	, PreviousFlowState(EShadowSlaveGameplayFlowState::None)
	, ActiveNightmareScenarioId(NAME_None)
	, bIsProcessingFlowTransition(false)
{
}

void UShadowSlaveGameplaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UShadowSlaveConversationSubsystem>();
	Collection.InitializeDependency<UShadowSlaveNightmareSubsystem>();
	Collection.InitializeDependency<UShadowSlaveStorySubsystem>();
	Collection.InitializeDependency<UShadowSlaveQuestSubsystem>();

	Super::Initialize(Collection);

	CurrentFlowState = EShadowSlaveGameplayFlowState::None;
	PreviousFlowState = EShadowSlaveGameplayFlowState::None;
	ActiveNightmareScenarioId = NAME_None;
	bIsProcessingFlowTransition = false;

	// Bind to authoritative ConversationSubsystem if available
	if (UShadowSlaveConversationSubsystem* ConvSub = GetConversationSubsystem())
	{
		ConvSub->OnConversationCompleted.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleConversationCompleted);
		ConvSub->OnConversationAborted.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleConversationAborted);
	}

	// Bind to authoritative NightmareSubsystem if available
	if (UShadowSlaveNightmareSubsystem* NightmareSub = GetNightmareSubsystem())
	{
		NightmareSub->OnScenarioStarted.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioStarted);
		NightmareSub->OnScenarioCompleted.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded);
		NightmareSub->OnScenarioFailed.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioFailed);
		NightmareSub->OnScenarioAborted.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded);
		NightmareSub->OnScenarioExited.AddDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded);
	}
}

void UShadowSlaveGameplaySubsystem::Deinitialize()
{
	if (UShadowSlaveConversationSubsystem* ConvSub = GetConversationSubsystem())
	{
		ConvSub->OnConversationCompleted.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleConversationCompleted);
		ConvSub->OnConversationAborted.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleConversationAborted);
	}

	if (UShadowSlaveNightmareSubsystem* NightmareSub = GetNightmareSubsystem())
	{
		NightmareSub->OnScenarioStarted.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioStarted);
		NightmareSub->OnScenarioCompleted.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded);
		NightmareSub->OnScenarioFailed.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioFailed);
		NightmareSub->OnScenarioAborted.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded);
		NightmareSub->OnScenarioExited.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded);
	}

	if (APawn* PlayerPawn = CurrentPlayerPawn.Get())
	{
		if (UShadowSlaveCombatComponent* Combat = PlayerPawn->FindComponentByClass<UShadowSlaveCombatComponent>())
		{
			Combat->OnCombatStateChanged.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandlePlayerCombatStateChanged);
		}
	}

	if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
	{
		QuestSub->UnregisterPlayerContext();
		if (AShadowSlaveCharacterBase* EnemyChar = Cast<AShadowSlaveCharacterBase>(CurrentCombatInstigator.Get()))
		{
			QuestSub->UnregisterCharacterSource(EnemyChar);
		}
	}

	CurrentPlayerPawn.Reset();
	CurrentPlayerController.Reset();
	CurrentInteractionTarget.Reset();
	CurrentConversationSpeaker.Reset();
	CurrentCombatInstigator.Reset();

	Super::Deinitialize();
}

bool UShadowSlaveGameplaySubsystem::CanTransitionFlowState(EShadowSlaveGameplayFlowState CurrentState, EShadowSlaveGameplayFlowState TargetState) const
{
	if (TargetState == EShadowSlaveGameplayFlowState::Unknown)
	{
		return false;
	}

	// Idempotent: transitioning to same state is always permitted
	if (CurrentState == TargetState)
	{
		return true;
	}

	switch (CurrentState)
	{
	case EShadowSlaveGameplayFlowState::Unknown:
	case EShadowSlaveGameplayFlowState::None:
		return TargetState == EShadowSlaveGameplayFlowState::Exploration ||
		       TargetState == EShadowSlaveGameplayFlowState::Nightmare ||
		       TargetState == EShadowSlaveGameplayFlowState::Dialogue ||
		       TargetState == EShadowSlaveGameplayFlowState::Transitioning ||
		       TargetState == EShadowSlaveGameplayFlowState::Paused;

	case EShadowSlaveGameplayFlowState::Exploration:
		return TargetState == EShadowSlaveGameplayFlowState::Dialogue ||
		       TargetState == EShadowSlaveGameplayFlowState::Combat ||
		       TargetState == EShadowSlaveGameplayFlowState::Nightmare ||
		       TargetState == EShadowSlaveGameplayFlowState::Transitioning ||
		       TargetState == EShadowSlaveGameplayFlowState::Paused ||
		       TargetState == EShadowSlaveGameplayFlowState::None;

	case EShadowSlaveGameplayFlowState::Dialogue:
		return TargetState == EShadowSlaveGameplayFlowState::Exploration ||
		       TargetState == EShadowSlaveGameplayFlowState::Combat ||
		       TargetState == EShadowSlaveGameplayFlowState::Nightmare ||
		       TargetState == EShadowSlaveGameplayFlowState::Transitioning ||
		       TargetState == EShadowSlaveGameplayFlowState::Paused ||
		       TargetState == EShadowSlaveGameplayFlowState::None;

	case EShadowSlaveGameplayFlowState::Combat:
		return TargetState == EShadowSlaveGameplayFlowState::Exploration ||
		       TargetState == EShadowSlaveGameplayFlowState::Nightmare ||
		       TargetState == EShadowSlaveGameplayFlowState::Dialogue ||
		       TargetState == EShadowSlaveGameplayFlowState::Transitioning ||
		       TargetState == EShadowSlaveGameplayFlowState::Paused ||
		       TargetState == EShadowSlaveGameplayFlowState::None;

	case EShadowSlaveGameplayFlowState::Nightmare:
		return TargetState == EShadowSlaveGameplayFlowState::Combat ||
		       TargetState == EShadowSlaveGameplayFlowState::Dialogue ||
		       TargetState == EShadowSlaveGameplayFlowState::Exploration ||
		       TargetState == EShadowSlaveGameplayFlowState::Transitioning ||
		       TargetState == EShadowSlaveGameplayFlowState::Paused ||
		       TargetState == EShadowSlaveGameplayFlowState::None;

	case EShadowSlaveGameplayFlowState::Transitioning:
		return TargetState == EShadowSlaveGameplayFlowState::Exploration ||
		       TargetState == EShadowSlaveGameplayFlowState::Nightmare ||
		       TargetState == EShadowSlaveGameplayFlowState::None ||
		       TargetState == EShadowSlaveGameplayFlowState::Paused;

	case EShadowSlaveGameplayFlowState::Paused:
		return TargetState != EShadowSlaveGameplayFlowState::Unknown;

	default:
		return false;
	}
}

bool UShadowSlaveGameplaySubsystem::RequestFlowStateTransition(EShadowSlaveGameplayFlowState NewState)
{
	if (NewState == EShadowSlaveGameplayFlowState::Unknown)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestFlowStateTransition - Rejected transition to Unknown state."));
		return false;
	}

	if (CurrentFlowState == NewState)
	{
		// Idempotent: already in the requested state; do not emit redundant delegate broadcast
		return true;
	}

	if (bIsProcessingFlowTransition)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestFlowStateTransition - Re-entrant transition rejected (%d -> %d)."),
			static_cast<uint8>(CurrentFlowState), static_cast<uint8>(NewState));
		return false;
	}

	if (!CanTransitionFlowState(CurrentFlowState, NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestFlowStateTransition - Rejected illegal state transition from %d to %d."),
			static_cast<uint8>(CurrentFlowState), static_cast<uint8>(NewState));
		return false;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingFlowTransition, true);

	const EShadowSlaveGameplayFlowState OldState = CurrentFlowState;
	PreviousFlowState = OldState;
	CurrentFlowState = NewState;

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveGameplaySubsystem: FlowState transitioned from %d to %d"),
		static_cast<uint8>(OldState), static_cast<uint8>(NewState));

	OnGameplayFlowStateChanged.Broadcast(NewState, OldState);
	return true;
}

bool UShadowSlaveGameplaySubsystem::StartGameplaySession(APlayerController* InPlayerController, APawn* InPlayerPawn)
{
	// 1. Resolve or assign player context if explicitly provided or discoverable
	if (InPlayerController || InPlayerPawn)
	{
		SetPlayerContext(InPlayerController, InPlayerPawn);
	}
	else if (!CurrentPlayerPawn.IsValid())
	{
		ResolvePlayerContext();
	}

	// 2. Handle repeated startup idempotently if already in Exploration
	if (CurrentFlowState == EShadowSlaveGameplayFlowState::Exploration)
	{
		return true;
	}

	// 3. Preserve any ongoing active gameplay flow without resetting or stomping state
	if (CurrentFlowState != EShadowSlaveGameplayFlowState::None && CurrentFlowState != EShadowSlaveGameplayFlowState::Unknown)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveGameplaySubsystem::StartGameplaySession - Active session already in progress (State: %d); preserving flow."),
			static_cast<uint8>(CurrentFlowState));
		return true;
	}

	// 4. Baseline transition into Exploration flow
	return RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration);
}

bool UShadowSlaveGameplaySubsystem::BeginExploration()
{
	return RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration);
}

bool UShadowSlaveGameplaySubsystem::BeginDialogue(UShadowSlaveDialogueDefinition* DialogueDef, AActor* SpeakerActor, AActor* InteractorActor)
{
	if (!DialogueDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginDialogue - DialogueDef is null."));
		return false;
	}

	UShadowSlaveConversationSubsystem* ConvSub = GetConversationSubsystem();
	if (!ConvSub)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginDialogue - ConversationSubsystem unavailable."));
		return false;
	}

	if (ConvSub->IsConversationActive())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginDialogue - A conversation is already active."));
		return false;
	}

	AActor* ActualInteractor = InteractorActor;
	if (!ActualInteractor)
	{
		ActualInteractor = GetPlayerPawn();
		if (!ActualInteractor)
		{
			ResolvePlayerContext();
			ActualInteractor = GetPlayerPawn();
		}
	}

	if (!CanTransitionFlowState(CurrentFlowState, EShadowSlaveGameplayFlowState::Dialogue) || bIsProcessingFlowTransition)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginDialogue - Cannot transition from %d to Dialogue."),
			static_cast<uint8>(CurrentFlowState));
		return false;
	}

	const EShadowSlaveGameplayFlowState StateBeforeDialogue = CurrentFlowState;
	if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Dialogue))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginDialogue - RequestFlowStateTransition to Dialogue failed."));
		return false;
	}

	const bool bStarted = ConvSub->StartConversation(DialogueDef, SpeakerActor, ActualInteractor);
	if (!bStarted)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginDialogue - ConversationSubsystem rejected StartConversation for '%s'."),
			*DialogueDef->DialogueId.ToString());
		RequestFlowStateTransition(StateBeforeDialogue);
		return false;
	}

	CurrentConversationSpeaker = SpeakerActor;
	CurrentInteractionTarget = SpeakerActor;

	if (SpeakerActor)
	{
		if (AShadowSlaveCharacterBase* CharSpeaker = Cast<AShadowSlaveCharacterBase>(SpeakerActor))
		{
			if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
			{
				QuestSub->RegisterCharacterSource(CharSpeaker);
			}
		}
	}

	return true;
}

bool UShadowSlaveGameplaySubsystem::EndDialogue(bool bAbort)
{
	UShadowSlaveConversationSubsystem* ConvSub = GetConversationSubsystem();
	if (ConvSub && ConvSub->IsConversationActive())
	{
		if (bAbort)
		{
			ConvSub->AbortConversation();
		}
		else
		{
			ConvSub->CompleteConversation();
		}
	}

	if (CurrentInteractionTarget == CurrentConversationSpeaker)
	{
		CurrentInteractionTarget.Reset();
	}
	CurrentConversationSpeaker.Reset();

	if (CurrentFlowState == EShadowSlaveGameplayFlowState::Dialogue)
	{
		const EShadowSlaveGameplayFlowState RestoreState =
			(!ActiveNightmareScenarioId.IsNone()) ? EShadowSlaveGameplayFlowState::Nightmare :
			(PreviousFlowState != EShadowSlaveGameplayFlowState::Dialogue && PreviousFlowState != EShadowSlaveGameplayFlowState::None)
				? PreviousFlowState : EShadowSlaveGameplayFlowState::Exploration;

		RequestFlowStateTransition(RestoreState);
	}

	return true;
}

bool UShadowSlaveGameplaySubsystem::BeginCombatFlow(AActor* InstigatingEnemy)
{
	if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Combat))
	{
		return false;
	}

	CurrentCombatInstigator = InstigatingEnemy;
	if (InstigatingEnemy)
	{
		if (AShadowSlaveCharacterBase* EnemyChar = Cast<AShadowSlaveCharacterBase>(InstigatingEnemy))
		{
			if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
			{
				QuestSub->RegisterCharacterSource(EnemyChar);
			}
		}
	}
	return true;
}

bool UShadowSlaveGameplaySubsystem::EndCombatFlow()
{
	if (CurrentCombatInstigator.IsValid())
	{
		if (AShadowSlaveCharacterBase* EnemyChar = Cast<AShadowSlaveCharacterBase>(CurrentCombatInstigator.Get()))
		{
			if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
			{
				QuestSub->UnregisterCharacterSource(EnemyChar);
			}
		}
	}
	CurrentCombatInstigator = nullptr;

	if (CurrentFlowState == EShadowSlaveGameplayFlowState::Combat)
	{
		const EShadowSlaveGameplayFlowState RestoreState =
			(!ActiveNightmareScenarioId.IsNone()) ? EShadowSlaveGameplayFlowState::Nightmare :
			(PreviousFlowState != EShadowSlaveGameplayFlowState::Combat && PreviousFlowState != EShadowSlaveGameplayFlowState::None)
				? PreviousFlowState : EShadowSlaveGameplayFlowState::Exploration;

		return RequestFlowStateTransition(RestoreState);
	}

	return true;
}

bool UShadowSlaveGameplaySubsystem::BeginNightmareFlow(FName ScenarioId)
{
	if (ScenarioId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginNightmareFlow - ScenarioId is None."));
		return false;
	}

	if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Nightmare))
	{
		return false;
	}

	ActiveNightmareScenarioId = ScenarioId;
	return true;
}

bool UShadowSlaveGameplaySubsystem::BeginNightmareScenario(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, APlayerController* InPlayerController)
{
	if (!ScenarioDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginNightmareScenario - ScenarioDef is null."));
		return false;
	}

	UShadowSlaveNightmareSubsystem* NightmareSub = GetNightmareSubsystem();
	if (!NightmareSub)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginNightmareScenario - NightmareSubsystem unavailable."));
		return false;
	}

	APlayerController* TargetPC = InPlayerController ? InPlayerController : GetPlayerController();
	if (!TargetPC)
	{
		ResolvePlayerContext();
		TargetPC = GetPlayerController();
	}

	if (!CanTransitionFlowState(CurrentFlowState, EShadowSlaveGameplayFlowState::Nightmare) || bIsProcessingFlowTransition)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginNightmareScenario - Cannot transition from %d to Nightmare."),
			static_cast<uint8>(CurrentFlowState));
		return false;
	}

	const bool bStarted = NightmareSub->StartScenario(ScenarioDef, TargetPC);
	if (!bStarted)
	{
		return false;
	}

	if (CurrentFlowState != EShadowSlaveGameplayFlowState::Nightmare)
	{
		if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Nightmare))
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::BeginNightmareScenario - Flow transition to Nightmare rejected after scenario started; aborting scenario '%s'."),
				*ScenarioDef->ScenarioId.ToString());
			NightmareSub->AbortScenario();
			return false;
		}
	}

	ActiveNightmareScenarioId = ScenarioDef->ScenarioId;
	return true;
}

bool UShadowSlaveGameplaySubsystem::EndNightmareFlow()
{
	if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration))
	{
		return false;
	}

	ActiveNightmareScenarioId = NAME_None;
	return true;
}

bool UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition(const FShadowSlaveGameplayTransitionRequest& Request)
{
	if (!Request.IsValid())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition - Invalid transition request (empty StoryId, TargetWorldId, and Reason)."));
		return false;
	}

	UShadowSlaveStorySubsystem* StorySub = nullptr;
	FName PreviousStepId = NAME_None;
	bool bStoryStepChanged = false;

	if (!Request.StoryId.IsNone())
	{
		StorySub = GetStorySubsystem();
		if (!StorySub)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition - StorySubsystem unavailable for StoryId '%s'."),
				*Request.StoryId.ToString());
			return false;
		}

		if (!StorySub->HasStoryDefinition(Request.StoryId))
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition - StoryId '%s' is not registered in StorySubsystem."),
				*Request.StoryId.ToString());
			return false;
		}

		if (!Request.StoryStepId.IsNone())
		{
			if (!StorySub->IsStoryActive(Request.StoryId))
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition - Cannot set step '%s' because Story '%s' is not active."),
					*Request.StoryStepId.ToString(), *Request.StoryId.ToString());
				return false;
			}

			PreviousStepId = StorySub->GetCurrentStoryStep(Request.StoryId);

			if (!StorySub->SetCurrentStoryStep(Request.StoryId, Request.StoryStepId))
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition - Failed to set story step '%s' on story '%s'."),
					*Request.StoryStepId.ToString(), *Request.StoryId.ToString());
				return false;
			}

			bStoryStepChanged = (PreviousStepId != Request.StoryStepId);
		}
	}

	if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Transitioning))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition - Flow state transition to Transitioning rejected."));

		if (bStoryStepChanged && StorySub)
		{
			StorySub->SetCurrentStoryStep(Request.StoryId, PreviousStepId);
		}

		return false;
	}

	OnGameplayTransitionRequested.Broadcast(Request);

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveGameplaySubsystem: Dispatched gameplay transition request (Story: '%s', World: '%s', Reason: '%s')"),
		*Request.StoryId.ToString(), *Request.TargetWorldId.ToString(), *Request.Reason.ToString());

	return true;
}

bool UShadowSlaveGameplaySubsystem::PauseGameplayFlow()
{
	if (CurrentFlowState == EShadowSlaveGameplayFlowState::Paused)
	{
		return true;
	}

	return RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Paused);
}

bool UShadowSlaveGameplaySubsystem::ResumeGameplayFlow()
{
	if (CurrentFlowState != EShadowSlaveGameplayFlowState::Paused)
	{
		return false;
	}

	const EShadowSlaveGameplayFlowState ResumeState =
		(PreviousFlowState != EShadowSlaveGameplayFlowState::Paused && PreviousFlowState != EShadowSlaveGameplayFlowState::None)
			? PreviousFlowState
			: EShadowSlaveGameplayFlowState::Exploration;

	return RequestFlowStateTransition(ResumeState);
}

bool UShadowSlaveGameplaySubsystem::ResolvePlayerContext()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}

	APawn* Pawn = PC->GetPawn();
	SetPlayerContext(PC, Pawn);
	return Pawn != nullptr;
}

void UShadowSlaveGameplaySubsystem::SetPlayerContext(APlayerController* InController, APawn* InPawn)
{
	const bool bPawnChanged = (CurrentPlayerPawn.Get() != InPawn);

	if (bPawnChanged)
	{
		if (APawn* OldPawn = CurrentPlayerPawn.Get())
		{
			if (UShadowSlaveCombatComponent* OldCombat = OldPawn->FindComponentByClass<UShadowSlaveCombatComponent>())
			{
				OldCombat->OnCombatStateChanged.RemoveDynamic(this, &UShadowSlaveGameplaySubsystem::HandlePlayerCombatStateChanged);
			}
		}

		CurrentPlayerPawn = InPawn;
		CurrentPlayerController = InController;

		if (InPawn)
		{
			if (UShadowSlaveCombatComponent* NewCombat = InPawn->FindComponentByClass<UShadowSlaveCombatComponent>())
			{
				NewCombat->OnCombatStateChanged.AddUniqueDynamic(this, &UShadowSlaveGameplaySubsystem::HandlePlayerCombatStateChanged);
			}

			if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
			{
				QuestSub->RegisterPlayerContext(InPawn);
			}
		}
		else
		{
			if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
			{
				QuestSub->UnregisterPlayerContext();
			}
		}

		OnPlayerContextUpdated.Broadcast(InPawn);
	}
	else
	{
		CurrentPlayerController = InController;
	}
}

APawn* UShadowSlaveGameplaySubsystem::GetPlayerPawn() const
{
	return CurrentPlayerPawn.Get();
}

APlayerController* UShadowSlaveGameplaySubsystem::GetPlayerController() const
{
	return CurrentPlayerController.Get();
}

AActor* UShadowSlaveGameplaySubsystem::GetInteractionTarget() const
{
	return CurrentInteractionTarget.Get();
}

void UShadowSlaveGameplaySubsystem::SetInteractionTarget(AActor* Target)
{
	CurrentInteractionTarget = Target;
	if (Target)
	{
		if (AShadowSlaveCharacterBase* CharTarget = Cast<AShadowSlaveCharacterBase>(Target))
		{
			if (UShadowSlaveQuestSubsystem* QuestSub = GetQuestSubsystem())
			{
				QuestSub->RegisterCharacterSource(CharTarget);
			}
		}
	}
}

AActor* UShadowSlaveGameplaySubsystem::GetConversationSpeaker() const
{
	return CurrentConversationSpeaker.Get();
}

AActor* UShadowSlaveGameplaySubsystem::GetCombatInstigator() const
{
	return CurrentCombatInstigator.Get();
}

FName UShadowSlaveGameplaySubsystem::GetActiveNightmareScenarioId() const
{
	return ActiveNightmareScenarioId;
}

UShadowSlaveWorldStateComponent* UShadowSlaveGameplaySubsystem::GetPlayerWorldState() const
{
	if (const APawn* Pawn = CurrentPlayerPawn.Get())
	{
		return Pawn->FindComponentByClass<UShadowSlaveWorldStateComponent>();
	}
	return nullptr;
}

UShadowSlaveWorldStateComponent* UShadowSlaveGameplaySubsystem::GetInteractionTargetWorldState() const
{
	if (const AActor* Target = CurrentInteractionTarget.Get())
	{
		return Target->FindComponentByClass<UShadowSlaveWorldStateComponent>();
	}
	return nullptr;
}

UShadowSlaveWorldStateComponent* UShadowSlaveGameplaySubsystem::GetWorldStateComponentForActor(const AActor* Actor) const
{
	if (Actor)
	{
		return Actor->FindComponentByClass<UShadowSlaveWorldStateComponent>();
	}
	return nullptr;
}

UShadowSlaveConversationSubsystem* UShadowSlaveGameplaySubsystem::GetConversationSubsystem() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShadowSlaveConversationSubsystem>();
	}
	return nullptr;
}

UShadowSlaveNightmareSubsystem* UShadowSlaveGameplaySubsystem::GetNightmareSubsystem() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShadowSlaveNightmareSubsystem>();
	}
	return nullptr;
}

UShadowSlaveStorySubsystem* UShadowSlaveGameplaySubsystem::GetStorySubsystem() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShadowSlaveStorySubsystem>();
	}
	return nullptr;
}

UShadowSlaveQuestSubsystem* UShadowSlaveGameplaySubsystem::GetQuestSubsystem() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShadowSlaveQuestSubsystem>();
	}
	return nullptr;
}

void UShadowSlaveGameplaySubsystem::HandleConversationCompleted(FName DialogueId)
{
	if (CurrentInteractionTarget == CurrentConversationSpeaker)
	{
		CurrentInteractionTarget.Reset();
	}
	CurrentConversationSpeaker.Reset();

	if (CurrentFlowState == EShadowSlaveGameplayFlowState::Dialogue)
	{
		const EShadowSlaveGameplayFlowState RestoreState =
			(!ActiveNightmareScenarioId.IsNone()) ? EShadowSlaveGameplayFlowState::Nightmare :
			(PreviousFlowState != EShadowSlaveGameplayFlowState::Dialogue && PreviousFlowState != EShadowSlaveGameplayFlowState::None)
				? PreviousFlowState : EShadowSlaveGameplayFlowState::Exploration;

		RequestFlowStateTransition(RestoreState);
	}
}

void UShadowSlaveGameplaySubsystem::HandleConversationAborted(FName DialogueId, FName LastNodeId)
{
	if (CurrentInteractionTarget == CurrentConversationSpeaker)
	{
		CurrentInteractionTarget.Reset();
	}
	CurrentConversationSpeaker.Reset();

	if (CurrentFlowState == EShadowSlaveGameplayFlowState::Dialogue)
	{
		const EShadowSlaveGameplayFlowState RestoreState =
			(!ActiveNightmareScenarioId.IsNone()) ? EShadowSlaveGameplayFlowState::Nightmare :
			(PreviousFlowState != EShadowSlaveGameplayFlowState::Dialogue && PreviousFlowState != EShadowSlaveGameplayFlowState::None)
				? PreviousFlowState : EShadowSlaveGameplayFlowState::Exploration;

		RequestFlowStateTransition(RestoreState);
	}
}

void UShadowSlaveGameplaySubsystem::HandleNightmareScenarioStarted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef)
{
	if (!ScenarioDef)
	{
		return;
	}

	if (CurrentFlowState != EShadowSlaveGameplayFlowState::Nightmare)
	{
		if (RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Nightmare))
		{
			ActiveNightmareScenarioId = ScenarioDef->ScenarioId;
		}
	}
	else
	{
		ActiveNightmareScenarioId = ScenarioDef->ScenarioId;
	}
}

void UShadowSlaveGameplaySubsystem::HandleNightmareScenarioEnded(UShadowSlaveNightmareScenarioDefinition* ScenarioDef)
{
	const FName EndedScenarioId = ScenarioDef ? ScenarioDef->ScenarioId : NAME_None;

	if (EndedScenarioId.IsNone() || ActiveNightmareScenarioId == EndedScenarioId)
	{
		ActiveNightmareScenarioId = NAME_None;

		if (CurrentFlowState == EShadowSlaveGameplayFlowState::Nightmare || CurrentFlowState == EShadowSlaveGameplayFlowState::Combat)
		{
			RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration);
		}
	}
}

void UShadowSlaveGameplaySubsystem::HandleNightmareScenarioFailed(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, EShadowSlaveScenarioFailureReason Reason)
{
	HandleNightmareScenarioEnded(ScenarioDef);
}

void UShadowSlaveGameplaySubsystem::HandlePlayerCombatStateChanged(ECombatState OldState, ECombatState NewState)
{
	if (NewState == ECombatState::Attacking || NewState == ECombatState::Dodging || NewState == ECombatState::Stunned)
	{
		if (CurrentFlowState != EShadowSlaveGameplayFlowState::Combat &&
		    CurrentFlowState != EShadowSlaveGameplayFlowState::Dialogue &&
		    CurrentFlowState != EShadowSlaveGameplayFlowState::Transitioning &&
		    CurrentFlowState != EShadowSlaveGameplayFlowState::Paused)
		{
			RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Combat);
		}
	}
	else if (NewState == ECombatState::Neutral)
	{
		// If combat ended and player has no active adversary, combat flow may conclude
		if (CurrentFlowState == EShadowSlaveGameplayFlowState::Combat && !CurrentCombatInstigator.IsValid())
		{
			EndCombatFlow();
		}
	}
}
