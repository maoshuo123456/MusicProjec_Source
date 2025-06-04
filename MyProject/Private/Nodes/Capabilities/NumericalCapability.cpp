// Fill out your copyright notice in the Description page of Project Settings.

#include "Nodes/Capabilities/NumericalCapability.h"
#include "Nodes/ItemNode.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/NodeSystemManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UNumericalCapability::UNumericalCapability()
{
    // 设置默认值
    CapabilityID = TEXT("NumericalCapability");
    CapabilityDescription = FText::FromString(TEXT("管理游戏中的各种数值系统"));
    UsagePrompt = TEXT("使用数值");
    
    // 启用Tick以处理回复和消耗
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    // 预定义数值默认值
    PlayerHealth = 100.0f;
    PlayerMaxHealth = 100.0f;
    MentalState = 100.0f;
    MaxMentalState = 100.0f;
    
    // 资源管理默认值
    bAutoConsume = false;
    ResourceUpdateInterval = 0.5f;
    
    // 感知系统默认值
    VisionRadius = 1000.0f;
    HearingRadius = 800.0f;
    DangerSenseRadius = 1500.0f;
    PerceptibleNodeClass = AInteractiveNode::StaticClass();
    
    // 好感度系统默认值
    AffinityMin = -100.0f;
    AffinityMax = 100.0f;
    
    // 内部状态
    CachedSystemManager = nullptr;
    AccumulatedDeltaTime = 0.0f;
}

void UNumericalCapability::Initialize_Implementation(AItemNode* Owner)
{
    Super::Initialize_Implementation(Owner);
    
    // 缓存系统管理器
    CachedSystemManager = GetNodeSystemManager();
    
    // 初始化预定义数值
    InitializePredefinedValues();
    
    // 从配置加载数据
    if (NumericalConfig.Num() > 0)
    {
        LoadNumericalConfig(NumericalConfig);
    }
    
    UE_LOG(LogTemp, Log, TEXT("NumericalCapability initialized for %s"), 
        Owner ? *Owner->GetNodeName() : TEXT("Unknown"));
}

void UNumericalCapability::BeginPlay()
{
    Super::BeginPlay();
    
    // 启动资源更新定时器
    if (bAutoConsume && ResourceUpdateInterval > 0.0f && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            ResourceUpdateTimerHandle,
            this,
            &UNumericalCapability::OnResourceUpdateTimer,
            ResourceUpdateInterval,
            true
        );
    }
}

void UNumericalCapability::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理定时器
    if (ResourceUpdateTimerHandle.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ResourceUpdateTimerHandle);
    }
    
    Super::EndPlay(EndPlayReason);
}

bool UNumericalCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanUse_Implementation(Data))
    {
        return false;
    }
    
    // 数值能力通常总是可用的
    return true;
}

bool UNumericalCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!Super::Use_Implementation(Data))
    {
        return false;
    }
    
    // 示例：使用时回复一些血量
    if (PlayerHealth < PlayerMaxHealth)
    {
        ModifyHealth(20.0f);
        UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Restored 20 health"));
        return true;
    }
    
    // 或者回复精神值
    if (MentalState < MaxMentalState)
    {
        ModifyMentalState(15.0f);
        UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Restored 15 mental state"));
        return true;
    }
    
    return true;
}

void UNumericalCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    Super::OnOwnerStateChanged_Implementation(NewState);
    
    // 某些状态可能影响数值
    if (NewState == ENodeState::Active)
    {
        // 激活时可能提供增益
        SetRegenerationRate(TEXT("Health"), 2.0f);
    }
    else if (NewState == ENodeState::Inactive)
    {
        // 非激活时停止回复
        SetRegenerationRate(TEXT("Health"), 0.0f);
    }
}

void UNumericalCapability::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!bCapabilityIsActive)
    {
        return;
    }
    
    // 处理回复
    ProcessRegeneration(DeltaTime);
    
    // 处理资源消耗
    if (bAutoConsume)
    {
        AccumulatedDeltaTime += DeltaTime;
        if (AccumulatedDeltaTime >= ResourceUpdateInterval)
        {
            ProcessResourceConsumption(AccumulatedDeltaTime);
            AccumulatedDeltaTime = 0.0f;
        }
    }
}

