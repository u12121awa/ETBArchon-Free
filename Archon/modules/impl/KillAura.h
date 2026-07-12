#pragma once
#include "../Module.h"
#include "../../Utils/Utils.h"
#include "../../Utils/Player/RotationUtil.h"
#include "../../Utils/Render/Render3DUtil.h"
#include "../../Utils/World/TimerHandler.h"
#include "../../Utils/World/WorldUtils.h"

class KillAura : public Module
{
public:
    NumberSetting* targetRange = New<>(NumberSetting(xorstr_("TargetRange"), 5.0f, 1.0f, 6.0f, xorstr_("%.1f")));
    NumberSetting* attackRange = New<>(NumberSetting(xorstr_("AttackRange"), 2.0f, 1.0f, 6.0f, xorstr_("%.1f")));
    NumberSetting* attackDelay = New<>(NumberSetting(xorstr_("AttackDelay"), 250.0f, 0.0f, 1000.0f, xorstr_("%.0f")));
    NumberSetting* rotationTime = New<>(NumberSetting(xorstr_("RotationTime"), 60.f, 0.f, 200.f, xorstr_("%.0f")));
    BooleanGroupSetting* predict = New<>(BooleanGroupSetting("Predict", true));
    NumberSetting* predictTicks = New<>(NumberSetting(xorstr_("PredictTicks"), 0.15f, 0.0f, 0.5f, xorstr_("%.2f")), predict);
    GroupSetting* checks = New<>(GroupSetting(xorstr_("Checks")));
    BooleanSetting* wallCheck = New<>(BooleanSetting(xorstr_("WallCheck"), true), checks);
    BooleanSetting* fakePlayerCheck = New<>(BooleanSetting(xorstr_("FakePlayerCheck"), true), checks);
    BooleanSetting* accurate = New<>(BooleanSetting(xorstr_("AccurateRayCast"), true), checks);


    TimerHandler attackTimer;
    ABPCharacter_Demo_C* CurrentTarget;
    Box3DData PlayerBox3DData;
    Box3DData SideBox3DData;

    KillAura() : Module(xorstr_("KillAura"))
    {
        CurrentTarget = nullptr;
        PlayerBox3DData = {};
        SideBox3DData = {};
    }

    void OnRender2D(DX12DrawContext context) override
    {
        auto World = Instances::GetWorld();
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!World || !PC || !Pawn) return;

