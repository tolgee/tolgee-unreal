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
	 * @brief Uploads keys present locally that are not yet present on the Tolgee backend
	 */
	void UploadMissingKeys();
	/**
	 * @brief Delete keys present on the Tolgee backend that are no longer present locally
	 */
	void PurgeUnusedKeys();

private:
	/**
	 * @brief Determine which local keys are not present in the remote configuration
	 */
	TArray<FLocalizationKey> GetMissingLocalKeys() const;
	/**
	 * @brief Gathers all the Localization keys available in the GameTargetSet LocalizationTargets which are correctly configured
	 */
	TArray<FLocalizationKey> GatherLocalKeys() const;
	/**
	 * @brief Gathers all the localization keys available in the specified LocalizationTargets
	 */
	TArray<FLocalizationKey> GetKeysFromTargets(TArray<ULocalizationTarget*> LocalizationTargets) const;
	/**
	 * @brief Determines which LocalizationTargets from the GameTargetSet are correctly configured
	 */
	TArray<ULocalizationTarget*> GatherValidLocalizationTargets() const;
	/**
	 * @brief Callback executed when the Keys upload to Tolgee backend is completed
	 */
	void OnMissingKeysUploaded(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/**
	 * @brief Determine which remote keys are not present in the local configuration
	 */
	TArray<FLocalizedKey> GetUnusedRemoteKeys() const;
	/**
	 * @brief Callback executed when the keys deleted from the Tolgee backend is completed
	 */
	void OnUnusedKeysPurged(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/**
	 * @brief Callback executed when the editor main frame is ready to display the login pop-up
	 */
	void OnMainFrameReady();
	/**
	 * @brief Exports the locally available data to a file on disk to package it in the final build
	 */
	void ExportLocalTranslations();

	// Begin UEditorSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// End UEditorSubsystem interface
};