void UNumericalCapability::SetValue(const FString& ValueID, float Value)
{
    if (ValueID.IsEmpty())
    {
        return;
    }
    
    NumericalValues.Add(ValueID, Value);
    ClampValue(ValueID);
    
    // 更新预定义值
    UpdatePredefinedValues();
    
    UE_LOG(LogTemp, Verbose, TEXT("NumericalCapability: Set %s to %f"), *ValueID, Value);
}

float UNumericalCapability::GetValue(const FString& ValueID) const
{
    if (NumericalValues.Contains(ValueID))
    {
        return NumericalValues[ValueID];
    }
    
    // 检查预定义值
    if (ValueID == TEXT("Health"))
    {
        return PlayerHealth;
    }
    else if (ValueID == TEXT("MentalState"))
    {
        return MentalState;
    }
    
    return 0.0f;
}

void UNumericalCapability::ModifyValue(const FString& ValueID, float Delta)
{
    float CurrentValue = GetValue(ValueID);
    SetValue(ValueID, CurrentValue + Delta);
}

void UNumericalCapability::RegisterNewValue(const FString& ValueID, float InitialValue, float MinVal, float MaxVal)
{
    if (ValueID.IsEmpty())
    {
        return;
    }
    
    NumericalValues.Add(ValueID, InitialValue);
    MinValues.Add(ValueID, MinVal);
    MaxValues.Add(ValueID, MaxVal);
    
    ClampValue(ValueID);
    
    UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Registered new value %s (%.1f - %.1f)"), 
        *ValueID, MinVal, MaxVal);
}

void UNumericalCapability::SetValueRange(const FString& ValueID, float MinVal, float MaxVal)
{
    MinValues.Add(ValueID, MinVal);
    MaxValues.Add(ValueID, MaxVal);
    
    ClampValue(ValueID);
}

void UNumericalCapability::SetRegenerationRate(const FString& ValueID, float Rate)
{
    if (Rate != 0.0f)
    {
        RegenerationRates.Add(ValueID, Rate);
    }
    else
    {
        RegenerationRates.Remove(ValueID);
    }
}

float UNumericalCapability::GetValuePercentage(const FString& ValueID) const
{
    float Current = GetValue(ValueID);
    float Max = MaxValues.Contains(ValueID) ? MaxValues[ValueID] : 100.0f;
    
    if (Max == 0.0f)
    {
        return 0.0f;
    }
    
    return (Current / Max) * 100.0f;
}

void UNumericalCapability::ModifyHealth(float Delta)
{
    PlayerHealth = FMath::Clamp(PlayerHealth + Delta, 0.0f, PlayerMaxHealth);
    SetValue(TEXT("Health"), PlayerHealth);
    
    // 检查死亡
    if (PlayerHealth <= 0.0f && OwnerItem)
    {
        OwnerItem->SetNodeState(ENodeState::Inactive);
    }
}

void UNumericalCapability::ModifyMentalState(float Delta)
{
    MentalState = FMath::Clamp(MentalState + Delta, 0.0f, MaxMentalState);
    SetValue(TEXT("MentalState"), MentalState);
    
    // 低精神值可能影响感知
    if (MentalState < MaxMentalState * 0.3f)
    {
        // 降低感知范围
        float Modifier = MentalState / (MaxMentalState * 0.3f);
        SetPerceptionRadius(EPerceptionType::Vision, VisionRadius * Modifier);
    }
}

float UNumericalCapability::GetHealthPercentage() const
{
    return PlayerMaxHealth > 0.0f ? (PlayerHealth / PlayerMaxHealth) * 100.0f : 0.0f;
}

float UNumericalCapability::GetMentalStatePercentage() const
{
    return MaxMentalState > 0.0f ? (MentalState / MaxMentalState) * 100.0f : 0.0f;
}

bool UNumericalCapability::ConsumeResource(const FString& ResourceID, float Amount)
{
    if (Amount <= 0.0f || !ResourcePools.Contains(ResourceID))
    {
        return false;
    }
    
    float CurrentAmount = ResourcePools[ResourceID];
    if (CurrentAmount < Amount)
    {
        UE_LOG(LogTemp, Warning, TEXT("NumericalCapability: Not enough resource %s (%.1f < %.1f)"), 
            *ResourceID, CurrentAmount, Amount);
        return false;
    }
    
    ResourcePools[ResourceID] = CurrentAmount - Amount;
    
    UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Consumed %.1f %s"), Amount, *ResourceID);
    return true;
}

