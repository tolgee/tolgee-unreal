// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeEditorIntegrationSubsystem.h"

#include <HttpModule.h>
#include <Interfaces/IHttpRequest.h>
#include <Interfaces/IHttpResponse.h>
#include <Internationalization/InternationalizationManifest.h>
#include <JsonObjectConverter.h>
#include <LocalizationCommandletTasks.h>
#include <LocalizationConfigurationScript.h>
#include <LocalizationSettings.h>
#include <LocalizationTargetTypes.h>
#include <Misc/FeedbackContext.h>
#include <Misc/MessageDialog.h>
#include <Serialization/JsonInternationalizationManifestSerializer.h>

#include "TolgeeLocalizationSubsystem.h"
#include "TolgeeLog.h"
#include "TolgeeSettings.h"
#include "TolgeeUtils.h"

#define LOCTEXT_NAMESPACE "Tolgee"

namespace
{
	bool IsSameKey(const FTolgeeKeyData& A, const FLocalizationKey& B)
	{
		return A.KeyName == B.Key && A.KeyNamespace == B.Namespace && A.GetDefaultText() == B.DefaultText;
	}
} // namespace

void UTolgeeEditorIntegrationSubsystem::UploadMissingKeys()
{
	TArray<FLocalizationKey> MissingLocalKeys = GetMissingLocalKeys();

	if (MissingLocalKeys.IsEmpty())
	{
		const FText NoKeysToUpload = LOCTEXT("NoKeysToUpload", "No Keys found to upload");
		FMessageDialog::Open(EAppMsgType::Ok, NoKeysToUpload);

		return;
	}

	const FText ConfirmUpload = FText::Format(FTextFormat(LOCTEXT("UploadMissingKeysConfirmation", "Upload {0} keys to Tolgee backend? This action is not reversible")), MissingLocalKeys.Num());
	const EAppReturnType::Type Choice = FMessageDialog::Open(EAppMsgType::OkCancel, ConfirmUpload);

	if (Choice == EAppReturnType::Cancel)
	{
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Upload request payload:"));
	FKeysImportPayload Payload;
	for (const auto& Key : MissingLocalKeys)
	{
		UE_LOG(LogTolgee, Log, TEXT("- namespace:%s key:%s default:%s"), *Key.Namespace, *Key.Key, *Key.DefaultText);

		FImportKeyItem KeyItem;
		KeyItem.Name = Key.Key;
		KeyItem.Namespace = Key.Namespace;

		const FString OriginalTranslationTag = FString::Printf(TEXT("%s%s"), *TolgeeUtils::DefaultTextPrefix, *Key.DefaultText);
		KeyItem.Tags.Emplace(OriginalTranslationTag);

		const FString OriginalHashTag = FString::Printf(TEXT("%s%s"), *TolgeeUtils::KeyHashPrefix, *TolgeeUtils::GetTranslationHash(Key.DefaultText));
		KeyItem.Tags.Emplace(OriginalHashTag);

		Payload.Keys.Add(KeyItem);
	}

	FString Json;
	FJsonObjectConverter::UStructToJsonObjectString(Payload, Json);

	const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("POST");
	HttpRequest->SetHeader(TEXT("X-API-Key"), GetDefault<UTolgeeSettings>()->ApiKey);
	HttpRequest->SetURL(TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/keys/import")));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Json);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnMissingKeysUploaded);
	HttpRequest->ProcessRequest();
}

void UTolgeeEditorIntegrationSubsystem::PurgeUnusedKeys()
{
	TArray<FTolgeeKeyData> UnusedRemoteKeys = GetUnusedRemoteKeys();

	if (UnusedRemoteKeys.IsEmpty())
	{
		const FText NoKeysToPurge = LOCTEXT("NoKeysToPurge", "No keys found to purge");
		FMessageDialog::Open(EAppMsgType::Ok, NoKeysToPurge);

		return;
	}

	const FText ConfirmPurge = FText::Format(FTextFormat(LOCTEXT("PurgeUnusedKeysConfirmation", "Delete {0} keys from Tolgee backend? This action is not reversible")), UnusedRemoteKeys.Num());
	const EAppReturnType::Type Choice = FMessageDialog::Open(EAppMsgType::OkCancel, ConfirmPurge);

	if (Choice == EAppReturnType::Cancel)
	{
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Purge request payload:"));
	FKeysDeletePayload Payload;
	for (const auto& Key : UnusedRemoteKeys)
	{
		UE_LOG(LogTolgee, Log, TEXT("- id:%s namespace:%s key:%s"), *LexToString(Key.KeyId), *Key.KeyNamespace, *Key.KeyName);

		Payload.Ids.Add(Key.KeyId);
	}

	FString Json;
	FJsonObjectConverter::UStructToJsonObjectString(Payload, Json);

	const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("DELETE");
	HttpRequest->SetHeader(TEXT("X-API-Key"), GetDefault<UTolgeeSettings>()->ApiKey);
	HttpRequest->SetURL(TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/keys")));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Json);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnUnusedKeysPurged);
	HttpRequest->ProcessRequest();
}

TArray<FLocalizationKey> UTolgeeEditorIntegrationSubsystem::GetMissingLocalKeys() const
{
	const UTolgeeLocalizationSubsystem* LocalizationSubsystem = GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>();
	const TArray<FTolgeeKeyData> RemoteKeys = LocalizationSubsystem->GetLastFetchedKeys();

	TArray<FLocalizationKey> LocalKeys = GatherLocalKeys();

	for (auto It = LocalKeys.CreateIterator(); It; ++It)
	{
		const FLocalizationKey& LocalizationKey = *It;
		const bool bFoundLocally = RemoteKeys.ContainsByPredicate(
			[LocalizationKey](const FTolgeeKeyData& Key)
			{
				return IsSameKey(Key, LocalizationKey);
			}
		);

		if (bFoundLocally)
		{
			It.RemoveCurrent();
		}
	}

	return LocalKeys;
}

TArray<FLocalizationKey> UTolgeeEditorIntegrationSubsystem::GatherLocalKeys() const
{
	TArray<ULocalizationTarget*> TargetObjectsToProcess = GatherValidLocalizationTargets();
	if (TargetObjectsToProcess.IsEmpty())
	{
		UE_LOG(LogTolgee, Error, TEXT("No valid TargetObjects found in GetGameTargetSet."));
		return {};
	}

	// Update the localization target before extract the keys out of them
	const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	const bool bWasSuccessful = LocalizationCommandletTasks::GatherTextForTargets(ParentWindow.ToSharedRef(), TargetObjectsToProcess);

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("GatherTextFromTargets failed to update TargetObjects."));
		return {};
	}

	return GetKeysFromTargets(TargetObjectsToProcess);
}

TArray<FLocalizationKey> UTolgeeEditorIntegrationSubsystem::GetKeysFromTargets(TArray<ULocalizationTarget*> LocalizationTargets) const
{
	TArray<FLocalizationKey> KeysFound;

	for (const ULocalizationTarget* LocalizationTarget : LocalizationTargets)
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadingManifestData", "Loading Entries from Current Translation Manifest..."), true);

		const FString ManifestFilePathToRead = LocalizationConfigurationScript::GetManifestPath(LocalizationTarget);
		const TSharedRef<FInternationalizationManifest> ManifestAtHeadRevision = MakeShared<FInternationalizationManifest>();

		if (!FJsonInternationalizationManifestSerializer::DeserializeManifestFromFile(ManifestFilePathToRead, ManifestAtHeadRevision))
		{
			UE_LOG(LogTemp, Error, TEXT("Could not read manifest file %s."), *ManifestFilePathToRead);
			continue;
		}

		const int32 TotalManifestEntries = ManifestAtHeadRevision->GetNumEntriesBySourceText();
		if (TotalManifestEntries < 1)
		{
			UE_LOG(LogTemp, Error, TEXT("Error Loading Translations!"));
			continue;
		}

		int32 CurrentManifestEntry = 0;
		for (auto ManifestItr = ManifestAtHeadRevision->GetEntriesBySourceTextIterator(); ManifestItr; ++ManifestItr, ++CurrentManifestEntry)
		{
			const FText ProgressText = FText::Format(LOCTEXT("LoadingManifestDataProgress", "Loading {0}/{1}"), FText::AsNumber(CurrentManifestEntry), FText::AsNumber(TotalManifestEntries));
			GWarn->StatusUpdate(CurrentManifestEntry, TotalManifestEntries, ProgressText);

			const TSharedRef<FManifestEntry> ManifestEntry = ManifestItr.Value();
			for (auto ContextIter(ManifestEntry->Contexts.CreateConstIterator()); ContextIter; ++ContextIter)
			{
				FLocalizationKey& NewKey = KeysFound.Emplace_GetRef();
				NewKey.Key = ContextIter->Key.GetString();
				NewKey.Namespace = ManifestEntry->Namespace.GetString();
				NewKey.DefaultText = ManifestEntry->Source.Text;
			}
		}

		GWarn->EndSlowTask();
	}

	return KeysFound;
}

TArray<ULocalizationTarget*> UTolgeeEditorIntegrationSubsystem::GatherValidLocalizationTargets() const
{
	TArray<ULocalizationTarget*> Result;
	const ULocalizationTargetSet* GameTargetSet = ULocalizationSettings::GetGameTargetSet();
	for (ULocalizationTarget* LocalizationTarget : GameTargetSet->TargetObjects)
	{
		if (LocalizationTarget)
		{
			const FLocalizationTargetSettings& LocalizationSettings = LocalizationTarget->Settings;
			const bool bValidCulture =
				!LocalizationSettings.SupportedCulturesStatistics.IsEmpty() && LocalizationSettings.SupportedCulturesStatistics.IsValidIndex(LocalizationSettings.NativeCultureIndex);
			if (!bValidCulture)
			{
				UE_LOG(LogTolgee, Warning, TEXT("Skipping: %s -> Invalid default culture"), *LocalizationTarget->Settings.Name);
				continue;
			}

			FText GatherTextError;
			if (LocalizationSettings.GatherFromTextFiles.IsEnabled && !LocalizationSettings.GatherFromTextFiles.Validate(false, GatherTextError))
			{
				UE_LOG(LogTolgee, Warning, TEXT("Skipping: %s -> GatherText: %s"), *LocalizationTarget->Settings.Name, *GatherTextError.ToString());
				continue;
			}
			FText GatherPackageError;
			if (LocalizationSettings.GatherFromPackages.IsEnabled && !LocalizationSettings.GatherFromPackages.Validate(false, GatherPackageError))
			{
				UE_LOG(LogTolgee, Warning, TEXT("Skipping: %s -> GatherPackage: %s"), *LocalizationTarget->Settings.Name, *GatherPackageError.ToString());
				continue;
			}
			FText GatherMetadataError;
			if (LocalizationSettings.GatherFromMetaData.IsEnabled && !LocalizationSettings.GatherFromMetaData.Validate(false, GatherMetadataError))
			{
				UE_LOG(LogTolgee, Warning, TEXT("Skipping: %s -> GatherMetadata: %s"), *LocalizationTarget->Settings.Name, *GatherMetadataError.ToString());
				continue;
			}

			Result.Add(LocalizationTarget);
		}
	}

	return Result;
}

void UTolgeeEditorIntegrationSubsystem::OnMissingKeysUploaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeEditorIntegrationSubsystem::OnMissingKeysUploaded"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to upload missing keys was unsuccessful."));
		return;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to upload missing keys received unexpected code: %s"), *LexToString(Response->GetResponseCode()));
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Missing keys uploaded successfully."));

	GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
}

