// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

/**
 * @brief Represents details about a possible operation (Upload, Update, Delete) 
 */
struct FTolgeeOperation
{
	/**
	 * Name of the operation displayed to the developer
	 */
	FText Title;
	/**
	 * Number of keys this operation will be affected by running this operation
	 */
	int NumberOfKeys = 0;
	/**
	 * Should we perform this operation after the dialog closes?
	 */
	bool bPerform = false;
};

/**
 * Window used to display the Sync dialog to chose what operations should be performed 
 */
class STolgeeSyncDialog : public SWindow
{
public:
	SLATE_BEGIN_ARGS(STolgeeSyncDialog) {}
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the sync operations widget 
	 */
	void Construct(const FArguments& InArgs, int NumToUpload, int NumToUpdate, int NumToDelete);
	/**
	 * Refreshes the Run button style based on the operation that will be executed
	 */
	void RefreshRunButton();
	/**
	 * Callback executed when the Run button is clicked
	 */
	FReply OnRunClicked();
	/**
	 * Callback executed when the Cancel button is clicked
	 */
	FReply OnCancelClicked();
	/**
	 * Constructs a text explaining all the operations that will be performed if the currently selected operations will be run
	 */
	FText GetOperationsSummary() const;
	/**
	 * Calculates the total number of keys that will be affected if the currently selected operations will be run
	 */
	int GetNumberOfKeysAffectedByOperations() const;
	/**
	 * Constructs a user-facing checkbox to enable/disable an operation
	 */
	TSharedRef<SCheckBox> MakeOperationCheckBox(FTolgeeOperation& Operation);
	/**
	 * Callback executed when a new checkbox state is set for an operation 
	 */
	void OnOperationStateChanged(ECheckBoxState NewCheckedState, FTolgeeOperation* Operation);
	/**
	 * Callback executed to determine the state of a checkbox for an operation 
	 */
	ECheckBoxState IsOperationChecked(FTolgeeOperation* Operation) const;
	/**
	 * Callback executed to determine if a checkbox is enabled for an operation 
	 */
	bool IsOperationEnabled(FTolgeeOperation* Operation) const;
	/**
	 * Callback executed to determine a checkbox title for an operation 
	 */
	FText GetOperationName(FTolgeeOperation* Operation) const;
	/**
	 * Reference to the used to Run the selected operations 
	 */
	TSharedPtr<SButton> RunButton;
	/**
	 * State of the UploadNew operation
	 */
	FTolgeeOperation UploadNew;
	/**
	 * State of the UpdateOutdated operation
	 */
	FTolgeeOperation UpdateOutdated;
	/**
	 * State of the DeleteUnused operation
	 */
	FTolgeeOperation DeleteUnused;
};