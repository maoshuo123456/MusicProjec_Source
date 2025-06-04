// Fill out your copyright notice in the Description page of Project Settings.


// ItemNode.cpp
#include "Nodes/ItemNode.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"

AItemNode::AItemNode()
{
    // 设置默认值
    NodeData.NodeType = ENodeType::Item;
    bIsCarryable = false;
    bAutoActivateCapabilities = true;
    
    // 物品节点默认显示UI
    bAlwaysShowUI = false;
    UIDisplayDistance = 800.0f;
}

void AItemNode::BeginPlay()
{
    Super::BeginPlay();
    
    // 初始化默认能力
    InitializeDefaultCapabilities();
    
    // 如果设置了自动激活，激活所有能力
    if (bAutoActivateCapabilities)
    {
        for (UItemCapability* Capability : Capabilities)
        {
            if (Capability && !Capability->IsActive())
            {
                Capability->Activate();
            }
        }
    }
}

void AItemNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理能力
    CleanupCapabilities();
    
    Super::EndPlay(EndPlayReason);
}

UItemCapability* AItemNode::AddCapability(TSubclassOf<UItemCapability> CapabilityClass)
{
    if (!CapabilityClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemNode %s: Cannot add null capability class"), *NodeData.NodeName);
        return nullptr;
    }

    // 检查是否已存在
    if (HasCapability(CapabilityClass))
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemNode %s: Capability %s already exists"), 
            *NodeData.NodeName, *CapabilityClass->GetName());
        return GetCapability(CapabilityClass);
    }

    // 创建新能力
    UItemCapability* NewCapability = NewObject<UItemCapability>(this, CapabilityClass);
    if (NewCapability)
    {
        NewCapability->RegisterComponent();
        AddCapabilityInstance(NewCapability);
        
        UE_LOG(LogTemp, Log, TEXT("ItemNode %s: Added capability %s"), 
            *NodeData.NodeName, *CapabilityClass->GetName());
    }
    
    return NewCapability;
}

void AItemNode::AddCapabilityInstance(UItemCapability* Capability)
{
    if (!Capability)
    {
        return;
    }

    // 避免重复添加
    if (Capabilities.Contains(Capability))
    {
        return;
    }

    Capabilities.Add(Capability);
    RegisterCapability(Capability);
    
    // 如果节点已经在游戏中且自动激活，立即激活能力
    if (HasActorBegunPlay() && bAutoActivateCapabilities && !Capability->IsActive())
    {
        Capability->Activate();
    }
}

bool AItemNode::RemoveCapability(TSubclassOf<UItemCapability> CapabilityClass)
{
    if (!CapabilityClass)
    {
        return false;
    }

    UItemCapability* Capability = GetCapability(CapabilityClass);
    if (Capability)
    {
        UnregisterCapability(Capability);
        Capabilities.Remove(Capability);
        
        // 销毁组件
        Capability->DestroyComponent();
        
        UE_LOG(LogTemp, Log, TEXT("ItemNode %s: Removed capability %s"), 
            *NodeData.NodeName, *CapabilityClass->GetName());
        
        return true;
    }
    
    return false;
}

UItemCapability* AItemNode::GetCapability(TSubclassOf<UItemCapability> CapabilityClass) const
{
    if (!CapabilityClass)
    {
        return nullptr;
    }

    // 先检查缓存
    if (CapabilityMap.Contains(CapabilityClass))
    {
        return CapabilityMap[CapabilityClass];
    }

    // 如果缓存中没有，遍历查找
    for (UItemCapability* Capability : Capabilities)
    {
        if (Capability && Capability->IsA(CapabilityClass))
        {
            return Capability;
        }
    }
    
    return nullptr;
}

bool AItemNode::HasCapability(TSubclassOf<UItemCapability> CapabilityClass) const
{
    return GetCapability(CapabilityClass) != nullptr;
}

bool AItemNode::UseCapability(TSubclassOf<UItemCapability> CapabilityClass, const FInteractionData& Data)
{
    UItemCapability* Capability = GetCapability(CapabilityClass);
    if (Capability && Capability->CanUse(Data))
    {
        bool bSuccess = Capability->Use(Data);
        OnCapabilityUsed(Capability, bSuccess);
        return bSuccess;
    }
    
    return false;
}