        if (CurrentTarget)
        {
            Render3DUtil::RenderBox(context.BackGroundDrawList, PlayerBox3DData);
            Render3DUtil::RenderBox(context.BackGroundDrawList, SideBox3DData);
        }
    }

    void OnUpdate() override
    {
        auto World = Instances::GetWorld();
        auto PC = Instances::GetPlayerController();
        auto Pawn = Instances::GetPlayerCharacter();
        if (!World || !PC || !Pawn)
        {
            CurrentTarget = nullptr;
            return;
        }

        ABP_Item_C* CurrentItem = Pawn->CurrentItem_Rep;

        if (!CurrentItem || !CurrentItem->IsA(ABP_Item_Knife_C::StaticClass()))
        {
            CurrentTarget = nullptr;
            return;
        }

        ABP_Item_Knife_C* knife = static_cast<ABP_Item_Knife_C*>(CurrentItem);

        if (!knife)
        {
            CurrentTarget = nullptr;
            return;
        }

        ABPCharacter_Demo_C* target = GetTarget();


        if (target)
        {
            CurrentTarget = target;
            FVector PrePos = PredictLocation(target->K2_GetActorLocation(), target, predictTicks->value);
            Render3DUtil::GetActorBoxData(target, RGBA(255, 0, 0, 255).ToU32(), PlayerBox3DData, &PrePos);
            FVector targetPos = GetPlayerRayCast(target);
            Render3DUtil::GetPointBoxData(targetPos, 5.0f, RGBA(255, 255, 255, 255).ToU32(), SideBox3DData);

            FRotator TargetRotation = RotationUtil::CalculateAimAngle(RotationUtil::GetEyePos(), targetPos);
            RotationUtil::SetRotation(TargetRotation, rotationTime->value, 10);

            if (attackTimer.passedMs((int)attackDelay->value))
            {
                float distanceCM = attackRange->value * 100.0f;

                if (GetDistanceCm(target) <= distanceCM)
                {
                    Pawn->UseItem_SERVER(knife);

                    attackTimer.reset();
                }
            }
        }
        else
        {
            CurrentTarget = nullptr;
        }
    }

    float GetTickTime(ABPCharacter_Demo_C* Target)
    {
        if (!Target) return 0.0167f;
        float freq = Target->NetUpdateFrequency;
        if (freq < 1.0f) freq = 60.0f;
        return 1.0f / freq;
    }

    FVector ClampPredictionToWorld(ABPCharacter_Demo_C* Target, FVector CurrentPos, FVector RawPredictedPos)
    {
        FHitResult Hit;
        TArray<AActor*> ActorsToIgnore;
        ActorsToIgnore.Add(Target);


        bool bHitWall = UKismetSystemLibrary::LineTraceSingle(
            Target,
            CurrentPos,
            RawPredictedPos,
            ETraceTypeQuery::TraceTypeQuery1,
            false,
            ActorsToIgnore,
            EDrawDebugTrace::None,
            &Hit,
            true,
            FLinearColor(), SDK::FLinearColor(), 0.0f
        );

        if (bHitWall)
        {
            FVector WallNormal = Hit.Normal;
            FVector ClampedPos = Hit.ImpactPoint + (WallNormal * 5.0f);

            return ClampedPos;
        }

        return RawPredictedPos;
    }

    FVector PredictLocation(FVector CurrentPos, ABPCharacter_Demo_C* Target, float PredictTime)
    {
        FVector Velocity = Target->GetVelocity();

        FVector PrePos = CurrentPos + Velocity * PredictTime;

        return ClampPredictionToWorld(Target, CurrentPos, PrePos);
    }

    FVector GetPlayerRayCast(ABPCharacter_Demo_C* player)
    {
        auto Pawn = Instances::GetPlayerCharacter();

        FVector Location = player->K2_GetActorLocation();

        if (predict->value && Pawn)
        {
            Location = PredictLocation(Location, player, predictTicks->value);
            return GetBestAimPoint(player, RotationUtil::GetCameraPos(), accurate->value, &Location);
        }


        return GetBestAimPoint(player, RotationUtil::GetCameraPos(), accurate->value);
    }

    float GetDistanceCm(ABPCharacter_Demo_C* target)
    {
        FVector SelfEyePos = RotationUtil::GetEyePos();
        FVector TargetPos = GetPlayerRayCast(target);

        return RotationUtil::GetDistanceCm(SelfEyePos, TargetPos);
    }

    std::vector<ABPCharacter_Demo_C*> GetPlayers()
    {
        std::vector<ABPCharacter_Demo_C*> players;

        auto World = Instances::GetWorld();

        if (!World) return players;

        TArray<AActor*> Players;
        Instances::GetGPStatics()->GetAllActorsOfClass(World, ABPCharacter_Demo_C::StaticClass(), &Players);


        if (Players.IsValid() && Players.Num() > 1)
        {
            for (auto player : Players)
            {
                players.push_back(static_cast<ABPCharacter_Demo_C*>(player));
            }
        }


        return players;
    }

    bool OnRayCast(ABPCharacter_Demo_C* Self, AActor* Target, FVector End)
    {
        if (!Self || !Target) return false;

        FVector Start = RotationUtil::GetEyePos();

        FHitResult HitOut;

        TArray<AActor*> ActorsToIgnore;
        ActorsToIgnore.Add(Self);

        bool bHit = UKismetSystemLibrary::LineTraceSingle(
            Self,
            Start,
            End,
            ETraceTypeQuery::TraceTypeQuery1,
            false,
            ActorsToIgnore,
            EDrawDebugTrace::None,
            &HitOut,
            false,
            FLinearColor(),
            FLinearColor(),
            0.0f
        );

        if (bHit)
        {
            if (HitOut.Actor == Target)
            {
                return true;
            }
            return false;
        }

        return true;
    }

    bool OnRayCast(ABPCharacter_Demo_C* Self, ABPCharacter_Demo_C* Target)
    {
        if (!Self || !Target) return false;

        FVector Start = RotationUtil::GetEyePos();

        FVector End = GetPlayerRayCast(Target);

        FHitResult HitOut;

        TArray<AActor*> ActorsToIgnore;
        ActorsToIgnore.Add(Self);

        bool bHit = UKismetSystemLibrary::LineTraceSingle(
            Self,
            Start,
            End,
            ETraceTypeQuery::TraceTypeQuery1,
            false,
            ActorsToIgnore,
            EDrawDebugTrace::None,
            &HitOut,
            false,
            FLinearColor(),
            FLinearColor(),
            0.0f
        );

        if (bHit)
        {
            if (HitOut.Actor == Target)
            {
                return true;
            }
            return false;
        }

        return true;
    }

    void GetRotatedBoxPoints(FVector Center, FRotator Rot, float Radius, float HalfHeight, bool bAccurate, TArray<FVector>& OutPoints)
    {
        float R = Radius;
        float H = HalfHeight;

        OutPoints.Add(Center);

        TArray<FVector> LocalPoints;

        LocalPoints.Add(FVector(R, 0, 0));
        LocalPoints.Add(FVector(-R, 0, 0));
        LocalPoints.Add(FVector(0, R, 0));
        LocalPoints.Add(FVector(0, -R, 0));

        LocalPoints.Add(FVector(0, 0, H));
        LocalPoints.Add(FVector(0, 0, -H));

        if (bAccurate)
        {
            float CornerR = R * 0.707f;

            float Steps[] = {H * 0.3f, H * 0.7f};

            for (float hOffset : Steps)
            {
                LocalPoints.Add(FVector(CornerR, CornerR, hOffset));
                LocalPoints.Add(FVector(CornerR, -CornerR, hOffset));
                LocalPoints.Add(FVector(-CornerR, CornerR, hOffset));
                LocalPoints.Add(FVector(-CornerR, -CornerR, hOffset));

                LocalPoints.Add(FVector(CornerR, CornerR, -hOffset));
                LocalPoints.Add(FVector(CornerR, -CornerR, -hOffset));
                LocalPoints.Add(FVector(-CornerR, CornerR, -hOffset));
                LocalPoints.Add(FVector(-CornerR, -CornerR, -hOffset));
            }

            LocalPoints.Add(FVector(CornerR, 0, H));
            LocalPoints.Add(FVector(-CornerR, 0, H));
            LocalPoints.Add(FVector(0, CornerR, H));
            LocalPoints.Add(FVector(0, -CornerR, H));

            LocalPoints.Add(FVector(CornerR, 0, -H));
            LocalPoints.Add(FVector(-CornerR, 0, -H));
            LocalPoints.Add(FVector(0, CornerR, -H));
            LocalPoints.Add(FVector(0, -CornerR, -H));
        }

        for (const FVector& LocalPos : LocalPoints)
        {
            FVector WorldPos = Center + Utils::CustomRotateVector(Rot, LocalPos);
            OutPoints.Add(WorldPos);
        }
    }

    FVector GetBestAimPoint(ACharacter* TargetCharacter, FVector EyePos, bool bAccurate,
                            FVector* CustomTargetLocation = nullptr)
    {
        auto Pawn = Instances::GetPlayerCharacter();
        if (!TargetCharacter) return {0, 0, 0};

        UCapsuleComponent* Cap = TargetCharacter->CapsuleComponent;
        if (!Cap) return {0, 0, 0};

        float Radius = Cap->GetScaledCapsuleRadius();
        float HalfHeight = Cap->GetScaledCapsuleHalfHeight();

        Radius *= 0.70f;
        HalfHeight *= 0.75f;

        FVector Origin;
        if (CustomTargetLocation != nullptr)
        {
            Origin = *CustomTargetLocation;
        }
        else
        {
            Origin = Cap->K2_GetComponentLocation();
        }

        FRotator Rot = Cap->K2_GetComponentRotation();

        TArray<FVector> Points;
        GetRotatedBoxPoints(Origin, Rot, Radius, HalfHeight, bAccurate, Points);

        FVector BestPoint = Origin;
        float MinDistSq = FLT_MAX;
        bool bFoundAny = false;

        for (const FVector& Point : Points)
        {
            float DistSq = RotationUtil::GetDistanceCm(EyePos, Point);

            if (DistSq < MinDistSq)
            {
                if (!wallCheck->value || OnRayCast(Pawn, TargetCharacter, Point))
                {
                    MinDistSq = DistSq;
                    BestPoint = Point;
                    bFoundAny = true;
                }
            }
        }

        if (!bFoundAny)
        {
            return {0, 0, 0};
        }

        return BestPoint;
    }

    ABPCharacter_Demo_C* FindNearestPlayerTo(ABPCharacter_Demo_C* self,
                                             const std::vector<ABPCharacter_Demo_C*>& players)
    {
        if (!self) return nullptr;

        ABPCharacter_Demo_C* best = nullptr;

        double bestD2 = targetRange->value * 100.0;

        for (ABPCharacter_Demo_C* p : players)
        {
            if (!p || p == self) continue;
            if (fakePlayerCheck->value && !p->PlayerState) continue;

            const double d2 = GetDistanceCm(p);

            if (d2 < bestD2)
            {
                best = p;
                bestD2 = d2;
            }
        }

        return best;
    }

    ABPCharacter_Demo_C* GetTarget()
    {
        auto Pawn = Instances::GetPlayerCharacter();
        if (!Pawn) return nullptr;

        std::vector<ABPCharacter_Demo_C*> players = GetPlayers();

        if (players.empty()) return nullptr;

        return FindNearestPlayerTo(Pawn, players);
    }
};
