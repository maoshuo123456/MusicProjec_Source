// Fill out your copyright notice in the Description page of Project Settings.

// ItemCapability.cpp
#include "Nodes/Capabilities/ItemCapability.h"
#include "Nodes/ItemNode.h"
#include "Engine/World.h"
#include "TimerManager.h"

UItemCapability::UItemCapability()
{
    // 设置组件tick
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    
    // 默认值
    bCapabilityIsActive = false;
    CooldownDuration = 0.0f;
    CurrentCooldown = 0.0f;
    CapabilityID = GetClass()->GetName();
    UsagePrompt = TEXT("Use");
}

void UItemCapability::BeginPlay()
{
    Super::BeginPlay();
    
    // 自动查找拥有者
    if (!OwnerItem)
    {
        OwnerItem = Cast<AItemNode>(GetOwner());
    }
    
    // 验证拥有者
    if (!ValidateOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemCapability %s: Invalid owner"), *CapabilityID);
        Deactivate();
    }
}

void UItemCapability::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理定时器
    if (CooldownTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
    }
    
    Super::EndPlay(EndPlayReason);
}

void UItemCapability::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // 更新冷却时间
    if (CurrentCooldown > 0.0f)
    {
        CurrentCooldown = FMath::Max(0.0f, CurrentCooldown - DeltaTime);
    }
}

void UItemCapability::Initialize_Implementation(AItemNode* Owner)
{
    OwnerItem = Owner;
    
    if (!ValidateOwner())
    {
        UE_LOG(LogTemp, Error, TEXT("ItemCapability %s: Failed to initialize with owner %s"), 
            *CapabilityID, Owner ? *Owner->GetNodeName() : TEXT("null"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ItemCapability %s initialized for %s"), 
        *CapabilityID, *OwnerItem->GetNodeName());
}

void UItemCapability::CapabilityActivate_Implementation()
{
    if (bCapabilityIsActive)
    {
        return;
    }
    
    if (!ValidateOwner())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemCapability %s: Cannot activate without valid owner"), *CapabilityID);
        return;
    }
    
    bCapabilityIsActive = true;
    
    // 如果需要tick来更新冷却，启用它
    if (CooldownDuration > 0.0f)
    {
        SetComponentTickEnabled(true);
    }
    
    UE_LOG(LogTemp, Log, TEXT("ItemCapability %s activated"), *CapabilityID);
}

void UItemCapability::CapabilityDeactivate_Implementation()
{
    if (!bCapabilityIsActive)
    {
        return;
    }
    
    bCapabilityIsActive = false;
    SetComponentTickEnabled(false);
    
    // 清理冷却
    ResetCooldown();
    
    UE_LOG(LogTemp, Log, TEXT("ItemCapability %s deactivated"), *CapabilityID);
}

bool UItemCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    // 基础检查
    if (!bCapabilityIsActive)
    {
        return false;
    }
    
    if (!ValidateOwner())
    {
        return false;
    }
    
    // 检查拥有者状态
    if (OwnerItem->GetNodeState() != ENodeState::Active)
    {
        return false;
    }
    
    // 检查冷却
    if (IsOnCooldown())
    {
        return false;
    }
    
    // 检查前置条件
    return CheckPrerequisites(Data);
}

bool UItemCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!CanUse(Data))
    {
        OnUseFailed(Data);
        return false;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ItemCapability %s used by %s"), 
        *CapabilityID, 
        Data.Instigator ? *Data.Instigator->GetName() : TEXT("Unknown"));
    
    // 启动冷却
    if (CooldownDuration > 0.0f)
    {
        StartCooldown();
    }
    
    OnUseSuccess(Data);
    return true;
}

float UItemCapability::GetCooldownProgress() const
{
    if (CooldownDuration <= 0.0f)
    {
        return 1.0f;
    }
    
    return 1.0f - (CurrentCooldown / CooldownDuration);
}

void UItemCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    // 根据拥有者状态调整能力状态
    switch (NewState)
    {
    case ENodeState::Active:
        // 拥有者激活时，能力可以被激活
        if (!bCapabilityIsActive && OwnerItem && OwnerItem->bAutoActivateCapabilities)
        {
            Activate();
        }
        break;
        
    case ENodeState::Inactive:
    case ENodeState::Locked:
    case ENodeState::Hidden:
        // 拥有者非激活时，停用能力
        if (bCapabilityIsActive)
        {
            Deactivate();
        }
        break;
        
    case ENodeState::Completed:
        // 完成状态下能力可能保持激活
        break;
    }
}

FCapabilityData UItemCapability::GetCapabilityInfo_Implementation() const
{
    FCapabilityData Info;
    Info.CapabilityClass = GetClass();
    Info.CapabilityID = CapabilityID;
    Info.bAutoActivate = true;
    
    // 添加一些基础参数
    Info.CapabilityParameters.Add(TEXT("CooldownDuration"), FString::SanitizeFloat(CooldownDuration));
    Info.CapabilityParameters.Add(TEXT("IsActive"), bCapabilityIsActive ? TEXT("true") : TEXT("false"));
    
    return Info;
}

void UItemCapability::OnUseSuccess_Implementation(const FInteractionData& Data)
{
    // 子类可以重写此方法处理成功使用
}

void UItemCapability::OnUseFailed_Implementation(const FInteractionData& Data)
{
    // 子类可以重写此方法处理使用失败
    if (IsOnCooldown())
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemCapability %s: Still on cooldown (%.1fs remaining)"), 
            *CapabilityID, CurrentCooldown);
    }
}

void UItemCapability::StartCooldown()
{
    if (CooldownDuration <= 0.0f)
    {
        return;
    }
    
    CurrentCooldown = CooldownDuration;
    
    // 设置定时器
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            CooldownTimerHandle,
            this,
            &UItemCapability::OnCooldownComplete,
            CooldownDuration,
            false
        );
    }
}

void UItemCapability::ResetCooldown()
{
    CurrentCooldown = 0.0f;
    
    if (CooldownTimerHandle.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
    }
}

bool UItemCapability::ValidateOwner_Implementation() const
{
    return OwnerItem != nullptr;
}

bool UItemCapability::CheckPrerequisites_Implementation(const FInteractionData& Data) const
{
    // 基类默认返回true，子类可以添加具体的前置条件检查
    return true;
}

void UItemCapability::OnCooldownComplete()
{
    CurrentCooldown = 0.0f;
    
    UE_LOG(LogTemp, Log, TEXT("ItemCapability %s: Cooldown complete"), *CapabilityID);
    
    // 如果不需要持续更新冷却显示，可以关闭tick
    if (!GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle))
    {
        SetComponentTickEnabled(false);
    }
}
