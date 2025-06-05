// Fill out your copyright notice in the Description page of Project Settings.

#include "Nodes/NodeSystemManager.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/SceneNode.h"
#include "Nodes/ItemNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Nodes/Capabilities/InteractiveCapability.h"
#include "Nodes/Capabilities/NarrativeCapability.h"
#include "Nodes/Capabilities/NumericalCapability.h"
#include "Nodes/Capabilities/SpatialCapability.h"
#include "Nodes/Capabilities/StateCapability.h"
#include "Nodes/Capabilities/SystemCapability.h"

ANodeSystemManager::ANodeSystemManager()
{
    PrimaryActorTick.bCanEverTick = true;

    // 默认配置
    NodeSpawnRadius = 500.0f;
    MaxNodesPerScene = 50;
    bAutoRegisterSpawnedNodes = true;
    bDebugDrawConnections = false;
    GenerationInterval = 0.1f;

    // 初始化状态
    ActiveSceneNode = nullptr;
    bIsTransitioning = false;
    TransitionProgress = 0.0f;
    TransitionTargetScene = nullptr;
}

void ANodeSystemManager::BeginPlay()
{
    Super::BeginPlay();

    // 设置生成队列定时器
    GetWorld()->GetTimerManager().SetTimer(
        GenerationTimerHandle,
        this,
        &ANodeSystemManager::ProcessGenerationQueue,
        GenerationInterval,
        true
    );

    // 设置验证定时器
    // GetWorld()->GetTimerManager().SetTimer(ValidationTimerHandle,this,&ANodeSystemManager::ValidateSystem,5.0f,true);


    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager initialized"));
}

void ANodeSystemManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理定时器
    GetWorld()->GetTimerManager().ClearTimer(GenerationTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(ValidationTimerHandle);

    // 清理所有节点和连接
    ResetSystem();

    Super::EndPlay(EndPlayReason);
}

void ANodeSystemManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 处理场景过渡
    if (bIsTransitioning && TransitionTargetScene)
    {
        TransitionProgress += DeltaTime;
        // 实际的过渡逻辑可以在这里实现
    }

    // 调试绘制
    if (bDebugDrawConnections)
    {
        for (const auto& Connection : ActiveConnections)
        {
            if (Connection && Connection->IsValid())
            {
                FVector Start, End;
                Start = Connection->GetSourceNode()->GetActorLocation();
                End = Connection->GetTargetNode()->GetActorLocation();
                
                FColor Color = Connection->GetConnectionColor().ToFColor(true);
                DrawDebugLine(GetWorld(), Start, End, Color, false, -1.0f, 0, 5.0f);
            }
        }
    }
}

