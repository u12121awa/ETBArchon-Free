#pragma once
#include "../../Core/Managers/RotationManager.h"

namespace RotationUtil
{
    FVector GetCameraPos();
    FVector GetEyePos();
    float GetEyeHeight();
    RotationManager* GetRotationManager();
    void SetRotation(FRotator NewTarget, float TimeMs, int Priority = 1);
    void StopRotation();

    __declspec(noinline) inline FRotator CalculateAimAngle(FVector MyEyePos, FVector TargetPos)
    {
        FRotator Result;
        
        double deltaX = (double)TargetPos.X - (double)MyEyePos.X;
        double deltaY = (double)TargetPos.Y - (double)MyEyePos.Y;
        double deltaZ = (double)TargetPos.Z - (double)MyEyePos.Z;
        
        double distXY = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        
        double yawRad = std::atan2(deltaY, deltaX);
        Result.Yaw = (float)(yawRad * (180.0 / M_PI));
        
        double pitchRad = std::atan2(deltaZ, distXY);
        Result.Pitch = (float)(pitchRad * (180.0 / M_PI));
        
        Result.Roll = 0.0f;

        return Result;
    }
    
    __declspec(noinline) inline float GetDistanceCm(FVector v1, FVector v2)
    {
        double dx = (double)v1.X - (double)v2.X;
        double dy = (double)v1.Y - (double)v2.Y;
        double dz = (double)v1.Z - (double)v2.Z;
        
        return (float) std::sqrt(dx * dx + dy * dy + dz * dz);
    }
}
