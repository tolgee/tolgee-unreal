// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeEditorIntegrationSubsystem.h"

#include <HttpModule.h>
#include <Interfaces/IHttpRequest.h>
#include <Interfaces/IHttpResponse.h>
#include <Misc/FileHelper.h>
#include <FileUtilities/ZipArchiveReader.h>

#include "TolgeeEditorSettings.h"
#include "TolgeeLog.h"
#include "TolgeeRuntimeSettings.h"
#include "TolgeeUtils.h"

void UTolgeeEditorIntegrationSubsystem::ManualFetch()
{
	FetchIUpdatesAreAvailableAsync();
}

void UTolgeeEditorIntegrationSubsystem::OnGameInstanceStart(UGameInstance* GameInstance)
{
	const UTolgeeRuntimeSettings* RuntimeSettings = GetDefault<UTolgeeRuntimeSettings>();
	if (RuntimeSettings->bUseCdnInEditor)
	{
		UE_LOG(LogTolgee, Display, TEXT("Tolgee is configured to use CDN in editor, fetching directly from Tolgee dashboard is disabled. If you want to use the dashboard data directly, disable CDN in the Tolgee settings."));
		return;
	}

	const UTolgeeEditorSettings* EditorSettings = GetDefault<UTolgeeEditorSettings>();
	if (EditorSettings->ProjectIds.IsEmpty())
	{
		UE_LOG(LogTolgee, Display, TEXT("CDN was disabled in editor, but no projects are configured. Static data will be used instead."));
		return;
	}

	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnRefreshTick);
	GameInstance->GetWorld()->GetTimerManager().SetTimer(RefreshTick, Delegate, EditorSettings->RefreshInterval, true);

	FetchAllProjects();
}

void UTolgeeEditorIntegrationSubsystem::OnGameInstanceEnd(bool bIsSimulating)
{
	ResetData();
}

TMap<FString, TArray<FTolgeeTranslationData>> UTolgeeEditorIntegrationSubsystem::GetDataToInject() const
{
	return CachedTranslations;
}

void UTolgeeEditorIntegrationSubsystem::FetchAllProjects()
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();

	for (const FString& ProjectId : Settings->ProjectIds)
	{
		const FString RequestUrl = FString::Printf(TEXT("%s/v2/projects/%s/export?format=PO"), *Settings->GetBaseUrl(), *ProjectId);
		UE_LOG(LogTolgee, Display, TEXT("Fetching localization data for project %s from Tolgee dashboard: %s"), *ProjectId, *RequestUrl);
		FetchFromDashboard(ProjectId, RequestUrl);
	}

	LastFetchTime = FDateTime::UtcNow();
}

void UTolgeeEditorIntegrationSubsystem::FetchFromDashboard(const FString& ProjectId, const FString& RequestUrl)
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();

	FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(RequestUrl);
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader(TEXT("X-API-Key"), Settings->ApiKey);
	TolgeeUtils::AddSdkHeaders(HttpRequest);

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnFetchedFromDashboard, ProjectId);
	HttpRequest->ProcessRequest();

	NumRequestsSent++;
}

void UTolgeeEditorIntegrationSubsystem::OnFetchedFromDashboard(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString ProjectId)
{
	if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Display, TEXT("Fetch successfully for %s to %s."), *ProjectId, *Request->GetURL());
		ReadTranslationsFromZipContent(ProjectId, Response->GetContent());
	}
	else
	{
		UE_LOG(LogTolgee, Error, TEXT("Request for %s to %s failed."), *ProjectId, *Request->GetURL());
	}

	NumRequestsCompleted++;

	if (NumRequestsCompleted == NumRequestsSent)
	{
		UE_LOG(LogTolgee, Display, TEXT("All requests completed. Refreshing translation data."));
		RefreshTranslationDataAsync();
	}
}

void UTolgeeEditorIntegrationSubsystem::ResetData()
{
	NumRequestsSent = 0;
	NumRequestsCompleted = 0;

	CachedTranslations.Empty();
	LastFetchTime = {0};
}

