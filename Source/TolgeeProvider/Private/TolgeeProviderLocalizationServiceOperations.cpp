// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeProviderLocalizationServiceOperations.h"

#include <LocalizationServiceOperations.h>
#include <Interfaces/IHttpResponse.h>
#include <HttpModule.h>

#include "TolgeeProviderLocalizationServiceCommand.h"
#include "TolgeeEditorSettings.h"
#include "TolgeeLog.h"
#include "TolgeeProviderUtils.h"

FName FTolgeeProviderUploadFileWorker::GetName() const
{
	return "UploadFileWorker";
}

bool FTolgeeProviderUploadFileWorker::Execute(FTolgeeProviderLocalizationServiceCommand& InCommand)
{
	TSharedPtr<FUploadLocalizationTargetFile> UploadFileOp = StaticCastSharedRef<FUploadLocalizationTargetFile>(InCommand.Operation);
	if (!UploadFileOp || !UploadFileOp.IsValid())
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Invalid operation"));

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}

	const FString FilePathAndName = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / UploadFileOp->GetInRelativeInputFilePathAndName());
	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FilePathAndName))
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Cannot load file %s"), *FilePathAndName);

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}

	const FGuid TargetGuid = UploadFileOp->GetInTargetGuid();
	const FString Locale = UploadFileOp->GetInLocale();

	//NOTE: This is currently not set by the Localization dashbard, when it becomes available send it as "removeOtherKeys"
	const bool bRemoveMissingKeys = !UploadFileOp->GetPreserveAllText();

	TSharedRef<FJsonObject> FileMapping = MakeShared<FJsonObject>();
	const FString FileName = FPaths::GetCleanFilename(FilePathAndName);
	FileMapping->SetStringField(TEXT("fileName"), FileName);

	TSharedRef<FJsonObject> Params = MakeShared<FJsonObject>();
	Params->SetBoolField(TEXT("convertPlaceholdersToIcu"), false);
	Params->SetBoolField(TEXT("createNewKeys"), true);
	Params->SetArrayField(TEXT("fileMappings"), {MakeShared<FJsonValueObject>(FileMapping)});
	Params->SetStringField(TEXT("forceMode"), TEXT("KEEP"));
	Params->SetBoolField(TEXT("overrideKeyDescriptions"), true);
	Params->SetBoolField(TEXT("removeOtherKeys"), true);
	Params->SetArrayField(TEXT("tagNewKeys"), {MakeShared<FJsonValueString>(TEXT("UnrealSDK"))});

	FString ParamsContents;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsContents);
	FJsonSerializer::Serialize(Params, Writer);

	const UTolgeeEditorSettings* ProviderSettings = GetDefault<UTolgeeEditorSettings>();
	const FTolgeePerTargetSettings* ProjectSettings = ProviderSettings->PerTargetSettings.Find(TargetGuid);
	const FString Url = FString::Printf(TEXT("%s/v2/projects/%s/single-step-import"), *ProviderSettings->ApiUrl, *ProjectSettings->ProjectId);

	FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("X-API-Key"), ProviderSettings->ApiKey);

	const FString Boundary = "---------------------------" + FString::FromInt(FDateTime::Now().GetTicks());
	TolgeeProviderUtils::AddMultiRequestHeader(HttpRequest, Boundary);
	TolgeeProviderUtils::AddMultiRequestPart(HttpRequest, Boundary, TEXT("name=\"params\""), ParamsContents);
	TolgeeProviderUtils::AddMultiRequestPart(HttpRequest, Boundary, TEXT("name=\"files\"; filename=\"test.po\""), FileContents);
	TolgeeProviderUtils::FinishMultiRequest(HttpRequest, Boundary);

	HttpRequest->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnHttpThread);

	HttpRequest->ProcessRequestUntilComplete();

	FHttpResponsePtr Response = HttpRequest->GetResponse();
	if (!Response)
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Failed to upload file %s"), *FilePathAndName);

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Failed to upload file %s. Response code: %d"), *FilePathAndName, Response->GetResponseCode());
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Response: %s"), *Response->GetContentAsString());

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}

	UE_LOG(LogTolgee, Display, TEXT("FTolgeeProviderUploadFileWorker: Successfully uploaded file %s. Content: %s"), *FilePathAndName, *Response->GetContentAsString());

	InCommand.bCommandSuccessful = true;
	return InCommand.bCommandSuccessful;
}

bool FTolgeeProviderUploadFileWorker::UpdateStates() const
{
	return true;
}

FName FTolgeeProviderDownloadFileWorker::GetName() const
{
	return "DownloadFileWorker";
}

bool FTolgeeProviderDownloadFileWorker::Execute(FTolgeeProviderLocalizationServiceCommand& InCommand)
{
	TSharedPtr<FDownloadLocalizationTargetFile> DownloadFileOp = StaticCastSharedRef<FDownloadLocalizationTargetFile>(InCommand.Operation);
	if (!DownloadFileOp || !DownloadFileOp.IsValid())
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderDownloadFileWorker: Invalid operation"));

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}

	const FString FilePathAndName = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / DownloadFileOp->GetInRelativeOutputFilePathAndName());
	const FGuid TargetGuid = DownloadFileOp->GetInTargetGuid();
	const FString Locale = DownloadFileOp->GetInLocale();

	const UTolgeeEditorSettings* ProviderSettings = GetDefault<UTolgeeEditorSettings>();
	const FTolgeePerTargetSettings* ProjectSettings = ProviderSettings->PerTargetSettings.Find(TargetGuid);

	const FString Url = FString::Printf(TEXT("%s/v2/projects/%s/export"), *ProviderSettings->ApiUrl, *ProjectSettings->ProjectId);

	FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(Url);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("X-API-Key"), ProviderSettings->ApiKey);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetBoolField(TEXT("escapeHtml"), false);
	Body->SetStringField(TEXT("format"), "PO");
	Body->SetBoolField(TEXT("supportArrays"), false);
	Body->SetBoolField(TEXT("zip"), false);
	Body->SetArrayField(TEXT("languages"), {MakeShared<FJsonValueString>(Locale)});

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(Body, Writer);

	HttpRequest->SetContentAsString(RequestBody);
	HttpRequest->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnHttpThread);

	HttpRequest->ProcessRequestUntilComplete();

	FHttpResponsePtr Response = HttpRequest->GetResponse();
	if (!Response)
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Failed to download file %s"), *FilePathAndName);

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}
	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Failed to download file %s. Response code: %d"), *FilePathAndName, Response->GetResponseCode());
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderUploadFileWorker: Response: %s"), *Response->GetContentAsString());

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}

	if (!FFileHelper::SaveStringToFile(Response->GetContentAsString(), *FilePathAndName, FFileHelper::EEncodingOptions::ForceUnicode))
	{
		UE_LOG(LogTolgee, Error, TEXT("FTolgeeProviderDownloadFileWorker: Failed to write file %s"), *FilePathAndName);

		InCommand.bCommandSuccessful = false;
		return InCommand.bCommandSuccessful;
	}

	UE_LOG(LogTolgee, Display, TEXT("FTolgeeProviderUploadFileWorker: Successfully downloaded file %s. Content: %s"), *FilePathAndName, *Response->GetContentAsString());

	InCommand.bCommandSuccessful = true;
	return InCommand.bCommandSuccessful;
}

bool FTolgeeProviderDownloadFileWorker::UpdateStates() const
{
	return true;
}