// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeProviderLocalizationServiceCommand.h"

#include "TolgeeProviderLocalizationServiceWorker.h"

FTolgeeProviderLocalizationServiceCommand::FTolgeeProviderLocalizationServiceCommand(const TSharedRef<ILocalizationServiceOperation>& InOperation, const TSharedRef<ITolgeeProviderLocalizationServiceWorker>& InWorker, const FLocalizationServiceOperationComplete& InOperationCompleteDelegate)
	: Operation(InOperation)
	  , Worker(InWorker)
	  , OperationCompleteDelegate(InOperationCompleteDelegate)
	  , bExecuteProcessed(0)
	  , bCommandSuccessful(false)
	  , bAutoDelete(true)
	  , Concurrency(ELocalizationServiceOperationConcurrency::Synchronous)
{
	check(IsInGameThread());
}


bool FTolgeeProviderLocalizationServiceCommand::DoWork()
{
	bCommandSuccessful = Worker->Execute(*this);
	FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);

	return bCommandSuccessful;
}

void FTolgeeProviderLocalizationServiceCommand::Abandon()
{
	FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);
}

void FTolgeeProviderLocalizationServiceCommand::DoThreadedWork()
{
	Concurrency = ELocalizationServiceOperationConcurrency::Asynchronous;
	DoWork();
}

ELocalizationServiceOperationCommandResult::Type FTolgeeProviderLocalizationServiceCommand::ReturnResults()
{
	// NOTE: Everything in this class is copied from the SourceControl implementation (Other Localization providers copied it too).
	// Except this function where the Operation doesn't support adding messages, so we output them directly

	FMessageLog LocalizationServiceLog("LocalizationService");

	for (FString& String : InfoMessages)
	{
		LocalizationServiceLog.Error(FText::FromString(String));
	}
	for (FString& String : ErrorMessages)
	{
		LocalizationServiceLog.Info(FText::FromString(String));
	}

	// run the completion delegate if we have one bound
	ELocalizationServiceOperationCommandResult::Type Result = bCommandSuccessful ? ELocalizationServiceOperationCommandResult::Succeeded : ELocalizationServiceOperationCommandResult::Failed;
	(void)OperationCompleteDelegate.ExecuteIfBound(Operation, Result);

	return Result;
}