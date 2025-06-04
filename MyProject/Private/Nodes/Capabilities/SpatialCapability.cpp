// Fill out your copyright notice in the Description page of Project Settings.

#include "Nodes/Capabilities/SpatialCapability.h"
#include "Nodes/ItemNode.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/NodeSystemManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

USpatialCapability::USpatialCapability()
{
    // 设置默认值
    CapabilityID = TEXT("SpatialCapability");
    CapabilityDescription = FText::FromString(TEXT("管理空间关系和节点容纳"));
    UsagePrompt = TEXT("进入/使用空间");
    
    // 容纳功能默认值
    bCanContainNodes = true;
    MaxContainedNodes = 5;
    AllowedNodeClass = AInteractiveNode::StaticClass();
    
    // 访问控制默认值
    bAllowsEntry = true;
    bAllowsExit = true;
    
    // 传送功能默认值
    TeleportDestination = FVector::ZeroVector;
    bTeleportToNode = false;
    
    // 预留功能默认值
    LightingRadius = 1000.0f;
    
    CachedSystemManager = nullptr;
}

void USpatialCapability::Initialize_Implementation(AItemNode* Owner)
{
    Super::Initialize_Implementation(Owner);
    
    // 缓存系统管理器
    CachedSystemManager = GetNodeSystemManager();
    
    // 从配置加载数据
    if (SpatialConfig.Num() > 0)
    {
        LoadSpatialConfig(SpatialConfig);
    }
    
    UE_LOG(LogTemp, Log, TEXT("SpatialCapability initialized for %s"), 
        Owner ? *Owner->GetNodeName() : TEXT("Unknown"));
}

bool USpatialCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanUse_Implementation(Data))
    {
        return false;
    }
    
    // 空间能力的使用取决于具体功能
    // 如果是容纳功能，检查是否还有空间
    if (bCanContainNodes && ContainedNodes.Num() >= MaxContainedNodes)
    {
        return false;
    }
    
    return true;
}

bool USpatialCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!Super::Use_Implementation(Data))
    {
        return false;
    }
    
    bool bSuccess = false;
    
    // 传送功能
    if (Data.InteractionType == EInteractionType::Click && Data.Instigator)
    {
        bSuccess = TeleportPlayer(Data.Instigator);
    }
    
    return bSuccess;
}

void USpatialCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    Super::OnOwnerStateChanged_Implementation(NewState);
    
    // 当拥有者状态改变时，可能影响容纳的节点
    if (NewState == ENodeState::Inactive || NewState == ENodeState::Hidden)
    {
        // 停用时可能需要释放所有节点
        if (ContainedNodes.Num() > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Owner deactivated, consider releasing contained nodes"));
        }
    }
}

bool USpatialCapability::ContainNode(AInteractiveNode* Node)
{
    if (!bCanContainNodes || !Node || !OwnerItem)
    {
        return false;
    }
    
    // 检查是否已经容纳
    if (ContainedNodes.Contains(Node))
    {
        UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Node %s already contained"), *Node->GetNodeName());
        return false;
    }
    
    // 检查容量
    if (ContainedNodes.Num() >= MaxContainedNodes)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Container full (%d/%d)"), 
            ContainedNodes.Num(), MaxContainedNodes);
        return false;
    }
    
    // 验证节点类型
    if (!ValidateNodeForContainment(Node))
    {
        return false;
    }
    
    // 创建Parent关系
    ANodeConnection* Connection = CreateParentConnection(Node);
    if (Connection)
    {
        ContainedNodes.Add(Node);
        NodeConnectionMap.Add(Node, Connection);
        
        // 监听节点销毁事件
        Node->OnDestroyed.AddDynamic(this, &USpatialCapability::OnContainedNodeDestroyed);
        
        UE_LOG(LogTemp, Log, TEXT("SpatialCapability: Successfully contained node %s"), *Node->GetNodeName());
        return true;
    }
    
    return false;
}

