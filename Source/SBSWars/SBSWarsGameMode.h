#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SBSWarsGameMode.generated.h"

UENUM(BlueprintType)
enum class ESBSGameModeId : uint8
{
	Deathmatch UMETA(DisplayName="Deathmatch"),
	TeamDeathmatch UMETA(DisplayName="Team Deathmatch"),
	CaptureTheFlag UMETA(DisplayName="Capture The Flag"),
	Domination UMETA(DisplayName="Domination"),
	KingOfTheHill UMETA(DisplayName="King Of The Hill")
};

UCLASS()
class SBSWARS_API ASBSWarsGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ASBSWarsGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SBS")
	ESBSGameModeId ModeId = ESBSGameModeId::TeamDeathmatch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SBS")
	int32 ScoreLimit = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SBS")
	int32 MaxPlayers = 64;

	virtual void BeginPlay() override;
};
