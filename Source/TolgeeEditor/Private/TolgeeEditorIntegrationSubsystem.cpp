// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeEditorIntegrationSubsystem.h"

#include <HttpManager.h>
#include <HttpModule.h>
#include <Interfaces/IHttpRequest.h>
#include <Interfaces/IHttpResponse.h>
#include <Interfaces/IMainFrameModule.h>
#include <Internationalization/InternationalizationManifest.h>
#include <JsonObjectConverter.h>
#include <LocalizationCommandletTasks.h>
#include <LocalizationConfigurationScript.h>
#include <LocalizationSettings.h>
#include <LocalizationTargetTypes.h>
#include <Misc/EngineVersionComparison.h>
#include <Misc/FeedbackContext.h>
#include <Misc/FileHelper.h>
#include <Misc/MessageDialog.h>
#include <Serialization/JsonInternationalizationManifestSerializer.h>
#include <Settings/ProjectPackagingSettings.h>

#include "STolgeeSyncDialog.h"
#include "TolgeeEditor.h"
#include "TolgeeLocalizationSubsystem.h"
#include "TolgeeLog.h"
#include "TolgeeSettings.h"
#include "TolgeeUtils.h"

#define LOCTEXT_NAMESPACE "Tolgee"

namespace
{
	bool IsSameKey(const FLocalizedKey& A, const FLocalizationKey& B)
	{
		return A.Name == B.Key && A.Namespace == B.Namespace && A.Hash == TolgeeUtils::GetTranslationHash(B.DefaultText);
	}

	bool IsSameKeyWithChangedHash(const FLocalizedKey& A, const FLocalizationKey& B)
	{
		return A.Name == B.Key && A.Namespace == B.Namespace && A.Hash != TolgeeUtils::GetTranslationHash(B.DefaultText);
	}

	bool IsSameKey(const FLocalizationKey& B, const FLocalizedKey& A)
	{
		return IsSameKey(A, B);
	}
} // namespace

template <typename T, typename U>
TArray<T> RemoveExisting(const TArray<T>& InArray, const TArray<U>& InExisting)
{
	TArray<T> Result;
	for (const T& InElement : InArray)
	{
		const bool bExists = InExisting.ContainsByPredicate([InElement](const U& Element)
		{
			return IsSameKey(InElement, Element);
		});

		if (!bExists)
		{
			Result.Add(InElement);
		}
	}

	return Result;
}

void UTolgeeEditorIntegrationSubsystem::Sync()
{
	// Gather local keys
	TValueOrError<TArray<FLocalizationKey>, FText> LocalKeysResult = GatherLocalKeys();
	if (LocalKeysResult.HasError())
	{
		FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Gathering local keys failed."));
		return;
	}

	const TArray<FLocalizationKey> LocalKeys = LocalKeysResult.GetValue();

	// Gather remote keys
	UTolgeeLocalizationSubsystem* LocalizationSubsystem = GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>();
	LocalizationSubsystem->ManualFetch();

	// We need to wait synchronously wait for the fetch response to ensure we have the latest data
	while (LocalizationSubsystem->IsFetchInprogress())
	{
		FPlatformProcess::Sleep(0.01f);

		if (IsInGameThread())
		{
			FHttpModule::Get().GetHttpManager().Tick(0.01f);
		}
	}

	const TArray<FLocalizedKey> RemoteKeys = LocalizationSubsystem->GetLocalizedDictionary().Keys;

	TArray<FLocalizationKey> KeysToAdd = RemoveExisting(LocalKeys, RemoteKeys);
	TArray<FLocalizedKey> KeysToRemove = RemoveExisting(RemoteKeys, LocalKeys);

	TArray<TPair<FLocalizationKey, FLocalizedKey>> KeysToUpdate;

	for (auto KeyToAddIt = KeysToAdd.CreateIterator(); KeyToAddIt; ++KeyToAddIt)
	{
		int32 KeyToRemoveIndex = KeysToRemove.IndexOfByPredicate([KeyToAdd = *KeyToAddIt](const FLocalizedKey& Element)
		{
			return IsSameKeyWithChangedHash(Element, KeyToAdd);
		});

		if (KeyToRemoveIndex != INDEX_NONE)
		{
			KeysToUpdate.Emplace(*KeyToAddIt, KeysToRemove[KeyToRemoveIndex]);

			KeyToAddIt.RemoveCurrent();
			KeysToRemove.RemoveAt(KeyToRemoveIndex);
		}
	}

	if (KeysToAdd.IsEmpty() && KeysToUpdate.IsEmpty() && KeysToRemove.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, INVTEXT("Everything is up to date."));
		return;
	}

	TSharedRef<STolgeeSyncDialog> SyncWindow = SNew(STolgeeSyncDialog, KeysToAdd.Num(), KeysToUpdate.Num(), KeysToRemove.Num());

	GEditor->EditorAddModalWindow(SyncWindow);

	const bool bRunUpload = SyncWindow->UploadNew.bPerform;
	const bool bRunUpdate = SyncWindow->UpdateOutdated.bPerform;
	const bool bRunDelete = SyncWindow->DeleteUnused.bPerform;

	if (bRunUpload)
	{
		UploadLocalKeys(KeysToAdd);
	}

	if (bRunUpdate)
	{
		UpdateOutdatedKeys(KeysToUpdate);
	}

	if (bRunDelete)
	{
		DeleteRemoteKeys(KeysToRemove);
	}
}

