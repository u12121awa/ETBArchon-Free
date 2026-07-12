#include "pch.h"
#include "Flight.h"

#include "FreeCam.h"
#include "../../Utils/InputUtils.h"

void Flight::OnUpdate()
{
    VMProtectBeginMutation("Flight::OnUpdate");

    auto freecam = ModuleManager::Get().GetModule<FreeCam>();

    auto PC = Instances::GetPlayerController();
    auto Pawn = Instances::GetPlayerCharacter();

    if (!PC || !Pawn)
    {
        VMProtectEnd();
        return;
    }

    auto MovementComponent = Pawn->CharacterMovement;
    auto CapsuleComponent = Pawn->CapsuleComponent;

    if (!MovementComponent || !CapsuleComponent)
    {
        VMProtectEnd();
        return;
    }


    if (!bIsInitialized)
    {
        VirtualLocation = Pawn->K2_GetActorLocation();
        bIsInitialized = true;
    }
    
    if (freecam && freecam->IsEnabled())
    {
        VMProtectEnd();
        return;
    }

    FVector moveDirection = {0.f, 0.f, 0.f};
    FRotator realRotation = Archon::GetRotationManager()->GetRealRotation();
    FRotator horizontalRotation = FRotator(0.f, realRotation.Yaw, 0.f);

    FVector forward = Archon::GetRotationManager()->GetForwardVector(horizontalRotation);
    FVector right = Archon::GetRotationManager()->GetRightVector(horizontalRotation);

    if (InputUtils::IsKeyDown('W', true)) moveDirection += forward;
    if (InputUtils::IsKeyDown('S', true)) moveDirection -= forward;
    if (InputUtils::IsKeyDown('D', true)) moveDirection += right;
    if (InputUtils::IsKeyDown('A', true)) moveDirection -= right;
    if (InputUtils::IsKeyDown(VK_SPACE, true)) moveDirection.Z += 1.0f;
    if (InputUtils::IsKeyDown(VK_LSHIFT, true)) moveDirection.Z -= 1.0f;

    if (!moveDirection.IsZero())
    {
        moveDirection.Normalize();

        FVector delta = moveDirection;
        delta.X *= hSpeed->value;
        delta.Y *= hSpeed->value;
        delta.Z *= vSpeed->value;

        VirtualLocation += delta;
    }

    if (noCollision->value && CapsuleComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
        CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MovementComponent->Velocity = {0, 0, 0};

    Pawn->K2_SetActorLocation(VirtualLocation, false, nullptr, false);
    VMProtectEnd();
}

