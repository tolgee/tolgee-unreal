// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeLocalizationProvider.h"

#include <DetailCategoryBuilder.h>
#include <ILocalizationServiceModule.h>
#include <IStructureDetailsView.h>
#include <Interfaces/IMainFrameModule.h>
#include <LocalizationCommandletTasks.h>
#include <LocalizationTargetTypes.h>

#include "TolgeeProviderLocalizationServiceCommand.h"
#include "TolgeeProviderLocalizationServiceWorker.h"
#include "TolgeeEditorSettings.h"

FString GetTempSubDirectory()
{
	return FPaths::ProjectSavedDir() / TEXT("Tolgee") / TEXT("Temp");
}

void FTolgeeLocalizationProvider::Init(bool bForceConnection)
{
}

void FTolgeeLocalizationProvider::Close()
{
}

const FName& FTolgeeLocalizationProvider::GetName() const
{
	static const FName ProviderName = "Tolgee";
	return ProviderName;
}

const FText FTolgeeLocalizationProvider::GetDisplayName() const
{
	return NSLOCTEXT("Tolgee", "TolgeeProviderName", "Tolgee");
}

FText FTolgeeLocalizationProvider::GetStatusText() const
{
	checkf(false, TEXT("Not used in 5.5+"));
	return FText::GetEmpty();
}

bool FTolgeeLocalizationProvider::IsEnabled() const
{
	return true;
}

bool FTolgeeLocalizationProvider::IsAvailable() const
{
	return true;
}

ELocalizationServiceOperationCommandResult::Type FTolgeeLocalizationProvider::GetState(const TArray<FLocalizationServiceTranslationIdentifier>& InTranslationIds,
                                                                                       TArray<TSharedRef<ILocalizationServiceState>>& OutState,
                                                                                       ELocalizationServiceCacheUsage::Type InStateCacheUsage)
{
	return ELocalizationServiceOperationCommandResult::Succeeded;
}

ELocalizationServiceOperationCommandResult::Type FTolgeeLocalizationProvider::Execute(const TSharedRef<ILocalizationServiceOperation>& InOperation, const TArray<FLocalizationServiceTranslationIdentifier>& InTranslationIds, ELocalizationServiceOperationConcurrency::Type InConcurrency, const FLocalizationServiceOperationComplete& InOperationCompleteDelegate)
{
	// Query to see if the we allow this operation
	TSharedPtr<ITolgeeProviderLocalizationServiceWorker> Worker = CreateWorker(InOperation->GetName());
	if (!Worker.IsValid())
	{
		// this operation is unsupported by this source control provider
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("OperationName"), FText::FromName(InOperation->GetName()));
		Arguments.Add(TEXT("ProviderName"), FText::FromName(GetName()));
		FText Message(FText::Format(INVTEXT("Operation '{OperationName}' not supported by revision control provider '{ProviderName}'"), Arguments));
		FMessageLog("LocalizationService").Error(Message);

		(void)InOperationCompleteDelegate.ExecuteIfBound(InOperation, ELocalizationServiceOperationCommandResult::Failed);
		return ELocalizationServiceOperationCommandResult::Failed;
	}

	FTolgeeProviderLocalizationServiceCommand* Command = new FTolgeeProviderLocalizationServiceCommand(InOperation, Worker.ToSharedRef());
	Command->OperationCompleteDelegate = InOperationCompleteDelegate;

	// fire off operation
	if (InConcurrency == ELocalizationServiceOperationConcurrency::Synchronous)
	{
		Command->bAutoDelete = false;

		return ExecuteSynchronousCommand(*Command);
	}

	Command->bAutoDelete = true;
	return IssueCommand(*Command);
}

bool FTolgeeLocalizationProvider::CanCancelOperation(const TSharedRef<ILocalizationServiceOperation>& InOperation) const
{
	return false;
}

void FTolgeeLocalizationProvider::CancelOperation(const TSharedRef<ILocalizationServiceOperation>& InOperation)
{
}

