#include "pch.h"
#include "RotationManager.h"

#include "../../modules/impl/FreeCam.h"
#include "../../Utils/InputUtils.h"

void RotationManager::OnStop()
{
    StopRotation();
    bIsStop = true;
    auto PC = Instances::GetPlayerController();
    if (!PC) return;
    PC->ControlRotation = RealRotation;
}

void RotationManager::SetRotation(
    FRotator NewTarget,
    float TimeMs,
    int Priority
)
{
    if (bIsRotating && Priority < CurrentPriority)
        return;
    
    CurrentPriority = Priority;

    float velocity = NormalizeAngle(NewTarget.Yaw - CurrentFakeRotation.Yaw);
    LastYawVelocity = velocity;

    TargetRotation = NewTarget;
    RotateDurationMs = TimeMs;
    StartRotation = CurrentFakeRotation;
    StartTime = std::chrono::steady_clock::now();

    bIsRotating = true;
}

    
void RotationManager::StopRotation()
{
    bIsRotating = false;
    CurrentFakeRotation = RealRotation;
    LastYawVelocity = 0;
    CurrentPriority = INT_MIN;
}

void RotationManager::OnUpdate()
{
    VMProtectBeginMutation("RotationManager::OnUpdate");

    auto PC = Instances::GetPlayerController();
    if (!PC || bIsStop)
    {
        VMProtectEnd();
        return;
    }

    EnsureInitialized(PC);

    auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();

    float CurrentFPS = 60.0f;
    if (ImGui::GetCurrentContext())
        CurrentFPS = ImGui::GetIO().Framerate;
    if (CurrentFPS < 10.0f) CurrentFPS = 60.0f;

    float BaseFPS = 75.0f;
    float FPSFactor = BaseFPS / CurrentFPS;

    if (cameraManager && cameraManager->IsEnabled() && cameraManager->cameraInputFix->value && PC->PlayerInput)
    {
        float RawYawInput = *(float*)((uintptr_t)PC->PlayerInput + 0x108);
        float RawPitchInput = *(float*)((uintptr_t)PC->PlayerInput + 0x10C);

        float NormalizedYaw = RawYawInput * FPSFactor;
        float NormalizedPitch = RawPitchInput * FPSFactor;

        float ScaleYaw = cameraManager->scaleYaw->value;
        float ScalePitch = cameraManager->scalePitch->value;

        RealRotation.Yaw = NormalizeAngle(RealRotation.Yaw + NormalizedYaw * ScaleYaw);
        RealRotation.Pitch = ClampPitch(RealRotation.Pitch + NormalizedPitch * ScalePitch);
    }

    VMProtectEnd();
}




void RotationManager::OnUpdateViewPoint(ULocalPlayer* LocalPlayer, FMinimalViewInfo* OutViewInfo)
{
    VMProtectBeginMutation("RotationManager::OnUpdateViewPoint");

    auto PC = Instances::GetPlayerController();
    if (!PC || bIsStop)
    {
        VMProtectEnd();
        return;
    }

    EnsureInitialized(PC);

    FRotator CurrentControlRot = PC->ControlRotation;

    auto cameraManager = ModuleManager::Get().GetModule<CameraManager>();
    auto freecam = ModuleManager::Get().GetModule<FreeCam>();
  
    const bool bUsingInputFix =(cameraManager && cameraManager->IsEnabled() && cameraManager->cameraInputFix->value && PC->PlayerInput);

    if (!bUsingInputFix)
    {
        float DeltaYaw = NormalizeAngle(CurrentControlRot.Yaw - LastAppliedControlRotation.Yaw);
        
        float DeltaPitch = NormalizeAngle(CurrentControlRot.Pitch - LastAppliedControlRotation.Pitch);

        RealRotation.Yaw   = NormalizeAngle(RealRotation.Yaw + DeltaYaw);
        RealRotation.Pitch = ClampPitch(RealRotation.Pitch + DeltaPitch);
    }
    
    if (bIsRotating)
        UpdateInterpolation();
    else
        CurrentFakeRotation = RealRotation;
    
    FRotator NewControlRot;
    if (freecam && freecam->IsEnabled() && freecam->rotationLock->value && freecam->bIsInitialized && !bIsRotating)
        NewControlRot = freecam->VirtualRotation;
    else
        NewControlRot = CurrentFakeRotation;

    PC->ControlRotation = NewControlRot;
    
    LastAppliedControlRotation = NewControlRot;

    FVector Local = Instances::GetPlayerLocation();
    
    float BackRoll = OutViewInfo->Rotation.Roll;
    
    if (!Local.IsZero())
        OutViewInfo->Rotation = RealRotation;

    if (cameraManager && cameraManager->IsEnabled())
        cameraManager->OnUpdateViewPoint(OutViewInfo);

    if (freecam && freecam->IsEnabled())
        freecam->OnUpdateViewPoint(OutViewInfo);
    
    if (!Local.IsZero() && (!cameraManager->IsEnabled() || !cameraManager->cameraFix->value || !cameraManager->ignoreRoll->value))
        OutViewInfo->Rotation.Roll = BackRoll;
    
    VMProtectEnd();
}


