// Fill out your copyright notice in the Description page of Project Settings.

#include "Nodes/SceneNode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ASceneNode::ASceneNode()
{
    // 设置默认值
    NodeData.NodeType = ENodeType::Scene;
    bIsActiveScene = false;
    SceneRadius = 2000.0f;
    
    // 场景节点默认不显示UI
    bAlwaysShowUI = false;
    UIDisplayDistance = 2000.0f;
}

void ASceneNode::BeginPlay()
{
    Super::BeginPlay();
    
    // 如果场景默认激活，则激活场景
    if (CurrentState == ENodeState::Active)
    {
        ActivateScene();
    }
}

void ASceneNode::AddChildNode(AInteractiveNode* Node)
{
    if (!Node || ChildNodes.Contains(Node))
    {
        return;
    }

    ChildNodes.Add(Node);
    RegisterChildNode(Node);
    
    // 如果场景激活，设置子节点位置
    if (bIsActiveScene)
    {
        // 根据场景状态更新子节点
        if (Node->GetNodeState() == ENodeState::Inactive)
        {
            Node->SetNodeState(ENodeState::Active);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Scene %s added child node %s"), *NodeData.NodeName, *Node->GetNodeName());
}

void ASceneNode::RemoveChildNode(AInteractiveNode* Node)
{
    if (!Node || !ChildNodes.Contains(Node))
    {
        return;
    }

    UnregisterChildNode(Node);
    ChildNodes.Remove(Node);
    
    UE_LOG(LogTemp, Log, TEXT("Scene %s removed child node %s"), 
        *NodeData.NodeName, *Node->GetNodeName());
}

void ASceneNode::RemoveChildNodeByID(const FString& NodeID)
{
    AInteractiveNode* Node = GetChildNode(NodeID);
    if (Node)
    {
        RemoveChildNode(Node);
    }
}

AInteractiveNode* ASceneNode::GetChildNode(const FString& NodeID) const
{
    if (ChildNodeMap.Contains(NodeID))
    {
        return ChildNodeMap[NodeID];
    }
    return nullptr;
}

TArray<AInteractiveNode*> ASceneNode::GetChildNodesByType(ENodeType Type) const
{
    TArray<AInteractiveNode*> FilteredNodes;
    
    for (AInteractiveNode* Node : ChildNodes)
    {
        if (Node && Node->GetNodeData().NodeType == Type)
        {
            FilteredNodes.Add(Node);
        }
    }
    
    return FilteredNodes;
}

TArray<AInteractiveNode*> ASceneNode::GetChildNodesByState(ENodeState State) const
{
    TArray<AInteractiveNode*> FilteredNodes;
    
    for (AInteractiveNode* Node : ChildNodes)
    {
        if (Node && Node->GetNodeState() == State)
        {
            FilteredNodes.Add(Node);
        }
    }
    
    return FilteredNodes;
}

void ASceneNode::ActivateScene()
{
    if (bIsActiveScene)
    {
        return;
    }

    bIsActiveScene = true;
    SetNodeState(ENodeState::Active);
    
    // 更新子节点状态
    UpdateChildrenStates();
    
    // 传播情绪
    PropagateEmotionToChildren();
    
    UE_LOG(LogTemp, Log, TEXT("Scene %s activated"), *NodeData.NodeName);
}

void ASceneNode::DeactivateScene()
{
    if (!bIsActiveScene)
    {
        return;
    }

    bIsActiveScene = false;
    
    // 停用所有子节点
    for (AInteractiveNode* Node : ChildNodes)
    {
        if (Node && Node->GetNodeState() == ENodeState::Active)
        {
            Node->SetNodeState(ENodeState::Inactive);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Scene %s deactivated"), *NodeData.NodeName);
}

void ASceneNode::SetSceneEmotion(const FEmotionData& Emotion)
{
    SceneEmotion = Emotion;
    
    if (bIsActiveScene)
    {
        PropagateEmotionToChildren();
    }
}

void ASceneNode::SpawnNodesFromData(const TArray<FNodeGenerateData>& SpawnDataArray)
{
    for (const FNodeGenerateData& SpawnData : SpawnDataArray)
    {
        AInteractiveNode* NewNode = SpawnNodeFromData(SpawnData);
        if (NewNode)
        {
            AddChildNode(NewNode);
        }
    }
}

void ASceneNode::QueueNodeSpawn(const FNodeGenerateData& SpawnData)
{
    PendingNodeSpawns.Add(SpawnData);
}

void ASceneNode::ProcessPendingSpawns()
{
    if (PendingNodeSpawns.Num() == 0)
    {
        return;
    }

    TArray<FNodeGenerateData> SpawnsToProcess = PendingNodeSpawns;
    PendingNodeSpawns.Empty();
    
    SpawnNodesFromData(SpawnsToProcess);
}

void ASceneNode::OnInteract_Implementation(const FInteractionData& Data)
{
    if (!CanInteract(Data))
    {
        return;
    }

    Super::OnInteract_Implementation(Data);
    
    // 场景交互通常意味着进入场景
    if (!bIsActiveScene)
    {
        ActivateScene();
    }
}

void ASceneNode::OnStateChanged_Implementation(ENodeState OldState, ENodeState NewState)
{
    Super::OnStateChanged_Implementation(OldState, NewState);
    
    // 根据场景状态更新
    switch (NewState)
    {
    case ENodeState::Active:
        if (!bIsActiveScene)
        {
            ActivateScene();
        }
        break;
    case ENodeState::Inactive:
    case ENodeState::Locked:
        if (bIsActiveScene)
        {
            DeactivateScene();
        }
        break;
    }
}

bool ASceneNode::ShouldTriggerStory_Implementation() const
{
    // 场景可能基于章节ID触发故事
    return Super::ShouldTriggerStory_Implementation() || !SceneStoryChapterID.IsEmpty();
}

TArray<FNodeGenerateData> ASceneNode::GetNodeSpawnData_Implementation() const
{
    // 返回待生成的节点数据
    return PendingNodeSpawns;
}

void ASceneNode::UpdateVisuals_Implementation()
{
    Super::UpdateVisuals_Implementation();
    
    // 场景可能有特殊的视觉效果
    if (NodeMesh && bIsActiveScene)
    {
        // 可以添加场景特有的视觉效果
    }
}

void ASceneNode::UpdateChildrenStates_Implementation()
{
    if (!bIsActiveScene)
    {
        return;
    }

    // 根据场景状态更新子节点
    for (AInteractiveNode* Node : ChildNodes)
    {
        if (!Node)
        {
            continue;
        }

        // 如果子节点是非激活状态，激活它
        if (Node->GetNodeState() == ENodeState::Inactive)
        {
            Node->SetNodeState(ENodeState::Active);
        }
    }
}

void ASceneNode::PropagateEmotionToChildren_Implementation()
{
    // 将场景情绪传播给子节点
    // 这里可以通过事件系统或直接调用子节点方法
    for (AInteractiveNode* Node : ChildNodes)
    {
        if (Node)
        {
            // 子节点可以根据场景情绪调整自己的表现
            Node->StoryContext.Add("SceneEmotion", UEnum::GetValueAsString(SceneEmotion.PrimaryEmotion));
            Node->StoryContext.Add("EmotionIntensity", FString::SanitizeFloat(SceneEmotion.Intensity));
        }
    }
}

//临时方法，动态设置子节点位置，后续可以删除
void ASceneNode::ArrangeChildNodes_Implementation()
{
    // 在场景中排列子节点
    if (ChildNodes.Num() == 0)
    {
        return;
    }

    // 简单的圆形排列
    float AngleStep = 360.0f / ChildNodes.Num();
    float CurrentAngle = 0.0f;
    float ArrangeRadius = FMath::Min(SceneRadius * 0.7f, 1000.0f);
    
    FVector SceneCenter = GetActorLocation();
    
    for (int32 i = 0; i < ChildNodes.Num(); i++)
    {
        if (ChildNodes[i])
        {
            float Radians = FMath::DegreesToRadians(CurrentAngle);
            FVector Offset(
                FMath::Cos(Radians) * ArrangeRadius,
                FMath::Sin(Radians) * ArrangeRadius,
                0.0f
            );
            
            FVector NewLocation = SceneCenter + Offset;
            ChildNodes[i]->SetActorLocation(NewLocation);
            
            CurrentAngle += AngleStep;
        }
    }
}

void ASceneNode::OnChildNodeStateChanged(AInteractiveNode* ChildNode, ENodeState OldState, ENodeState NewState)
{
    if (!ChildNode)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Scene %s: Child %s state changed from %d to %d"),
        *NodeData.NodeName, *ChildNode->GetNodeName(), (int32)OldState, (int32)NewState);
    
    // 可以在这里处理子节点状态变化的逻辑
    // 例如：所有子节点完成时，场景也完成
    if (NewState == ENodeState::Completed)
    {
        bool bAllCompleted = true;
        for (AInteractiveNode* Node : ChildNodes)
        {
            if (Node && Node->GetNodeState() != ENodeState::Completed)
            {
                bAllCompleted = false;
                break;
            }
        }
        
        if (bAllCompleted && CurrentState != ENodeState::Completed)
        {
            SetNodeState(ENodeState::Completed);
        }
    }
}

void ASceneNode::OnChildNodeInteracted(AInteractiveNode* ChildNode, const FInteractionData& InteractionData)
{
    if (!ChildNode)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Scene %s: Child %s was interacted"),
        *NodeData.NodeName, *ChildNode->GetNodeName());
    
    // 可以在这里处理子节点交互的逻辑
}

void ASceneNode::OnChildNodeStoryTriggered(AInteractiveNode* ChildNode, const TArray<FString>& EventIDs)
{
    if (!ChildNode)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Scene %s: Child %s triggered story events"),
        *NodeData.NodeName, *ChildNode->GetNodeName());
    
    // 可以在这里处理子节点故事触发的逻辑
    // 例如：添加到场景的事件队列
    for (const FString& EventID : EventIDs)
    {
        AddTriggerEvent(EventID);
    }
}

void ASceneNode::RegisterChildNode(AInteractiveNode* Node)
{
    if (!Node)
    {
        return;
    }

    // 添加到映射
    ChildNodeMap.Add(Node->GetNodeID(), Node);
    
    // 订阅子节点事件
    Node->OnNodeStateChanged.AddDynamic(this, &ASceneNode::OnChildNodeStateChanged);
    Node->OnNodeInteracted.AddDynamic(this, &ASceneNode::OnChildNodeInteracted);
    Node->OnNodeStoryTriggered.AddDynamic(this, &ASceneNode::OnChildNodeStoryTriggered);
}

void ASceneNode::UnregisterChildNode(AInteractiveNode* Node)
{
    if (!Node)
    {
        return;
    }

    // 从映射中移除
    ChildNodeMap.Remove(Node->GetNodeID());
    
    // 取消订阅事件
    Node->OnNodeStateChanged.RemoveDynamic(this, &ASceneNode::OnChildNodeStateChanged);
    Node->OnNodeInteracted.RemoveDynamic(this, &ASceneNode::OnChildNodeInteracted);
    Node->OnNodeStoryTriggered.RemoveDynamic(this, &ASceneNode::OnChildNodeStoryTriggered);
}

AInteractiveNode* ASceneNode::SpawnNodeFromData(const FNodeGenerateData& SpawnData)
{
    if (!SpawnData.NodeClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Scene %s: Cannot spawn node without class"), *NodeData.NodeName);
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 生成节点
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AInteractiveNode* NewNode = World->SpawnActor<AInteractiveNode>(
        SpawnData.NodeClass,
        SpawnData.SpawnTransform,
        SpawnParams
    );
    
    if (NewNode)
    {
        // 初始化节点
        NewNode->Initialize(SpawnData.NodeData);
        
        // 设置情绪上下文
        if (SpawnData.EmotionContext.Intensity > 0.0f)
        {
            NewNode->StoryContext.Add("SpawnEmotion", UEnum::GetValueAsString(SpawnData.EmotionContext.PrimaryEmotion));
            NewNode->StoryContext.Add("SpawnEmotionIntensity", FString::SanitizeFloat(SpawnData.EmotionContext.Intensity));
        }
        
        UE_LOG(LogTemp, Log, TEXT("Scene %s spawned node %s"), *NodeData.NodeName, *NewNode->GetNodeName());
    }
    
    return NewNode;
}

