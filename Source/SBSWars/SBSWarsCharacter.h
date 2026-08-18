#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SBSWarsCharacter.generated.h"

UENUM(BlueprintType)
enum class ESBSFaction : uint8
{
	SBSAlliance,
	CyberDominion
};

UENUM(BlueprintType)
enum class ESBSClass : uint8
{
	Assault,
	Heavy,
	Recon,
	Engineer,
	Medic
};

UCLASS()
class SBSWARS_API ASBSWarsCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	ASBSWarsCharacter();

	UPROPERTY(Replicated, BlueprintReadWrite, Category="SBS")
	ESBSFaction Faction = ESBSFaction::SBSAlliance;

	UPROPERTY(Replicated, BlueprintReadWrite, Category="SBS")
	ESBSClass ClassId = ESBSClass::Assault;

	UPROPERTY(Replicated, BlueprintReadWrite, Category="SBS")
	float Health = 100.f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
