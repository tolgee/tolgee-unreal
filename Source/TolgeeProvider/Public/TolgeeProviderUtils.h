// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <Interfaces/IHttpRequest.h>

namespace TolgeeProviderUtils
{
	/**
	 * Adds the multi-part request header to the HTTP request.
	 */
	void AddMultiRequestHeader(const FHttpRequestRef& HttpRequest, const FString& Boundary);
	/**
	 * Adds a multi-part request part to the HTTP with the given content based on the boundary and extra headers.
	 */
	void AddMultiRequestPart(const FHttpRequestRef& HttpRequest, const FString& Boundary, const FString& ExtraHeaders, const FString& Value);
	/**
	 * Finishes the multi-part request by appending the boundary end to the HTTP request.
	 */
	void FinishMultiRequest(const FHttpRequestRef& HttpRequest, const FString& Boundary);
	/**
	 * Gets the request content from the HTTP request as a string.
	 */
	FString GetRequestContent(const FHttpRequestRef& HttpRequest);
}