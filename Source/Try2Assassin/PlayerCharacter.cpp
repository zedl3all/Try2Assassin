// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

APlayerCharacter::APlayerCharacter()
{
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	USkeletalMeshComponent* MainMesh = GetMesh();

	if (!MainMesh)
	{
		return;
	}

	TArray<USkeletalMeshComponent*> MeshComponents;
	GetComponents<USkeletalMeshComponent>(MeshComponents);

	for (USkeletalMeshComponent* ChildMesh : MeshComponents)
	{
		if (!ChildMesh || ChildMesh == MainMesh)
		{
			continue;
		}

		const FString ComponentName = ChildMesh->GetName();

		if (ComponentName == TEXT("rogue_head") ||
			ComponentName == TEXT("rogue_hood") ||
			ComponentName == TEXT("rogue_mantle") ||
			ComponentName == TEXT("rogue_shoulderpad"))
		{
			ChildMesh->SetLeaderPoseComponent(MainMesh);
		}
	}
}