void RotationManager::OnFunctionEvent(FunctionEvent* event)
{
    VMProtectBeginMutation("RotationManager::OnFunctionEvent");
    auto freecam = ModuleManager::Get().GetModule<FreeCam>();
    if (!bIsRotating || bIsStop || (freecam && freecam->IsEnabled()))
    {
        VMProtectEnd();
        return;
    }
        
    std::string funcName = event->functionName;
    
    if (funcName.find(xorstr_("InpAxisEvt_MoveForward")) != std::string::npos)
        ApplyMoveFix((float*)event->parms, true);
    else if (funcName.find(xorstr_("InpAxisEvt_MoveRight")) != std::string::npos)
        ApplyMoveFix((float*)event->parms, false);

    VMProtectEnd();
}

__declspec(noinline) void RotationManager::UpdateInterpolation()
{
    VMProtectBeginMutation("RotationManager::UpdateInterpolation");
    
    auto now = std::chrono::steady_clock::now();
       
    float elapsedMs = std::chrono::duration<float, std::milli>(now - StartTime).count();

    if (elapsedMs >= RotateDurationMs)
    {
        CurrentFakeRotation = RealRotation;
        LastYawVelocity = 0;
        bIsRotating = false;
        CurrentPriority = INT_MIN;
    }
    else
    {
        float deltaYaw = NormalizeAngle(TargetRotation.Yaw - StartRotation.Yaw);
        float deltaPitch = ClampPitch(TargetRotation.Pitch - StartRotation.Pitch);

        CurrentFakeRotation.Yaw = NormalizeAngle(StartRotation.Yaw + deltaYaw);
        CurrentFakeRotation.Pitch = ClampPitch(StartRotation.Pitch + deltaPitch);
    }
    VMProtectEnd();
}

void RotationManager::ApplyMoveFix(float* inputVal, bool bIsMoveForward)
{
    VMProtectBeginMutation("RotationManager::ApplyMoveFix");
    if (!inputVal)
    {
        VMProtectEnd();
        return;
    }
        
    float DeltaYaw = NormalizeAngle(RealRotation.Yaw - CurrentFakeRotation.Yaw);
    DeltaYaw += LastYawVelocity;
    if (std::abs(DeltaYaw) < 1.0f)
    {
        VMProtectEnd();
        return;
    }

    float DeltaRad = DegToRad(DeltaYaw);
    float Cos = cosf(DeltaRad);
    float Sin = sinf(DeltaRad);

    float Fwd = 0.f, Rgt = 0.f;
    if (InputUtils::IsKeyDown('W', true)) Fwd = 1.f;
    else if (InputUtils::IsKeyDown('S', true)) Fwd = -1.f;
    if (InputUtils::IsKeyDown('D', true)) Rgt = 1.f;
    else if (InputUtils::IsKeyDown('A', true)) Rgt = -1.f;

    if (Fwd == 0.f && Rgt == 0.f)
    {
        VMProtectEnd();
        return;
    }

    if (bIsMoveForward) *inputVal = Fwd * Cos - Rgt * Sin;
    else *inputVal = Fwd * Sin + Rgt * Cos;

    VMProtectEnd();
}