int32 AItemNode::UseAllCapabilities(const FInteractionData& Data)
{
    int32 UsedCount = 0;
    
    for (UItemCapability* Capability : Capabilities)
    {
        if (Capability && Capability->CanUse(Data))
        {
            if (Capability->Use(Data))
            {
                UsedCount++;
                OnCapabilityUsed(Capability, true);
            }
        }
    }
    
    return UsedCount;
}

void AItemNode::AddUnlockStory(const FString& StoryID)
{
    if (!StoryID.IsEmpty() && !UnlockStoryIDs.Contains(StoryID))
    {
        UnlockStoryIDs.Add(StoryID);
    }
}

void AItemNode::AddConditionalSpawn(const FString& Condition, const FNodeGenerateData& SpawnData)
{
    ConditionalSpawns.Add(Condition, SpawnData);
}

TArray<FNodeGenerateData> AItemNode::GetConditionalSpawns(const FString& Condition) const
{
    TArray<FNodeGenerateData> Result;
    
    for (const auto& Pair : ConditionalSpawns)
    {
        if (Pair.Key == Condition || Condition.IsEmpty())
        {
            Result.Add(Pair.Value);
        }
    }
    
    return Result;
}

void AItemNode::OnInteract_Implementation(const FInteractionData& Data)
{
    if (!CanInteract(Data))
    {
        return;
    }

    Super::OnInteract_Implementation(Data);
    
    // 尝试使用所有能力
    int32 UsedCapabilities = UseAllCapabilities(Data);
    
    // 如果没有能力被使用，执行默认行为
    if (UsedCapabilities == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("ItemNode %s: No capabilities used, performing default interaction"), 
            *NodeData.NodeName);
        
        // 默认交互：如果是激活状态，标记为完成
        if (CurrentState == ENodeState::Active)
        {
            SetNodeState(ENodeState::Completed);
        }
    }
    
    // 检查故事条件
    CheckStoryConditions();
}

bool AItemNode::CanInteract_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanInteract_Implementation(Data))
    {
        return false;
    }

    // 检查是否有任何能力可以使用
    for (UItemCapability* Capability : Capabilities)
    {
        if (Capability && Capability->CanUse(Data))
        {
            return true;
        }
    }
    
    // 如果没有能力，仍然允许基础交互
    return true;
}

void AItemNode::OnStateChanged_Implementation(ENodeState OldState, ENodeState NewState)
{
    Super::OnStateChanged_Implementation(OldState, NewState);
    
    // 通知所有能力状态变化
    NotifyCapabilitiesStateChange();
    
    // 特定状态处理
    switch (NewState)
    {
    case ENodeState::Active:
        // 激活时可能需要激活能力
        if (bAutoActivateCapabilities)
        {
            for (UItemCapability* Capability : Capabilities)
            {
                if (Capability && !Capability->IsActive())
                {
                    Capability->Activate();
                }
            }
        }
        break;
        
    case ENodeState::Inactive:
    case ENodeState::Locked:
        // 停用所有能力
        for (UItemCapability* Capability : Capabilities)
        {
            if (Capability && Capability->IsActive())
            {
                Capability->Deactivate();
            }
        }
        break;
    }
}

bool AItemNode::ShouldTriggerStory_Implementation() const
{
    return Super::ShouldTriggerStory_Implementation() || HasUnlockStories();
}

TArray<FNodeGenerateData> AItemNode::GetNodeSpawnData_Implementation() const
{
    TArray<FNodeGenerateData> AllSpawns = Super::GetNodeSpawnData_Implementation();
    
    // 添加条件生成的节点
    // 这里简单地添加所有条件生成，实际使用中应该根据具体条件
    for (const auto& Pair : ConditionalSpawns)
    {
        AllSpawns.Add(Pair.Value);
    }
    
    return AllSpawns;
}

FText AItemNode::GetInteractionPrompt_Implementation() const
{
    // 尝试获取最佳能力提示
    FString BestPrompt = GetBestCapabilityPrompt();
    
    if (!BestPrompt.IsEmpty())
    {
        return FText::FromString(BestPrompt);
    }
    
    // 使用默认提示
    return Super::GetInteractionPrompt_Implementation();
}

