// Fill out your copyright notice in the Description page of Project Settings.

// InteractiveCapability.cpp
#include "Nodes/Capabilities/InteractiveCapability.h"
#include "Nodes/ItemNode.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/NodeSystemManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UInteractiveCapability::UInteractiveCapability()
{
    // 设置默认值
    CapabilityID = TEXT("InteractiveCapability");
    CapabilityDescription = FText::FromString(TEXT("处理玩家与节点的直接交互"));
    UsagePrompt = TEXT("交互");
    
    // 观察功能默认值
    ObservationDistance = 1000.0f;
    bDistanceAffectsDetail = true;
    
    // 对话功能默认值
    CurrentDialogueState = TEXT("Initial");
    bEmotionAffectsDialogue = false;
    
    // 物品交换默认值
    ItemNodeClass = AItemNode::StaticClass();
    
    // 答案验证默认值
    MaxAttempts = 3;
    bCaseSensitive = false;
    
    // 比对功能默认值
    ComparisonThreshold = 0.8f;
    
    CachedSystemManager = nullptr;
}

void UInteractiveCapability::Initialize_Implementation(AItemNode* Owner)
{
    Super::Initialize_Implementation(Owner);
    
    // 缓存系统管理器
    CachedSystemManager = GetNodeSystemManager();
    
    // 从配置加载数据
    if (InteractionConfig.Num() > 0)
    {
        LoadInteractionConfig(InteractionConfig);
    }
    
    UE_LOG(LogTemp, Log, TEXT("InteractiveCapability initialized for %s"), 
        Owner ? *Owner->GetNodeName() : TEXT("Unknown"));
}

bool UInteractiveCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanUse_Implementation(Data))
    {
        return false;
    }
    
    // 根据交互类型检查
    switch (Data.InteractionType)
    {
    case EInteractionType::Click:
        // 点击可以触发对话或观察
        return DialogueOptions.Num() > 0 || ObservableInfo.Num() > 0;
        
    case EInteractionType::Drag:
        // 拖拽可能用于物品交换
        return GivableItems.Num() > 0 || AcceptableItems.Num() > 0;
        
    default:
        return true;
    }
}

bool UInteractiveCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!Super::Use_Implementation(Data))
    {
        return false;
    }
    
    bool bSuccess = false;
    
    // 根据交互类型执行不同功能
    switch (Data.InteractionType)
    {
    case EInteractionType::Click:
        // 如果有对话选项，优先处理对话
        if (DialogueOptions.Num() > 0)
        {
            // 处理第一个可用的对话选项
            TArray<FString> Options = GetAvailableDialogueOptions();
            if (Options.Num() > 0)
            {
                ProcessDialogue(Options[0]);
                bSuccess = true;
            }
        }
        // 否则显示观察信息
        else if (ObservableInfo.Num() > 0 && Data.Instigator)
        {
            float Distance = FVector::Dist(
                OwnerItem->GetActorLocation(),
                Data.InteractionLocation
            );
            FString ObservationText = GetObservationText(Distance);
            UE_LOG(LogTemp, Log, TEXT("Observation: %s"), *ObservationText);
            bSuccess = true;
        }
        break;
        
    case EInteractionType::Drag:
        // 处理物品交换
        if (GivableItems.Num() > 0)
        {
            TArray<FString> Items;
            GivableItems.GetKeys(Items);
            if (Items.Num() > 0)
            {
                bSuccess = GiveItem(Items[0]);
            }
        }
        break;
        
    default:
        break;
    }
    
    return bSuccess;
}

void UInteractiveCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    Super::OnOwnerStateChanged_Implementation(NewState);
    
    // 某些状态下可能改变交互选项
    if (NewState == ENodeState::Completed)
    {
        // 完成状态可能解锁新的对话选项
        if (InteractionConfig.Contains(TEXT("CompletedDialogue")))
        {
            FString CompletedDialogue = InteractionConfig[TEXT("CompletedDialogue")];
            AddDialogueOption(TEXT("Completed"), CompletedDialogue, TEXT("感谢您的帮助！"));
        }
    }
}