void FTolgeeLocalizationProvider::Tick()
{
	bool bStatesUpdated = false;
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FTolgeeProviderLocalizationServiceCommand& Command = *CommandQueue[CommandIndex];
		if (Command.bExecuteProcessed)
		{
			// Remove command from the queue
			CommandQueue.RemoveAt(CommandIndex);

			// let command update the states of any files
			bStatesUpdated |= Command.Worker->UpdateStates();

			// dump any messages to output log
			OutputCommandMessages(Command);

			Command.ReturnResults();

			// commands that are left in the array during a tick need to be deleted
			if (Command.bAutoDelete)
			{
				// Only delete commands that are not running 'synchronously'
				delete &Command;
			}

			// only do one command per tick loop, as we dont want concurrent modification
			// of the command queue (which can happen in the completion delegate)
			break;
		}
	}

	// NOTE: Currently. the ILocalizationServiceProvider doesn't have a StateChanged delegate like the ISourceControlProvider, but it might get one in the future.
	//if (bStatesUpdated)
	//{
	//	OnSourceControlStateChanged.Broadcast();
	//}
}

void FTolgeeLocalizationProvider::CustomizeSettingsDetails(IDetailCategoryBuilder& DetailCategoryBuilder) const
{
	UTolgeeEditorSettings* MutableSettings = GetMutableDefault<UTolgeeEditorSettings>();

	FAddPropertyParams AddPropertyParams;
	AddPropertyParams.HideRootObjectNode(true);
	DetailCategoryBuilder.AddExternalObjectProperty({MutableSettings}, GET_MEMBER_NAME_CHECKED(UTolgeeEditorSettings, ApiUrl), EPropertyLocation::Common, AddPropertyParams);
	DetailCategoryBuilder.AddExternalObjectProperty({MutableSettings}, GET_MEMBER_NAME_CHECKED(UTolgeeEditorSettings, ApiKey), EPropertyLocation::Common, AddPropertyParams);
}

void FTolgeeLocalizationProvider::CustomizeTargetDetails(IDetailCategoryBuilder& DetailCategoryBuilder, TWeakObjectPtr<ULocalizationTarget> LocalizationTarget) const
{
	checkf(false, TEXT("Not used in 5.5+"));
}

void FTolgeeLocalizationProvider::CustomizeTargetToolbar(TSharedRef<FExtender>& MenuExtender, TWeakObjectPtr<ULocalizationTarget> LocalizationTarget) const
{
	FTolgeeLocalizationProvider* ThisProvider = const_cast<FTolgeeLocalizationProvider*>(this);

	MenuExtender->AddToolBarExtension(
		"LocalizationService",
		EExtensionHook::First,
		nullptr,
		FToolBarExtensionDelegate::CreateRaw(ThisProvider, &FTolgeeLocalizationProvider::AddTargetToolbarButtons, LocalizationTarget)
	);
}

void FTolgeeLocalizationProvider::CustomizeTargetSetToolbar(TSharedRef<FExtender>& MenuExtender, TWeakObjectPtr<ULocalizationTargetSet> LocalizationTargetSet) const
{
	FTolgeeLocalizationProvider* ThisProvider = const_cast<FTolgeeLocalizationProvider*>(this);

	MenuExtender->AddToolBarExtension(
		"LocalizationService",
		EExtensionHook::First,
		nullptr,
		FToolBarExtensionDelegate::CreateRaw(ThisProvider, &FTolgeeLocalizationProvider::AddTargetSetToolbarButtons, LocalizationTargetSet)
	);
}

