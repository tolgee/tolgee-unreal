// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "STolgeeSyncDialog.h"

#include <Brushes/SlateRoundedBoxBrush.h>
#include <Styling/StyleColors.h>
#include <Widgets/Layout/SUniformGridPanel.h>

void STolgeeSyncDialog::Construct(const FArguments& InArgs, int NumToUpload, int NumToUpdate, int NumToDelete)
{
	UploadNew.Title = INVTEXT("Upload new");
	UploadNew.NumberOfKeys = NumToUpload;

	UpdateOutdated.Title = INVTEXT("Update outdated");
	UpdateOutdated.NumberOfKeys = NumToUpdate;

	DeleteUnused.Title = INVTEXT("Delete unused");
	DeleteUnused.NumberOfKeys = NumToDelete;

	SWindow::Construct(SWindow::FArguments()
	                   .Title(INVTEXT("Tolgee Sync"))
	                   .SizingRule(ESizingRule::Autosized)
	                   .SupportsMaximize(false)
	                   .SupportsMinimize(false)
		[
			SNew(SBorder)
			.Padding(4.f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					MakeOperationCheckBox(UploadNew)
				]

				+ SVerticalBox::Slot()
				[
					MakeOperationCheckBox(UpdateOutdated)
				]

				+ SVerticalBox::Slot()
				[
					MakeOperationCheckBox(DeleteUnused)
				]

				+ SVerticalBox::Slot()
				[
					SNew(SSpacer)
				]

				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Text(this, &STolgeeSyncDialog::GetOperationsSummary)
				]

				+ SVerticalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(8)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SAssignNew(RunButton, SButton)
						.Text(INVTEXT("Run"))
						.OnClicked(this, &STolgeeSyncDialog::OnRunClicked)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.Text(INVTEXT("Cancel"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &STolgeeSyncDialog::OnCancelClicked)
					]
				]
			]
		]);

	RefreshRunButton();
}

void STolgeeSyncDialog::RefreshRunButton()
{
	constexpr float InputFocusThickness = 1.0f;
	static FButtonStyle BaseButton = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

	static FButtonStyle UploadButton = BaseButton
	                                   .SetNormal(FSlateRoundedBoxBrush(FStyleColors::Primary, 4.0f, FStyleColors::Input, InputFocusThickness))
	                                   .SetHovered(FSlateRoundedBoxBrush(FStyleColors::PrimaryHover, 4.0f, FStyleColors::Input, InputFocusThickness))
	                                   .SetPressed(FSlateRoundedBoxBrush(FStyleColors::PrimaryPress, 4.0f, FStyleColors::Input, InputFocusThickness));


	static FButtonStyle DeleteButton = BaseButton
	                                   .SetNormal(FSlateRoundedBoxBrush(COLOR("#E00000FF"), 4.0f, FStyleColors::Input, InputFocusThickness))
	                                   .SetHovered(FSlateRoundedBoxBrush(COLOR("FF0F0EFF"), 4.0f, FStyleColors::Input, InputFocusThickness))
	                                   .SetPressed(FSlateRoundedBoxBrush(COLOR("a00000"), 4.0f, FStyleColors::Input, InputFocusThickness));

	static FButtonStyle WarningButton = BaseButton
	                                    .SetNormal(FSlateRoundedBoxBrush(COLOR("#E07000FF"), 4.0f, FStyleColors::Input, InputFocusThickness))
	                                    .SetHovered(FSlateRoundedBoxBrush(COLOR("#FF870EFF"), 4.0f, FStyleColors::Input, InputFocusThickness))
	                                    .SetPressed(FSlateRoundedBoxBrush(COLOR("#A05000FF"), 4.0f, FStyleColors::Input, InputFocusThickness));

	if (DeleteUnused.bPerform)
	{
		RunButton->SetEnabled(true);
		RunButton->SetButtonStyle(&DeleteButton);
	}
	else if (UpdateOutdated.bPerform)
	{
		RunButton->SetEnabled(true);
		RunButton->SetButtonStyle(&WarningButton);
	}
	else if (UploadNew.bPerform)
	{
		RunButton->SetEnabled(true);
		RunButton->SetButtonStyle(&UploadButton);
	}
	else
	{
		RunButton->SetEnabled(false);
		RunButton->SetButtonStyle(&BaseButton);
	}
}

FReply STolgeeSyncDialog::OnRunClicked()
{
	RequestDestroyWindow();

	return FReply::Handled();
}

FReply STolgeeSyncDialog::OnCancelClicked()
{
	UploadNew.bPerform = false;
	UpdateOutdated.bPerform = false;
	DeleteUnused.bPerform = false;

	RequestDestroyWindow();

	return FReply::Handled();
}

FText STolgeeSyncDialog::GetOperationsSummary() const
{
	return FText::Format(INVTEXT("{0} operations will be performed"), GetNumberOfOperationToPerform());
}

int STolgeeSyncDialog::GetNumberOfOperationToPerform() const
{
	int Total = 0;
	if (UploadNew.bPerform)
	{
		Total += UploadNew.NumberOfKeys;
	}
	if (UpdateOutdated.bPerform)
	{
		Total += UpdateOutdated.NumberOfKeys;
	}
	if (DeleteUnused.bPerform)
	{
		Total += DeleteUnused.NumberOfKeys;
	}

	return Total;
}

TSharedRef<SCheckBox> STolgeeSyncDialog::MakeOperationCheckBox(FTolgeeOperation& Operation)
{
	return SNew(SCheckBox)
		.IsChecked(this, &STolgeeSyncDialog::IsOperationChecked, &Operation)
		.IsEnabled(this, &STolgeeSyncDialog::IsOperationEnabled, &Operation)
		.OnCheckStateChanged(this, &STolgeeSyncDialog::OnOperationStateChanged, &Operation)
		[
			SNew(STextBlock)
			.Text(this, &STolgeeSyncDialog::GetOperationName, &Operation)
		];
}

void STolgeeSyncDialog::OnOperationStateChanged(ECheckBoxState NewCheckedState, FTolgeeOperation* Operation)
{
	Operation->bPerform = NewCheckedState == ECheckBoxState::Checked;

	RefreshRunButton();
}

ECheckBoxState STolgeeSyncDialog::IsOperationChecked(FTolgeeOperation* Operation) const
{
	return Operation->bPerform ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool STolgeeSyncDialog::IsOperationEnabled(FTolgeeOperation* Operation) const
{
	return Operation->NumberOfKeys > 0;
}

FText STolgeeSyncDialog::GetOperationName(FTolgeeOperation* Operation) const
{
	return FText::Format(INVTEXT("{0}({1})"), Operation->Title, Operation->NumberOfKeys);

}