FString UInteractiveCapability::GetObservationText(float Distance) const
{
    if (ObservableInfo.Num() == 0)
    {
        return TEXT("");
    }
    
    // 收集所有观察信息
    FString FullText;
    for (const auto& Info : ObservableInfo)
    {
        if (!FullText.IsEmpty())
        {
            FullText += TEXT("\n");
        }
        FullText += FString::Printf(TEXT("%s: %s"), *Info.Key, *Info.Value);
    }
    
    // 根据距离调整细节
    if (bDistanceAffectsDetail)
    {
        return GetDetailLevelText(FullText, Distance);
    }
    
    return FullText;
}

TArray<FString> UInteractiveCapability::GetAvailableObservationKeys() const
{
    TArray<FString> Keys;
    ObservableInfo.GetKeys(Keys);
    return Keys;
}

void UInteractiveCapability::AddObservableInfo(const FString& Key, const FString& Info)
{
    ObservableInfo.Add(Key, Info);
}

FString UInteractiveCapability::ProcessDialogue(const FString& OptionID)
{
    if (!DialogueResponses.Contains(OptionID))
    {
        return TEXT("...");
    }
    
    FString Response = DialogueResponses[OptionID];
    
    // 记录对话历史
    DialogueHistory.Add(FString::Printf(TEXT("Option: %s"), *OptionID));
    DialogueHistory.Add(FString::Printf(TEXT("Response: %s"), *Response));
    
    // 更新对话状态
    CurrentDialogueState = OptionID;
    
    // 记录对话事件
    RecordDialogueEvent(TEXT("DialogueProcessed"), OptionID);
    
    UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Processed dialogue option %s"), *OptionID);
    
    return Response;
}

TArray<FString> UInteractiveCapability::GetAvailableDialogueOptions() const
{
    TArray<FString> Options;
    
    // 根据当前状态筛选可用选项
    for (const auto& Option : DialogueOptions)
    {
        // 可以添加条件检查
        Options.Add(Option.Key);
    }
    
    return Options;
}

void UInteractiveCapability::AddDialogueOption(const FString& OptionID, const FString& Text, const FString& Response)
{
    DialogueOptions.Add(OptionID, Text);
    DialogueResponses.Add(OptionID, Response);
}

void UInteractiveCapability::ClearDialogueHistory()
{
    DialogueHistory.Empty();
    CurrentDialogueState = TEXT("Initial");
}

bool UInteractiveCapability::GiveItem(const FString& ItemID)
{
    if (!GivableItems.Contains(ItemID) || !OwnerItem)
    {
        return false;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return false;
    }
    
    // 创建物品节点
    FNodeGenerateData ItemData;
    ItemData.NodeData.NodeID = FString::Printf(TEXT("Item_%s_%d"), *ItemID, FDateTime::Now().GetTicks());
    ItemData.NodeData.NodeName = GivableItems[ItemID];
    ItemData.NodeData.NodeType = ENodeType::Item;
    ItemData.NodeData.InitialState = ENodeState::Active;
    ItemData.NodeClass = ItemNodeClass;
    ItemData.SpawnTransform.SetLocation(OwnerItem->GetActorLocation() + FVector(100, 0, 0));
    
    AInteractiveNode* ItemNode = SystemManager->CreateNode(ItemNodeClass, ItemData);
    if (ItemNode)
    {
        // 创建Trigger关系
        ANodeConnection* Connection = CreateTriggerConnection(ItemNode);
        if (Connection)
        {
            ItemConnectionMap.Add(ItemID, Connection);
            
            // 从可给予列表中移除
            GivableItems.Remove(ItemID);
            
            UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Successfully gave item %s"), *ItemID);
            return true;
        }
    }
    
    return false;
}

bool UInteractiveCapability::CanReceiveItem(const FString& ItemID) const
{
    return AcceptableItems.Contains(ItemID);
}

bool UInteractiveCapability::ReceiveItem(const FString& ItemID)
{
    if (!CanReceiveItem(ItemID))
    {
        return false;
    }
    
    // 记录接收的物品
    ReceivedItems.Add(ItemID);
    
    // 可能触发状态变化
    if (ReceivedItems.Num() >= AcceptableItems.Num() && OwnerItem)
    {
        // 接收所有需要的物品后，可能完成任务
        OwnerItem->SetNodeState(ENodeState::Completed);
    }
    
    UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Received item %s"), *ItemID);
    return true;
}

TArray<FString> UInteractiveCapability::GetGivableItems() const
{
    TArray<FString> Items;
    GivableItems.GetKeys(Items);
    return Items;
}

