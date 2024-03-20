#include "CGameInstance.h"
#include "Widgets/CMainMenu.h"
#include "Widgets/CGameMenu.h"
#include "OnlineSessionSettings.h"

const static FName SESSION_NAME = TEXT("GameSession");

UCGameInstance::UCGameInstance(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> menuWidgetClass(TEXT("/Game/Widgets/WB_MainMenu"));
	if (menuWidgetClass.Succeeded())
		MenuWidgetClass = menuWidgetClass.Class;

	ConstructorHelpers::FClassFinder<UUserWidget> gameWidgetClass(TEXT("/Game/Widgets/WB_GameMenu"));
	if (gameWidgetClass.Succeeded())
		GameWidgetClass = gameWidgetClass.Class;
}

void UCGameInstance::Init()
{
	IOnlineSubsystem* oss = IOnlineSubsystem::Get();
	if (!!oss)
	{
		UE_LOG(LogTemp, Error, TEXT("OSS Name : %s"), *oss->GetSubsystemName().ToString());

		SessionInterface = oss->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnCreateSessionCompleted);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnDestroySessionCompleted);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UCGameInstance::OnFindSessionCompleted);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnJoinSessionCompleted);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OSS Not Found"));
	}
}

void UCGameInstance::Host()
{
	if (SessionInterface.IsValid() == false) return;

	auto exsistingSession = SessionInterface->GetNamedSession(SESSION_NAME);
	
	if (!!exsistingSession)
	{
		SessionInterface->DestroySession(SESSION_NAME);
	}
	else
	{
		CreateSession();
	}
}

void UCGameInstance::CreateSession()
{
	if (SessionInterface.IsValid() == false) return;

	FOnlineSessionSettings sessionSettings;
	sessionSettings.bIsLANMatch = true;
	sessionSettings.NumPublicConnections = 5;
	sessionSettings.bShouldAdvertise = true;

	SessionInterface->CreateSession(0, SESSION_NAME, sessionSettings);
}

void UCGameInstance::Join(uint32 InIndex)
{
	if (SessionInterface.IsValid() == false) return;
	if (SessionSearch.IsValid() == false) return;

	if (!!MenuWidget)
		MenuWidget->SetInputGameMode();

	SessionInterface->JoinSession(0, SESSION_NAME, SessionSearch->SearchResults[InIndex]);
}

void UCGameInstance::LoadMainMenu()
{
	if (MenuWidgetClass == nullptr) return;

	MenuWidget = CreateWidget<UCMainMenu>(this, MenuWidgetClass);
	if (MenuWidget == nullptr) return;
	MenuWidget->SetInputUIMode();

	MenuWidget->SetOwingGameInstance(this);
}

void UCGameInstance::LoadGameMenu()
{
	if (GameWidgetClass == nullptr) return;

	GameWidget = CreateWidget<UCGameMenu>(this, GameWidgetClass);
	if (GameWidget == nullptr) return;
	GameWidget->SetInputUIMode();

	GameWidget->SetOwingGameInstance(this);
}

void UCGameInstance::TravelToMainMenu()
{
	APlayerController* controller = GetFirstLocalPlayerController();
	if (controller == nullptr) return;

	controller->ClientTravel("/Game/Maps/MainMenu", ETravelType::TRAVEL_Absolute);
}

void UCGameInstance::ShowJoinableSessionList()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	if (SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Start Find Sessions"));

		SessionSearch->bIsLanQuery = true;
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	}
}

void UCGameInstance::OnCreateSessionCompleted(FName InSessionName, bool InSuccess)
{
	UE_LOG(LogTemp, Error, TEXT("CREATED"));

	if (InSuccess == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not create session!!!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("SessionName : %s"), *InSessionName.ToString());
	
	if (!!MenuWidget)
		MenuWidget->SetInputGameMode();

	UEngine* engine = GetEngine();
	if (engine == nullptr) return;
	engine->AddOnScreenDebugMessage(-1, 2, FColor::Green, TEXT("Host"), true, FVector2D(2));

	UWorld* world = GetWorld();
	if (world == nullptr) return;

	world->ServerTravel("/Game/Maps/Play?listen");
}

void UCGameInstance::OnDestroySessionCompleted(FName InSessionName, bool InSuccess)
{
	UE_LOG(LogTemp, Error, TEXT("DESTROIED"));

	if (SessionInterface.IsValid() == false) return;
	
	CreateSession();
}

void UCGameInstance::OnFindSessionCompleted(bool InSuccess)
{
	if (InSuccess == true && SessionSearch.IsValid() && MenuWidget != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Finished Find Session"));

		TArray<FString> sessionNames;
		for (const auto& searchResult : SessionSearch->SearchResults)
		{
			UE_LOG(LogTemp, Error, TEXT("Found Session ID : %s"), *searchResult.GetSessionIdStr());
			UE_LOG(LogTemp, Error, TEXT("Ping : %d"), searchResult.PingInMs);

			sessionNames.Add(searchResult.GetSessionIdStr());
		}

		MenuWidget->SetSessionList(sessionNames);
	}
}

void UCGameInstance::OnJoinSessionCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type InResult)
{
	FString address;
	if (SessionInterface->GetResolvedConnectString(InSessionName, address) == false)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not resolve IP address"));
		return;
	}

	UEngine* engine = GetEngine();
	if (engine == nullptr) return;
	engine->AddOnScreenDebugMessage(-1, 2, FColor::Red, FString::Printf(TEXT("Join to %s"), *address), true, FVector2D(2));

	APlayerController* controller = GetFirstLocalPlayerController();
	if (controller == nullptr) return;

	controller->ClientTravel(address, ETravelType::TRAVEL_Absolute);
}