void FTolgeeLocalizationProvider::AddTargetToolbarButtons(FToolBarBuilder& ToolbarBuilder, TWeakObjectPtr<ULocalizationTarget> InLocalizationTarget)
{
	if (InLocalizationTarget->IsMemberOfEngineTargetSet())
	{
		return;
	}

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTolgeeLocalizationProvider::ImportAllCulturesForTargetFromTolgee, InLocalizationTarget)
		),
		NAME_None,
		NSLOCTEXT("Tolgee", "TolgeeImportTarget", "Tolgee Pull"),
		NSLOCTEXT("Tolgee", "TolgeeImportTargetTip", "Imports all cultures for this target from Tolgee"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LocalizationDashboard.ImportTextAllTargetsAllCultures")
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTolgeeLocalizationProvider::ExportAllCulturesForTargetToTolgee, InLocalizationTarget)
		),
		NAME_None,
		NSLOCTEXT("Tolgee", "TolgeeExportTarget", "Tolgee Push"),
		NSLOCTEXT("Tolgee", "TolgeeExportTargetTip", "Exports all cultures for this target to Tolgee"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LocalizationDashboard.ExportTextAllTargetsAllCultures")
	);

	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateRaw(this, &FTolgeeLocalizationProvider::CreateProjectSettingsWidget, InLocalizationTarget),
		NSLOCTEXT("Tolgee", "TolgeeSettings", "Tolgee Settings"),
		NSLOCTEXT("Tolgee", "TolgeeSettings", "Tolgee Settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LocalizationDashboard.CompileTextAllTargetsAllCultures")
	);
}

void FTolgeeLocalizationProvider::AddTargetSetToolbarButtons(FToolBarBuilder& ToolbarBuilder, TWeakObjectPtr<ULocalizationTargetSet> InLocalizationTargetSet)
{
	if (InLocalizationTargetSet.IsValid() && InLocalizationTargetSet->TargetObjects.Num() > 0 && InLocalizationTargetSet->TargetObjects[0]->IsMemberOfEngineTargetSet())
	{
		return;
	}

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTolgeeLocalizationProvider::ImportAllTargetsForSetFromTolgee, InLocalizationTargetSet)
		),
		NAME_None,
		NSLOCTEXT("Tolgee", "TolgeeImportSet", "Tolgee Pull"),
		NSLOCTEXT("Tolgee", "TolgeeImportSetTip", "Imports all targets for this set from Tolgee"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LocalizationDashboard.ImportTextAllTargetsAllCultures")
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTolgeeLocalizationProvider::ExportAllTargetsForSetToTolgee, InLocalizationTargetSet)
		),
		NAME_None,
		NSLOCTEXT("Tolgee", "TolgeeExportSet", "Tolgee Push"),
		NSLOCTEXT("Tolgee", "TolgeeExportSetTip", "Exports all targets for this set to Tolgee"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LocalizationDashboard.ExportTextAllTargetsAllCultures")
	);

}

void FTolgeeLocalizationProvider::ImportAllCulturesForTargetFromTolgee(TWeakObjectPtr<ULocalizationTarget> LocalizationTarget)
{
	// Delete old files if they exists so we don't accidentally export old data
	const FString AbsoluteFolderPath = FPaths::ConvertRelativePathToFull(GetTempSubDirectory() / LocalizationTarget->Settings.Name);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteDirectoryRecursively(*AbsoluteFolderPath);

	TArray<FCultureStatistics> CultureStats = LocalizationTarget->Settings.SupportedCulturesStatistics;
	FScopedSlowTask SlowTask(CultureStats.Num(), INVTEXT("Downloading Files from Localization Service..."));
	for (FCultureStatistics CultureStat : CultureStats)
	{
		ILocalizationServiceProvider& Provider = ILocalizationServiceModule::Get().GetProvider();
		TSharedRef<FDownloadLocalizationTargetFile> DownloadTargetFileOp = ILocalizationServiceOperation::Create<FDownloadLocalizationTargetFile>();
		DownloadTargetFileOp->SetInTargetGuid(LocalizationTarget->Settings.Guid);
		DownloadTargetFileOp->SetInLocale(CultureStat.CultureName);

		// NOTE: For some reason the base FDownloadLocalizationTargetFile prefers relative paths, so we will make it relative
		FString Path = AbsoluteFolderPath / CultureStat.CultureName / LocalizationTarget->Settings.Name + ".po";
		FPaths::MakePathRelativeTo(Path, *FPaths::ProjectDir());
		DownloadTargetFileOp->SetInRelativeOutputFilePathAndName(Path);

		Provider.Execute(DownloadTargetFileOp);
		SlowTask.EnterProgressFrame(1, FText::Format(INVTEXT("Downloading {0}"), FText::FromString(CultureStat.CultureName)));
	}

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	LocalizationCommandletTasks::ImportTextForTarget(MainFrameParentWindow.ToSharedRef(), LocalizationTarget.Get(), AbsoluteFolderPath);
	UpdateTargetFromReports(LocalizationTarget);
}