bool USpatialCapability::ReleaseNode(AInteractiveNode* Node)
{
    if (!Node || !ContainedNodes.Contains(Node))
    {
        return false;
    }
    
    // 移除Parent关系
    RemoveParentConnection(Node);
    
    // 从列表中移除
    ContainedNodes.Remove(Node);
    NodeConnectionMap.Remove(Node);
    
    // 取消监听
    Node->OnDestroyed.RemoveDynamic(this, &USpatialCapability::OnContainedNodeDestroyed);
    
    UE_LOG(LogTemp, Log, TEXT("SpatialCapability: Released node %s"), *Node->GetNodeName());
    return true;
}

bool USpatialCapability::ReleaseAllNodes()
{
    TArray<AInteractiveNode*> NodesToRelease = ContainedNodes;
    bool bAllReleased = true;
    
    for (AInteractiveNode* Node : NodesToRelease)
    {
        if (!ReleaseNode(Node))
        {
            bAllReleased = false;
        }
    }
    
    return bAllReleased;
}

bool USpatialCapability::IsNodeContained(AInteractiveNode* Node) const
{
    return Node && ContainedNodes.Contains(Node);
}

bool USpatialCapability::CanNodeEnter(AInteractiveNode* RequestingNode) const
{
    if (!RequestingNode || !bAllowsEntry)
    {
        return false;
    }
    
    // 检查特定权限
    FString NodeID = RequestingNode->GetNodeID();
    if (AccessPermissions.Contains(NodeID))
    {
        return AccessPermissions[NodeID];
    }
    
    // 检查节点类型
    if (AllowedNodeClass && !RequestingNode->IsA(AllowedNodeClass))
    {
        return false;
    }
    
    // 检查容量
    if (bCanContainNodes && ContainedNodes.Num() >= MaxContainedNodes)
    {
        return false;
    }
    
    return true;
}

bool USpatialCapability::CanNodeExit(AInteractiveNode* RequestingNode) const
{
    if (!RequestingNode || !bAllowsExit)
    {
        return false;
    }
    
    // 检查节点是否被容纳
    if (!IsNodeContained(RequestingNode))
    {
        return true; // 不在里面的节点总是可以"离开"
    }
    
    // 可以添加更多的离开条件检查
    return true;
}

void USpatialCapability::SetNodeAccess(const FString& NodeID, bool bAllowAccess)
{
    if (NodeID.IsEmpty())
    {
        return;
    }
    
    AccessPermissions.Add(NodeID, bAllowAccess);
    
    UE_LOG(LogTemp, Log, TEXT("SpatialCapability: Set access for node %s to %s"), 
        *NodeID, bAllowAccess ? TEXT("Allowed") : TEXT("Denied"));
}

bool USpatialCapability::TeleportPlayer(APlayerController* Player)
{
    if (!Player || !Player->GetPawn())
    {
        return false;
    }
    
    FVector TargetLocation = TeleportDestination;
    
    // 如果设置了传送到节点
    if (bTeleportToNode && !TeleportTargetNodeID.IsEmpty())
    {
        ANodeSystemManager* SystemManager = GetNodeSystemManager();
        if (SystemManager)
        {
            AInteractiveNode* TargetNode = SystemManager->GetNode(TeleportTargetNodeID);
            if (TargetNode)
            {
                TargetLocation = TargetNode->GetActorLocation();
            }
        }
    }
    
    // 执行传送
    if (!TargetLocation.IsZero())
    {
        Player->GetPawn()->SetActorLocation(TargetLocation);
        
        UE_LOG(LogTemp, Log, TEXT("SpatialCapability: Teleported player to %s"), *TargetLocation.ToString());
        return true;
    }
    
    return false;
}

void USpatialCapability::SetTeleportTarget(const FVector& Location)
{
    TeleportDestination = Location;
    bTeleportToNode = false;
}