void UTolgeeEditorIntegrationSubsystem::UploadLocalKeys(TArray<FLocalizationKey> NewLocalKeys)
{
	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();
	const FString DefaultLocale = Settings->Languages.Num() > 0 ? Settings->Languages[0] : TEXT("en");

	TArray<TSharedPtr<FJsonValue>> Keys;

	UE_LOG(LogTolgee, Log, TEXT("Upload request payload:"));
	for (const auto& Key : NewLocalKeys)
	{
		UE_LOG(LogTolgee, Log, TEXT("- namespace:%s key:%s default:%s"), *Key.Namespace, *Key.Key, *Key.DefaultText);

		FKeyItemPayload KeyItem;
		KeyItem.Name = Key.Key;
		KeyItem.Namespace = Key.Namespace;

		const FString HashTag = FString::Printf(TEXT("%s%s"), *TolgeeUtils::KeyHashPrefix, *LexToString(TolgeeUtils::GetTranslationHash(Key.DefaultText)));
		KeyItem.Tags.Emplace(HashTag);

		TSharedPtr<FJsonObject> KeyObject = FJsonObjectConverter::UStructToJsonObject(KeyItem);

		TSharedRef<FJsonObject> TranslationObject = MakeShareable(new FJsonObject);
		TranslationObject->SetStringField(DefaultLocale, Key.DefaultText);
		KeyObject->SetObjectField(TEXT("translations"), TranslationObject);

		Keys.Add(MakeShared<FJsonValueObject>(KeyObject));
	}

	TSharedRef<FJsonObject> PayloadJson = MakeShared<FJsonObject>();
	PayloadJson->SetArrayField(TEXT("keys"), Keys);

	FString Json;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(PayloadJson, Writer);

	const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/keys/import")));
	HttpRequest->SetVerb("POST");
	HttpRequest->SetHeader(TEXT("X-API-Key"), GetDefault<UTolgeeSettings>()->ApiKey);
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Type"), TolgeeUtils::GetSdkType());
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Version"), TolgeeUtils::GetSdkVersion());
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Json);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnLocalKeysUploaded);
	HttpRequest->ProcessRequest();
}

void UTolgeeEditorIntegrationSubsystem::DeleteRemoteKeys(TArray<FLocalizedKey> UnusedRemoteKeys)
{
	UE_LOG(LogTolgee, Log, TEXT("Delete request payload:"));
	FKeysDeletePayload Payload;
	for (const auto& Key : UnusedRemoteKeys)
	{
		if (Key.RemoteId.IsSet())
		{
			UE_LOG(LogTolgee, Log, TEXT("- id:%s namespace:%s key:%s"), *LexToString(Key.RemoteId.GetValue()), *Key.Namespace, *Key.Name);
			Payload.Ids.Add(Key.RemoteId.GetValue());
		}
		else
		{
			UE_LOG(LogTolgee, Warning, TEXT("- namespace:%s key:%s -> Cannot be deleted: Invalid id."), *Key.Namespace, *Key.Name);
		}
	}

	FString Json;
	FJsonObjectConverter::UStructToJsonObjectString(Payload, Json);

	const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/keys")));
	HttpRequest->SetVerb("DELETE");
	HttpRequest->SetHeader(TEXT("X-API-Key"), GetDefault<UTolgeeSettings>()->ApiKey);
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Type"), TolgeeUtils::GetSdkType());
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Version"), TolgeeUtils::GetSdkVersion());
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(Json);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnRemoteKeysDeleted);
	HttpRequest->ProcessRequest();
}

