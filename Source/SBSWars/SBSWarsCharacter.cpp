#include "SBSWarsCharacter.h"
#include "Net/UnrealNetwork.h"

ASBSWarsCharacter::ASBSWarsCharacter()
{
	bReplicates = true;
	SetReplicateMovement(true);
}

void ASBSWarsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASBSWarsCharacter, Faction);
	DOREPLIFETIME(ASBSWarsCharacter, ClassId);
	DOREPLIFETIME(ASBSWarsCharacter, Health);
}
