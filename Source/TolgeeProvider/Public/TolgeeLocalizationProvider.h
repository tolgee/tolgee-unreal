// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include "TolgeeProviderLocalizationServiceWorker.h"

#include <ILocalizationServiceProvider.h>

class FTolgeeProviderLocalizationServiceCommand;

DECLARE_DELEGATE_RetVal(FTolgeeProviderLocalizationServiceWorkerRef, FGetTolgeeProviderLocalizationServiceWorker)

class FTolgeeLocalizationProvider final : public ILocalizationServiceProvider
{
public:
	// ~Begin ILocalizationServiceProvider interface
	virtual void Init(bool bForceConnection = true) override;
	virtual void Close() override;
	virtual const FName& GetName() const override;
	virtual const FText GetDisplayName() const override;
	virtual FText GetStatusText() const override;
	virtual bool IsEnabled() const override;
	virtual bool IsAvailable() const override;
	virtual ELocalizationServiceOperationCommandResult::Type GetState(const TArray<FLocalizationServiceTranslationIdentifier>& InTranslationIds, TArray<TSharedRef<ILocalizationServiceState>>& OutState, ELocalizationServiceCacheUsage::Type InStateCacheUsage) override;
	virtual ELocalizationServiceOperationCommandResult::Type Execute(const TSharedRef<ILocalizationServiceOperation>& InOperation, const TArray<FLocalizationServiceTranslationIdentifier>& InTranslationIds, ELocalizationServiceOperationConcurrency::Type InConcurrency = ELocalizationServiceOperationConcurrency::Synchronous, const FLocalizationServiceOperationComplete& InOperationCompleteDelegate = FLocalizationServiceOperationComplete()) override;
	virtual bool CanCancelOperation(const TSharedRef<ILocalizationServiceOperation>& InOperation) const override;
	virtual void CancelOperation(const TSharedRef<ILocalizationServiceOperation>& InOperation) override;
	virtual void Tick() override;
	virtual void CustomizeSettingsDetails(IDetailCategoryBuilder& DetailCategoryBuilder) const override;
	virtual void CustomizeTargetDetails(IDetailCategoryBuilder& DetailCategoryBuilder, TWeakObjectPtr<ULocalizationTarget> LocalizationTarget) const override;
	virtual void CustomizeTargetToolbar(TSharedRef<FExtender>& MenuExtender, TWeakObjectPtr<ULocalizationTarget> LocalizationTarget) const override;
	virtual void CustomizeTargetSetToolbar(TSharedRef<FExtender>& MenuExtender, TWeakObjectPtr<ULocalizationTargetSet> LocalizationTargetSet) const override;
	// ~End ILocalizationServiceProvider interface

	/**
	 * Add the buttons to the toolbar for this localization target
	 */
	void AddTargetToolbarButtons(FToolBarBuilder& ToolbarBuilder, TWeakObjectPtr<ULocalizationTarget> InLocalizationTarget);
	/**
	 * Add the buttons to the toolbar for this set of localization targets
	 */
	void AddTargetSetToolbarButtons(FToolBarBuilder& ToolbarBuilder, TWeakObjectPtr<ULocalizationTargetSet> InLocalizationTargetSet);
	/**
	 * Download and import all translations for all cultures for the specified target from Tolgee 
	 */
	void ImportAllCulturesForTargetFromTolgee(TWeakObjectPtr<ULocalizationTarget> LocalizationTarget);
	/**
	 * Export and upload all cultures for a localization target to Tolgee 
	 */
	void ExportAllCulturesForTargetToTolgee(TWeakObjectPtr<ULocalizationTarget> LocalizationTarget);
	/**
	* Download and import all translations for all cultures for all targets for the specified target set from Tolgee
	*/
	void ImportAllTargetsForSetFromTolgee(TWeakObjectPtr<ULocalizationTargetSet> LocalizationTargetSet);
	/**
	 * Export and upload all cultures for all targets for a localization target set to Tolgee
	 */
	void ExportAllTargetsForSetToTolgee(TWeakObjectPtr<ULocalizationTargetSet> LocalizationTargetSet);
	/**
	 * Create a widget to configure the target's settings.
	 * NOTE: This spawns a widget to edit the matching sub-property of the provider settings
	 */
	TSharedRef<SWidget> CreateProjectSettingsWidget(TWeakObjectPtr<ULocalizationTarget> InLocalizationTarget);
	/**
	 * Refreshes the localization dashboard UI, specifically the word counter
	 */
	void UpdateTargetFromReports(TWeakObjectPtr<ULocalizationTarget> InLocalizationTarget);
	/**
	 * Sends messages from the command to the output log
	 */
	void OutputCommandMessages(const FTolgeeProviderLocalizationServiceCommand& InCommand) const;
	/**
	 * Helper function to register a worker type with this provider
	 */
	template <typename Type>
	void RegisterWorker(const FName& InName);
	/**
	 * Create work instances for the specified operation
	 */
	TSharedPtr<ITolgeeProviderLocalizationServiceWorker> CreateWorker(const FName& InOperationName) const;
	/**
	 * Execute the command synchronously
	 */
	ELocalizationServiceOperationCommandResult::Type ExecuteSynchronousCommand(FTolgeeProviderLocalizationServiceCommand& InCommand);
	/**
	 * Execute the command asynchronously
	 */
	ELocalizationServiceOperationCommandResult::Type IssueCommand(FTolgeeProviderLocalizationServiceCommand& InCommand);

	TMap<FName, FGetTolgeeProviderLocalizationServiceWorker> WorkersMap;

	TArray<FTolgeeProviderLocalizationServiceCommand*> CommandQueue;
};

template <typename Type>
void FTolgeeLocalizationProvider::RegisterWorker(const FName& InName)
{
	FGetTolgeeProviderLocalizationServiceWorker Delegate = FGetTolgeeProviderLocalizationServiceWorker::CreateLambda([]()
	{
		return MakeShared<Type>();
	});

	WorkersMap.Add(InName, Delegate);
}