TValueOrError<TArray<FLocalizationKey>, FText> UTolgeeEditorIntegrationSubsystem::GatherLocalKeys() const
{
	TArray<ULocalizationTarget*> TargetObjectsToProcess = GatherValidLocalizationTargets();
	if (TargetObjectsToProcess.Num() == 0)
	{
		return MakeError(INVTEXT("No valid TargetObjects found in GetGameTargetSet."));
	}

	// Update the localization target before extract the keys out of them
	const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	const bool bWasSuccessful = LocalizationCommandletTasks::GatherTextForTargets(ParentWindow.ToSharedRef(), TargetObjectsToProcess);

	if (!bWasSuccessful)
	{
		return MakeError(INVTEXT("GatherTextFromTargets failed to update TargetObjects."));
	}

	return MakeValue(GetKeysFromTargets(TargetObjectsToProcess));
}

TArray<FLocalizationKey> UTolgeeEditorIntegrationSubsystem::GetKeysFromTargets(TArray<ULocalizationTarget*> LocalizationTargets) const
{
	TArray<FLocalizationKey> KeysFound;

	const UTolgeeSettings* TolgeeSettings = GetDefault<UTolgeeSettings>();

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

			if (TolgeeSettings->bIgnoreGeneratedKeys && ManifestEntry->Namespace.IsEmpty())
			{
				// Only generated keys have no namespace. Keys from string tables are required to have a one.
				continue;
			}

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
			const bool bValidCulture = LocalizationSettings.SupportedCulturesStatistics.IsValidIndex(LocalizationSettings.NativeCultureIndex);
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

void UTolgeeEditorIntegrationSubsystem::OnLocalKeysUploaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeEditorIntegrationSubsystem::OnLocalKeysUploaded"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to upload new keys was unsuccessful."));
		return;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to upload new keys received unexpected code: %s content: %s"), *LexToString(Response->GetResponseCode()), *Response->GetContentAsString());
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("New keys uploaded successfully."));

	GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
}

void UTolgeeEditorIntegrationSubsystem::OnRemoteKeysDeleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeEditorIntegrationSubsystem::OnRemoteKeysDeleted"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to delete unused keys was unsuccessful."));
		return;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to delete unused keys received unexpected code: %s content: %s"), *LexToString(Response->GetResponseCode()), *Response->GetContentAsString());
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Unused keys deleted successfully."));

	GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
}

void UTolgeeEditorIntegrationSubsystem::UpdateOutdatedKeys(TArray<TPair<FLocalizationKey, FLocalizedKey>> OutdatedKeys)
{
	for (const auto& KeyToUpdate : OutdatedKeys)
	{
		UE_LOG(LogTolgee, Log, TEXT("Update request payload:"));
		UE_LOG(LogTolgee, Log, TEXT("- id:%s namespace:%s key:%s"), *LexToString(KeyToUpdate.Value.RemoteId.GetValue()), *KeyToUpdate.Value.Namespace, *KeyToUpdate.Value.Name);
		
		FKeyItemPayload Payload;
		Payload.Name = KeyToUpdate.Value.Name;
		Payload.Namespace = KeyToUpdate.Value.Namespace;
		
		const FString HashTag = FString::Printf(TEXT("%s%s"), *TolgeeUtils::KeyHashPrefix, *LexToString(TolgeeUtils::GetTranslationHash(KeyToUpdate.Key.DefaultText)));
		Payload.Tags.Add(HashTag);

		FString Json;
		FJsonObjectConverter::UStructToJsonObjectString(Payload, Json);

		FString KeyUrl = FString::Printf(TEXT("v2/projects/keys/%lld/complex-update"), KeyToUpdate.Value.RemoteId.GetValue());
		const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetURL(TolgeeUtils::GetUrlEndpoint(KeyUrl));
		HttpRequest->SetVerb("PUT");
		HttpRequest->SetHeader(TEXT("X-API-Key"), GetDefault<UTolgeeSettings>()->ApiKey);
		HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Type"), TolgeeUtils::GetSdkType());
		HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Version"), TolgeeUtils::GetSdkVersion());
		HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		HttpRequest->SetContentAsString(Json);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnOutdatedKeyUpdated);
		HttpRequest->ProcessRequest();
	}
}