void USpatialCapability::SetTeleportTargetNode(const FString& NodeID)
{
    TeleportTargetNodeID = NodeID;
    bTeleportToNode = true;
}

void USpatialCapability::LoadSpatialConfig(const TMap<FString, FString>& Config)
{
    for (const auto& Pair : Config)
    {
        ApplyConfigValue(Pair.Key, Pair.Value);
    }
}

void USpatialCapability::ApplyConfigValue(const FString& Key, const FString& Value)
{
    if (Key == TEXT("MaxContainedNodes"))
    {
        MaxContainedNodes = FCString::Atoi(*Value);
    }
    else if (Key == TEXT("AllowsEntry"))
    {
        bAllowsEntry = Value.ToBool();
    }
    else if (Key == TEXT("AllowsExit"))
    {
        bAllowsExit = Value.ToBool();
    }
    else if (Key == TEXT("TeleportTargetNode"))
    {
        SetTeleportTargetNode(Value);
    }
    else if (Key == TEXT("LightingRadius"))
    {
        LightingRadius = FCString::Atof(*Value);
    }
    
    // 保存到配置
    SpatialConfig.Add(Key, Value);
}

ANodeSystemManager* USpatialCapability::GetNodeSystemManager() const
{
    if (CachedSystemManager)
    {
        return CachedSystemManager;
    }
    
    // 查找系统管理器
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

ANodeConnection* USpatialCapability::CreateParentConnection(AInteractiveNode* ChildNode)
{
    if (!OwnerItem || !ChildNode)
    {
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("SpatialCapability: No NodeSystemManager found"));
        return nullptr;
    }
    
    // 创建Parent关系
    FNodeRelationData RelationData;
    RelationData.SourceNodeID = OwnerItem->GetNodeID();
    RelationData.TargetNodeID = ChildNode->GetNodeID();
    RelationData.RelationType = ENodeRelationType::Parent;
    RelationData.Weight = 1.0f;
    RelationData.bBidirectional = true; // Parent关系通常是双向的
    
    return SystemManager->CreateConnection(OwnerItem, ChildNode, RelationData);
}

void USpatialCapability::RemoveParentConnection(AInteractiveNode* ChildNode)
{
    if (!OwnerItem || !ChildNode)
    {
        return;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return;
    }
    
    // 从映射中获取连接
    ANodeConnection** ConnectionPtr = NodeConnectionMap.Find(ChildNode);
    if (ConnectionPtr && *ConnectionPtr)
    {
        SystemManager->RemoveConnection(*ConnectionPtr);
    }
}

bool USpatialCapability::ValidateNodeForContainment(AInteractiveNode* Node) const
{
    if (!Node)
    {
        return false;
    }
    
    // 检查节点类型
    if (AllowedNodeClass && !Node->IsA(AllowedNodeClass))
    {
        UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Node %s is not of allowed type"), *Node->GetNodeName());
        return false;
    }
    
    // 不能容纳自己
    if (Node == OwnerItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Cannot contain self"));
        return false;
    }
    
    // 检查节点状态
    ENodeState NodeState = Node->GetNodeState();
    if (NodeState == ENodeState::Hidden || NodeState == ENodeState::Locked)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Cannot contain node in state %d"), (int32)NodeState);
        return false;
    }
    
    return true;
}

void USpatialCapability::OnContainedNodeDestroyed(AActor* DestroyedActor)
{
    AInteractiveNode* DestroyedNode = Cast<AInteractiveNode>(DestroyedActor);
    if (DestroyedNode && ContainedNodes.Contains(DestroyedNode))
    {
        // 清理引用
        ContainedNodes.Remove(DestroyedNode);
        NodeConnectionMap.Remove(DestroyedNode);
        
        UE_LOG(LogTemp, Warning, TEXT("SpatialCapability: Contained node %s was destroyed"), 
            *DestroyedNode->GetNodeName());
    }
}