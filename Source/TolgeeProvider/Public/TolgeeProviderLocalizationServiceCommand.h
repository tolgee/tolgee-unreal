// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <ILocalizationServiceProvider.h>
#include <Misc/IQueuedWork.h>

class ITolgeeProviderLocalizationServiceWorker;

class FTolgeeProviderLocalizationServiceCommand : public IQueuedWork
{
public:
	FTolgeeProviderLocalizationServiceCommand(const TSharedRef<ILocalizationServiceOperation>& InOperation, const TSharedRef<ITolgeeProviderLocalizationServiceWorker>& InWorker, const FLocalizationServiceOperationComplete& InOperationCompleteDelegate = FLocalizationServiceOperationComplete());

	/**
	 * This function handles the core threaded operations.
	 * All tasks associated with this queued object should be executed within this method.
	 */
	bool DoWork();

	/**
	 * Notifies queued work of abandonment for cleanup. Invoked only if abandoned before completion.
	 * The object must delete itself using its allocated heap.
	 */
	virtual void Abandon() override;

	/**
	 * Used to signal the object to clean up, but only after it has completed its work.
	 */
	virtual void DoThreadedWork() override;

	/**
	 * Save any results and call any registered callbacks.
	 */
	ELocalizationServiceOperationCommandResult::Type ReturnResults();

	/**
	 * Operation we want to perform - contains outward-facing parameters & results
	 */
	TSharedRef<ILocalizationServiceOperation> Operation;

	/**
	 * The object that will actually do the work
	 */
	TSharedRef<ITolgeeProviderLocalizationServiceWorker> Worker;

	/**
	 * Delegate to notify when this operation completes
	 */
	FLocalizationServiceOperationComplete OperationCompleteDelegate;

	/**
	 * If true, this command has been processed by the Localization service thread
	 */
	volatile int32 bExecuteProcessed;

	/**
	 * If true, the Localization service command succeeded
	 */
	bool bCommandSuccessful;

	/**
	 * If true, this command will be automatically cleaned up in Tick()
	 */
	bool bAutoDelete;

	/**
	 * Whether we are running multi-treaded or not
	 */
	ELocalizationServiceOperationConcurrency::Type Concurrency;

	/**
	 * Info and/or warning message storage
	 */
	TArray<FString> InfoMessages;

	/**
	 * Potential error message storage
	 */
	TArray<FString> ErrorMessages;
};