// Fill out your copyright notice in the Description page of Project Settings.

// StateCapability.cpp

#include "Nodes/Capabilities/StateCapability.h"
#include "Nodes/ItemNode.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/NodeSystemManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

UStateCapability::UStateCapability()
{
    // 设置默认值
    CapabilityID = TEXT("StateCapability");
    CapabilityDescription = FText::FromString(TEXT("管理节点状态变化和传播"));
    UsagePrompt = TEXT("改变状态");
    
    // 状态管理默认值
    PossibleStates = { ENodeState::Active, ENodeState::Inactive, ENodeState::Completed };
    CurrentInternalState = ENodeState::Inactive;
    StateTransitionDuration = 0.5f;
    
    // 状态影响默认值
    StateChangeRadius = 500.0f;
    AffectedNodeClass = AInteractiveNode::StaticClass();
    bPropagateThoughDependency = true;
    
    // 状态条件默认值
    bAutoCheckState = false;
    StateCheckInterval = 1.0f;
    
    // 外观变形默认值
    bChangeAppearanceOnStateChange = false;
    
    // 内部状态
    CachedSystemManager = nullptr;
    OriginalMesh = nullptr;
    OriginalMaterial = nullptr;
    bIsTransitioning = false;
    TransitionTargetState = ENodeState::Inactive;
}

void UStateCapability::Initialize_Implementation(AItemNode* Owner)
{
    Super::Initialize_Implementation(Owner);
    
    // 缓存系统管理器
    CachedSystemManager = GetNodeSystemManager();
    
    // 保存原始外观
    if (Owner && Owner->NodeMesh)
    {
        OriginalMesh = Owner->NodeMesh->GetStaticMesh();
        if (Owner->NodeMesh->GetNumMaterials() > 0)
        {
            OriginalMaterial = Owner->NodeMesh->GetMaterial(0);
        }
    }
    
    // 设置初始内部状态
    if (Owner)
    {
        CurrentInternalState = Owner->GetNodeState();
    }
    
    // 从配置加载数据
    if (StateConfig.Num() > 0)
    {
        LoadStateConfig(StateConfig);
    }
    
    UE_LOG(LogTemp, Log, TEXT("StateCapability initialized for %s"), 
        Owner ? *Owner->GetNodeName() : TEXT("Unknown"));
}

void UStateCapability::BeginPlay()
{
    Super::BeginPlay();
    
    // 启动自动状态检查
    if (bAutoCheckState && StateCheckInterval > 0.0f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            StateCheckTimerHandle,
            this,
            &UStateCapability::OnStateCheckTimer,
            StateCheckInterval,
            true
        );
    }
}

void UStateCapability::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理定时器
    if (StateCheckTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(StateCheckTimerHandle);
    }
    
    if (StateTransitionTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(StateTransitionTimerHandle);
    }
    
    Super::EndPlay(EndPlayReason);
}

bool UStateCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanUse_Implementation(Data))
    {
        return false;
    }
    
    // 正在转换状态时不能使用
    if (bIsTransitioning)
    {
        return false;
    }
    
    // 检查是否有可用的状态转换
    return PossibleStates.Num() > 0;
}

bool UStateCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!Super::Use_Implementation(Data))
    {
        return false;
    }
    
    // 循环切换到下一个可能的状态
    if (PossibleStates.Num() > 0)
    {
        int32 CurrentIndex = PossibleStates.IndexOfByKey(CurrentInternalState);
        int32 NextIndex = (CurrentIndex + 1) % PossibleStates.Num();
        
        ChangeOwnState(PossibleStates[NextIndex]);
        return true;
    }
    
    return false;
}

void UStateCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    Super::OnOwnerStateChanged_Implementation(NewState);
    
    // 同步内部状态
    CurrentInternalState = NewState;
    
    // 可能需要更新外观
    if (bChangeAppearanceOnStateChange)
    {
        UpdateNodeAppearance(NewState);
    }
}

void UStateCapability::ChangeOwnState(ENodeState NewState)
{
    if (!OwnerItem || CurrentInternalState == NewState)
    {
        return;
    }
    
    // 验证状态转换
    if (!ValidateStateTransition(CurrentInternalState, NewState))
    {
        UE_LOG(LogTemp, Warning, TEXT("StateCapability: Invalid state transition from %d to %d"), 
            (int32)CurrentInternalState, (int32)NewState);
        return;
    }
    
    // 处理状态转换
    ProcessStateTransition(CurrentInternalState, NewState);
    
    // 更新内部状态
    CurrentInternalState = NewState;
    
    // 更新拥有者状态
    OwnerItem->SetNodeState(NewState);
    
    // 传播状态变化
    if (bPropagateThoughDependency)
    {
        PropagateStateChange(NewState);
    }
    
    UE_LOG(LogTemp, Log, TEXT("StateCapability: Changed own state to %d"), (int32)NewState);
}

