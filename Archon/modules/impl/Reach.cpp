#include "pch.h"
#include "Reach.h"

#include "CameraManager.h"
#include "../../Archon.h"
#include "../../Core/Managers/ModuleManager.h"
#include "../../Utils/World/WorldUtils.h"

void Reach::OnFunctionEvent(FunctionEvent* event)
{
    auto PC = Instances::GetPlayerController();
    auto Pawn = Instances::GetPlayerCharacter();
    if (!PC || !Pawn || !Pawn->Mesh) return;


    auto fullName = event->functionName;

    if (fullName == xorstr_("Function Backrooms.FancyCharacter.Interact"))
    {
        ACharacter* target = static_cast<ACharacter*>(event->context);
        if (!target || Pawn != target) return;


        AActor* IActor = Pawn->CurrentInteractableActor;

        if (IActor)
        {
            if (IActor->IsA(ABP_Pushable_C::StaticClass()))
            {
                ABP_Pushable_C* aI2Actor = static_cast<ABP_Pushable_C*>(IActor);
                if (aI2Actor)
                {
                    event->cancel = true;
                    auto loc = Pawn->K2_GetActorLocation();
                    if (Pawn->CurrentPushableActor_0)
                    {
                        Pawn->StopPushingActor_MC(Pawn->CurrentPushableActor_0);
                        Pawn->StopPushingActor_SERVER(Pawn->CurrentPushableActor_0);
                        Pawn->CurrentPushableActor_0 = nullptr;
                    }
                    else
                    {
                        auto rotation = Archon::GetRotationManager()->GetRealRotation();
                        Pawn->StartPushingActor_MC(aI2Actor, loc, rotation);
                        Pawn->StartPushingActor_SERVER(aI2Actor, loc, rotation);
                        Pawn->CurrentPushableActor_0 = aI2Actor;
                    }
                }
            }
        }
    }
}

void Reach::OnUpdate()
{
    auto PC = Instances::GetPlayerController();
    auto Pawn = Instances::GetPlayerCharacter();
    if (!PC || !Pawn) return;

    FVector Local = Instances::GetPlayerLocation();
    if (Local.IsZero()) return;

    auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();

    FVector PivotPos = Local;
    PivotPos.Z += cameraManager ? cameraManager->GetEyeHeight() : 64.0f;

    if (interactFix->value)
    {
        Pawn->TraceLength = 0;

        FVector StartLocation = PivotPos;
        FRotator ViewRotation = Archon::GetRotationManager()->GetRealRotation();
        FVector Forward = Archon::GetRotationManager()->GetForwardVector(ViewRotation);
        FVector EndLocation = StartLocation + Forward * interactDistance->value;

        auto SystemLib = Instances::GetSystemLib();
        TArray<AActor*> IgnoredActors;
        IgnoredActors.Add(Pawn);

        AActor* FinalTarget = nullptr;


        FVector PostLocation;

        FHitResult HitResult;
        bool bHitLine = SystemLib->LineTraceSingle(
            Pawn, StartLocation, EndLocation,
            ETraceTypeQuery::TraceTypeQuery2,
            true,
            IgnoredActors, EDrawDebugTrace::None, &HitResult, true,
            FLinearColor(255, 255, 255, 255), FLinearColor(255, 255, 255, 255), 0.0f
        );

        if (bHitLine)
        {
            PostLocation = HitResult.Location;
            AActor* HitActor = HitResult.Actor.Get();

            if (HitActor && (
                HitActor->IsA(ABP_DroppedItem_C::StaticClass()) ||
                HitActor->IsA(AInteractableActor::StaticClass()) ||
                HitActor->IsA(AInteractablePawn::StaticClass())))
            {
                FinalTarget = HitActor;
            }
        }

        if (!FinalTarget)
        {
            float Radius = smartFindRadius->value;

            TArray<EObjectTypeQuery> ObjectTypes;
            ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery2);
            ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery3);
            ObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery4);


            TArray<AActor*> OutActors;

            bool bResult = UKismetSystemLibrary::SphereOverlapActors(
                Pawn,
                PostLocation,
                Radius,
                ObjectTypes,
                AActor::StaticClass(),
                IgnoredActors,
                &OutActors
            );

            if (bResult)
            {
                AActor* BestCandidate = nullptr;
                float BestDistSq = FLT_MAX;

                for (AActor* HitActor : OutActors)
                {
                    if (!HitActor) continue;

                    if (HitActor->IsA(ABP_DroppedItem_C::StaticClass()) ||
                        HitActor->IsA(AInteractableActor::StaticClass()) ||
                        HitActor->IsA(AInteractablePawn::StaticClass()))
                    {
                        float DistSq = Pawn->GetDistanceTo(HitActor);

                        if (DistSq < BestDistSq)
                        {
                            BestDistSq = DistSq;
                            BestCandidate = HitActor;
                        }
                    }
                }

                if (BestCandidate)
                {
                    FinalTarget = BestCandidate;
                }
            }
        }

        if (FinalTarget)
        {
            if (FinalTarget->IsA(ABP_DroppedItem_C::StaticClass()))
            {
                Pawn->CurrentFocusedItem = static_cast<ABP_DroppedItem_C*>(FinalTarget);
                Pawn->CurrentInteractableActor = FinalTarget;
            }
            else
            {
                Pawn->CurrentFocusedItem = nullptr;
                Pawn->CurrentInteractableActor = FinalTarget;
            }
            Pawn->ChangeCrosshairVisibility(true);
        }
        else
        {
            Pawn->CurrentFocusedItem = nullptr;
            Pawn->CurrentInteractableActor = nullptr;
            Pawn->ChangeCrosshairVisibility(false);
        }
    }
    else
    {
        Pawn->TraceLength = interactDistance->value;
    }
}

void OnPush()
{
    auto PC = Instances::GetPlayerController();
    auto Pawn = Instances::GetPlayerCharacter();
    if (!PC || !Pawn) return;

    FVector Local = Instances::GetPlayerLocation();
    if (Local.IsZero()) return;

    auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();

    FVector PivotPos = Local;
    PivotPos.Z += cameraManager ? cameraManager->GetEyeHeight() : 64.0f;


    FVector StartLocation = PivotPos;
    FRotator ViewRotation = Archon::GetRotationManager()->GetRealRotation();
    FVector Forward = Archon::GetRotationManager()->GetForwardVector(ViewRotation);
    FVector EndLocation = StartLocation + Forward * 3000;

    auto SystemLib = Instances::GetSystemLib();
    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(Pawn);


    FHitResult HitResult;
    bool bHitLine = SystemLib->LineTraceSingle(
        Pawn, StartLocation, EndLocation,
        ETraceTypeQuery::TraceTypeQuery1,
        false,
        IgnoredActors, EDrawDebugTrace::None, &HitResult, false,
        FLinearColor(255, 255, 255, 255), FLinearColor(255, 255, 255, 255), 0.0f
    );

    if (bHitLine)
    {
        AActor* HitActor = HitResult.Actor.Get();

        if (HitActor)
        {
            Log::Info(HitActor->GetFullName());
            Pawn->SpawnEquipItem_SERVER(HitActor->Class);
        }
    }
}

void Reach::OnKeyEvent(KeyEvent* event)
{
    if (event->mouse == pBind->bind.mouse && event->type == Pressed && event->key == pBind->bind.key)
    {
        FTickCallbackSystem::RegisterCallback([]
        {
            OnPush();
        });
    }

}