// 节点创建实现
AInteractiveNode* ANodeSystemManager::CreateNode(TSubclassOf<AInteractiveNode> NodeClass, const FNodeGenerateData& GenerateData)
{
    if (!NodeClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Cannot create node without class"));
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 计算生成位置
    FVector SpawnLocation = GenerateData.SpawnTransform.GetLocation();
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = CalculateNodeSpawnLocation(GetActorLocation());
    }

    FTransform SpawnTransform = GenerateData.SpawnTransform;
    SpawnTransform.SetLocation(SpawnLocation);

    // 生成节点
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AInteractiveNode* NewNode = World->SpawnActor<AInteractiveNode>(
        NodeClass,
        SpawnTransform,
        SpawnParams
    );

    if (NewNode)
    {
        // 初始化节点
        NewNode->Initialize(GenerateData.NodeData);

        // 设置情绪上下文
        if (GenerateData.EmotionContext.Intensity > 0.0f)
        {
            NewNode->StoryContext.Add("EmotionType", UEnum::GetValueAsString(GenerateData.EmotionContext.PrimaryEmotion));
            NewNode->StoryContext.Add("EmotionIntensity", FString::SanitizeFloat(GenerateData.EmotionContext.Intensity));
        }

        // 自动注册
        if (bAutoRegisterSpawnedNodes)
        {
            RegisterNode(NewNode);
        }

        // 处理能力
        if (AItemNode* ItemNode = Cast<AItemNode>(NewNode))
        {
            for (const FCapabilityData& CapData : GenerateData.Capabilities)
            {
                TSubclassOf<UItemCapability> CapClass;
                if (CapData.CapabilityType != ECapabilityType::None)
                {
                    switch (CapData.CapabilityType)
                    {
                        case ECapabilityType::Spatial:
                            CapClass = USpatialCapability::StaticClass();
                            break;
                        case ECapabilityType::State:
                            CapClass = UStateCapability::StaticClass();
                            break;
                        case ECapabilityType::Interactive:
                            CapClass = UInteractiveCapability::StaticClass();
                            break;
                        case ECapabilityType::Narrative:
                            CapClass = UNarrativeCapability::StaticClass();
                            break;
                        case ECapabilityType::Numerical:
                            CapClass = UNumericalCapability::StaticClass();
                            break;
                        case ECapabilityType::System:
                            CapClass = USystemCapability::StaticClass();
                            break;
                        case ECapabilityType::None:
                            break;
                    }
                }
                if (CapClass)
                {
                    UItemCapability* Capability = ItemNode->AddCapability(CapClass);
                    if (Capability)
                    {
                        // 设置能力ID（如果提供）
                        if (!CapData.CapabilityID.IsEmpty())
                        {
                            Capability->CapabilityID = CapData.CapabilityID;
                        }
                        
                        // 根据能力类型应用特定配置
                        switch (CapData.CapabilityType)
                        {
                            case ECapabilityType::Spatial:
                            {
                                USpatialCapability* SpatialCap = Cast<USpatialCapability>(Capability);
                                if (SpatialCap)
                                {
                                    // 应用空间能力配置
                                    SpatialCap->bCanContainNodes = CapData.SpatialConfig.bCanContainNodes;
                                    SpatialCap->MaxContainedNodes = CapData.SpatialConfig.MaxContainedNodes;
                                }
                                break;
                            }
                            case ECapabilityType::State:
                            {
                                UStateCapability* StateCap = Cast<UStateCapability>(Capability);
                                if (StateCap)
                                {
                                    // 应用状态能力配置
                                    StateCap->PossibleStates = CapData.StateConfig.PossibleStates;
                                    StateCap->StateChangeRadius = CapData.StateConfig.StateChangeRadius;
                                }
                                break;
                            }
                            case ECapabilityType::Interactive:
                            {
                                UInteractiveCapability* InteractiveCap = Cast<UInteractiveCapability>(Capability);
                                if (InteractiveCap)
                                {
                                    // 应用交互能力配置
                                    InteractiveCap->AllowedInteractions = CapData.InteractiveConfig.AllowedInteractions;
                                    InteractiveCap->ObservableInfo = CapData.InteractiveConfig.ObservableInfo;
                                    InteractiveCap->DialogueOptions = CapData.InteractiveConfig.DialogueOptions;
                                    InteractiveCap->MaxAttempts = CapData.InteractiveConfig.MaxAttempts;
                                }
                                break;
                            }
                            case ECapabilityType::Narrative:
                            {
                                UNarrativeCapability* NarrativeCap = Cast<UNarrativeCapability>(Capability);
                                if (NarrativeCap)
                                {
                                    // 应用叙事能力配置
                                    NarrativeCap->StoryProgressionPath = CapData.NarrativeConfig.StoryProgressionPath;
                                }
                                break;
                            }
                            case ECapabilityType::System:
                            {
                                USystemCapability* SystemCap = Cast<USystemCapability>(Capability);
                                if (SystemCap)
                                {
                                    // 应用系统能力配置
                                    SystemCap->TimeScale = CapData.SystemConfig.TimeScale;
                                    SystemCap->ConditionRules = CapData.SystemConfig.ConditionRules;
                                }
                                break;
                            }
                            case ECapabilityType::Numerical:
                            {
                                UNumericalCapability* NumericalCap = Cast<UNumericalCapability>(Capability);
                                if (NumericalCap)
                                {
                                    // 应用数值能力配置
                                }
                                break;
                            }
                            case ECapabilityType::None:
                                // 无特定类型，不应用特定配置
                                break;
                        }
                        
                        // 应用通用参数
                        // for (const auto& Param : CapData.CapabilityParameters)
                        // {
                        //     // 这里可以处理通用参数
                        //     UE_LOG(LogTemp, Log, TEXT("Setting capability parameter %s = %s for %s"), 
                        //         *Param.Key, *Param.Value, *Capability->GetCapabilityID());
                        // }
                        
                        // 如果需要自动激活，则激活能力
                        if (CapData.bAutoActivate)
                        {
                            Capability->Activate();
                        }
                    }
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Created node %s"), *NewNode->GetNodeID());
    }

    return NewNode;
}

ASceneNode* ANodeSystemManager::CreateSceneNode(const FNodeGenerateData& GenerateData)
{
    TSubclassOf<AInteractiveNode> SceneClass = DefaultSceneNodeClass;
    if (GenerateData.NodeClass && GenerateData.NodeClass->IsChildOf(ASceneNode::StaticClass()))
    {
        SceneClass = GenerateData.NodeClass;
    }

    return Cast<ASceneNode>(CreateNode(SceneClass, GenerateData));
}

AItemNode* ANodeSystemManager::CreateItemNode(const FNodeGenerateData& GenerateData)
{
    TSubclassOf<AInteractiveNode> ItemClass = DefaultItemNodeClass;
    if (GenerateData.NodeClass && GenerateData.NodeClass->IsChildOf(AItemNode::StaticClass()))
    {
        ItemClass = GenerateData.NodeClass;
    }

    return Cast<AItemNode>(CreateNode(ItemClass, GenerateData));
}

AInteractiveNode* ANodeSystemManager::SpawnNodeAtLocation(TSubclassOf<AInteractiveNode> NodeClass, const FVector& Location, const FNodeData& NodeData)
{
    FNodeGenerateData GenerateData;
    GenerateData.NodeClass = NodeClass;
    GenerateData.NodeData = NodeData;
    GenerateData.SpawnTransform.SetLocation(Location);

    return CreateNode(NodeClass, GenerateData);
}

// 节点注册实现
bool ANodeSystemManager::RegisterNode(AInteractiveNode* Node)
{
    if (!Node)
    {
        return false;
    }

    FString NodeID = Node->GetNodeID();
    if (NodeID.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Cannot register node without ID"));
        return false;
    }

    // 检查是否已注册
    if (NodeRegistry.Contains(NodeID))
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Node %s already registered"), *NodeID);
        return false;
    }

    // 添加到注册表
    NodeRegistry.Add(NodeID, Node);

    // 更新索引
    UpdateNodeTypeMap(Node, true);
    UpdateNodeTagMap(Node, true);

    // 添加到活动列表
    if (Node->GetNodeState() == ENodeState::Active)
    {
        ActiveNodes.AddUnique(Node);
    }

    // 注册事件
    RegisterNodeEvents(Node);

    // 广播事件
    OnNodeRegistered.Broadcast(Node);

    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Registered node %s"), *NodeID);
    return true;
}

bool ANodeSystemManager::UnregisterNodeByID(const FString& NodeID)
{
    AInteractiveNode* Node = GetNode(NodeID);
    return UnregisterNode(Node);
}

bool ANodeSystemManager::UnregisterNode(AInteractiveNode* Node)
{
    if (!Node)
    {
        return false;
    }

    FString NodeID = Node->GetNodeID();

    // 从注册表移除
    if (!NodeRegistry.Remove(NodeID))
    {
        return false;
    }

    // 更新索引
    UpdateNodeTypeMap(Node, false);
    UpdateNodeTagMap(Node, false);

    // 从活动列表移除
    ActiveNodes.Remove(Node);

    // 移除所有相关连接
    RemoveAllConnectionsForNode(NodeID);

    // 注销事件
    UnregisterNodeEvents(Node);

    // 广播事件
    OnNodeUnregistered.Broadcast(Node);

    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Unregistered node %s"), *NodeID);
    return true;
}

// 节点查询实现
AInteractiveNode* ANodeSystemManager::GetNode(const FString& NodeID) const
{
    if (NodeRegistry.Contains(NodeID))
    {
        return NodeRegistry[NodeID];
    }
    return nullptr;
}

TArray<AInteractiveNode*> ANodeSystemManager::GetNodesByType(ENodeType Type) const
{
    FString TypeKey = UEnum::GetValueAsString(Type);
    if (NodeTypeMap.Contains(TypeKey))
    {
        return NodeTypeMap[TypeKey];
    }
    return TArray<AInteractiveNode*>();
}

TArray<AInteractiveNode*> ANodeSystemManager::GetNodesByState(ENodeState State) const
{
    TArray<AInteractiveNode*> Result;
    
    for (const auto& Pair : NodeRegistry)
    {
        if (Pair.Value && Pair.Value->GetNodeState() == State)
        {
            Result.Add(Pair.Value);
        }
    }
    
    return Result;
}

TArray<AInteractiveNode*> ANodeSystemManager::GetNodesByTag(const FGameplayTag& Tag) const
{
    TArray<AInteractiveNode*> Result;
    FString TagString = Tag.ToString();
    
    if (NodeTagMap.Contains(TagString))
    {
        Result = NodeTagMap[TagString];
    }
    
    return Result;
}

TArray<AInteractiveNode*> ANodeSystemManager::GetNodesInRadius(const FVector& Center, float Radius) const
{
    TArray<AInteractiveNode*> Result;
    float RadiusSq = Radius * Radius;
    
    for (const auto& Pair : NodeRegistry)
    {
        if (Pair.Value)
        {
            float DistSq = FVector::DistSquared(Pair.Value->GetActorLocation(), Center);
            if (DistSq <= RadiusSq)
            {
                Result.Add(Pair.Value);
            }
        }
    }
    
    return Result;
}

TArray<AItemNode*> ANodeSystemManager::FindNodesWithCapability(TSubclassOf<UItemCapability> CapabilityClass) const
{
    TArray<AItemNode*> Result;
    
    if (!CapabilityClass)
    {
        return Result;
    }
    
    for (const auto& Pair : NodeRegistry)
    {
        if (AItemNode* ItemNode = Cast<AItemNode>(Pair.Value))
        {
            if (ItemNode->HasCapability(CapabilityClass))
            {
                Result.Add(ItemNode);
            }
        }
    }
    
    return Result;
}

// 连接管理实现
ANodeConnection* ANodeSystemManager::CreateConnection(AInteractiveNode* Source, AInteractiveNode* Target, const FNodeRelationData& RelationData)
{
    if (!Source || !Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Cannot create connection without valid nodes"));
        return nullptr;
    }

    if (Source == Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Cannot create self-connection"));
        return nullptr;
    }

    // 检查是否已存在连接
    ANodeConnection* ExistingConnection = GetConnection(Source->GetNodeID(), Target->GetNodeID());
    if (ExistingConnection)
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Connection already exists between %s and %s"), 
            *Source->GetNodeID(), *Target->GetNodeID());
        return ExistingConnection;
    }

    // 创建连接
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    TSubclassOf<ANodeConnection> ConnectionClass = DefaultConnectionClass;
    if (!ConnectionClass)
    {
        ConnectionClass = ANodeConnection::StaticClass();
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ANodeConnection* NewConnection = World->SpawnActor<ANodeConnection>(ConnectionClass, SpawnParams);
    if (NewConnection)
    {
        // 初始化连接
        NewConnection->Initialize(Source, Target, RelationData.RelationType);
        NewConnection->SetConnectionWeight(RelationData.Weight);
        NewConnection->SetBidirectional(RelationData.bBidirectional);

        // 添加到注册表
        if (!ConnectionRegistry.Contains(Source->GetNodeID()))
        {
            ConnectionRegistry.Add(Source->GetNodeID(), TArray<ANodeConnection*>());
        }
        ConnectionRegistry[Source->GetNodeID()].Add(NewConnection);

        if (RelationData.bBidirectional || Target->GetNodeID() != Source->GetNodeID())
        {
            if (!ConnectionRegistry.Contains(Target->GetNodeID()))
            {
                ConnectionRegistry.Add(Target->GetNodeID(), TArray<ANodeConnection*>());
            }
            ConnectionRegistry[Target->GetNodeID()].Add(NewConnection);
        }

        // 添加到活动连接
        ActiveConnections.Add(NewConnection);

        // 注册事件
        RegisterConnectionEvents(NewConnection);

        // 广播事件
        OnConnectionCreated.Broadcast(NewConnection);

        UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Created connection between %s and %s"), 
            *Source->GetNodeID(), *Target->GetNodeID());
    }

    return NewConnection;
}

