// Fill out your copyright notice in the Description page of Project Settings.

#include "Nodes/Capabilities/SystemCapability.h"
#include "Nodes/ItemNode.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/NodeSystemManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"

USystemCapability::USystemCapability()
{
    // 设置默认值
    CapabilityID = TEXT("SystemCapability");
    CapabilityDescription = FText::FromString(TEXT("控制游戏系统层面的功能"));
    UsagePrompt = TEXT("系统控制");
    
    // 时间控制默认值
    TimeScale = 1.0f;
    bCanPauseTime = false;
    TimeDuration = 0.0f;
    RemainingTimeDuration = 0.0f;
    
    // 条件系统默认值
    bAutoEvaluateConditions = false;
    ConditionCheckInterval = 1.0f;
    
    // 关系管理默认值
    MaxRelationships = 10;
    ConnectionClass = ANodeConnection::StaticClass();
    
    // 规则修改默认值
    bCanModifyPermanently = false;
    
    // 威胁管理默认值
    ThreatUpdateInterval = 0.5f;
    
    // 概率控制默认值
    GlobalProbabilityModifier = 1.0f;
    RandomSeed = -1; // 使用默认随机种子
    
    // 节点生成默认值
    SpawnRadius = 500.0f;
    MaxGeneratedNodes = 20;
    
    // 内部状态
    CachedSystemManager = nullptr;
    OriginalTimeScale = 1.0f;
    bTimeControlActive = false;
}

void USystemCapability::Initialize_Implementation(AItemNode* Owner)
{
    Super::Initialize_Implementation(Owner);
    
    // 缓存系统管理器
    CachedSystemManager = GetNodeSystemManager();
    
    // 初始化随机数生成器
    if (RandomSeed >= 0)
    {
        RandomStream.Initialize(RandomSeed);
    }
    else
    {
        RandomStream.GenerateNewSeed();
    }
    
    // 从配置加载数据
    if (SystemConfig.Num() > 0)
    {
        LoadSystemConfig(SystemConfig);
    }
    
    UE_LOG(LogTemp, Log, TEXT("SystemCapability initialized for %s"), 
        Owner ? *Owner->GetNodeName() : TEXT("Unknown"));
}

void USystemCapability::BeginPlay()
{
    Super::BeginPlay();
    
    // 启动自动条件评估
    if (bAutoEvaluateConditions && ConditionCheckInterval > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            ConditionCheckTimerHandle,
            this,
            &USystemCapability::OnConditionCheckTimer,
            ConditionCheckInterval,
            true
        );
    }
    
    // 启动威胁更新
    if (ThreatUpdateInterval > 0.0f && ThreatLevels.Num() > 0)
    {
        GetWorld()->GetTimerManager().SetTimer(
            ThreatUpdateTimerHandle,
            this,
            &USystemCapability::OnThreatUpdateTimer,
            ThreatUpdateInterval,
            true
        );
    }
    
    // 保存原始时间缩放
    if (UGameplayStatics::GetGameInstance(this))
    {
        OriginalTimeScale = UGameplayStatics::GetGlobalTimeDilation(this);
    }
}

void USystemCapability::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 恢复时间
    if (bTimeControlActive)
    {
        RestoreNormalTime();
    }
    
    // 清理定时器
    if (ConditionCheckTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ConditionCheckTimerHandle);
    }
    
    if (TimeControlTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(TimeControlTimerHandle);
    }
    
    if (ThreatUpdateTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ThreatUpdateTimerHandle);
    }
    
    // 清理生成的内容
    ClearGeneratedNodes();
    RemoveAllThreats();
    
    Super::EndPlay(EndPlayReason);
}

bool USystemCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanUse_Implementation(Data))
    {
        return false;
    }
    
    // 系统能力通常总是可用的
    return true;
}

bool USystemCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!Super::Use_Implementation(Data))
    {
        return false;
    }
    
    // 评估所有条件
    EvaluateAllConditions();
    
    // 可以触发一个随机事件
    if (EventProbabilities.Num() > 0)
    {
        TArray<FString> EventIDs;
        EventProbabilities.GetKeys(EventIDs);
        
        for (const FString& EventID : EventIDs)
        {
            if (RollProbability(EventID))
            {
                UE_LOG(LogTemp, Log, TEXT("SystemCapability: Triggered random event %s"), *EventID);
                
                if (OwnerItem)
                {
                    OwnerItem->AddTriggerEvent(EventID);
                }
                return true;
            }
        }
    }
    
    return true;
}

void USystemCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    Super::OnOwnerStateChanged_Implementation(NewState);
    
    // 某些状态可能触发规则变化
    if (NewState == ENodeState::Completed)
    {
        // 应用所有规则修改
        for (const auto& Rule : WorldRules)
        {
            ModifyWorldRule(Rule.Key, Rule.Value);
        }
    }
}

void USystemCapability::SetTimeScale(float Scale, float Duration)
{
    if (!bCanPauseTime && Scale == 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("SystemCapability: Cannot pause time"));
        return;
    }
    
    TimeScale = FMath::Clamp(Scale, 0.0f, 10.0f);
    TimeDuration = Duration;
    RemainingTimeDuration = Duration;
    
    // 应用时间缩放
    UGameplayStatics::SetGlobalTimeDilation(this, TimeScale);
    bTimeControlActive = true;
    
    // 如果有持续时间，设置定时器
    if (Duration > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimeControlTimerHandle,
            this,
            &USystemCapability::OnTimeControlEnd,
            Duration,
            false
        );
    }
    
    UE_LOG(LogTemp, Log, TEXT("SystemCapability: Set time scale to %f for %f seconds"), 
        TimeScale, Duration);
}

void USystemCapability::PauseTime(bool bPause)
{
    if (!bCanPauseTime)
    {
        return;
    }
    
    if (bPause)
    {
        SetTimeScale(0.0f, 0.0f);
    }
    else
    {
        RestoreNormalTime();
    }
}

float USystemCapability::GetCurrentTimeScale() const
{
    return UGameplayStatics::GetGlobalTimeDilation(this);
}

void USystemCapability::RestoreNormalTime()
{
    UGameplayStatics::SetGlobalTimeDilation(this, OriginalTimeScale);
    bTimeControlActive = false;
    RemainingTimeDuration = 0.0f;
    
    if (TimeControlTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(TimeControlTimerHandle);
    }
}

bool USystemCapability::EvaluateCondition(const FString& ConditionID)
{
    if (!ConditionRules.Contains(ConditionID))
    {
        return false;
    }
    
    FString Rule = ConditionRules[ConditionID];
    bool bResult = EvaluateConditionRule(Rule);
    
    ConditionStates.Add(ConditionID, bResult);
    
    return bResult;
}

void USystemCapability::AddCondition(const FString& ConditionID, const FString& Rule)
{
    ConditionRules.Add(ConditionID, Rule);
    ConditionStates.Add(ConditionID, false);
}

void USystemCapability::SetConditionState(const FString& ConditionID, bool bState)
{
    ConditionStates.Add(ConditionID, bState);
}

bool USystemCapability::GetConditionState(const FString& ConditionID) const
{
    if (ConditionStates.Contains(ConditionID))
    {
        return ConditionStates[ConditionID];
    }
    return false;
}

void USystemCapability::EvaluateAllConditions()
{
    for (const auto& Condition : ConditionRules)
    {
        EvaluateCondition(Condition.Key);
    }
}

ANodeConnection* USystemCapability::EstablishRelation(const FString& NodeA_ID, const FString& NodeB_ID, ENodeRelationType Type)
{
    if (CreatedConnections.Num() >= MaxRelationships)
    {
        UE_LOG(LogTemp, Warning, TEXT("SystemCapability: Maximum relationships reached"));
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    ANodeConnection* Connection = SystemManager->CreateConnectionBetween(NodeA_ID, NodeB_ID, Type);
    if (Connection)
    {
        CreatedConnections.Add(Connection);
        
        UE_LOG(LogTemp, Log, TEXT("SystemCapability: Established %s relation between %s and %s"), 
            *UEnum::GetValueAsString(Type), *NodeA_ID, *NodeB_ID);
    }
    
    return Connection;
}

bool USystemCapability::RemoveRelation(const FString& NodeA_ID, const FString& NodeB_ID)
{
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return false;
    }
    
    int32 RemovedCount = SystemManager->RemoveConnectionsBetween(NodeA_ID, NodeB_ID);
    
    // 清理本地引用
    CleanupInvalidConnections();
    
    return RemovedCount > 0;
}

void USystemCapability::AddRelationTemplate(const FString& TemplateID, ENodeRelationType Type)
{
    RelationTemplates.Add(TemplateID, Type);
}