void UNumericalCapability::ReplenishResource(const FString& ResourceID, float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }
    
    float CurrentAmount = ResourcePools.Contains(ResourceID) ? ResourcePools[ResourceID] : 0.0f;
    float MaxAmount = MaxValues.Contains(ResourceID) ? MaxValues[ResourceID] : 100.0f;
    
    ResourcePools.Add(ResourceID, FMath::Min(CurrentAmount + Amount, MaxAmount));
    
    UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Replenished %.1f %s"), Amount, *ResourceID);
}

float UNumericalCapability::GetResourceAmount(const FString& ResourceID) const
{
    return ResourcePools.Contains(ResourceID) ? ResourcePools[ResourceID] : 0.0f;
}

bool UNumericalCapability::HasEnoughResource(const FString& ResourceID, float Amount) const
{
    return GetResourceAmount(ResourceID) >= Amount;
}

void UNumericalCapability::SetConsumptionRate(const FString& ResourceID, float Rate)
{
    if (Rate > 0.0f)
    {
        ConsumptionRates.Add(ResourceID, Rate);
    }
    else
    {
        ConsumptionRates.Remove(ResourceID);
    }
}

void UNumericalCapability::UpdateProgress(const FString& ProgressID, float Delta)
{
    float CurrentProgress = ProgressTrackers.Contains(ProgressID) ? ProgressTrackers[ProgressID] : 0.0f;
    SetProgress(ProgressID, CurrentProgress + Delta);
}

void UNumericalCapability::SetProgress(const FString& ProgressID, float Value)
{
    ProgressTrackers.Add(ProgressID, FMath::Max(0.0f, Value));
    
    // 检查里程碑
    CheckAndTriggerMilestones(ProgressID);
}

float UNumericalCapability::GetProgress(const FString& ProgressID) const
{
    return ProgressTrackers.Contains(ProgressID) ? ProgressTrackers[ProgressID] : 0.0f;
}

bool UNumericalCapability::HasReachedMilestone(const FString& ProgressID) const
{
    if (!ProgressTrackers.Contains(ProgressID) || !ProgressMilestones.Contains(ProgressID))
    {
        return false;
    }
    
    return ProgressTrackers[ProgressID] >= ProgressMilestones[ProgressID];
}

void UNumericalCapability::RegisterMilestone(const FString& ProgressID, float MilestoneValue, const FString& Reward)
{
    ProgressMilestones.Add(ProgressID, MilestoneValue);
    MilestoneRewards.Add(ProgressID, Reward);
}

TArray<AInteractiveNode*> UNumericalCapability::PerceiveNodesInRadius(float Radius, TSubclassOf<AInteractiveNode> NodeClass) const
{
    TArray<AInteractiveNode*> PerceivedNodes;
    
    if (!OwnerItem || Radius <= 0.0f)
    {
        return PerceivedNodes;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return PerceivedNodes;
    }
    
    // 获取范围内的节点
    TArray<AInteractiveNode*> NodesInRadius = SystemManager->GetNodesInRadius(
        OwnerItem->GetActorLocation(), 
        Radius
    );
    
    // 过滤节点类型
    TSubclassOf<AInteractiveNode> FilterClass = NodeClass ? NodeClass : PerceptibleNodeClass;
    
    for (AInteractiveNode* Node : NodesInRadius)
    {
        if (Node && Node != OwnerItem && Node->IsA(FilterClass))
        {
            // 检查节点是否可被感知
            if (Node->GetNodeState() != ENodeState::Hidden)
            {
                PerceivedNodes.Add(Node);
            }
        }
    }
    
    return PerceivedNodes;
}

TArray<AInteractiveNode*> UNumericalCapability::PerceiveNodesByType(EPerceptionType Type) const
{
    float Radius = GetPerceptionRadius(Type);
    
    if (Type == EPerceptionType::All)
    {
        // 使用最大感知半径
        Radius = FMath::Max3(VisionRadius, HearingRadius, DangerSenseRadius);
    }
    
    return PerceiveNodesInRadius(Radius, PerceptibleNodeClass);
}

