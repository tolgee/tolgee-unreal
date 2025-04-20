// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include "TolgeeProviderLocalizationServiceWorker.h"

class FTolgeeProviderUploadFileWorker : public ITolgeeProviderLocalizationServiceWorker
{
	// ~Begin ITolgeeProviderLocalizationServiceWorker 
	virtual FName GetName() const override;
	virtual bool Execute(FTolgeeProviderLocalizationServiceCommand& InCommand) override;
	virtual bool UpdateStates() const override;
	// ~End ITolgeeProviderLocalizationServiceWorker
};

class FTolgeeProviderDownloadFileWorker : public ITolgeeProviderLocalizationServiceWorker
{
	// ~Begin ITolgeeProviderLocalizationServiceWorker 
	virtual FName GetName() const override;
	virtual bool Execute(FTolgeeProviderLocalizationServiceCommand& InCommand) override;
	virtual bool UpdateStates() const override;
	// ~End ITolgeeProviderLocalizationServiceWorker
};