int32 USystemCapability::GetActiveRelationCount() const
{
    int32 ValidCount = 0;
    for (const ANodeConnection* Connection : CreatedConnections)
    {
        if (Connection && Connection->IsValid())
        {
            ValidCount++;
        }
    }
    return ValidCount;
}

void USystemCapability::ModifyWorldRule(const FString& RuleID, const FString& NewValue)
{
    // 备份原始值
    if (!OriginalRules.Contains(RuleID) && WorldRules.Contains(RuleID))
    {
        OriginalRules.Add(RuleID, WorldRules[RuleID]);
    }
    
    WorldRules.Add(RuleID, NewValue);
    
    // 应用规则（这里需要根据具体规则类型实现）
    if (RuleID == TEXT("Gravity"))
    {
        float GravityScale = FCString::Atof(*NewValue);
        // 可以通过物理设置修改重力
    }
    else if (RuleID == TEXT("TimeFlow"))
    {
        float TimeFlow = FCString::Atof(*NewValue);
        SetTimeScale(TimeFlow, 0.0f);
    }
    // 添加更多规则类型
    
    UE_LOG(LogTemp, Log, TEXT("SystemCapability: Modified rule %s to %s"), *RuleID, *NewValue);
}

void USystemCapability::RestoreOriginalRule(const FString& RuleID)
{
    if (OriginalRules.Contains(RuleID))
    {
        ModifyWorldRule(RuleID, OriginalRules[RuleID]);
        
        if (!bCanModifyPermanently)
        {
            OriginalRules.Remove(RuleID);
        }
    }
}

void USystemCapability::RestoreAllRules()
{
    TArray<FString> RuleIDs;
    OriginalRules.GetKeys(RuleIDs);
    
    for (const FString& RuleID : RuleIDs)
    {
        RestoreOriginalRule(RuleID);
    }
}

FString USystemCapability::GetRuleValue(const FString& RuleID) const
{
    if (WorldRules.Contains(RuleID))
    {
        return WorldRules[RuleID];
    }
    return TEXT("");
}

void USystemCapability::SetThreatLevel(const FString& ThreatID, float Level)
{
    ThreatLevels.Add(ThreatID, FMath::Clamp(Level, 0.0f, 1.0f));
    
    // 更新现有威胁的行为
    UpdateThreatBehavior(ThreatID);
}

void USystemCapability::RegisterThreat(const FString& ThreatID)
{
    if (!ActiveThreatIDs.Contains(ThreatID))
    {
        ActiveThreatIDs.Add(ThreatID);
        
        // 设置默认威胁等级
        if (!ThreatLevels.Contains(ThreatID))
        {
            ThreatLevels.Add(ThreatID, 0.5f);
        }
        
        UE_LOG(LogTemp, Log, TEXT("SystemCapability: Registered threat %s"), *ThreatID);
    }
}

void USystemCapability::UpdateThreatBehavior(const FString& ThreatID)
{
    if (!ThreatLevels.Contains(ThreatID))
    {
        return;
    }
    
    float ThreatLevel = ThreatLevels[ThreatID];
    
    // 处理威胁更新逻辑
    ProcessThreatUpdate(ThreatID, ThreatLevel);
    
    // 可以触发相关事件
    if (OwnerItem)
    {
        OwnerItem->AddTriggerEvent(FString::Printf(TEXT("ThreatUpdate_%s_%f"), *ThreatID, ThreatLevel));
    }
}

void USystemCapability::RemoveAllThreats()
{
    ActiveThreatIDs.Empty();
    ThreatLevels.Empty();
    
    UE_LOG(LogTemp, Log, TEXT("SystemCapability: Removed all threats"));
}

float USystemCapability::GetThreatLevel(const FString& ThreatID) const
{
    return ThreatLevels.Contains(ThreatID) ? ThreatLevels[ThreatID] : 0.0f;
}

float USystemCapability::GetEventProbability(const FString& EventID) const
{
    if (EventProbabilities.Contains(EventID))
    {
        return EventProbabilities[EventID] * GlobalProbabilityModifier;
    }
    return 0.0f;
}

void USystemCapability::SetEventProbability(const FString& EventID, float Probability)
{
    EventProbabilities.Add(EventID, FMath::Clamp(Probability, 0.0f, 1.0f));
}

