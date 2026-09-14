// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/ShadowSlaveSaveGame.h"

UShadowSlaveSaveGame::UShadowSlaveSaveGame()
{
	SaveVersion = CurrentSaveVersion;
	SaveSlotName = TEXT("DefaultSaveSlot");
	UserIndex = 0;
	Timestamp = FDateTime::UtcNow();
	SaveTitle = TEXT("AutoSave");
}
