// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeLocalizationInjectorSubsystem.h"

#include <Async/Async.h>
#include <Engine/World.h>
#include <Internationalization/TextLocalizationResource.h>

#if WITH_LOCALIZATION_MODULE
#include <PortableObjectFormatDOM.h>
#include <PortableObjectPipeline.h>
#endif

#include "TolgeeLog.h"
#include "TolgeeTextSource.h"

void UTolgeeLocalizationInjectorSubsystem::OnGameInstanceStart(UGameInstance* GameInstance)
{
}

void UTolgeeLocalizationInjectorSubsystem::OnGameInstanceEnd(bool bIsSimulating)
{
}

void UTolgeeLocalizationInjectorSubsystem::GetLocalizedResources(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource) const
{
	TMap<FString, TArray<FTolgeeTranslationData>> DataToInject = GetDataToInject();
	for (const TPair<FString, TArray<FTolgeeTranslationData>>& CachedTranslation : DataToInject)
	{
		if (!InPrioritizedCultures.Contains(CachedTranslation.Key))
		{
			continue;
		}

		for (const FTolgeeTranslationData& TranslationData : CachedTranslation.Value)
		{
			const FTextKey InNamespace = TranslationData.ParsedNamespace;
			const FTextKey InKey = TranslationData.ParsedKey;
			const FString InLocalizedString = TranslationData.Translation;

			if (FTextLocalizationResource::FEntry* ExistingEntry = InOutLocalizedResource.Entries.Find(FTextId(InNamespace, InKey)))
			{
				//NOTE: -1 is a higher than usual priority, meaning this entry will override any existing one. See FTextLocalizationResource::ShouldReplaceEntry 
				InOutLocalizedResource.AddEntry(InNamespace, InKey, ExistingEntry->SourceStringHash, InLocalizedString, -1);
			}
			else
			{
				UE_LOG(LogTolgee, Warning, TEXT("Failed to inject translation for %s:%s. Default entry not found."), *InNamespace.ToString(), *InKey.ToString());
			}
		}
	}
}

TMap<FString, TArray<FTolgeeTranslationData>> UTolgeeLocalizationInjectorSubsystem::GetDataToInject() const
{
	return {};
}

void UTolgeeLocalizationInjectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TextSource = MakeShared<FTolgeeTextSource>();
	TextSource->GetLocalizedResources.BindUObject(this, &ThisClass::GetLocalizedResources);
	FTextLocalizationManager::Get().RegisterTextSource(TextSource.ToSharedRef());

	FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnGameInstanceStart);

#if WITH_EDITOR
	FEditorDelegates::PrePIEEnded.AddUObject(this, &ThisClass::OnGameInstanceEnd);
#endif
}

void UTolgeeLocalizationInjectorSubsystem::RefreshTranslationDataAsync()
{
	UE_LOG(LogTolgee, Verbose, TEXT("RefreshTranslationDataAsync requested."));

	AsyncTask(
		ENamedThreads::AnyHiPriThreadHiPriTask,
		[=]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UTolgeeLocalizationInjectorSubsystem::RefreshResources)

			UE_LOG(LogTolgee, Verbose, TEXT("RefreshTranslationDataAsync executing."));

			FTextLocalizationManager::Get().RefreshResources();
		}
	);
}

TArray<FTolgeeTranslationData> UTolgeeLocalizationInjectorSubsystem::ExtractTranslationsFromPO(const FString& PoContent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTolgeeLocalizationInjectorSubsystem::ExtractTranslationsFromPO)

	TArray<FTolgeeTranslationData> Result;

#if WITH_LOCALIZATION_MODULE
	FPortableObjectFormatDOM PortableObject;
	PortableObject.FromString(PoContent);

	for (auto EntryPairIter = PortableObject.GetEntriesIterator(); EntryPairIter; ++EntryPairIter)
	{
		auto POEntry = EntryPairIter->Value;
		if (POEntry->MsgId.IsEmpty() || POEntry->MsgStr.Num() == 0 || POEntry->MsgStr[0].IsEmpty())
		{
			// We ignore the header entry or entries with no translation.
			continue;
		}

		FTolgeeTranslationData TranslationData;

		constexpr ELocalizedTextCollapseMode InTextCollapseMode = ELocalizedTextCollapseMode::IdenticalTextIdAndSource;
		constexpr EPortableObjectFormat InPOFormat = EPortableObjectFormat::Crowdin;

		PortableObjectPipeline::ParseBasicPOFileEntry(*POEntry, TranslationData.ParsedNamespace, TranslationData.ParsedKey, TranslationData.SourceText, TranslationData.Translation, InTextCollapseMode, InPOFormat);

		Result.Add(TranslationData);
	}
#else
	else
	{
		UE_LOG(LogTolgee, Error, TEXT("Localization module is not available. Cannot extract translations from PO content."));
	}
#endif

	return Result;
}