bool UInteractiveCapability::ValidateAnswer(const FString& QuestionID, const FString& Answer)
{
    if (!CorrectAnswers.Contains(QuestionID))
    {
        return false;
    }
    
    // 检查尝试次数
    int32 CurrentAttempts = AttemptCounts.Contains(QuestionID) ? AttemptCounts[QuestionID] : 0;
    if (CurrentAttempts >= MaxAttempts)
    {
        UE_LOG(LogTemp, Warning, TEXT("InteractiveCapability: Max attempts reached for question %s"), *QuestionID);
        return false;
    }
    
    // 更新尝试次数
    AttemptCounts.Add(QuestionID, CurrentAttempts + 1);
    
    // 验证答案
    FString CorrectAnswer = CorrectAnswers[QuestionID];
    bool bIsCorrect = false;
    
    if (bCaseSensitive)
    {
        bIsCorrect = Answer.Equals(CorrectAnswer);
    }
    else
    {
        bIsCorrect = Answer.Equals(CorrectAnswer, ESearchCase::IgnoreCase);
    }
    
    if (bIsCorrect)
    {
        UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Correct answer for question %s"), *QuestionID);
        
        // 可能触发状态变化
        if (OwnerItem)
        {
            OwnerItem->SetNodeState(ENodeState::Completed);
        }
    }
    
    return bIsCorrect;
}

int32 UInteractiveCapability::GetRemainingAttempts(const FString& QuestionID) const
{
    int32 CurrentAttempts = AttemptCounts.Contains(QuestionID) ? AttemptCounts[QuestionID] : 0;
    return FMath::Max(0, MaxAttempts - CurrentAttempts);
}

void UInteractiveCapability::ResetAttempts(const FString& QuestionID)
{
    AttemptCounts.Remove(QuestionID);
}

bool UInteractiveCapability::CompareWithNode(AInteractiveNode* OtherNode)
{
    if (!OtherNode || !OwnerItem || ComparisonKeys.Num() == 0)
    {
        return false;
    }
    
    // 计算相似度
    float Similarity = CalculateSimilarity(OtherNode);
    
    if (Similarity >= ComparisonThreshold)
    {
        // 创建Mutual关系
        ANodeConnection* Connection = CreateMutualConnection(OtherNode);
        if (Connection)
        {
            UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Nodes are similar (%.2f%%), created mutual connection"), 
                Similarity * 100.0f);
            return true;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Nodes are not similar enough (%.2f%% < %.2f%%)"), 
        Similarity * 100.0f, ComparisonThreshold * 100.0f);
    return false;
}

float UInteractiveCapability::CalculateSimilarity(AInteractiveNode* OtherNode) const
{
    if (!OtherNode || ComparisonKeys.Num() == 0)
    {
        return 0.0f;
    }
    
    int32 MatchCount = 0;
    
    for (const FString& Key : ComparisonKeys)
    {
        if (CompareNodeProperties(OwnerItem, OtherNode, Key))
        {
            MatchCount++;
        }
    }
    
    return (float)MatchCount / (float)ComparisonKeys.Num();
}

void UInteractiveCapability::UpdateDialogueBranch_Implementation(const FEmotionData& PlayerEmotion)
{
    if (!bEmotionAffectsDialogue)
    {
        return;
    }
    
    // 根据情绪类型调整对话选项
    FString EmotionKey = UEnum::GetValueAsString(PlayerEmotion.PrimaryEmotion);
    
    // 查找情绪相关的对话选项
    if (InteractionConfig.Contains(EmotionKey + TEXT("_Dialogue")))
    {
        FString EmotionDialogue = InteractionConfig[EmotionKey + TEXT("_Dialogue")];
        FString EmotionResponse = InteractionConfig[EmotionKey + TEXT("_Response")];
        
        AddDialogueOption(EmotionKey, EmotionDialogue, EmotionResponse);
    }
    
    UE_LOG(LogTemp, Log, TEXT("InteractiveCapability: Updated dialogue for emotion %s"), *EmotionKey);
}

void UInteractiveCapability::LoadInteractionConfig(const TMap<FString, FString>& Config)
{
    for (const auto& Pair : Config)
    {
        ApplyConfigValue(Pair.Key, Pair.Value);
    }
}

