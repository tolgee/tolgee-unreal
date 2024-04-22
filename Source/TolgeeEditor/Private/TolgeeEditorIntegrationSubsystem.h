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
	/**
	 * Collects the local keys and fetches the remotes keys. After comparing the 2, it lets the users chose how to update the keys
	 */
	void Sync();

private:
	/**
	 * Sends a request to the backend to import new keys
	 */
	void UploadLocalKeys(TArray<FLocalizationKey> NewLocalKeys);
	/**
	 * Callback executed after a request to import new keys is completed 
	 */
	void OnLocalKeysUploaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/**
	 * Sends a request to the backend to delete unused keys
	 */
	void DeleteRemoteKeys(TArray<FLocalizedKey> UnusedRemoteKeys);
	/**
	 * Callback executed after a request to delete unused remote keys is completed 
	 */
	void OnRemoteKeysDeleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/**
	 * Sends multiple requests to the backend to update the outdated keys' tag based on the new source string
	 */
	void UpdateOutdatedKeys(TArray<TPair<FLocalizationKey, FLocalizedKey>> OutdatedKeys);
	/**
	 * Callback executed after a request to update an outdated key's tag is completed
	 */
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
