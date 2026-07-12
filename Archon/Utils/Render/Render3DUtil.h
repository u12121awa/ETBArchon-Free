#pragma once
#include "../../Core/Instance/Instances.hpp"

struct Box3DData
{
    ImVec2 Corners[8];
    ImColor Color;
    bool bIsValid;
};

namespace Render3DUtil
{
    inline static std::vector<std::pair<int, int>> m_BonePairs = {
        // Spine
        {1, 2}, // pelvis -> spine_01
        {2, 3}, // spine_01 -> spine_02
        {3, 4}, // spine_02 -> spine_03
        {4, 47}, // spine_03 -> neck_01
        {47, 48}, // neck_01 -> head

        // Left arm
        {4, 5}, // spine_03 -> clavicle_l
        {5, 6}, // clavicle_l -> UpperArm_L
        {6, 7}, // UpperArm_L -> lowerarm_l
        {7, 8}, // lowerarm_l -> Hand_L
        {8, 9}, // Hand_L -> index_01_l
        {9, 10}, // index_01_l -> index_02_l
        {10, 11}, // index_02_l -> index_03_l
        {8, 12}, // Hand_L -> middle_01_l
        {12, 13}, // middle_01_l -> middle_02_l
        {13, 14}, // middle_02_l -> middle_03_l
        {8, 15}, // Hand_L -> pinky_01_l
        {15, 16}, // pinky_01_l -> pinky_02_l
        {16, 17}, // pinky_02_l -> pinky_03_l
        {8, 18}, // Hand_L -> ring_01_l
        {18, 19}, // ring_01_l -> ring_02_l
        {19, 20}, // ring_02_l -> ring_03_l
        {8, 21}, // Hand_L -> thumb_01_l
        {21, 22}, // thumb_01_l -> thumb_02_l
        {22, 23}, // thumb_02_l -> thumb_03_l

        // Right arm
        {4, 26}, // spine_03 -> clavicle_r
        {26, 27}, // clavicle_r -> UpperArm_R
        {27, 28}, // UpperArm_R -> lowerarm_r
        {28, 29}, // lowerarm_r -> Hand_R
        {29, 30}, // Hand_R -> index_01_r
        {30, 31}, // index_01_r -> index_02_r
        {31, 32}, // index_02_r -> index_03_r
        {29, 33}, // Hand_R -> middle_01_r
        {33, 34}, // middle_01_r -> middle_02_r
        {34, 35}, // middle_02_r -> middle_03_r
        {29, 36}, // Hand_R -> pinky_01_r
        {36, 37}, // pinky_01_r -> pinky_02_r
        {37, 38}, // pinky_02_r -> pinky_03_r
        {29, 39}, // Hand_R -> ring_01_r
        {39, 40}, // ring_01_r -> ring_02_r
        {40, 41}, // ring_02_r -> ring_03_r
        {29, 42}, // Hand_R -> thumb_01_r
        {42, 43}, // thumb_01_r -> thumb_02_r
        {43, 44}, // thumb_02_r -> thumb_03_r

        // Left leg
        {1, 49}, // pelvis -> Thigh_L
        {49, 50}, // Thigh_L -> calf_l
        {50, 52}, // calf_l -> Foot_L
        {52, 53}, // Foot_L -> ball_l

        // Right leg
        {1, 55}, // pelvis -> Thigh_R
        {55, 56}, // Thigh_R -> calf_r
        {56, 58}, // calf_r -> Foot_R
        {58, 59} // Foot_R -> ball_r
    };


    inline void RenderSkeleton(ACharacter* pawn, ImDrawList* drawList, ImColor color)
    {
        auto PC = Instances::GetPlayerController();

        if (!pawn || !PC || m_BonePairs.empty())
            return;

        USkeletalMeshComponent* mesh = pawn->Mesh;
        if (!mesh)
            return;

        for (const std::pair<int, int>& pair : m_BonePairs)
        {
            const int bone1Index = pair.first;
            const int bone2Index = pair.second;

            const FVector boneLoc1 = pawn->Mesh->GetSocketLocation(pawn->Mesh->GetBoneName(bone1Index));
            const FVector boneLoc2 = pawn->Mesh->GetSocketLocation(pawn->Mesh->GetBoneName(bone2Index));

            FVector2D boneScreen;
            FVector2D prevBoneScreen;

            if (
                !PC->ProjectWorldLocationToScreen(boneLoc1, &boneScreen, false) ||
                !PC->ProjectWorldLocationToScreen(boneLoc2, &prevBoneScreen, false)
            )
                continue;


            drawList->AddLine(
                ImVec2(boneScreen.X, boneScreen.Y),
                ImVec2(prevBoneScreen.X, prevBoneScreen.Y),
                color,
                1.0f
            );
        }
    }
    