bool USystemCapability::RollProbability(const FString& EventID) const
{
    float Probability = GetEventProbability(EventID);
    float Roll = RandomStream.FRand();
    
    return Roll < Probability;
}

void USystemCapability::SetRandomSeed(int32 Seed)
{
    RandomSeed = Seed;
    RandomStream.Initialize(Seed);
}

AInteractiveNode* USystemCapability::GenerateNode(const FString& TemplateID, const FVector& Location)
{
    if (GeneratedNodes.Num() >= MaxGeneratedNodes)
    {
        UE_LOG(LogTemp, Warning, TEXT("SystemCapability: Maximum generated nodes reached"));
        return nullptr;
    }
    
    if (!NodeTemplates.Contains(TemplateID))
    {
        UE_LOG(LogTemp, Warning, TEXT("SystemCapability: Template %s not found"), *TemplateID);
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    // 创建节点数据
    FNodeGenerateData GenerateData;
    GenerateData.NodeData.NodeID = FString::Printf(TEXT("Generated_%s_%d"), *TemplateID, FDateTime::Now().GetTicks());
    GenerateData.NodeData.NodeName = TemplateID;
    GenerateData.NodeData.NodeType = ENodeType::Custom;
    GenerateData.NodeData.InitialState = ENodeState::Active;
    GenerateData.NodeClass = NodeTemplates[TemplateID];
    GenerateData.SpawnTransform.SetLocation(Location);
    
    AInteractiveNode* NewNode = SystemManager->CreateNode(GenerateData.NodeClass, GenerateData);
    if (NewNode)
    {
        GeneratedNodes.Add(NewNode);
        
        UE_LOG(LogTemp, Log, TEXT("SystemCapability: Generated node %s at %s"), 
            *TemplateID, *Location.ToString());
    }
    
    return NewNode;
}

TArray<AInteractiveNode*> USystemCapability::GenerateNodeCluster(const FString& TemplateID, int32 Count)
{
    TArray<AInteractiveNode*> Cluster;
    
    for (int32 i = 0; i < Count; i++)
    {
        FVector Location = GenerateRandomLocation();
        AInteractiveNode* Node = GenerateNode(TemplateID, Location);
        if (Node)
        {
            Cluster.Add(Node);
        }
    }
    
    return Cluster;
}

void USystemCapability::RegisterNodeTemplate(const FString& TemplateID, TSubclassOf<AInteractiveNode> NodeClass)
{
    if (NodeClass)
    {
        NodeTemplates.Add(TemplateID, NodeClass);
    }
}

void USystemCapability::ClearGeneratedNodes()
{
    for (AInteractiveNode* Node : GeneratedNodes)
    {
        if (Node)
        {
            Node->Destroy();
        }
    }
    
    GeneratedNodes.Empty();
}

void USystemCapability::LoadSystemConfig(const TMap<FString, FString>& Config)
{
    for (const auto& Pair : Config)
    {
        ApplyConfigValue(Pair.Key, Pair.Value);
    }
}

void USystemCapability::ApplyConfigValue(const FString& Key, const FString& Value)
{
    if (Key == TEXT("TimeScale"))
    {
        TimeScale = FCString::Atof(*Value);
    }
    else if (Key == TEXT("MaxRelationships"))
    {
        MaxRelationships = FCString::Atoi(*Value);
    }
    else if (Key == TEXT("SpawnRadius"))
    {
        SpawnRadius = FCString::Atof(*Value);
    }
    else if (Key == TEXT("MaxGeneratedNodes"))
    {
        MaxGeneratedNodes = FCString::Atoi(*Value);
    }
    else if (Key == TEXT("GlobalProbabilityModifier"))
    {
        GlobalProbabilityModifier = FCString::Atof(*Value);
    }
    else if (Key.StartsWith(TEXT("Condition_")))
    {
        FString ConditionID = Key.RightChop(10); // 移除 "Condition_"
        AddCondition(ConditionID, Value);
    }
    else if (Key.StartsWith(TEXT("Rule_")))
    {
        FString RuleID = Key.RightChop(5); // 移除 "Rule_"
        WorldRules.Add(RuleID, Value);
    }
    else if (Key.StartsWith(TEXT("Probability_")))
    {
        FString EventID = Key.RightChop(12); // 移除 "Probability_"
        SetEventProbability(EventID, FCString::Atof(*Value));
    }
    
    // 保存到配置
    SystemConfig.Add(Key, Value);
}

ANodeSystemManager* USystemCapability::GetNodeSystemManager() const
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

void USystemCapability::UpdateTimeControl(float DeltaTime)
{
    if (bTimeControlActive && TimeDuration > 0.0f)
    {
        RemainingTimeDuration = FMath::Max(0.0f, RemainingTimeDuration - DeltaTime);
    }
}

bool USystemCapability::EvaluateConditionRule(const FString& Rule) const
{
    // 简单的条件评估实现
    // 可以扩展为更复杂的表达式解析
    
    // 检查节点状态条件
    if (Rule.StartsWith(TEXT("NodeState:")))
    {
        FString Condition = Rule.RightChop(10);
        TArray<FString> Parts;
        Condition.ParseIntoArray(Parts, TEXT("=="));
        
        if (Parts.Num() == 2)
        {
            FString NodeID = Parts[0].TrimStartAndEnd();
            FString StateStr = Parts[1].TrimStartAndEnd();
            
            ANodeSystemManager* SystemManager = GetNodeSystemManager();
            if (SystemManager)
            {
                AInteractiveNode* Node = SystemManager->GetNode(NodeID);
                if (Node)
                {
                    ENodeState RequiredState = static_cast<ENodeState>(FCString::Atoi(*StateStr));
                    return Node->GetNodeState() == RequiredState;
                }
            }
        }
    }
    // 检查概率条件
    else if (Rule.StartsWith(TEXT("Probability:")))
    {
        FString EventID = Rule.RightChop(12);
        return RollProbability(EventID);
    }
    // 检查数值比较
    else if (Rule.Contains(TEXT(">")))
    {
        TArray<FString> Parts;
        Rule.ParseIntoArray(Parts, TEXT(">"));
        
        if (Parts.Num() == 2)
        {
            float Left = FCString::Atof(*Parts[0].TrimStartAndEnd());
            float Right = FCString::Atof(*Parts[1].TrimStartAndEnd());
            return Left > Right;
        }
    }
    
    return false;
}

FVector USystemCapability::GenerateRandomLocation() const
{
    if (!OwnerItem)
    {
        return FVector::ZeroVector;
    }
    
    FVector BaseLocation = OwnerItem->GetActorLocation();
    float Angle = RandomStream.FRandRange(0.0f, 360.0f);
    float Distance = RandomStream.FRandRange(100.0f, SpawnRadius);
    
    float X = BaseLocation.X + Distance * FMath::Cos(FMath::DegreesToRadians(Angle));
    float Y = BaseLocation.Y + Distance * FMath::Sin(FMath::DegreesToRadians(Angle));
    float Z = BaseLocation.Z;
    
    return FVector(X, Y, Z);
}

void USystemCapability::CleanupInvalidConnections()
{
    CreatedConnections.RemoveAll([](const ANodeConnection* Connection)
    {
        return !Connection || !Connection->IsValid();
    });
}

void USystemCapability::ProcessThreatUpdate(const FString& ThreatID, float ThreatLevel)
{
    // 这里可以实现威胁更新的具体逻辑
    // 例如：根据威胁等级调整游戏难度、生成事件等
    
    if (ThreatLevel > 0.8f)
    {
        // 高威胁等级
        SetEventProbability(FString::Printf(TEXT("HighThreat_%s"), *ThreatID), 0.9f);
    }
    else if (ThreatLevel > 0.5f)
    {
        // 中等威胁等级
        SetEventProbability(FString::Printf(TEXT("MediumThreat_%s"), *ThreatID), 0.5f);
    }
    else
    {
        // 低威胁等级
        SetEventProbability(FString::Printf(TEXT("LowThreat_%s"), *ThreatID), 0.2f);
    }
}

void USystemCapability::OnConditionCheckTimer()
{
    EvaluateAllConditions();
}

void USystemCapability::OnTimeControlEnd()
{
    RestoreNormalTime();
    
    UE_LOG(LogTemp, Log, TEXT("SystemCapability: Time control ended, restored normal time"));
}

void USystemCapability::OnThreatUpdateTimer()
{
    // 更新所有威胁的行为
    for (const auto& Threat : ThreatLevels)
    {
        UpdateThreatBehavior(Threat.Key);
    }
    
    // 清理无效的威胁ID
    ActiveThreatIDs.RemoveAll([this](const FString& ThreatID)
    {
        return !ThreatLevels.Contains(ThreatID);
    });
}