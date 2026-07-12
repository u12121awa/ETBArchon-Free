#pragma once

#include <chrono>
#include <cmath>
#include <algorithm>

#include "ModuleManager.h"
#include "../../modules/impl/CameraManager.h"
#include "../../Utils/Utils.h"
#include "../Instance/Instances.hpp"

class RotationManager
{
private:
    FRotator RealRotation;  
    FRotator LastAppliedControlRotation;
    bool bInitialized;
    
    FRotator CurrentFakeRotation;
    FRotator TargetRotation;
    FRotator StartRotation;
    std::chrono::steady_clock::time_point StartTime;
    float RotateDurationMs;
    bool bIsRotating;
    bool bIsStop;
    float LastYawVelocity;
    int CurrentPriority = INT_MIN;
public:
    
    RotationManager()
    {
        RealRotation = {0.f, 0.f, 0.f};
        StartRotation = {0.f, 0.f, 0.f};
        TargetRotation = {0.f, 0.f, 0.f};
        LastAppliedControlRotation = {0.f, 0.f, 0.f};
        CurrentFakeRotation = {0.f, 0.f, 0.f};
        bIsRotating = false;
        bInitialized = false;
        bIsStop = false;
        RotateDurationMs = 0.f;
        LastYawVelocity = 0.f;
    }
    void SetRotation(FRotator NewTarget, float TimeMs, int Priority = 1);
    void StopRotation();
    void OnStop();

    void OnUpdate();
    void OnUpdateViewPoint(ULocalPlayer* LocalPlayer, FMinimalViewInfo* OutViewInfo);
    void OnFunctionEvent(FunctionEvent* event);
    
    FRotator GetRealRotation() { return RealRotation; }
    FRotator GetFakeRotation() { return CurrentFakeRotation; }

    
#define M_PI 3.14159265358979323846f
    float DegToRad(float deg) { return deg * (M_PI / 180.f); }
    
    FVector CustomRotateVector(FRotator Rot, FVector V)
    {
        float SP = sin(DegToRad(Rot.Pitch));
        float CP = cos(DegToRad(Rot.Pitch));
        float SY = sin(DegToRad(Rot.Yaw));
        float CY = cos(DegToRad(Rot.Yaw));
        float SR = sin(DegToRad(Rot.Roll));
        float CR = cos(DegToRad(Rot.Roll));
        
        FVector X_Axis;
        X_Axis.X = CP * CY;
        X_Axis.Y = CP * SY;
        X_Axis.Z = SP;

        FVector Y_Axis;
        Y_Axis.X = SR * SP * CY - CR * SY;
        Y_Axis.Y = SR * SP * SY + CR * CY;
        Y_Axis.Z = -SR * CP;

        FVector Z_Axis;
        Z_Axis.X = -(CR * SP * CY + SR * SY);
        Z_Axis.Y = CY * SR - CR * SP * SY;
        Z_Axis.Z = CR * CP;
        
        FVector Result;
        Result.X = (V.X * X_Axis.X) + (V.Y * Y_Axis.X) + (V.Z * Z_Axis.X);
        Result.Y = (V.X * X_Axis.Y) + (V.Y * Y_Axis.Y) + (V.Z * Z_Axis.Y);
        Result.Z = (V.X * X_Axis.Z) + (V.Y * Y_Axis.Z) + (V.Z * Z_Axis.Z);

        return Result;
    }
    
    static float UnwindDegrees(float A)
    {
        while(A > 180.f)
        {
            A -= 360.f;
        }

        while(A < -180.f)
        {
            A += 360.f;
        }

        return A;
    }

    static float ClampPitch(float pitch)
    {
        pitch = UnwindDegrees(pitch);
        return std::clamp(pitch, -89.f, 89.f);
    }

    float NormalizeAngle(float angle)
    {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }
    
    FVector GetForwardVector(FRotator Rot)
    {
        float cp = cosf(DegToRad(Rot.Pitch));
        float sp = sinf(DegToRad(Rot.Pitch));
        float cy = cosf(DegToRad(Rot.Yaw));
        float sy = sinf(DegToRad(Rot.Yaw));
        return FVector(cp * cy, cp * sy, sp);
    }
    

    FVector GetRightVector(FRotator Rot)
    {
        float cp = cosf(DegToRad(Rot.Pitch));
        float sp = sinf(DegToRad(Rot.Pitch));
        float cy = cosf(DegToRad(Rot.Yaw));
        float sy = sinf(DegToRad(Rot.Yaw));
        float cr = cosf(DegToRad(Rot.Roll));
        float sr = sinf(DegToRad(Rot.Roll));
        
        return FVector(
            -sy * cr + cy * sp * sr,
            cy * cr + sy * sp * sr,
            -cp * sr
        );
    }
    
    FVector GetUpVector(FRotator Rot)
    {
        float cp = cosf(DegToRad(Rot.Pitch));
        float sp = sinf(DegToRad(Rot.Pitch));
        float cy = cosf(DegToRad(Rot.Yaw));
        float sy = sinf(DegToRad(Rot.Yaw));
        float cr = cosf(DegToRad(Rot.Roll));
        float sr = sinf(DegToRad(Rot.Roll));
        
        return FVector(
            sy * sr + cy * sp * cr,
            -cy * sr + sy * sp * cr,
            cp * cr
        );
    }
    
private:
    __declspec(noinline) void UpdateInterpolation();
    
    void ApplyMoveFix(float* inputVal, bool bIsMoveForward);
    
    void EnsureInitialized(APlayerController* PC)
    {
        if (bInitialized || !PC) return;

        RealRotation = PC->ControlRotation;
        CurrentFakeRotation = RealRotation;
        StartRotation = RealRotation;
        TargetRotation  = RealRotation;
        LastAppliedControlRotation = PC->ControlRotation;
        bIsRotating = false;
        bIsStop = false;
        RotateDurationMs = 0.f;
        LastYawVelocity = 0.f;

        bInitialized = true;
    }
};