ANodeConnection* ANodeSystemManager::CreateConnectionBetween(const FString& SourceID, const FString& TargetID, ENodeRelationType Type)
{
    AInteractiveNode* Source = GetNode(SourceID);
    AInteractiveNode* Target = GetNode(TargetID);

    if (!Source || !Target)
    {
        return nullptr;
    }

    FNodeRelationData RelationData;
    RelationData.SourceNodeID = SourceID;
    RelationData.TargetNodeID = TargetID;
    RelationData.RelationType = Type;
    RelationData.Weight = 1.0f;

    return CreateConnection(Source, Target, RelationData);
}

bool ANodeSystemManager::RemoveConnection(ANodeConnection* Connection)
{
    if (!Connection)
    {
        return false;
    }

    // 从注册表移除
    FString SourceID = Connection->GetSourceNode() ? Connection->GetSourceNode()->GetNodeID() : "";
    FString TargetID = Connection->GetTargetNode() ? Connection->GetTargetNode()->GetNodeID() : "";

    bool bRemoved = false;

    if (ConnectionRegistry.Contains(SourceID))
    {
        ConnectionRegistry[SourceID].Remove(Connection);
        if (ConnectionRegistry[SourceID].Num() == 0)
        {
            ConnectionRegistry.Remove(SourceID);
        }
        bRemoved = true;
    }

    if (ConnectionRegistry.Contains(TargetID))
    {
        ConnectionRegistry[TargetID].Remove(Connection);
        if (ConnectionRegistry[TargetID].Num() == 0)
        {
            ConnectionRegistry.Remove(TargetID);
        }
    }

    // 从活动连接移除
    ActiveConnections.Remove(Connection);

    // 注销事件
    UnregisterConnectionEvents(Connection);

    // 广播事件
    OnConnectionRemoved.Broadcast(Connection);

    // 销毁连接
    Connection->Destroy();

    return bRemoved;
}