void UStateCapability::ChangeTargetNodeState(const FString& TargetNodeID, ENodeState NewState)
{
    if (TargetNodeID.IsEmpty())
    {
        return;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return;
    }
    
    AInteractiveNode* TargetNode = SystemManager->GetNode(TargetNodeID);
    if (TargetNode)
    {
        // 添加到目标节点状态映射
        TargetNodeStates.Add(TargetNodeID, NewState);
        
        // 改变目标节点状态
        TargetNode->SetNodeState(NewState);
        
        UE_LOG(LogTemp, Log, TEXT("StateCapability: Changed target node %s state to %d"), 
            *TargetNodeID, (int32)NewState);
    }
}

void UStateCapability::ChangeNodesInRadius(ENodeState NewState)
{
    if (StateChangeRadius <= 0.0f)
    {
        return;
    }
    
    TArray<AInteractiveNode*> NodesInRadius = GetNodesInRadius();
    
    for (AInteractiveNode* Node : NodesInRadius)
    {
        if (Node && Node != OwnerItem)
        {
            Node->SetNodeState(NewState);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StateCapability: Changed %d nodes in radius to state %d"), 
        NodesInRadius.Num(), (int32)NewState);
}

void UStateCapability::ApplyStateToConnectedNodes(ENodeState NewState)
{
    if (!OwnerItem)
    {
        return;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return;
    }
    
    // 获取所有连接的节点
    TArray<ANodeConnection*> Connections = SystemManager->GetConnectionsForNode(OwnerItem->GetNodeID());
    
    for (ANodeConnection* Connection : Connections)
    {
        if (Connection && Connection->IsValid())
        {
            AInteractiveNode* ConnectedNode = Connection->GetOppositeNode(OwnerItem);
            if (ConnectedNode)
            {
                ConnectedNode->SetNodeState(NewState);
            }
        }
    }
}

bool UStateCapability::IsStateAvailable(ENodeState State) const
{
    return PossibleStates.Contains(State);
}

bool UStateCapability::CheckStateConditions() const
{
    if (StateConditions.Num() == 0)
    {
        return true;
    }
    
    // 简单的条件检查实现
    // 可以扩展为更复杂的条件系统
    for (const auto& Condition : StateConditions)
    {
        if (Condition.Key == TEXT("RequireState") && OwnerItem)
        {
            ENodeState RequiredState = static_cast<ENodeState>(FCString::Atoi(*Condition.Value));
            if (OwnerItem->GetNodeState() != RequiredState)
            {
                return false;
            }
        }
        // 可以添加更多条件类型
    }
    
    return true;
}

void UStateCapability::AddStateCondition(const FString& Key, const FString& Value)
{
    StateConditions.Add(Key, Value);
}

void UStateCapability::RemoveStateCondition(const FString& Key)
{
    StateConditions.Remove(Key);
}

void UStateCapability::TransformAppearance(int32 FormIndex)
{
    if (!OwnerItem || !OwnerItem->NodeMesh)
    {
        return;
    }
    
    // 改变网格
    if (StateMeshClasses.IsValidIndex(FormIndex) && StateMeshClasses[FormIndex])
    {
        UStaticMesh* NewMesh = StateMeshClasses[FormIndex]->GetDefaultObject<UStaticMesh>();
        if (NewMesh)
        {
            OwnerItem->NodeMesh->SetStaticMesh(NewMesh);
        }
    }
    
    // 改变材质
    if (StateMaterialClasses.IsValidIndex(FormIndex) && StateMaterialClasses[FormIndex])
    {
        UMaterialInterface* NewMaterial = StateMaterialClasses[FormIndex]->GetDefaultObject<UMaterialInterface>();
        if (NewMaterial)
        {
            OwnerItem->NodeMesh->SetMaterial(0, NewMaterial);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StateCapability: Transformed appearance to form %d"), FormIndex);
}

void UStateCapability::RestoreOriginalAppearance()
{
    if (!OwnerItem || !OwnerItem->NodeMesh)
    {
        return;
    }
    
    if (OriginalMesh)
    {
        OwnerItem->NodeMesh->SetStaticMesh(OriginalMesh);
    }
    
    if (OriginalMaterial)
    {
        OwnerItem->NodeMesh->SetMaterial(0, OriginalMaterial);
    }
}

void UStateCapability::PropagateStateChange_Implementation(ENodeState NewState)
{
    if (!bPropagateThoughDependency)
    {
        return;
    }
    
    // 获取所有Dependency连接
    TArray<ANodeConnection*> DependencyConnections = GetDependencyConnections();
    
    for (ANodeConnection* Connection : DependencyConnections)
    {
        if (Connection && Connection->CanPropagateState(NewState))
        {
            Connection->PropagateState(OwnerItem, NewState);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("StateCapability: Propagated state %d through %d dependency connections"), 
        (int32)NewState, DependencyConnections.Num());
}

void UStateCapability::LoadStateConfig(const TMap<FString, FString>& Config)
{
    for (const auto& Pair : Config)
    {
        ApplyConfigValue(Pair.Key, Pair.Value);
    }
}

void UStateCapability::ApplyConfigValue(const FString& Key, const FString& Value)
{
    if (Key == TEXT("StateChangeRadius"))
    {
        StateChangeRadius = FCString::Atof(*Value);
    }
    else if (Key == TEXT("StateTransitionDuration"))
    {
        StateTransitionDuration = FCString::Atof(*Value);
    }
    else if (Key == TEXT("AutoCheckState"))
    {
        bAutoCheckState = Value.ToBool();
    }
    else if (Key == TEXT("StateCheckInterval"))
    {
        StateCheckInterval = FCString::Atof(*Value);
    }
    else if (Key == TEXT("PropagateThoughDependency"))
    {
        bPropagateThoughDependency = Value.ToBool();
    }
    else if (Key.StartsWith(TEXT("TargetNode_")))
    {
        // 格式: TargetNode_NodeID = State
        FString NodeID = Key.RightChop(11); // 移除 "TargetNode_"
        ENodeState State = static_cast<ENodeState>(FCString::Atoi(*Value));
        TargetNodeStates.Add(NodeID, State);
    }
    
    // 保存到配置
    StateConfig.Add(Key, Value);
}

ANodeSystemManager* UStateCapability::GetNodeSystemManager() const
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

void UStateCapability::ProcessStateTransition(ENodeState FromState, ENodeState ToState)
{
    if (StateTransitionDuration <= 0.0f)
    {
        // 立即转换
        if (bChangeAppearanceOnStateChange)
        {
            UpdateNodeAppearance(ToState);
        }
        return;
    }
    
    // 开始转换
    bIsTransitioning = true;
    TransitionTargetState = ToState;
    
    // 设置转换完成定时器
    GetWorld()->GetTimerManager().SetTimer(
        StateTransitionTimerHandle,
        [this]()
        {
            bIsTransitioning = false;
            if (bChangeAppearanceOnStateChange)
            {
                UpdateNodeAppearance(TransitionTargetState);
            }
        },
        StateTransitionDuration,
        false
    );
}

bool UStateCapability::ValidateStateTransition(ENodeState FromState, ENodeState ToState) const
{
    // 检查目标状态是否在可能状态列表中
    if (!IsStateAvailable(ToState))
    {
        return false;
    }
    
    // 可以添加更多的转换规则
    // 例如：某些状态不能直接转换到其他状态
    
    return true;
}

void UStateCapability::UpdateNodeAppearance(ENodeState State)
{
    // 根据状态选择外观索引
    int32 AppearanceIndex = -1;
    
    switch (State)
    {
    case ENodeState::Inactive:
        AppearanceIndex = 0;
        break;
    case ENodeState::Active:
        AppearanceIndex = 1;
        break;
    case ENodeState::Completed:
        AppearanceIndex = 2;
        break;
    case ENodeState::Locked:
        AppearanceIndex = 3;
        break;
    case ENodeState::Hidden:
        AppearanceIndex = 4;
        break;
    }
    
    if (AppearanceIndex >= 0)
    {
        TransformAppearance(AppearanceIndex);
    }
}

TArray<AInteractiveNode*> UStateCapability::GetNodesInRadius() const
{
    TArray<AInteractiveNode*> Result;
    
    if (!OwnerItem || StateChangeRadius <= 0.0f)
    {
        return Result;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return Result;
    }
    
    // 获取范围内的节点
    TArray<AInteractiveNode*> NodesInRadius = SystemManager->GetNodesInRadius(
        OwnerItem->GetActorLocation(), 
        StateChangeRadius
    );
    
    // 过滤节点类型
    for (AInteractiveNode* Node : NodesInRadius)
    {
        if (Node && Node != OwnerItem)
        {
            if (!AffectedNodeClass || Node->IsA(AffectedNodeClass))
            {
                Result.Add(Node);
            }
        }
    }
    
    return Result;
}

TArray<ANodeConnection*> UStateCapability::GetDependencyConnections() const
{
    TArray<ANodeConnection*> Result;
    
    if (!OwnerItem)
    {
        return Result;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return Result;
    }
    
    // 获取所有连接
    TArray<ANodeConnection*> AllConnections = SystemManager->GetConnectionsForNode(OwnerItem->GetNodeID());
    
    // 过滤Dependency类型的连接
    for (ANodeConnection* Connection : AllConnections)
    {
        if (Connection && Connection->RelationType == ENodeRelationType::Dependency)
        {
            Result.Add(Connection);
        }
    }
    
    return Result;
}

void UStateCapability::OnStateCheckTimer()
{
    if (!bAutoCheckState)
    {
        return;
    }
    
    // 检查状态条件
    if (CheckStateConditions())
    {
        // 如果条件满足，可以执行某些操作
        // 例如：自动改变状态
        for (const auto& TargetState : TargetNodeStates)
        {
            ChangeTargetNodeState(TargetState.Key, TargetState.Value);
        }
    }
}