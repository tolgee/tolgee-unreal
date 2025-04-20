// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeProviderUtils.h"

void TolgeeProviderUtils::AddMultiRequestHeader(const FHttpRequestRef& HttpRequest, const FString& Boundary)
{
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("multipart/form-data; boundary =" + Boundary));
}

void TolgeeProviderUtils::AddMultiRequestPart(const FHttpRequestRef& HttpRequest, const FString& Boundary, const FString& ExtraHeaders, const FString& Value)
{
	const FString BoundaryBegin = FString(TEXT("--")) + Boundary + FString(TEXT("\r\n"));

	FString Data = GetRequestContent(HttpRequest);
	Data += FString(TEXT("\r\n"))
		+ BoundaryBegin
		+ FString(TEXT("Content-Disposition: form-data;"))
		+ ExtraHeaders
		+ FString(TEXT("\r\n\r\n"))
		+ Value;

	HttpRequest->SetContentAsString(Data);
}

void TolgeeProviderUtils::FinishMultiRequest(const FHttpRequestRef& HttpRequest, const FString& Boundary)
{
	const FString BoundaryEnd = FString(TEXT("\r\n--")) + Boundary + FString(TEXT("--\r\n"));

	FString Data = GetRequestContent(HttpRequest);
	Data.Append(BoundaryEnd);

	HttpRequest->SetContentAsString(Data);
}

FString TolgeeProviderUtils::GetRequestContent(const FHttpRequestRef& HttpRequest)
{
	TArray<uint8> Content = HttpRequest->GetContent();

	FString Result;
	FFileHelper::BufferToString(Result, Content.GetData(), Content.Num());

	return Result;
}