int32 ANodeSystemManager::RemoveConnectionsBetween(const FString& NodeA, const FString& NodeB)
{
    int32 RemovedCount = 0;
    TArray<ANodeConnection*> ConnectionsToRemove;

    // 查找所有相关连接
    if (ConnectionRegistry.Contains(NodeA))
    {
        for (ANodeConnection* Connection : ConnectionRegistry[NodeA])
        {
            if (Connection && Connection->IsConnecting(GetNode(NodeA), GetNode(NodeB)))
            {
                ConnectionsToRemove.AddUnique(Connection);
            }
        }
    }

    // 移除连接
    for (ANodeConnection* Connection : ConnectionsToRemove)
    {
        if (RemoveConnection(Connection))
        {
            RemovedCount++;
        }
    }

    return RemovedCount;
}

int32 ANodeSystemManager::RemoveAllConnectionsForNode(const FString& NodeID)
{
    int32 RemovedCount = 0;
    TArray<ANodeConnection*> ConnectionsToRemove;

    // 收集所有相关连接
    if (ConnectionRegistry.Contains(NodeID))
    {
        ConnectionsToRemove = ConnectionRegistry[NodeID];
    }

    // 移除连接
    for (ANodeConnection* Connection : ConnectionsToRemove)
    {
        if (RemoveConnection(Connection))
        {
            RemovedCount++;
        }
    }

    return RemovedCount;
}

// 连接查询实现
ANodeConnection* ANodeSystemManager::GetConnection(const FString& SourceID, const FString& TargetID) const
{
    if (!ConnectionRegistry.Contains(SourceID))
    {
        return nullptr;
    }

    for (ANodeConnection* Connection : ConnectionRegistry[SourceID])
    {
        if (Connection && Connection->GetTargetNode() && 
            Connection->GetTargetNode()->GetNodeID() == TargetID)
        {
            return Connection;
        }
    }

    return nullptr;
}

TArray<ANodeConnection*> ANodeSystemManager::GetConnectionsForNode(const FString& NodeID) const
{
    if (ConnectionRegistry.Contains(NodeID))
    {
        return ConnectionRegistry[NodeID];
    }
    return TArray<ANodeConnection*>();
}

TArray<ANodeConnection*> ANodeSystemManager::GetOutgoingConnections(const FString& NodeID) const
{
    TArray<ANodeConnection*> Result;
    
    if (ConnectionRegistry.Contains(NodeID))
    {
        for (ANodeConnection* Connection : ConnectionRegistry[NodeID])
        {
            if (Connection && Connection->GetSourceNode() && 
                Connection->GetSourceNode()->GetNodeID() == NodeID)
            {
                Result.Add(Connection);
            }
        }
    }
    
    return Result;
}

TArray<ANodeConnection*> ANodeSystemManager::GetIncomingConnections(const FString& NodeID) const
{
    TArray<ANodeConnection*> Result;
    
    if (ConnectionRegistry.Contains(NodeID))
    {
        for (ANodeConnection* Connection : ConnectionRegistry[NodeID])
        {
            if (Connection && Connection->GetTargetNode() && 
                Connection->GetTargetNode()->GetNodeID() == NodeID)
            {
                Result.Add(Connection);
            }
        }
    }
    
    return Result;
}

TArray<AInteractiveNode*> ANodeSystemManager::GetConnectedNodes(const FString& NodeID, ENodeRelationType RelationType) const
{
    TArray<AInteractiveNode*> Result;
    TArray<ANodeConnection*> Connections = GetConnectionsForNode(NodeID);
    
    for (ANodeConnection* Connection : Connections)
    {
        if (Connection && Connection->RelationType == RelationType)
        {
            AInteractiveNode* OtherNode = Connection->GetOppositeNode(GetNode(NodeID));
            if (OtherNode)
            {
                Result.AddUnique(OtherNode);
            }
        }
    }
    
    return Result;
}

