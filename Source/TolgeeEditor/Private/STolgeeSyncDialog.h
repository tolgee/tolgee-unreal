// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

struct FTolgeeOperation
{
	FText Title;
	int NumberOfKeys = 0;
	bool bPerform = false;
};

class STolgeeSyncDialog : public SWindow
{
public:
	SLATE_BEGIN_ARGS(STolgeeSyncDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, int NumToUpload, int NumToUpdate, int NumToDelete);

	void RefreshRunButton();

	FReply OnRunClicked();
	FReply OnCancelClicked();

	FText GetOperationsSummary() const;
	int GetNumberOfOperationToPerform() const;

	TSharedRef<SCheckBox> MakeOperationCheckBox(FTolgeeOperation& Operation);
	void OnOperationStateChanged(ECheckBoxState NewCheckedState, FTolgeeOperation* Operation);
	ECheckBoxState IsOperationChecked(FTolgeeOperation* Operation) const;
	bool IsOperationEnabled(FTolgeeOperation* Operation) const;
	FText GetOperationName(FTolgeeOperation* Operation) const;

	TSharedPtr<SButton> RunButton;

	FTolgeeOperation UploadNew;
	FTolgeeOperation UpdateOutdated;
	FTolgeeOperation DeleteUnused;
};