TArray<FTolgeeKeyData> UTolgeeEditorIntegrationSubsystem::GetUnusedRemoteKeys() const
{
	const UTolgeeLocalizationSubsystem* LocalizationSubsystem = GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>();
	TArray<FTolgeeKeyData> RemoteKeys = LocalizationSubsystem->GetLastFetchedKeys();

	const TArray<FLocalizationKey> LocalKeys = GatherLocalKeys();

	for (auto It = RemoteKeys.CreateIterator(); It; ++It)
	{
		const FTolgeeKeyData& TolgeeKey = *It;
		const bool bFoundLocally = LocalKeys.ContainsByPredicate(
			[TolgeeKey](const FLocalizationKey& Key)
			{
				return IsSameKey(TolgeeKey, Key);
			}
		);

		if (bFoundLocally)
		{
			It.RemoveCurrent();
		}
	}

	return RemoteKeys;
}

void UTolgeeEditorIntegrationSubsystem::OnUnusedKeysPurged(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeEditorIntegrationSubsystem::OnUnusedKeysPurged"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to purge unused keys was unsuccessful."));
		return;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to purge unused keys received unexpected code: %s"), *LexToString(Response->GetResponseCode()));
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Unused keys purged successfully."));

	GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
}

#undef LOCTEXT_NAMESPACE