// 场景管理实现
bool ANodeSystemManager::SetActiveScene(ASceneNode* Scene)
{
    if (!Scene)
    {
        return false;
    }

    ASceneNode* OldScene = ActiveSceneNode;
    ActiveSceneNode = Scene;

    // 停用旧场景
    if (OldScene && OldScene != Scene)
    {
        OldScene->DeactivateScene();
    }

    // 激活新场景
    Scene->ActivateScene();

    // 广播事件
    OnSceneChanged.Broadcast(OldScene, Scene);
    OnSystemStateChanged.Broadcast(FString::Printf(TEXT("Scene changed to %s"), *Scene->GetNodeName()));

    return true;
}

void ANodeSystemManager::TransitionToScene(ASceneNode* NewScene, float TransitionDuration)
{
    if (!NewScene || NewScene == ActiveSceneNode)
    {
        return;
    }

    bIsTransitioning = true;
    TransitionProgress = 0.0f;
    TransitionTargetScene = NewScene;

    // 开始过渡效果
    FTimerHandle TransitionHandle;
    GetWorld()->GetTimerManager().SetTimer(TransitionHandle,
        [this]()
        {
            SetActiveScene(TransitionTargetScene);
            bIsTransitioning = false;
            TransitionTargetScene = nullptr;
        },
        TransitionDuration, false);
}

// 生成队列实现
void ANodeSystemManager::QueueNodeGeneration(const FNodeGenerateData& GenerateData)
{
    NodeGenerationQueue.Enqueue(GenerateData);
}

void ANodeSystemManager::QueueConnectionGeneration(const FNodeRelationData& RelationData)
{
    ConnectionGenerationQueue.Enqueue(RelationData);
}

void ANodeSystemManager::ProcessGenerationQueue()
{
    ProcessNodeGeneration();
    ProcessConnectionGeneration();
}

void ANodeSystemManager::ClearGenerationQueues()
{
    NodeGenerationQueue.Empty();
    ConnectionGenerationQueue.Empty();
}

// 高级查询实现
TArray<AInteractiveNode*> ANodeSystemManager::ExecuteNodeQuery(const FNodeQueryParams& QueryParams) const
{
    TArray<AInteractiveNode*> Result;
    
    for (const auto& Pair : NodeRegistry)
    {
        AInteractiveNode* Node = Pair.Value;
        if (!Node)
        {
            continue;
        }

        // 检查节点类型
        if (QueryParams.NodeTypes.Num() > 0 && 
            !QueryParams.NodeTypes.Contains(Node->GetNodeData().NodeType))
        {
            continue;
        }

        // 检查节点状态
        if (QueryParams.NodeStates.Num() > 0 && 
            !QueryParams.NodeStates.Contains(Node->GetNodeState()))
        {
            continue;
        }

        // 检查标签
        if (QueryParams.TagQuery.IsEmpty() == false)
        {
            FGameplayTagContainer NodeTags = Node->GetNodeData().NodeTags;
            if (!QueryParams.TagQuery.Matches(NodeTags))
            {
                continue;
            }
        }

        // 检查距离
        if (QueryParams.MaxDistance > 0.0f)
        {
            // 需要一个参考点，这里假设使用玩家位置
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC && PC->GetPawn())
            {
                float Distance = FVector::Dist(Node->GetActorLocation(), PC->GetPawn()->GetActorLocation());
                if (Distance > QueryParams.MaxDistance)
                {
                    continue;
                }
            }
        }

        // 检查是否包含非激活节点
        if (!QueryParams.bIncludeInactive && Node->GetNodeState() == ENodeState::Inactive)
        {
            continue;
        }

        Result.Add(Node);
    }
    
    return Result;
}

TArray<AInteractiveNode*> ANodeSystemManager::FindPath(AInteractiveNode* Start, AInteractiveNode* End) const
{
    TArray<AInteractiveNode*> Path;
    
    if (!Start || !End || Start == End)
    {
        return Path;
    }

    // 简单的BFS路径查找
    TQueue<AInteractiveNode*> Queue;
    TMap<AInteractiveNode*, AInteractiveNode*> CameFrom;
    TSet<AInteractiveNode*> Visited;

    Queue.Enqueue(Start);
    Visited.Add(Start);

    while (!Queue.IsEmpty())
    {
        AInteractiveNode* Current;
        Queue.Dequeue(Current);

        if (Current == End)
        {
            // 重建路径
            AInteractiveNode* Node = End;
            while (Node != Start)
            {
                Path.Insert(Node, 0);
                Node = CameFrom[Node];
            }
            Path.Insert(Start, 0);
            break;
        }

        // 获取所有连接的节点
        TArray<ANodeConnection*> Connections = GetConnectionsForNode(Current->GetNodeID());
        for (ANodeConnection* Connection : Connections)
        {
            if (!Connection)
            {
                continue;
            }

            AInteractiveNode* Neighbor = Connection->GetOppositeNode(Current);
            if (Neighbor && !Visited.Contains(Neighbor))
            {
                Visited.Add(Neighbor);
                CameFrom.Add(Neighbor, Current);
                Queue.Enqueue(Neighbor);
            }
        }
    }

    return Path;
}