void UInteractiveCapability::ApplyConfigValue(const FString& Key, const FString& Value)
{
    if (Key == TEXT("ObservationDistance"))
    {
        ObservationDistance = FCString::Atof(*Value);
    }
    else if (Key == TEXT("MaxAttempts"))
    {
        MaxAttempts = FCString::Atoi(*Value);
    }
    else if (Key == TEXT("ComparisonThreshold"))
    {
        ComparisonThreshold = FCString::Atof(*Value);
    }
    else if (Key == TEXT("EmotionAffectsDialogue"))
    {
        bEmotionAffectsDialogue = Value.ToBool();
    }
    else if (Key.StartsWith(TEXT("Observable_")))
    {
        FString InfoKey = Key.RightChop(11); // 移除 "Observable_"
        ObservableInfo.Add(InfoKey, Value);
    }
    else if (Key.StartsWith(TEXT("Dialogue_")))
    {
        FString DialogueID = Key.RightChop(9); // 移除 "Dialogue_"
        DialogueOptions.Add(DialogueID, Value);
    }
    else if (Key.StartsWith(TEXT("Response_")))
    {
        FString DialogueID = Key.RightChop(9); // 移除 "Response_"
        DialogueResponses.Add(DialogueID, Value);
    }
    
    // 保存到配置
    InteractionConfig.Add(Key, Value);
}

ANodeSystemManager* UInteractiveCapability::GetNodeSystemManager() const
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

ANodeConnection* UInteractiveCapability::CreateTriggerConnection(AInteractiveNode* TargetNode)
{
    if (!OwnerItem || !TargetNode)
    {
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    // 创建Trigger关系
    FNodeRelationData RelationData;
    RelationData.SourceNodeID = OwnerItem->GetNodeID();
    RelationData.TargetNodeID = TargetNode->GetNodeID();
    RelationData.RelationType = ENodeRelationType::Trigger;
    RelationData.Weight = 1.0f;
    RelationData.bBidirectional = false;
    
    return SystemManager->CreateConnection(OwnerItem, TargetNode, RelationData);
}

ANodeConnection* UInteractiveCapability::CreateMutualConnection(AInteractiveNode* OtherNode)
{
    if (!OwnerItem || !OtherNode)
    {
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    // 创建Mutual关系
    FNodeRelationData RelationData;
    RelationData.SourceNodeID = OwnerItem->GetNodeID();
    RelationData.TargetNodeID = OtherNode->GetNodeID();
    RelationData.RelationType = ENodeRelationType::Mutual;
    RelationData.Weight = 1.0f;
    RelationData.bBidirectional = true; // Mutual关系必须是双向的
    
    return SystemManager->CreateConnection(OwnerItem, OtherNode, RelationData);
}

FString UInteractiveCapability::GetDetailLevelText(const FString& FullText, float Distance) const
{
    if (Distance <= ObservationDistance * 0.3f)
    {
        // 近距离：完整细节
        return FullText;
    }
    else if (Distance <= ObservationDistance * 0.7f)
    {
        // 中距离：部分细节
        return FullText.Left(FullText.Len() * 0.7f) + TEXT("...");
    }
    else if (Distance <= ObservationDistance)
    {
        // 远距离：基本信息
        return FullText.Left(FullText.Len() * 0.3f) + TEXT("...");
    }
    else
    {
        // 超出范围
        return TEXT("太远了，看不清楚");
    }
}

bool UInteractiveCapability::CompareNodeProperties(AInteractiveNode* NodeA, AInteractiveNode* NodeB, const FString& PropertyKey) const
{
    if (!NodeA || !NodeB)
    {
        return false;
    }
    
    // 比较节点数据中的自定义属性
    FNodeData DataA = NodeA->GetNodeData();
    FNodeData DataB = NodeB->GetNodeData();
    
    if (DataA.CustomProperties.Contains(PropertyKey) && DataB.CustomProperties.Contains(PropertyKey))
    {
        return DataA.CustomProperties[PropertyKey].Equals(DataB.CustomProperties[PropertyKey]);
    }
    
    // 比较基本属性
    if (PropertyKey == TEXT("NodeType"))
    {
        return DataA.NodeType == DataB.NodeType;
    }
    else if (PropertyKey == TEXT("State"))
    {
        return NodeA->GetNodeState() == NodeB->GetNodeState();
    }
    
    return false;
}

void UInteractiveCapability::RecordDialogueEvent(const FString& EventType, const FString& EventData)
{
    // 记录对话事件，可以用于故事系统
    if (OwnerItem)
    {
        OwnerItem->AddTriggerEvent(FString::Printf(TEXT("%s_%s"), *EventType, *EventData));
    }
}