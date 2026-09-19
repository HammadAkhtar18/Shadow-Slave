// Copyright Epic Games, Inc. All Rights Reserved.

#include "Content/ShadowSlaveContentRegistrySubsystem.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Core/ShadowSlaveLogChannels.h"

UShadowSlaveContentRegistrySubsystem::UShadowSlaveContentRegistrySubsystem()
{
}

void UShadowSlaveContentRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ClearRegistry();
	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveContentRegistrySubsystem: Initialized."));
}

void UShadowSlaveContentRegistrySubsystem::Deinitialize()
{
	ClearRegistry();
	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveContentRegistrySubsystem: Deinitialized."));
	Super::Deinitialize();
}

bool UShadowSlaveContentRegistrySubsystem::RegisterEntry(const FShadowSlaveContentRegistryEntry& Entry)
{
	if (Entry.ContentId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Cannot register entry with None ContentId."));
		return false;
	}

	if (RegistryEntries.Contains(Entry.ContentId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Duplicate ContentId '%s' rejected. An entry with this ID is already registered."),
			*Entry.ContentId.ToString());
		return false;
	}

	if (!Entry.IsValid())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Cannot register entry for ContentId '%s': no valid SoftDefinition or LoadedDefinition provided."),
			*Entry.ContentId.ToString());
		return false;
	}

	if (Entry.LoadedDefinition)
	{
		FString ErrorMessage;
		if (!Entry.LoadedDefinition->IsValidDefinition(&ErrorMessage))
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Cannot register invalid definition for ContentId '%s': %s"),
				*Entry.ContentId.ToString(), *ErrorMessage);
			return false;
		}
	}

	RegistryEntries.Add(Entry.ContentId, Entry);

	if (Entry.ContentType != EShadowSlaveContentType::None)
	{
		EntriesByType.FindOrAdd(Entry.ContentType).AddUnique(Entry.ContentId);
	}

	return true;
}

bool UShadowSlaveContentRegistrySubsystem::RegisterDefinition(UShadowSlaveContentDefinition* Definition)
{
	if (!Definition)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Cannot register null content definition."));
		return false;
	}

	if (Definition->ContentId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Cannot register definition with None ContentId."));
		return false;
	}

	FString ErrorMessage;
	if (!Definition->IsValidDefinition(&ErrorMessage))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Cannot register invalid definition '%s': %s"),
			*Definition->ContentId.ToString(), *ErrorMessage);
		return false;
	}

	FShadowSlaveContentRegistryEntry Entry;
	Entry.ContentId = Definition->ContentId;
	Entry.ContentType = Definition->ContentType;
	Entry.SoftDefinition = Definition;
	Entry.PrimaryAssetId = Definition->GetPrimaryAssetId();
	Entry.LoadedDefinition = Definition;

	return RegisterEntry(Entry);
}

bool UShadowSlaveContentRegistrySubsystem::UnregisterEntry(FName ContentId)
{
	if (ContentId.IsNone())
	{
		return false;
	}

	FShadowSlaveContentRegistryEntry RemovedEntry;
	if (RegistryEntries.RemoveAndCopyValue(ContentId, RemovedEntry))
	{
		if (RemovedEntry.ContentType != EShadowSlaveContentType::None)
		{
			if (TArray<FName>* TypeList = EntriesByType.Find(RemovedEntry.ContentType))
			{
				TypeList->Remove(ContentId);
				if (TypeList->IsEmpty())
				{
					EntriesByType.Remove(RemovedEntry.ContentType);
				}
			}
		}
		return true;
	}

	return false;
}

void UShadowSlaveContentRegistrySubsystem::ClearRegistry()
{
	RegistryEntries.Empty();
	EntriesByType.Empty();
}

bool UShadowSlaveContentRegistrySubsystem::HasContent(FName ContentId) const
{
	if (ContentId.IsNone())
	{
		return false;
	}
	return RegistryEntries.Contains(ContentId);
}

bool UShadowSlaveContentRegistrySubsystem::FindEntry(FName ContentId, FShadowSlaveContentRegistryEntry& OutEntry) const
{
	if (ContentId.IsNone())
	{
		return false;
	}

	if (const FShadowSlaveContentRegistryEntry* Found = RegistryEntries.Find(ContentId))
	{
		OutEntry = *Found;
		return true;
	}

	return false;
}

UShadowSlaveContentDefinition* UShadowSlaveContentRegistrySubsystem::ResolveContentDefinition(FName ContentId, bool bAllowSynchronousLoad) const
{
	if (ContentId.IsNone())
	{
		return nullptr;
	}

	const FShadowSlaveContentRegistryEntry* EntryPtr = RegistryEntries.Find(ContentId);
	if (!EntryPtr)
	{
		return nullptr;
	}

	if (EntryPtr->LoadedDefinition)
	{
		return EntryPtr->LoadedDefinition;
	}

	if (EntryPtr->SoftDefinition.IsValid())
	{
		return EntryPtr->SoftDefinition.Get();
	}

	if (bAllowSynchronousLoad && !EntryPtr->SoftDefinition.IsNull())
	{
		UShadowSlaveContentDefinition* Loaded = EntryPtr->SoftDefinition.LoadSynchronous();
		if (!Loaded)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveContentRegistrySubsystem: Failed to synchronously load asset for ContentId '%s' at path '%s'."),
				*ContentId.ToString(), *EntryPtr->SoftDefinition.ToString());
		}
		return Loaded;
	}

	return nullptr;
}

bool UShadowSlaveContentRegistrySubsystem::IsDefinitionLoaded(FName ContentId) const
{
	if (ContentId.IsNone())
	{
		return false;
	}

	const FShadowSlaveContentRegistryEntry* EntryPtr = RegistryEntries.Find(ContentId);
	if (!EntryPtr)
	{
		return false;
	}

	return (EntryPtr->LoadedDefinition != nullptr) || EntryPtr->SoftDefinition.IsValid();
}

int32 UShadowSlaveContentRegistrySubsystem::GetRegisteredContentCount() const
{
	return RegistryEntries.Num();
}

void UShadowSlaveContentRegistrySubsystem::GetAllRegisteredContentIds(TArray<FName>& OutContentIds) const
{
	OutContentIds.Reset();
	RegistryEntries.GetKeys(OutContentIds);
}

int32 UShadowSlaveContentRegistrySubsystem::GetRegisteredContentCountByType(EShadowSlaveContentType Type) const
{
	if (const TArray<FName>* TypeList = EntriesByType.Find(Type))
	{
		return TypeList->Num();
	}
	return 0;
}

void UShadowSlaveContentRegistrySubsystem::GetContentIdsByType(EShadowSlaveContentType Type, TArray<FName>& OutContentIds) const
{
	OutContentIds.Reset();
	if (const TArray<FName>* TypeList = EntriesByType.Find(Type))
	{
		OutContentIds = *TypeList;
	}
}

TArray<FShadowSlaveContentRegistryEntry> UShadowSlaveContentRegistrySubsystem::GetEntriesByType(EShadowSlaveContentType Type) const
{
	TArray<FShadowSlaveContentRegistryEntry> Result;
	if (const TArray<FName>* TypeList = EntriesByType.Find(Type))
	{
		Result.Reserve(TypeList->Num());
		for (const FName& Id : *TypeList)
		{
			if (const FShadowSlaveContentRegistryEntry* Entry = RegistryEntries.Find(Id))
			{
				Result.Add(*Entry);
			}
		}
	}
	return Result;
}