void AItemNode::UpdateVisuals_Implementation()
{
    Super::UpdateVisuals_Implementation();
    
    // 物品可能有特殊的视觉效果
    if (NodeMesh)
    {
        // 可携带物品可能有不同的表现
        if (bIsCarryable && CurrentState == ENodeState::Active)
        {
            // 添加可携带物品的视觉提示
        }
    }
}

void AItemNode::RegisterCapability(UItemCapability* Capability)
{
    if (!Capability)
    {
        return;
    }

    // 更新缓存
    CapabilityMap.Add(Capability->GetClass(), Capability);
    
    // 初始化能力
    Capability->Initialize(this);
}

void AItemNode::UnregisterCapability(UItemCapability* Capability)
{
    if (!Capability)
    {
        return;
    }

    // 从缓存中移除
    CapabilityMap.Remove(Capability->GetClass());
    
    // 停用能力
    if (Capability->IsActive())
    {
        Capability->Deactivate();
    }
}

void AItemNode::NotifyCapabilitiesStateChange()
{
    for (UItemCapability* Capability : Capabilities)
    {
        if (Capability)
        {
            Capability->OnOwnerStateChanged(CurrentState);
        }
    }
}

void AItemNode::CheckStoryConditions_Implementation()
{
    // 检查是否应该解锁故事
    if (CurrentState == ENodeState::Completed && HasUnlockStories())
    {
        // 触发故事解锁
        for (const FString& StoryID : UnlockStoryIDs)
        {
            AddTriggerEvent(FString::Printf(TEXT("UnlockStory_%s"), *StoryID));
        }
    }
    
    // 检查条件生成
    if (CurrentState == ENodeState::Completed)
    {
        // 基于完成状态的生成
        TArray<FNodeGenerateData> CompletionSpawns = GetConditionalSpawns(TEXT("OnComplete"));
        if (CompletionSpawns.Num() > 0)
        {
            // 这些数据会被父场景或系统管理器处理
            UE_LOG(LogTemp, Log, TEXT("ItemNode %s: Has %d nodes to spawn on completion"), 
                *NodeData.NodeName, CompletionSpawns.Num());
        }
    }
}

void AItemNode::OnCapabilityUsed_Implementation(UItemCapability* Capability, bool bSuccess)
{
    if (!Capability)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ItemNode %s: Capability %s used, Success: %s"), 
        *NodeData.NodeName, 
        *Capability->GetClass()->GetName(),
        bSuccess ? TEXT("Yes") : TEXT("No"));
    
    // 可以在这里处理能力使用后的逻辑
    if (bSuccess)
    {
        // 成功使用能力可能触发特定事件
        AddTriggerEvent(FString::Printf(TEXT("CapabilityUsed_%s"), *Capability->GetCapabilityID()));
    }
}

FString AItemNode::GetBestCapabilityPrompt_Implementation() const
{
    // 找到第一个可用能力的提示
    for (UItemCapability* Capability : Capabilities)
    {
        if (Capability && Capability->IsActive())
        {
            FString Prompt = Capability->GetUsagePrompt();
            if (!Prompt.IsEmpty())
            {
                return Prompt;
            }
        }
    }
    
    return FString();
}

void AItemNode::InitializeDefaultCapabilities()
{
    // 子类可以在这里添加默认能力
    // 例如：基于节点标签或属性自动添加能力
    
    // 检查自定义属性中是否定义了默认能力
    if (NodeData.CustomProperties.Contains(TEXT("DefaultCapabilities")))
    {
        FString CapabilitiesStr = NodeData.CustomProperties[TEXT("DefaultCapabilities")];
        TArray<FString> CapabilityNames;
        CapabilitiesStr.ParseIntoArray(CapabilityNames, TEXT(","));
        
        // 这里需要一个能力类注册表来查找能力类
        // 暂时留空，等能力系统实现后再完善
    }
}

void AItemNode::CleanupCapabilities()
{
    // 清理所有能力
    for (UItemCapability* Capability : Capabilities)
    {
        if (Capability)
        {
            UnregisterCapability(Capability);
        }
    }
    
    Capabilities.Empty();
    CapabilityMap.Empty();
}