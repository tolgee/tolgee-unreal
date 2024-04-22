// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <EditorSubsystem.h>
#include <Interfaces/IHttpRequest.h>

#include "TolgeeEditorRequestData.h"

#include "TolgeeEditorIntegrationSubsystem.generated.h"

struct FLocalizedKey;
struct FTolgeeKeyData;

class ULocalizationTarget;

/**
 * Subsystem responsible for updating the Tolgee backend based on the locally available data
 * e.g.: upload local keys missing from remote, delete remote keys not present locally
 */
UCLASS()
class UTolgeeEditorIntegrationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	void Sync();

private:
	void UploadLocalKeys(TArray<FLocalizationKey> NewLocalKeys);
	void OnLocalKeysUploaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void DeleteRemoteKeys(TArray<FLocalizedKey> UnusedRemoteKeys);
	void OnRemoteKeysDeleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UpdateOutdatedKeys(TArray<TPair<FLocalizationKey, FLocalizedKey>> OutdatedKeys);
	void OnOutdatedKeyUpdated(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/**
	 * @brief Gathers all the Localization keys available in the GameTargetSet LocalizationTargets which are correctly configured
	 */
	TValueOrError<TArray<FLocalizationKey>, FText> GatherLocalKeys() const;
	/**
	 * @brief Gathers all the localization keys available in the specified LocalizationTargets
	 */
	TArray<FLocalizationKey> GetKeysFromTargets(TArray<ULocalizationTarget*> LocalizationTargets) const;
	/**
	 * @brief Determines which LocalizationTargets from the GameTargetSet are correctly configured
	 */
	TArray<ULocalizationTarget*> GatherValidLocalizationTargets() const;
	/**
	 * @brief Callback executed when the editor main frame is ready to display the login pop-up
	 */
	void OnMainFrameReady(TSharedPtr<SWindow> InRootWindow, bool bIsRunningStartupDialog);
	/**
	 * @brief Exports the locally available data to a file on disk to package it in the final build
	 */
	void ExportLocalTranslations();

	// Begin UEditorSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// End UEditorSubsystem interface
};