void UNumericalCapability::SetPerceptionRadius(EPerceptionType Type, float Radius)
{
    switch (Type)
    {
    case EPerceptionType::Vision:
        VisionRadius = FMath::Max(0.0f, Radius);
        break;
    case EPerceptionType::Hearing:
        HearingRadius = FMath::Max(0.0f, Radius);
        break;
    case EPerceptionType::Danger:
        DangerSenseRadius = FMath::Max(0.0f, Radius);
        break;
    case EPerceptionType::All:
        VisionRadius = HearingRadius = DangerSenseRadius = FMath::Max(0.0f, Radius);
        break;
    }
}

float UNumericalCapability::GetPerceptionRadius(EPerceptionType Type) const
{
    switch (Type)
    {
    case EPerceptionType::Vision:
        return VisionRadius;
    case EPerceptionType::Hearing:
        return HearingRadius;
    case EPerceptionType::Danger:
        return DangerSenseRadius;
    case EPerceptionType::All:
        return FMath::Max3(VisionRadius, HearingRadius, DangerSenseRadius);
    default:
        return 0.0f;
    }
}

void UNumericalCapability::ModifyAffinity_Implementation(const FString& TargetID, float Delta)
{
    if (TargetID.IsEmpty())
    {
        return;
    }
    
    float CurrentAffinity = AffinityValues.Contains(TargetID) ? AffinityValues[TargetID] : 0.0f;
    float NewAffinity = FMath::Clamp(CurrentAffinity + Delta, AffinityMin, AffinityMax);
    
    AffinityValues.Add(TargetID, NewAffinity);
    
    // 可能创建或更新Emotional关系
    if (FMath::Abs(NewAffinity) > 50.0f)
    {
        CreateEmotionalConnection(TargetID);
    }
    
    UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Modified affinity for %s by %.1f (now %.1f)"), 
        *TargetID, Delta, NewAffinity);
}

float UNumericalCapability::GetAffinity(const FString& TargetID) const
{
    return AffinityValues.Contains(TargetID) ? AffinityValues[TargetID] : 0.0f;
}

void UNumericalCapability::LoadNumericalConfig(const TMap<FString, FString>& Config)
{
    for (const auto& Pair : Config)
    {
        ApplyConfigValue(Pair.Key, Pair.Value);
    }
}

void UNumericalCapability::ApplyConfigValue(const FString& Key, const FString& Value)
{
    if (Key == TEXT("PlayerMaxHealth"))
    {
        PlayerMaxHealth = FCString::Atof(*Value);
        PlayerHealth = FMath::Min(PlayerHealth, PlayerMaxHealth);
    }
    else if (Key == TEXT("MaxMentalState"))
    {
        MaxMentalState = FCString::Atof(*Value);
        MentalState = FMath::Min(MentalState, MaxMentalState);
    }
    else if (Key == TEXT("VisionRadius"))
    {
        VisionRadius = FCString::Atof(*Value);
    }
    else if (Key == TEXT("HearingRadius"))
    {
        HearingRadius = FCString::Atof(*Value);
    }
    else if (Key == TEXT("DangerSenseRadius"))
    {
        DangerSenseRadius = FCString::Atof(*Value);
    }
    else if (Key == TEXT("AutoConsume"))
    {
        bAutoConsume = Value.ToBool();
    }
    else if (Key.StartsWith(TEXT("Value_")))
    {
        FString ValueID = Key.RightChop(6); // 移除 "Value_"
        SetValue(ValueID, FCString::Atof(*Value));
    }
    else if (Key.StartsWith(TEXT("Resource_")))
    {
        FString ResourceID = Key.RightChop(9); // 移除 "Resource_"
        ResourcePools.Add(ResourceID, FCString::Atof(*Value));
    }
    else if (Key.StartsWith(TEXT("Progress_")))
    {
        FString ProgressID = Key.RightChop(9); // 移除 "Progress_"
        SetProgress(ProgressID, FCString::Atof(*Value));
    }
    
    // 保存到配置
    NumericalConfig.Add(Key, Value);
}

ANodeSystemManager* UNumericalCapability::GetNodeSystemManager() const
{
    if (CachedSystemManager)
    {
        return CachedSystemManager;
    }
    
    UWorld* World = GetWorld();
    if (World)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, ANodeSystemManager::StaticClass(), FoundActors);
        
        if (FoundActors.Num() > 0)
        {
            return Cast<ANodeSystemManager>(FoundActors[0]);
        }
    }
    
    return nullptr;
}