void FTolgeeLocalizationProvider::ExportAllCulturesForTargetToTolgee(TWeakObjectPtr<ULocalizationTarget> LocalizationTarget)
{
	// Delete old files if they exists so we don't accidentally export old data
	const FString AbsoluteFolderPath = FPaths::ConvertRelativePathToFull(GetTempSubDirectory() / LocalizationTarget->Settings.Name);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteDirectoryRecursively(*AbsoluteFolderPath);

	// Currently Unreal's format uses msgctx which is not supported by Tolgee, so we will use Crowdin format instead which doesn't use it.
	// TODO: Revisit after https://github.com/tolgee/tolgee-platform/issues/3053
	LocalizationTarget->Settings.ExportSettings.POFormat = EPortableObjectFormat::Crowdin;

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	LocalizationCommandletTasks::ExportTextForTarget(MainFrameParentWindow.ToSharedRef(), LocalizationTarget.Get(), AbsoluteFolderPath);

	TArray<FCultureStatistics> CultureStats = LocalizationTarget->Settings.SupportedCulturesStatistics;
	FScopedSlowTask SlowTask(CultureStats.Num(), INVTEXT("Uploading Files to Localization Service..."));
	for (FCultureStatistics CultureStat : CultureStats)
	{
		ILocalizationServiceProvider& Provider = ILocalizationServiceModule::Get().GetProvider();
		TSharedRef<FUploadLocalizationTargetFile> UploadFileOp = ILocalizationServiceOperation::Create<FUploadLocalizationTargetFile>();
		UploadFileOp->SetInTargetGuid(LocalizationTarget->Settings.Guid);
		UploadFileOp->SetInLocale(CultureStat.CultureName);

		// NOTE: For some reason the base FUploadLocalizationTargetFile prefers relative paths, so we will make it relative
		FString Path = AbsoluteFolderPath / CultureStat.CultureName / LocalizationTarget->Settings.Name + ".po";
		FPaths::MakePathRelativeTo(Path, *FPaths::ProjectDir());
		UploadFileOp->SetInRelativeInputFilePathAndName(Path);

		Provider.Execute(UploadFileOp);
		SlowTask.EnterProgressFrame(1, FText::Format(INVTEXT("Uploading {0}"), FText::FromString(CultureStat.CultureName)));
	}
}

void FTolgeeLocalizationProvider::ImportAllTargetsForSetFromTolgee(TWeakObjectPtr<ULocalizationTargetSet> LocalizationTargetSet)
{
	for (ULocalizationTarget* LocalizationTarget : LocalizationTargetSet->TargetObjects)
	{
		ImportAllCulturesForTargetFromTolgee(LocalizationTarget);
	}
}

void FTolgeeLocalizationProvider::ExportAllTargetsForSetToTolgee(TWeakObjectPtr<ULocalizationTargetSet> LocalizationTargetSet)
{
	for (ULocalizationTarget* LocalizationTarget : LocalizationTargetSet->TargetObjects)
	{
		ExportAllCulturesForTargetToTolgee(LocalizationTarget);
	}
}

TSharedRef<SWidget> FTolgeeLocalizationProvider::CreateProjectSettingsWidget(TWeakObjectPtr<ULocalizationTarget> InLocalizationTarget)
{
	const FGuid& TargetGuid = InLocalizationTarget->Settings.Guid;

	UTolgeeEditorSettings* MutableSettings = GetMutableDefault<UTolgeeEditorSettings>();
	FTolgeePerTargetSettings ProjectSettings = MutableSettings->PerTargetSettings.FindOrAdd(TargetGuid);

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.bShowScrollBar = false;

	FStructureDetailsViewArgs StructureViewArgs;

	TSharedPtr<TStructOnScope<FTolgeePerTargetSettings>> Struct = MakeShared<TStructOnScope<FTolgeePerTargetSettings>>();
	Struct->InitializeAs<FTolgeePerTargetSettings>(ProjectSettings);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TSharedPtr<IStructureDetailsView> StructureDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, Struct);

	StructureDetailsView->GetOnFinishedChangingPropertiesDelegate().AddLambda([MutableSettings, Struct, TargetGuid](const FPropertyChangedEvent& PropertyChangedEvent)
	{
		MutableSettings->PerTargetSettings[TargetGuid] = *Struct->Cast<FTolgeePerTargetSettings>();
		MutableSettings->SaveConfig();
	});

	return StructureDetailsView->GetWidget().ToSharedRef();
}

