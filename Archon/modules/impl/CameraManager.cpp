#include "pch.h"
#include "CameraManager.h"

#include "FreeCam.h"
#include "../../Archon.h"
#include "../../Utils/World/ActorUtil.h"


void CameraManager::OnFunctionEvent(FunctionEvent* event)
{
    auto PC = Instances::GetPlayerController();
    auto Pawn = Instances::GetPlayerCharacter();
    if (!PC || !Pawn || !Pawn->Mesh) return;

    return;

    auto fullName = event->functionName;

    if (fullName == xorstr_("Function Backrooms.FancyCharacter.Interact"))
    {
        ACharacter* target = static_cast<ACharacter*>(event->context);
        if (!target || Pawn != target) return;

        event->cancel = true;

        AActor* IActor = Pawn->CurrentInteractableActor;

        if (IActor)
        {
            if (IActor->IsA(AInteractableActor::StaticClass()))
            {
                auto aI2Actor = static_cast<AInteractableActor*>(IActor);
                if (aI2Actor)
                {
                    aI2Actor->OnUsedServer();
                    Log::Info("Used Server1");
                }
                return;
            }
            if (IActor->IsA(AInteractablePawn::StaticClass()))
            {
                auto aI2Pawn = static_cast<AInteractablePawn*>(IActor);
                if (aI2Pawn)
                {
                    aI2Pawn->OnUsedServer(target);
                    Log::Info("Used Server");
                }
                return;
            }
        }
    }
}


void CameraManager::OnUpdateViewPoint(FMinimalViewInfo* viewInfo)
{
    if (!IsEnabled()) return;
    FVector Local = Instances::GetPlayerLocation();
    if (Local.IsZero()) return;

    auto PC = Instances::GetPlayerController();
    auto Pawn = Instances::GetPlayerCharacter();
    if (!PC || !Pawn || !Pawn->Mesh) return;

    FVector PivotPos = Local;
    PivotPos.Z += GetEyeHeight();

    if (armsFix->value)
    {
        auto RealRotation = Archon::GetRotationManager()->GetRealRotation();
        float Off_X = offsetX->value;
        float Off_Y = offsetY->value;
        float Off_Z = offsetZ->value;
        FVector LocalOffset = FVector(Off_X, Off_Y, Off_Z);


        FVector WorldOffset = Archon::GetRotationManager()->CustomRotateVector(RealRotation, LocalOffset);
        FVector FinalLoc = PivotPos + WorldOffset;


        FRotator FinalRot;
        FinalRot.Yaw = RealRotation.Yaw - 90.000183f;
        FinalRot.Pitch = 0;
        FinalRot.Roll = -RealRotation.Pitch;

        Pawn->Arms->K2_SetWorldLocationAndRotation(FinalLoc, FinalRot, false, nullptr, true);
    }
    else
    {
        Pawn->Arms->RelativeLocation = {3.0f, 0.0f, -67.5f};
        Pawn->Arms->RelativeRotation = {0.0f, -90.000183f, 0.0f};
    }

    std::string mode = viewMode->GetCurrent();
    
    auto freecam = ModuleManager::Get().GetModule<FreeCam>();
    if (freecam && freecam->IsEnabled())
    {
        SetVisibility(false);
    } else
    {
        SetVisibility(mode == xorstr_("First"));
    }
    
    if (IsEnabled()) Pawn->ToggleBody(false);

    if (mode == xorstr_("First"))
    {
        if (cameraFix->value) viewInfo->Location = PivotPos;
        return;
    }

    FRotator ViewRot = Archon::GetRotationManager()->GetRealRotation();
    FVector Forward = Archon::GetRotationManager()->GetForwardVector(ViewRot);
    FVector Right = Archon::GetRotationManager()->GetRightVector(ViewRot);
    FVector Up = {0.f, 0.f, 1.f};

    FVector DesiredLoc = PivotPos;

    if (mode == xorstr_("Third"))
    {
        DesiredLoc = DesiredLoc - (Forward * distance->value);
        DesiredLoc = DesiredLoc + (Right * offsetRight->value);
        DesiredLoc = DesiredLoc + (Up * offsetUp->value);
    }
    else if (mode == xorstr_("Second"))
    {
        DesiredLoc = DesiredLoc + (Forward * distance->value);
        DesiredLoc = DesiredLoc + (Right * offsetRight->value);
        DesiredLoc = DesiredLoc + (Up * offsetUp->value);

        FRotator FaceRot = ViewRot;
        FaceRot.Yaw += 180.f;
        FaceRot.Pitch *= -1.0f;
        viewInfo->Rotation = FaceRot;
    }

    auto GPStatics = Instances::GetGPStatics();
    auto SystemLib = Instances::GetSystemLib();
    
    if (!antiClip->value || !SystemLib || !GPStatics)
    {
        viewInfo->Location = DesiredLoc;
        return;
    }

    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(Pawn);

    TArray<FHitResult> HitResults;
    
    bool bHitAny = SystemLib->LineTraceMulti(
        Pawn,
        PivotPos,
        DesiredLoc,
        ETraceTypeQuery::TraceTypeQuery1,
        false, 
        IgnoredActors,
        EDrawDebugTrace::None,
        &HitResults,
        true,
        FLinearColor(), FLinearColor(), 0.0f
    );
    
    FHitResult ValidHit;
    bool bFoundWall = false;

    if (bHitAny)
    {
        for (int i = 0; i < HitResults.Num(); i++)
        {
            FHitResult& TempHit = HitResults[i];
            AActor* HitActor = TempHit.Actor.Get();

            if (!HitActor) continue;
            
            
            if (HitActor->IsA(ABP_DroppedItem_C::StaticClass()) || HitActor->IsA(ACharacter::StaticClass()))
            {
                continue;
            }
            
            ValidHit = TempHit;
            bFoundWall = true;
            break;
        }
    }
    
    if (bFoundWall)
    {
        FVector TraceVec = PivotPos - DesiredLoc;
        float TraceDist = std::sqrt(TraceVec.X * TraceVec.X + TraceVec.Y * TraceVec.Y + TraceVec.Z * TraceVec.Z);

        if (TraceDist > 0.0f)
        {
            float Scale = 1.0f / TraceDist;
            FVector OffsetVec;
            OffsetVec.X = TraceVec.X * Scale * 10.0f;
            OffsetVec.Y = TraceVec.Y * Scale * 10.0f;
            OffsetVec.Z = TraceVec.Z * Scale * 10.0f;
            
            viewInfo->Location.X = ValidHit.Location.X + OffsetVec.X;
            viewInfo->Location.Y = ValidHit.Location.Y + OffsetVec.Y;
            viewInfo->Location.Z = ValidHit.Location.Z + OffsetVec.Z;
        }
        else
        {
            viewInfo->Location = ValidHit.Location;
        }
    }
    else
    {
        viewInfo->Location = DesiredLoc;
    }
}