bool UTolgeeEditorIntegrationSubsystem::ReadTranslationsFromZipContent(const FString& ProjectId, const TArray<uint8>& ResponseContent)
{
	static FCriticalSection ReadFromFileCriticalSection;
	FScopeLock Lock(&ReadFromFileCriticalSection);

	const FString TempPath = FPaths::ProjectSavedDir() / TEXT("Tolgee") / TEXT("In-Context") / ProjectId + TEXT(".zip");
	const bool bSaveSuccess = FFileHelper::SaveArrayToFile(ResponseContent, *TempPath);
	if (!bSaveSuccess)
	{
		UE_LOG(LogTolgee, Error, TEXT("Failed to save zip for project %s as file to %s"), *ProjectId, *TempPath);
		return false;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	IFileHandle* ArchiveFileHandle = PlatformFile.OpenRead(*TempPath);
	FZipArchiveReader ZipReader = {ArchiveFileHandle};
	if (!ZipReader.IsValid())
	{
		UE_LOG(LogTolgee, Error, TEXT("Failed to open zip for project %s located at %s"), *ProjectId, *TempPath);
		return false;
	}

	const TArray<FString> FileNames = ZipReader.GetFileNames();
	for (const FString& FileName : FileNames)
	{
		TArray<uint8> FileBuffer;
		if (ZipReader.TryReadFile(FileName, FileBuffer))
		{
			const FString InCulture = FPaths::GetBaseFilename(FileName);
			const FString FileContents = FString(FileBuffer.Num(), UTF8_TO_TCHAR(FileBuffer.GetData()));

			TArray<FTolgeeTranslationData> Translations = ExtractTranslationsFromPO(FileContents);
			CachedTranslations.FindOrAdd(InCulture).Append(Translations);
		}
		else
		{
			UE_LOG(LogTolgee, Warning, TEXT("Failed to read file %s for project %s inside zip at %s"), *FileName, *ProjectId, *TempPath);
		}
	}

	return true;
}

void UTolgeeEditorIntegrationSubsystem::OnRefreshTick()
{
	FetchIUpdatesAreAvailableAsync();
}

void UTolgeeEditorIntegrationSubsystem::FetchIUpdatesAreAvailableAsync()
{
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask,
	          [this]()
	          {
		          FetchIfProjectsWereUpdated();
	          });
}

void UTolgeeEditorIntegrationSubsystem::FetchIfProjectsWereUpdated()
{
	if (bRequestInProgress)
	{
		return;
	}

	TGuardValue ScopedRequestProgress(bRequestInProgress, true);

	FDateTime LastProjectUpdate = {0};
	TArray<FHttpRequestPtr> PendingRequests;
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();

	for (const FString& ProjectId : Settings->ProjectIds)
	{
		const FString RequestUrl = FString::Printf(TEXT("%s/v2/projects/%s/stats"), *Settings->GetBaseUrl(), *ProjectId);

		FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetVerb("GET");
		HttpRequest->SetURL(RequestUrl);
		HttpRequest->SetHeader(TEXT("X-API-Key"), Settings->ApiKey);
		TolgeeUtils::AddSdkHeaders(HttpRequest);

		HttpRequest->ProcessRequest();

		PendingRequests.Add(HttpRequest);
	}


	while (!PendingRequests.IsEmpty())
	{
		FPlatformProcess::Sleep(0.1f);

		for (auto RequestIt = PendingRequests.CreateIterator(); RequestIt; ++RequestIt)
		{
			const FHttpRequestPtr& Request = *RequestIt;
			if (EHttpRequestStatus::IsFinished(Request->GetStatus()))
			{
				const FHttpResponsePtr Response = Request->GetResponse();
				if (Response && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
				{
					const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
					TSharedPtr<FJsonObject> JsonObject;

					if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
					{
						const TArray<TSharedPtr<FJsonValue>> LanguageStats = JsonObject->GetArrayField(TEXT("languageStats"));
						for (const TSharedPtr<FJsonValue>& Language : LanguageStats)
						{
							const double LanguageUpdateTime = Language->AsObject()->GetNumberField(TEXT("translationsUpdatedAt"));
							const int64 UnixTimestampSeconds = LanguageUpdateTime / 1000;
							const FDateTime UpdateTime = FDateTime::FromUnixTimestamp(UnixTimestampSeconds);

							LastProjectUpdate = LastProjectUpdate < UpdateTime ? UpdateTime : LastProjectUpdate;
						}
					}
				}

				RequestIt.RemoveCurrent();
			}
		}
	}

	if (LastProjectUpdate > LastFetchTime)
	{
		ResetData();
		FetchAllProjects();
	}
	else
	{
		UE_LOG(LogTolgee, Display, TEXT("No new updates since last fetch at %s, last project update was at %s"), *LastFetchTime.ToString(), *LastProjectUpdate.ToString());
	}
}