    inline bool GetPointBoxData(FVector Center, float Size, ImColor color, Box3DData& OutData)
    {
       
        OutData.bIsValid = false;
        OutData.Color = color;

        auto PC = Instances::GetPlayerController();
        if (!PC) return false;

        float Half = Size / 2.0f; // 2.5cm
        
        const FVector WorldPoints[] = {
            FVector(Center.X + Half, Center.Y + Half, Center.Z + Half), // 0
            FVector(Center.X + Half, Center.Y - Half, Center.Z + Half), // 1
            FVector(Center.X - Half, Center.Y + Half, Center.Z + Half), // 2
            FVector(Center.X - Half, Center.Y - Half, Center.Z + Half), // 3
            FVector(Center.X + Half, Center.Y + Half, Center.Z - Half), // 4
            FVector(Center.X + Half, Center.Y - Half, Center.Z - Half), // 5
            FVector(Center.X - Half, Center.Y + Half, Center.Z - Half), // 6
            FVector(Center.X - Half, Center.Y - Half, Center.Z - Half)  // 7
        };
        
        for (int i = 0; i < 8; i++)
        {
            FVector2D ScreenPos;
            if (PC->ProjectWorldLocationToScreen(WorldPoints[i], &ScreenPos, false))
            {
                OutData.Corners[i] = ImVec2(ScreenPos.X, ScreenPos.Y);
            }
            else
            {
                return false;
            }
        }

        OutData.bIsValid = true;
        return true;
    }
    
    inline bool GetActorBoxData(ACharacter* pawn, ImColor color, Box3DData& OutData, FVector* CustomLocation = nullptr)
    {
        OutData.bIsValid = false;
        OutData.Color = color;

        auto PC = Instances::GetPlayerController();
        if (!PC || !pawn) return false;
        
        UCapsuleComponent* Cap = pawn->CapsuleComponent;
        if (!Cap || !Utils::ShouldUsePointer64(Cap)) return false;

        float Radius = Cap->GetScaledCapsuleRadius();
        float HalfHeight = Cap->GetScaledCapsuleHalfHeight();
        
        FVector Origin;
        if (CustomLocation) Origin = *CustomLocation;
        else Origin = Cap->K2_GetComponentLocation();

        FRotator Rot = Cap->K2_GetComponentRotation();
        
        const FVector LocalPoints[] = {
            FVector(Radius, Radius, HalfHeight),   // 0
            FVector(Radius, -Radius, HalfHeight),  // 1
            FVector(-Radius, Radius, HalfHeight),  // 2
            FVector(-Radius, -Radius, HalfHeight), // 3
            FVector(Radius, Radius, -HalfHeight),  // 4
            FVector(Radius, -Radius, -HalfHeight), // 5
            FVector(-Radius, Radius, -HalfHeight), // 6
            FVector(-Radius, -Radius, -HalfHeight) // 7
        };
        
        for (int i = 0; i < 8; i++)
        {
            FVector WorldPoint = Origin + Utils::CustomRotateVector(Rot, LocalPoints[i]);
            FVector2D ScreenPos;

            if (PC->ProjectWorldLocationToScreen(WorldPoint, &ScreenPos, false))
            {
                OutData.Corners[i] = ImVec2(ScreenPos.X, ScreenPos.Y);
            }
            else
            {
                return false;
            }
        }

        OutData.bIsValid = true;
        return true;
    }
    
    inline void RenderBox(ImDrawList* drawList, const Box3DData& Data)
    {
        if (!Data.bIsValid) return;

        const ImVec2* p = Data.Corners;
        ImColor col = Data.Color;
        
        drawList->AddLine(p[0], p[1], col);
        drawList->AddLine(p[1], p[3], col);
        drawList->AddLine(p[3], p[2], col);
        drawList->AddLine(p[2], p[0], col);
        
        drawList->AddLine(p[4], p[5], col);
        drawList->AddLine(p[5], p[7], col);
        drawList->AddLine(p[7], p[6], col);
        drawList->AddLine(p[6], p[4], col);
        
        drawList->AddLine(p[0], p[4], col);
        drawList->AddLine(p[1], p[5], col);
        drawList->AddLine(p[2], p[6], col);
        drawList->AddLine(p[3], p[7], col);
    }
}
