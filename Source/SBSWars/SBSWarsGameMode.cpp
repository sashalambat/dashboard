#include "SBSWarsGameMode.h"
#include "SBSWarsCharacter.h"

ASBSWarsGameMode::ASBSWarsGameMode()
{
	DefaultPawnClass = ASBSWarsCharacter::StaticClass();
	bUseSeamlessTravel = true;
}

void ASBSWarsGameMode::BeginPlay()
{
	Super::BeginPlay();
}
