// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ShadowSlavePlayerCameraManager.h"

AShadowSlavePlayerCameraManager::AShadowSlavePlayerCameraManager()
{
	ViewPitchMin = DefaultPitchMin;
	ViewPitchMax = DefaultPitchMax;
	ViewYawMin = 0.0f;
	ViewYawMax = 360.0f;
	ViewRollMin = 0.0f;
	ViewRollMax = 0.0f;
}