TArray<AInteractiveNode*> ANodeSystemManager::GetNodeHierarchy(AInteractiveNode* RootNode) const
{
    TArray<AInteractiveNode*> Hierarchy;
    
    if (!RootNode)
    {
        return Hierarchy;
    }

    // // DFS遍历
    // TSet<AInteractiveNode*> Visited;
    // TArray<AInteractiveNode*> Stack;
    //
    // Stack.Add(RootNode);
    //
    // while (Stack.Num() > 0)
    // {
    //     AInteractiveNode* Current = Stack.Pop();
    //     
    //     if (Visited.Contains(Current))
    //     {
    //         continue;
    //     }
    //     
    //     Visited.Add(Current);
    //     Hierarchy.Add(Current);
    //     
    //     // 如果是场景节点，添加其子节点
    //     if (ASceneNode* SceneNode = Cast<ASceneNode>(Current))
    //     {
    //         TArray<AInteractiveNode*> Children = SceneNode->GetAllChildNodes();
    //         for (int32 i = Children.Num() - 1; i >= 0; i--)
    //         {
    //             if (Children[i] && !Visited.Contains(Children[i]))
    //             {
    //                 Stack.Add(Children[i]);
    //             }
    //         }
    //     }
    //     
    //     // 添加连接的节点
    //     TArray<ANodeConnection*> Connections = GetOutgoingConnections(Current->GetNodeID());
    //     for (ANodeConnection* Connection : Connections)
    //     {
    //         if (Connection && Connection->GetTargetNode() && !Visited.Contains(Connection->GetTargetNode()))
    //         {
    //             Stack.Add(Connection->GetTargetNode());
    //         }
    //     }
    // }
    
    return Hierarchy;
}

// 系统管理实现
bool ANodeSystemManager::SaveSystemState(const FString& SaveName)
{
    FSystemState SaveData;
    SaveData.SaveTime = FDateTime::Now();
    SaveData.ActiveSceneID = ActiveSceneNode ? ActiveSceneNode->GetNodeID() : "";

    // 保存节点数据
    for (const auto& Pair : NodeRegistry)
    {
        if (Pair.Value)
        {
            SaveData.SavedNodes.Add(Pair.Value->GetNodeData());
        }
    }

    // 保存连接数据
    TSet<ANodeConnection*> ProcessedConnections;
    for (const auto& Pair : ConnectionRegistry)
    {
        for (ANodeConnection* Connection : Pair.Value)
        {
            if (Connection && !ProcessedConnections.Contains(Connection))
            {
                ProcessedConnections.Add(Connection);
                
                FNodeRelationData RelationData;
                RelationData.SourceNodeID = Connection->GetSourceNode()->GetNodeID();
                RelationData.TargetNodeID = Connection->GetTargetNode()->GetNodeID();
                RelationData.RelationType = Connection->RelationType;
                RelationData.Weight = Connection->ConnectionWeight;
                RelationData.bBidirectional = Connection->bIsBidirectional;
                
                SaveData.SavedConnections.Add(RelationData);
            }
        }
    }

    // 保存系统元数据
    SaveData.SystemData = SystemMetadata;

    // TODO: 实际的保存逻辑（序列化到文件或SaveGame）
    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Saved system state as %s"), *SaveName);
    return true;
}

bool ANodeSystemManager::LoadSystemState(const FString& SaveName)
{
    // TODO: 实际的加载逻辑
    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Loading system state %s"), *SaveName);
    return false;
}

void ANodeSystemManager::ResetSystem()
{
    // 清理所有连接
    TArray<ANodeConnection*> AllConnections = ActiveConnections;
    for (ANodeConnection* Connection : AllConnections)
    {
        RemoveConnection(Connection);
    }

    // 清理所有节点
    TArray<FString> NodeIDs;
    NodeRegistry.GetKeys(NodeIDs);
    for (const FString& NodeID : NodeIDs)
    {
        UnregisterNodeByID(NodeID);
    }

    // 清空注册表
    NodeRegistry.Empty();
    ConnectionRegistry.Empty();
    NodeTypeMap.Empty();
    NodeTagMap.Empty();
    ActiveNodes.Empty();
    ActiveConnections.Empty();

    // 重置状态
    ActiveSceneNode = nullptr;
    bIsTransitioning = false;

    // 清空队列
    ClearGenerationQueues();

    OnSystemStateChanged.Broadcast(TEXT("System reset"));
}

bool ANodeSystemManager::ValidateSystem()
{
    bool bIsValid = true;
    int32 InvalidNodes = 0;
    int32 InvalidConnections = 0;

    // 验证节点
    TArray<FString> InvalidNodeIDs;
    for (const auto& Pair : NodeRegistry)
    {
        if (!IsValid(Pair.Value))
        {
            InvalidNodeIDs.Add(Pair.Key);
            InvalidNodes++;
            bIsValid = false;
        }
    }

    // 移除无效节点
    for (const FString& NodeID : InvalidNodeIDs)
    {
        NodeRegistry.Remove(NodeID);
    }

    // 验证连接
    TArray<ANodeConnection*> InvalidConnectionsList;
    for (const auto& Pair : ConnectionRegistry)
    {
        for (ANodeConnection* Connection : Pair.Value)
        {
            if (!IsValid(Connection) || !Connection->IsValid())
            {
                InvalidConnectionsList.AddUnique(Connection);
                InvalidConnections++;
                bIsValid = false;
            }
        }
    }

    // 移除无效连接
    for (ANodeConnection* Connection : InvalidConnectionsList)
    {
        RemoveConnection(Connection);
    }

    if (!bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Validation found %d invalid nodes and %d invalid connections"), 
            InvalidNodes, InvalidConnections);
        CleanupInvalidReferences();
    }

    return bIsValid;
}

