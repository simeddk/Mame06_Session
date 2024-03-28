#include "FPSGameMode.h"
#include "Global.h"
#include "FPSHUD.h"
#include "Characters/FPSCharacter.h"
#include "Engine/TargetPoint.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Actors/CSpawnPoint.h"
#include "CGameState.h"
#include "CPlayerState.h"

AFPSGameMode::AFPSGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Player/BP_CPlayer"));

	DefaultPawnClass = PlayerPawnClassFinder.Class;
	HUDClass = AFPSHUD::StaticClass();
	GameStateClass = ACGameState::StaticClass();
	PlayerStateClass = ACPlayerState::StaticClass();
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
}

void AFPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AFPSCharacter* playerPawn = Cast<AFPSCharacter>(NewPlayer->GetPawn());
	ACPlayerState* playerState = Cast<ACPlayerState>(NewPlayer->PlayerState);

	if (!!playerPawn && !!playerState)
	{
		//Set PlayerState to Character from Controller
		playerPawn->SetPlayerState(playerState);

		//Team Contact
		if (RedTeamCharacters.Num() > BlueTeamCharacters.Num())
		{
			BlueTeamCharacters.Add(playerPawn);
			playerState->Team = ETeamType::Blue;

			NewPlayer->StartSpot = FindPlayerStart(NewPlayer, "BlueTeam");
		}
		else
		{
			RedTeamCharacters.Add(playerPawn);
			playerState->Team = ETeamType::Red;

			NewPlayer->StartSpot = FindPlayerStart(NewPlayer, "RedTeam");
		}
		
		
		//Team information to Character
		playerPawn->CurrentTeam = playerState->Team;
		playerPawn->SetTeamColor(playerState->Team);

		for (TActorIterator<ACSpawnPoint> iter(GetWorld()); iter; ++iter)
		{
			if (iter->GetTeam() == ETeamType::Red)
				RedTeamSpawnPoints.Add(*iter);
			else
				BlueTeamSpawnPoints.Add(*iter);
		}

		//Move To Spawn Point
		MoveToSpawnPoint(playerPawn);
		SpawnHost();
	}
}

void AFPSGameMode::SpawnHost()
{
	UWorld* world = GetWorld();
	if (!!world)
	{
		APlayerController* hostController = world->GetFirstPlayerController();
		if (!!hostController)
		{
			APawn* hostPawn = hostController->GetPawn();
			AFPSCharacter* hostCharacter = Cast<AFPSCharacter>(hostPawn);

			if (!!hostCharacter)
				MoveToSpawnPoint(hostCharacter);
		}
	}
}

void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	
}

void AFPSGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (WaitingForSpawnCharacters.Num() > 0)
	{
		for (const auto& waitingCharacter : WaitingForSpawnCharacters)
			MoveToSpawnPoint(waitingCharacter);
	}
}

void AFPSGameMode::MoveToSpawnPoint(AFPSCharacter* InPlayer)
{
	TArray<AActor*> centerPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), centerPoints);

	TArray<ACSpawnPoint*>* targetPoints;

	if (InPlayer->CurrentTeam == ETeamType::Red)
		targetPoints = &RedTeamSpawnPoints;
	else
		targetPoints = &BlueTeamSpawnPoints;

	for (const auto& point : *targetPoints)
	{
		//SpawnPoint is not blocked
		if (point->IsBlocked() == false)
		{
			InPlayer->SetActorLocation(point->GetActorLocation());

			point->UpdateOverlaps();

			if (WaitingForSpawnCharacters.Find(InPlayer) >= 0)
				WaitingForSpawnCharacters.Remove(InPlayer);

			return;
		}
	}

	//SpawnPoint is blocked
	if (WaitingForSpawnCharacters.Find(InPlayer) < 0)
		WaitingForSpawnCharacters.Add(InPlayer);
}
