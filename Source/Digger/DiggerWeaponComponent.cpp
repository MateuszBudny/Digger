// Copyright Epic Games, Inc. All Rights Reserved.


#include "DiggerWeaponComponent.h"
#include "DiggerCharacter.h"
#include "DiggerProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UDiggerWeaponComponent::UDiggerWeaponComponent()
{
	// Default offset from the character location for projectiles to spawn
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
}

APlayerController* UDiggerWeaponComponent::GetAPlayerController()
{
	if (Character == nullptr)
		return nullptr;

	return Cast<APlayerController>(Character->GetController());
}

bool UDiggerWeaponComponent::IsCharacterHoldingItem()
{ 
	if (Character == nullptr)
		return false;

	return Character->GetInstanceComponents().FindItemByClass<UDiggerWeaponComponent>();
}

void UDiggerWeaponComponent::Fire()
{
	if (Character == nullptr || GetAPlayerController() == nullptr)
	{
		return;
	}

	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			const FRotator SpawnRotation = GetAPlayerController()->PlayerCameraManager->GetCameraRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = GetOwner()->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);
	
			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	
			// Spawn the projectile at the muzzle
			World->SpawnActor<ADiggerProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}
	
	// Try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

bool UDiggerWeaponComponent::AttachWeapon(ADiggerCharacter* TargetCharacter)
{
	if (Character)
		return false;

	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon component yet
	if (!Character || !GetAPlayerController() || IsCharacterHoldingItem())
	{
		Character = nullptr;
		return false;
	}

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// Set up action bindings
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetAPlayerController()->GetLocalPlayer()))
	{
		// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
		Subsystem->AddMappingContext(FireMappingContext, 1);
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(GetAPlayerController()->InputComponent))
	{
		// Fire
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UDiggerWeaponComponent::Fire);
		// Drop Tool
		EnhancedInputComponent->BindAction(DropToolAction, ETriggerEvent::Triggered, this, &UDiggerWeaponComponent::DropCurrentTool);
	}

	return true;
}

void UDiggerWeaponComponent::DropCurrentTool()
{
	if (Character == nullptr)
		return;

	DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetAPlayerController()->GetLocalPlayer()))
	{
		// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
		Subsystem->RemoveMappingContext(FireMappingContext);
	}

	// Notify that the tool has been dropped
	OnToolDrop.Broadcast(Character);

	Character = nullptr;
}

void UDiggerWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ensure we have a character owner
	if (Character != nullptr)
	{
		// remove the input mapping context from the Player Controller
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}
		}
	}

	// maintain the EndPlay call chain
	Super::EndPlay(EndPlayReason);
}