// 事件处理实现
void ANodeSystemManager::OnNodeStateChanged(AInteractiveNode* Node, ENodeState OldState, ENodeState NewState)
{
    if (!Node)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Node %s state changed from %s to %s"),
        *Node->GetNodeID(),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState));

    // 更新活动节点列表
    if (NewState == ENodeState::Active)
    {
        ActiveNodes.AddUnique(Node);
    }
    else
    {
        ActiveNodes.Remove(Node);
    }

    // 处理完成状态
    if (NewState == ENodeState::Completed)
    {
        ActivateDependentNodes(Node);
    }

    // 传播系统事件
    FGameEventData EventData;
    EventData.EventID = FString::Printf(TEXT("NodeStateChanged_%s"), *Node->GetNodeID());
    EventData.EventType = EGameEventType::StateChange;
    EventData.SourceNodeID = Node->GetNodeID();
    EventData.EventParameters.Add("OldState", UEnum::GetValueAsString(OldState));
    EventData.EventParameters.Add("NewState", UEnum::GetValueAsString(NewState));
    
    PropagateSystemEvent(EventData);
}

void ANodeSystemManager::OnNodeInteracted(AInteractiveNode* Node, const FInteractionData& Data)
{
    if (!Node)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("NodeSystemManager: Node %s interacted"), *Node->GetNodeID());

    // 传播系统事件
    FGameEventData EventData;
    EventData.EventID = FString::Printf(TEXT("NodeInteracted_%s"), *Node->GetNodeID());
    EventData.EventType = EGameEventType::NodeInteraction;
    EventData.SourceNodeID = Node->GetNodeID();
    
    PropagateSystemEvent(EventData);
}

void ANodeSystemManager::OnNodeDestroyed(AActor* DestroyedActor)
{
    AInteractiveNode* Node = Cast<AInteractiveNode>(DestroyedActor);
    if (Node)
    {
        UnregisterNode(Node);
    }
}

void ANodeSystemManager::OnConnectionStateChanged(ANodeConnection* Connection)
{
    if (!Connection)
    {
        return;
    }

    // 更新活动连接列表
    if (Connection->bIsActive)
    {
        ActiveConnections.AddUnique(Connection);
    }
    else
    {
        ActiveConnections.Remove(Connection);
    }
}

// 内部方法实现
void ANodeSystemManager::ProcessNodeGeneration()
{
    if (NodeGenerationQueue.IsEmpty())
    {
        return;
    }

    FNodeGenerateData GenerateData;
    if (NodeGenerationQueue.Dequeue(GenerateData))
    {
        // 检查是否超过最大节点数
        if (ActiveSceneNode && ActiveSceneNode->GetChildNodeCount() >= MaxNodesPerScene)
        {
            UE_LOG(LogTemp, Warning, TEXT("NodeSystemManager: Scene has reached max nodes limit"));
            return;
        }

        AInteractiveNode* NewNode = CreateNode(GenerateData.NodeClass, GenerateData);
        if (NewNode)
        {
            // 如果有活动场景，将节点添加到场景
            if (ActiveSceneNode)
            {
                ActiveSceneNode->AddChildNode(NewNode);
            }

            // 处理关系
            for (const FNodeRelationData& RelationData : GenerateData.Relations)
            {
                if (RelationData.TargetNodeID == GenerateData.NodeData.NodeID)
                {
                    // 创建到新节点的连接
                    CreateConnectionBetween(RelationData.SourceNodeID, NewNode->GetNodeID(), RelationData.RelationType);
                }
                else
                {
                    // 创建从新节点的连接
                    CreateConnectionBetween(NewNode->GetNodeID(), RelationData.TargetNodeID, RelationData.RelationType);
                }
            }
        }
    }
}

void ANodeSystemManager::ProcessConnectionGeneration()
{
    if (ConnectionGenerationQueue.IsEmpty())
    {
        return;
    }

    FNodeRelationData RelationData;
    if (ConnectionGenerationQueue.Dequeue(RelationData))
    {
        AInteractiveNode* Source = GetNode(RelationData.SourceNodeID);
        AInteractiveNode* Target = GetNode(RelationData.TargetNodeID);

        if (Source && Target)
        {
            CreateConnection(Source, Target, RelationData);
        }
        else
        {
            // 如果节点还不存在，重新加入队列
            ConnectionGenerationQueue.Enqueue(RelationData);
        }
    }
}

void ANodeSystemManager::UpdateNodeIndices()
{
    // 重建类型索引
    NodeTypeMap.Empty();
    for (const auto& Pair : NodeRegistry)
    {
        if (Pair.Value)
        {
            UpdateNodeTypeMap(Pair.Value, true);
        }
    }

    // 重建标签索引
    NodeTagMap.Empty();
    for (const auto& Pair : NodeRegistry)
    {
        if (Pair.Value)
        {
            UpdateNodeTagMap(Pair.Value, true);
        }
    }
}

void ANodeSystemManager::CleanupInvalidReferences()
{
    // 清理无效的节点引用
    TArray<FString> KeysToRemove;
    for (const auto& Pair : NodeRegistry)
    {
        if (!IsValid(Pair.Value))
        {
            KeysToRemove.Add(Pair.Key);
        }
    }

    for (const FString& Key : KeysToRemove)
    {
        NodeRegistry.Remove(Key);
    }

    // 清理无效的连接引用
    TArray<FString> ConnectionKeysToRemove;
    for (const auto& Pair : ConnectionRegistry)
    {
        bool bHasValidConnections = false;
        for (ANodeConnection* Connection : Pair.Value)
        {
            if (IsValid(Connection))
            {
                bHasValidConnections = true;
                break;
            }
        }

        if (!bHasValidConnections)
        {
            ConnectionKeysToRemove.Add(Pair.Key);
        }
    }

    for (const FString& Key : ConnectionKeysToRemove)
    {
        ConnectionRegistry.Remove(Key);
    }

    // 更新索引
    UpdateNodeIndices();
}