void UTolgeeEditorIntegrationSubsystem::OnOutdatedKeyUpdated(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeEditorIntegrationSubsystem::OnOutdatedKeysUpdated"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to updated outdated keys was unsuccessful."));
		return;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to updated outdated keys received unexpected code: %s content: %s"), *LexToString(Response->GetResponseCode()), *Response->GetContentAsString());
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Outdated keys updated successfully."));

	GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
}

void UTolgeeEditorIntegrationSubsystem::OnMainFrameReady(TSharedPtr<SWindow> InRootWindow, bool bIsRunningStartupDialog)
{
	// The browser plugin CEF version is too old to render the Tolgee website before 5.0
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	const UTolgeeSettings* TolgeeSettings = GetDefault<UTolgeeSettings>();

	// If the API Key is not set in the settings, the user has not completed the setup, therefore we will our welcome tab.
	if (TolgeeSettings && TolgeeSettings->ApiKey.IsEmpty())
	{
		FTolgeeEditorModule& TolgeeEditorModule = FTolgeeEditorModule::Get();
		TolgeeEditorModule.ActivateWindowTab();
	}
#endif
}

void UTolgeeEditorIntegrationSubsystem::ExportLocalTranslations()
{
	const UTolgeeLocalizationSubsystem* LocalizationSubsystem = GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>();
	while (LocalizationSubsystem->GetLocalizedDictionary().Keys.Num() == 0)
	{
		constexpr float SleepInterval = 0.1;

		UE_LOG(LogTolgee, Display, TEXT("Waiting for translation data. Retrying in %s second."), *LexToString(SleepInterval));
		FPlatformProcess::Sleep(SleepInterval);
		FHttpModule::Get().GetHttpManager().Tick(SleepInterval);
	}

	UE_LOG(LogTolgee, Display, TEXT("Got translation data."));
	const FLocalizedDictionary& Dictionary = LocalizationSubsystem->GetLocalizedDictionary();

	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(Dictionary, JsonString))
	{
		UE_LOG(LogTolgee, Error, TEXT("Couldn't convert the localized dictionary to string"));
		return;
	}

	const FString Filename = TolgeeUtils::GetLocalizationSourceFile();
	if (!FFileHelper::SaveStringToFile(JsonString, *Filename))
	{
		UE_LOG(LogTolgee, Error, TEXT("Couldn't save the localized dictionary to file: %s"), *Filename);
		return;
	}

	const FDirectoryPath TolgeeLocalizationPath = TolgeeUtils::GetLocalizationDirectory();
	UProjectPackagingSettings* ProjectPackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
	const bool bContainsPath = ProjectPackagingSettings->DirectoriesToAlwaysStageAsNonUFS.ContainsByPredicate(
		[TolgeeLocalizationPath](const FDirectoryPath& Path)
		{
			return Path.Path == TolgeeLocalizationPath.Path;
		}
	);

	if (!bContainsPath)
	{
		ProjectPackagingSettings->DirectoriesToAlwaysStageAsNonUFS.Add(TolgeeLocalizationPath);
		ProjectPackagingSettings->SaveConfig();
	}


	UE_LOG(LogTolgee, Display, TEXT("Localized dictionary succesfully saved to file: %s"), *Filename);
}

void UTolgeeEditorIntegrationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UTolgeeLocalizationSubsystem* LocalizationSubsystem = GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>();
	LocalizationSubsystem->ManualFetch();

	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	if (MainFrameModule.IsWindowInitialized())
	{
		const TSharedPtr<SWindow> RootWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		OnMainFrameReady(RootWindow, false);
	}
	else
	{
		MainFrameModule.OnMainFrameCreationFinished().AddUObject(this, &UTolgeeEditorIntegrationSubsystem::OnMainFrameReady);
	}

#if ENGINE_MAJOR_VERSION > 4
	const bool bIsRunningCookCommandlet = IsRunningCookCommandlet();
#else
	const FString Commandline = FCommandLine::Get();
	const bool bIsRunningCookCommandlet = IsRunningCommandlet() && Commandline.Contains(TEXT("run=cook"));
#endif

	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();
	if (bIsRunningCookCommandlet && !Settings->bLiveTranslationUpdates)
	{
		ExportLocalTranslations();
	}
}

#undef LOCTEXT_NAMESPACE