void UNumericalCapability::ProcessRegeneration(float DeltaTime)
{
    for (const auto& Regen : RegenerationRates)
    {
        if (Regen.Value != 0.0f)
        {
            ModifyValue(Regen.Key, Regen.Value * DeltaTime);
        }
    }
}

void UNumericalCapability::ProcessResourceConsumption(float DeltaTime)
{
    for (const auto& Consumption : ConsumptionRates)
    {
        if (Consumption.Value > 0.0f)
        {
            float ConsumeAmount = Consumption.Value * DeltaTime;
            
            if (!ConsumeResource(Consumption.Key, ConsumeAmount))
            {
                // 资源耗尽，可能触发某些效果
                if (OwnerItem)
                {
                    OwnerItem->AddTriggerEvent(FString::Printf(TEXT("ResourceDepleted_%s"), *Consumption.Key));
                }
            }
        }
    }
}

void UNumericalCapability::CheckAndTriggerMilestones(const FString& ProgressID)
{
    if (!ProgressMilestones.Contains(ProgressID))
    {
        return;
    }
    
    float CurrentProgress = GetProgress(ProgressID);
    float MilestoneValue = ProgressMilestones[ProgressID];
    
    // 检查是否第一次达到里程碑
    float LastChecked = LastMilestoneChecked.Contains(ProgressID) ? LastMilestoneChecked[ProgressID] : 0.0f;
    
    if (CurrentProgress >= MilestoneValue && LastChecked < MilestoneValue)
    {
        // 触发里程碑奖励
        if (MilestoneRewards.Contains(ProgressID))
        {
            FString Reward = MilestoneRewards[ProgressID];
            
            if (OwnerItem)
            {
                OwnerItem->AddTriggerEvent(FString::Printf(TEXT("MilestoneReached_%s_%s"), *ProgressID, *Reward));
            }
            
            UE_LOG(LogTemp, Log, TEXT("NumericalCapability: Milestone reached for %s, reward: %s"), 
                *ProgressID, *Reward);
        }
        
        // 更新最后检查的进度
        LastMilestoneChecked.Add(ProgressID, CurrentProgress);
    }
}

void UNumericalCapability::ClampValue(const FString& ValueID)
{
    if (!NumericalValues.Contains(ValueID))
    {
        return;
    }
    
    float CurrentValue = NumericalValues[ValueID];
    float MinVal = MinValues.Contains(ValueID) ? MinValues[ValueID] : -MAX_FLT;
    float MaxVal = MaxValues.Contains(ValueID) ? MaxValues[ValueID] : MAX_FLT;
    
    NumericalValues[ValueID] = FMath::Clamp(CurrentValue, MinVal, MaxVal);
}

void UNumericalCapability::InitializePredefinedValues()
{
    // 注册预定义数值
    RegisterNewValue(TEXT("Health"), PlayerHealth, 0.0f, PlayerMaxHealth);
    RegisterNewValue(TEXT("MentalState"), MentalState, 0.0f, MaxMentalState);
}

void UNumericalCapability::UpdatePredefinedValues()
{
    // 同步预定义值
    if (NumericalValues.Contains(TEXT("Health")))
    {
        PlayerHealth = NumericalValues[TEXT("Health")];
    }
    
    if (NumericalValues.Contains(TEXT("MentalState")))
    {
        MentalState = NumericalValues[TEXT("MentalState")];
    }
}

ANodeConnection* UNumericalCapability::CreateEmotionalConnection(const FString& TargetNodeID)
{
    if (!OwnerItem || TargetNodeID.IsEmpty())
    {
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    // 创建Emotional关系
    FNodeRelationData RelationData;
    RelationData.SourceNodeID = OwnerItem->GetNodeID();
    RelationData.TargetNodeID = TargetNodeID;
    RelationData.RelationType = ENodeRelationType::Emotional;
    RelationData.Weight = FMath::Abs(GetAffinity(TargetNodeID)) / AffinityMax;
    RelationData.bBidirectional = true;
    
    AInteractiveNode* TargetNode = SystemManager->GetNode(TargetNodeID);
    if (TargetNode)
    {
        return SystemManager->CreateConnection(OwnerItem, TargetNode, RelationData);
    }
    
    return nullptr;
}

void UNumericalCapability::OnResourceUpdateTimer()
{
    // 定时处理资源消耗
    if (bAutoConsume)
    {
        ProcessResourceConsumption(ResourceUpdateInterval);
    }
}