void ANodeSystemManager::PropagateSystemEvent(const FGameEventData& EventData)
{
    // 这里可以实现事件传播逻辑
    // 例如：通知事件管理器、触发故事系统等
}

bool ANodeSystemManager::CheckPrerequisites(AInteractiveNode* Node) const
{
    if (!Node)
    {
        return false;
    }

    // 获取所有前置条件连接
    TArray<ANodeConnection*> Prerequisites = GetIncomingConnections(Node->GetNodeID());
    
    for (ANodeConnection* Connection : Prerequisites)
    {
        if (Connection && Connection->RelationType == ENodeRelationType::Prerequisite)
        {
            AInteractiveNode* PrereqNode = Connection->GetSourceNode();
            if (PrereqNode && PrereqNode->GetNodeState() != ENodeState::Completed)
            {
                return false;
            }
        }
    }

    return true;
}

void ANodeSystemManager::ActivateDependentNodes(AInteractiveNode* CompletedNode)
{
    if (!CompletedNode)
    {
        return;
    }

    // 获取所有依赖此节点的连接
    TArray<ANodeConnection*> DependentConnections = GetOutgoingConnections(CompletedNode->GetNodeID());

    for (ANodeConnection* Connection : DependentConnections)
    {
        if (!Connection)
        {
            continue;
        }

        // 根据连接类型处理
        if (Connection->RelationType == ENodeRelationType::Prerequisite ||
            Connection->RelationType == ENodeRelationType::Dependency)
        {
            AInteractiveNode* DependentNode = Connection->GetTargetNode();
            if (DependentNode && DependentNode->GetNodeState() == ENodeState::Locked)
            {
                // 检查是否所有前置条件都满足
                if (CheckPrerequisites(DependentNode))
                {
                    DependentNode->SetNodeState(ENodeState::Active);
                }
            }
        }
    }
}

// 辅助方法实现
void ANodeSystemManager::RegisterNodeEvents(AInteractiveNode* Node)
{
    if (!Node)
    {
        return;
    }

    Node->OnNodeStateChanged.AddDynamic(this, &ANodeSystemManager::OnNodeStateChanged);
    Node->OnNodeInteracted.AddDynamic(this, &ANodeSystemManager::OnNodeInteracted);
    Node->OnDestroyed.AddDynamic(this, &ANodeSystemManager::OnNodeDestroyed);
}

void ANodeSystemManager::UnregisterNodeEvents(AInteractiveNode* Node)
{
    if (!Node)
    {
        return;
    }

    Node->OnNodeStateChanged.RemoveDynamic(this, &ANodeSystemManager::OnNodeStateChanged);
    Node->OnNodeInteracted.RemoveDynamic(this, &ANodeSystemManager::OnNodeInteracted);
    Node->OnDestroyed.RemoveDynamic(this, &ANodeSystemManager::OnNodeDestroyed);
}

void ANodeSystemManager::RegisterConnectionEvents(ANodeConnection* Connection)
{
    if (!Connection)
    {
        return;
    }

    // 可以添加连接相关的事件监听
}

void ANodeSystemManager::UnregisterConnectionEvents(ANodeConnection* Connection)
{
    if (!Connection)
    {
        return;
    }

    // 移除连接相关的事件监听
}

void ANodeSystemManager::UpdateNodeTypeMap(AInteractiveNode* Node, bool bAdd)
{
    if (!Node)
    {
        return;
    }

    FString TypeKey = UEnum::GetValueAsString(Node->GetNodeData().NodeType);

    if (bAdd)
    {
        if (!NodeTypeMap.Contains(TypeKey))
        {
            NodeTypeMap.Add(TypeKey, TArray<AInteractiveNode*>());
        }
        NodeTypeMap[TypeKey].AddUnique(Node);
    }
    else
    {
        if (NodeTypeMap.Contains(TypeKey))
        {
            NodeTypeMap[TypeKey].Remove(Node);
            if (NodeTypeMap[TypeKey].Num() == 0)
            {
                NodeTypeMap.Remove(TypeKey);
            }
        }
    }
}

void ANodeSystemManager::UpdateNodeTagMap(AInteractiveNode* Node, bool bAdd)
{
    if (!Node)
    {
        return;
    }

    FGameplayTagContainer NodeTags = Node->GetNodeData().NodeTags;
    
    for (const FGameplayTag& Tag : NodeTags)
    {
        FString TagString = Tag.ToString();
        
        if (bAdd)
        {
            if (!NodeTagMap.Contains(TagString))
            {
                NodeTagMap.Add(TagString, TArray<AInteractiveNode*>());
            }
            NodeTagMap[TagString].AddUnique(Node);
        }
        else
        {
            if (NodeTagMap.Contains(TagString))
            {
                NodeTagMap[TagString].Remove(Node);
                if (NodeTagMap[TagString].Num() == 0)
                {
                    NodeTagMap.Remove(TagString);
                }
            }
        }
    }
}

FVector ANodeSystemManager::CalculateNodeSpawnLocation(const FVector& BaseLocation) const
{
    // 在基础位置周围随机生成
    float Angle = FMath::RandRange(0.0f, 360.0f);
    float Distance = FMath::RandRange(100.0f, NodeSpawnRadius);
    
    float X = BaseLocation.X + Distance * FMath::Cos(FMath::DegreesToRadians(Angle));
    float Y = BaseLocation.Y + Distance * FMath::Sin(FMath::DegreesToRadians(Angle));
    float Z = BaseLocation.Z;
    
    return FVector(X, Y, Z);
}
