// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectCharacter.h"
#include "MyProjectProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"


//////////////////////////////////////////////////////////////////////////
// AMyProjectCharacter

AMyProjectCharacter::AMyProjectCharacter()
{
	// Character doesnt have a rifle at start
	bHasRifle = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// 创建交互管理器组件
	InteractionManager = CreateDefaultSubobject<UPlayerInteractionManager>(TEXT("InteractionManager"));

}

void AMyProjectCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	if (!InteractionManager)
	{
		UE_LOG(LogTemp, Log, TEXT("InteractionManager component created failed"));
	}


}

//////////////////////////////////////////////////////////////////////////// Input

void AMyProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Look);

		// 鼠标左键绑定
		if (MouseLeftButtonAction)
		{
			EnhancedInputComponent->BindAction(MouseLeftButtonAction, ETriggerEvent::Started, this, &AMyProjectCharacter::OnMouseLeftButtonPressed);
			EnhancedInputComponent->BindAction(MouseLeftButtonAction, ETriggerEvent::Completed, this, &AMyProjectCharacter::OnMouseLeftButtonReleased);
		}
		// 鼠标右键绑定
		if (MouseRightButtonAction)
		{
			EnhancedInputComponent->BindAction(MouseRightButtonAction, ETriggerEvent::Started, this, &AMyProjectCharacter::OnMouseRightButtonPressed);
			EnhancedInputComponent->BindAction(MouseRightButtonAction, ETriggerEvent::Completed, this, &AMyProjectCharacter::OnMouseRightButtonReleased);
		}
        
		// 鼠标移动绑定
		if (MouseMoveAction)
		{
			EnhancedInputComponent->BindAction(MouseMoveAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::OnMouseMoved);
		}
	}
}


void AMyProjectCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AMyProjectCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMyProjectCharacter::SetHasRifle(bool bNewHasRifle)
{
	bHasRifle = bNewHasRifle;
}

bool AMyProjectCharacter::GetHasRifle()
{
	return bHasRifle;
}

void AMyProjectCharacter::OnMouseLeftButtonPressed()
{
	if (InteractionManager)
	{
		InteractionManager->OnMouseButtonDown(true);
	}
}

void AMyProjectCharacter::OnMouseLeftButtonReleased()
{
	if (InteractionManager)
	{
		InteractionManager->OnMouseButtonUp(true);
	}
}

void AMyProjectCharacter::OnMouseRightButtonPressed()
{
	if (InteractionManager)
	{
		InteractionManager->OnMouseButtonDown(false);
	}
}

void AMyProjectCharacter::OnMouseRightButtonReleased()
{
	if (InteractionManager)
	{
		InteractionManager->OnMouseButtonUp(false);
	}
}

void AMyProjectCharacter::OnMouseMoved(const FInputActionValue& Value)
{
	if (InteractionManager)
	{
		FVector2D MouseDelta = Value.Get<FVector2D>();
		// 获取当前鼠标位置并传递
		FVector2D MousePosition;
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			PC->GetMousePosition(MousePosition.X, MousePosition.Y);
			InteractionManager->OnMouseMove(MousePosition);
		}
	}
}