void FTolgeeLocalizationProvider::UpdateTargetFromReports(TWeakObjectPtr<ULocalizationTarget> InLocalizationTarget)
{
	// NOTE: This function seems to be copy pasted in a lot of places FLocalizationTargetDetailCustomization/SLocalizationTargetEditorCultureRow(original), FLocalizationTargetSetDetailCustomizationm, UpdateTargetFromReports 
	ULocalizationTarget* LocalizationTarget = InLocalizationTarget.Get();
	LocalizationTarget->UpdateWordCountsFromCSV();
	LocalizationTarget->UpdateStatusFromConflictReport();
}

void FTolgeeLocalizationProvider::OutputCommandMessages(const FTolgeeProviderLocalizationServiceCommand& InCommand) const
{
	FMessageLog LocalizationServiceLog("LocalizationService");

	for (int32 ErrorIndex = 0; ErrorIndex < InCommand.ErrorMessages.Num(); ++ErrorIndex)
	{
		LocalizationServiceLog.Error(FText::FromString(InCommand.ErrorMessages[ErrorIndex]));
	}

	for (int32 InfoIndex = 0; InfoIndex < InCommand.InfoMessages.Num(); ++InfoIndex)
	{
		LocalizationServiceLog.Info(FText::FromString(InCommand.InfoMessages[InfoIndex]));
	}
}

TSharedPtr<ITolgeeProviderLocalizationServiceWorker> FTolgeeLocalizationProvider::CreateWorker(const FName& InOperationName) const
{
	if (const FGetTolgeeProviderLocalizationServiceWorker* Operation = WorkersMap.Find(InOperationName))
	{
		return Operation->Execute();
	}

	return nullptr;
}

ELocalizationServiceOperationCommandResult::Type FTolgeeLocalizationProvider::ExecuteSynchronousCommand(FTolgeeProviderLocalizationServiceCommand& InCommand)
{
	ELocalizationServiceOperationCommandResult::Type Result = ELocalizationServiceOperationCommandResult::Failed;

	// Display the progress dialog if a string was provided
	{
		// Issue the command asynchronously...
		IssueCommand(InCommand);

		// ... then wait for its completion (thus making it synchronous)
		while (!InCommand.bExecuteProcessed)
		{
			// Tick the command queue and update progress.
			Tick();

			// Sleep for a bit so we don't busy-wait so much.
			FPlatformProcess::Sleep(0.01f);
		}

		// always do one more Tick() to make sure the command queue is cleaned up.
		Tick();

		if (InCommand.bCommandSuccessful)
		{
			Result = ELocalizationServiceOperationCommandResult::Succeeded;
		}
	}

	// Delete the command now (asynchronous commands are deleted in the Tick() method)
	check(!InCommand.bAutoDelete);

	// ensure commands that are not auto deleted do not end up in the command queue
	if (CommandQueue.Contains(&InCommand))
	{
		CommandQueue.Remove(&InCommand);
	}
	delete &InCommand;

	return Result;
}

ELocalizationServiceOperationCommandResult::Type FTolgeeLocalizationProvider::IssueCommand(FTolgeeProviderLocalizationServiceCommand& InCommand)
{
	if (GThreadPool != nullptr)
	{
		// Queue this to our worker thread(s) for resolving
		GThreadPool->AddQueuedWork(&InCommand);
		CommandQueue.Add(&InCommand);
		return ELocalizationServiceOperationCommandResult::Succeeded;
	}
	else
	{
		FText Message(INVTEXT("There are no threads available to process the localization service provider command."));
		FMessageLog("LocalizationService").Error(Message);

		InCommand.bCommandSuccessful = false;
		return InCommand.ReturnResults();
	}
}
