// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

class FTolgeeProviderLocalizationServiceCommand;

/**
 * Base class for worker implementation that will be run to perform the commands.
 */
class ITolgeeProviderLocalizationServiceWorker
{
public:
	virtual ~ITolgeeProviderLocalizationServiceWorker() = default;

	/**
	 * Used to uniquely identify the worker type.
	 */
	virtual FName GetName() const = 0;
	/**
	 * Used to perform any work that is necessary to complete the command.
	 * NOTE: Make sure you block the execution until the command is completed.
	 */
	virtual bool Execute(FTolgeeProviderLocalizationServiceCommand& InCommand) = 0;
	/**
	 * Used to update the state of localization items after the command is completed.
	 * NOTE: This will always run on the main thread.
	 */
	virtual bool UpdateStates() const = 0;
};

using FTolgeeProviderLocalizationServiceWorkerRef = TSharedRef<ITolgeeProviderLocalizationServiceWorker>;