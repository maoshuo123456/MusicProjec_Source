// Fill out your copyright notice in the Description page of Project Settings.


// NarrativeCapability.cpp
#include "Nodes/Capabilities/NarrativeCapability.h"
#include "Nodes/ItemNode.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/NodeConnection.h"
#include "Nodes/NodeSystemManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/RandomShuffle.h"

UNarrativeCapability::UNarrativeCapability()
{
    // 设置默认值
    CapabilityID = TEXT("NarrativeCapability");
    CapabilityDescription = FText::FromString(TEXT("管理故事进展和叙事元素"));
    UsagePrompt = TEXT("推进故事");
    
    // 故事管理默认值
    CurrentStoryBeat = TEXT("Start");
    CurrentStoryIndex = 0;
    bAutoAdvanceStory = false;
    
    // 事件系统默认值
    EventTriggerDelay = 0.0f;
    
    // 文本生成默认值
    TextGenerationPrefix = TEXT("");
    TextGenerationSuffix = TEXT("");
    
    // 线索系统默认值
    MaxCluesPerInteraction = 1;
    bRandomizeClueOrder = false;
    
    // 组合验证默认值
    bOrderMatters = false;
    
    // 记忆系统默认值
    MaxMemoryCount = 10;
    
    CachedSystemManager = nullptr;
    CurrentClueIndex = 0;
}

void UNarrativeCapability::Initialize_Implementation(AItemNode* Owner)
{
    Super::Initialize_Implementation(Owner);
    
    // 缓存系统管理器
    CachedSystemManager = GetNodeSystemManager();
    
    // 初始化线索顺序
    if (bRandomizeClueOrder && AvailableClues.Num() > 0)
    {
        AvailableClues.GetKeys(ShuffledClues);
        Algo::RandomShuffle(ShuffledClues);
    }
    
    // 从配置加载数据
    if (NarrativeConfig.Num() > 0)
    {
        LoadNarrativeConfig(NarrativeConfig);
    }
    
    UE_LOG(LogTemp, Log, TEXT("NarrativeCapability initialized for %s"), 
        Owner ? *Owner->GetNodeName() : TEXT("Unknown"));
}

bool UNarrativeCapability::CanUse_Implementation(const FInteractionData& Data) const
{
    if (!Super::CanUse_Implementation(Data))
    {
        return false;
    }
    
    // 检查是否有可用的叙事功能
    return StoryProgressionPath.Num() > 0 || 
           TriggerableEventIDs.Num() > 0 || 
           (AvailableClues.Num() > ProvidedClues.Num());
}

bool UNarrativeCapability::Use_Implementation(const FInteractionData& Data)
{
    if (!Super::Use_Implementation(Data))
    {
        return false;
    }
    
    bool bSuccess = false;
    
    // 优先推进故事
    if (StoryProgressionPath.Num() > 0 && CurrentStoryIndex < StoryProgressionPath.Num() - 1)
    {
        AdvanceStory(StoryProgressionPath[CurrentStoryIndex + 1]);
        bSuccess = true;
    }
    // 其次提供线索
    else if (AvailableClues.Num() > ProvidedClues.Num())
    {
        TArray<FString> NextClues = GetNextClues(MaxCluesPerInteraction);
        for (const FString& ClueID : NextClues)
        {
            ProvideClue(ClueID);
        }
        bSuccess = NextClues.Num() > 0;
    }
    // 最后触发事件
    else if (TriggerableEventIDs.Num() > 0)
    {
        for (const FString& EventID : TriggerableEventIDs)
        {
            if (!HasEventBeenTriggered(EventID))
            {
                TriggerEvent(EventID);
                bSuccess = true;
                break;
            }
        }
    }
    
    return bSuccess;
}

void UNarrativeCapability::OnOwnerStateChanged_Implementation(ENodeState NewState)
{
    Super::OnOwnerStateChanged_Implementation(NewState);
    
    // 某些状态可能触发故事进展
    if (NewState == ENodeState::Completed && bAutoAdvanceStory)
    {
        ProcessStoryAdvancement();
    }
}

void UNarrativeCapability::AdvanceStory(const FString& NextBeat)
{
    if (NextBeat.IsEmpty() || !OwnerItem)
    {
        return;
    }
    
    // 更新当前故事节拍
    FString PreviousBeat = CurrentStoryBeat;
    CurrentStoryBeat = NextBeat;
    
    // 更新故事索引
    int32 NewIndex = StoryProgressionPath.IndexOfByKey(NextBeat);
    if (NewIndex != INDEX_NONE)
    {
        CurrentStoryIndex = NewIndex;
    }
    
    // 记录到记忆系统
    RecordMemory(FString::Printf(TEXT("Story: %s -> %s"), *PreviousBeat, *NextBeat));
    
    // 获取下一个节点
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (SystemManager)
    {
        // 查找具有对应故事片段的节点
        TArray<AInteractiveNode*> AllNodes = SystemManager->GetNodesByType(ENodeType::Story);
        for (AInteractiveNode* Node : AllNodes)
        {
            if (Node && Node->StoryFragmentID == NextBeat)
            {
                // 创建Sequence关系
                CreateSequenceConnection(Node);
                
                // 激活下一个节点
                Node->SetNodeState(ENodeState::Active);
                break;
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("NarrativeCapability: Advanced story from %s to %s"), 
        *PreviousBeat, *NextBeat);
}

void UNarrativeCapability::JumpToStoryBeat(const FString& BeatID)
{
    if (StoryProgressionPath.Contains(BeatID))
    {
        AdvanceStory(BeatID);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("NarrativeCapability: Story beat %s not found in progression path"), *BeatID);
    }
}

FString UNarrativeCapability::GetCurrentStoryFragment() const
{
    if (StoryFragments.Contains(CurrentStoryBeat))
    {
        return StoryFragments[CurrentStoryBeat];
    }
    return TEXT("");
}

float UNarrativeCapability::GetStoryProgress() const
{
    if (StoryProgressionPath.Num() <= 1)
    {
        return 1.0f;
    }
    
    return (float)CurrentStoryIndex / (float)(StoryProgressionPath.Num() - 1);
}

void UNarrativeCapability::AddStoryFragment(const FString& BeatID, const FString& Fragment)
{
    StoryFragments.Add(BeatID, Fragment);
    
    // 如果不在进展路径中，添加到路径
    if (!StoryProgressionPath.Contains(BeatID))
    {
        StoryProgressionPath.Add(BeatID);
    }
}

bool UNarrativeCapability::TriggerEvent(const FString& EventID)
{
    if (!TriggerableEventIDs.Contains(EventID) || HasEventBeenTriggered(EventID))
    {
        return false;
    }
    
    // 如果有延迟，加入队列
    if (EventTriggerDelay > 0.0f)
    {
        QueueEvent(EventID, EventTriggerDelay);
        return true;
    }
    
    // 立即触发
    TriggeredEvents.Add(EventID);
    
    // 通知拥有者节点
    if (OwnerItem)
    {
        OwnerItem->AddTriggerEvent(EventID);
    }
    
    // 创建事件触发连接
    CreateEventTriggerConnection(EventID);
    
    // 记录到记忆
    RecordMemory(FString::Printf(TEXT("Event: %s"), *EventID));
    
    UE_LOG(LogTemp, Log, TEXT("NarrativeCapability: Triggered event %s"), *EventID);
    return true;
}

void UNarrativeCapability::QueueEvent(const FString& EventID, float Delay)
{
    EventQueue.Enqueue(TPair<FString, float>(EventID, Delay));
    
    // 如果没有正在运行的定时器，启动一个
    if (!EventDelayTimerHandle.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            EventDelayTimerHandle,
            this,
            &UNarrativeCapability::OnEventTriggerDelay,
            0.1f,
            true
        );
    }
}

bool UNarrativeCapability::HasEventBeenTriggered(const FString& EventID) const
{
    return TriggeredEvents.Contains(EventID);
}

void UNarrativeCapability::ResetEvents()
{
    TriggeredEvents.Empty();
    EventQueue.Empty();
    
    if (EventDelayTimerHandle.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EventDelayTimerHandle);
    }
}

FString UNarrativeCapability::GenerateContextualText(const TMap<FString, FString>& Context)
{
    FString GeneratedText = TextGenerationPrefix;
    
    // 查找匹配的模板
    for (const auto& Template : TextTemplates)
    {
        bool bAllKeywordsMatch = true;
        
        // 检查上下文是否包含所需关键词
        for (const FString& Keyword : ContextualKeywords)
        {
            if (!Context.Contains(Keyword))
            {
                bAllKeywordsMatch = false;
                break;
            }
        }
        
        if (bAllKeywordsMatch)
        {
            GeneratedText += ProcessTemplate(Template.Key, Context);
            break;
        }
    }
    
    GeneratedText += TextGenerationSuffix;
    
    return GeneratedText;
}

void UNarrativeCapability::AddTextTemplate(const FString& TemplateID, const FString& Template)
{
    TextTemplates.Add(TemplateID, Template);
}

FString UNarrativeCapability::ProcessTemplate(const FString& TemplateID, const TMap<FString, FString>& Variables) const
{
    if (!TextTemplates.Contains(TemplateID))
    {
        return TEXT("");
    }
    
    FString Template = TextTemplates[TemplateID];
    return ReplaceTemplateVariables(Template, Variables);
}

bool UNarrativeCapability::ProvideClue(const FString& ClueID)
{
    if (!AvailableClues.Contains(ClueID) || HasClueBeenProvided(ClueID))
    {
        return false;
    }
    
    ProvidedClues.Add(ClueID);
    
    // 获取线索内容
    FString ClueContent = AvailableClues[ClueID];
    
    // 记录到记忆
    RecordMemory(FString::Printf(TEXT("Clue: %s"), *ClueID));
    
    // 通知拥有者
    if (OwnerItem)
    {
        OwnerItem->AddTriggerEvent(FString::Printf(TEXT("ClueProvided_%s"), *ClueID));
    }
    
    UE_LOG(LogTemp, Log, TEXT("NarrativeCapability: Provided clue %s: %s"), *ClueID, *ClueContent);
    return true;
}

TArray<FString> UNarrativeCapability::GetNextClues(int32 Count)
{
    TArray<FString> NextClues;
    
    if (bRandomizeClueOrder)
    {
        // 使用随机顺序
        while (NextClues.Num() < Count && CurrentClueIndex < ShuffledClues.Num())
        {
            FString ClueID = ShuffledClues[CurrentClueIndex];
            if (!HasClueBeenProvided(ClueID))
            {
                NextClues.Add(ClueID);
            }
            CurrentClueIndex++;
        }
    }
    else
    {
        // 使用原始顺序
        TArray<FString> AllClueIDs;
        AvailableClues.GetKeys(AllClueIDs);
        
        for (const FString& ClueID : AllClueIDs)
        {
            if (!HasClueBeenProvided(ClueID))
            {
                NextClues.Add(ClueID);
                if (NextClues.Num() >= Count)
                {
                    break;
                }
            }
        }
    }
    
    return NextClues;
}

bool UNarrativeCapability::HasClueBeenProvided(const FString& ClueID) const
{
    return ProvidedClues.Contains(ClueID);
}

int32 UNarrativeCapability::GetRemainingClueCount() const
{
    return FMath::Max(0, AvailableClues.Num() - ProvidedClues.Num());
}

bool UNarrativeCapability::ValidateCombination(const FString& ComboID, const TArray<FString>& Elements)
{
    if (!RequiredCombinations.Contains(ComboID))
    {
        return false;
    }
    
    // 将逗号分隔的字符串转换为数组
    FString RequiredString = RequiredCombinations[ComboID];
    TArray<FString> Required;
    RequiredString.ParseIntoArray(Required, TEXT(","), true);
    
    // 清理空格
    for (FString& Elem : Required)
    {
        Elem = Elem.TrimStartAndEnd();
    }
    
    bool bIsValid = MatchesCombination(Required, Elements);
    
    if (bIsValid)
    {
        CombinationStatus.Add(ComboID, true);
        
        // 记录到记忆
        RecordImportantMemory(FString::Printf(TEXT("Combination: %s completed"), *ComboID), 5);
        
        // 可能触发完成状态
        if (OwnerItem && CombinationStatus.Num() >= RequiredCombinations.Num())
        {
            OwnerItem->SetNodeState(ENodeState::Completed);
        }
    }
    
    return bIsValid;
}

void UNarrativeCapability::RegisterCombination(const FString& ComboID, const TArray<FString>& RequiredElements)
{
    // 将数组转换为逗号分隔的字符串
    FString ElementsString = FString::Join(RequiredElements, TEXT(","));
    RequiredCombinations.Add(ComboID, ElementsString);
    CombinationStatus.Add(ComboID, false);
}

bool UNarrativeCapability::IsCombinationComplete(const FString& ComboID) const
{
    if (CombinationStatus.Contains(ComboID))
    {
        return CombinationStatus[ComboID];
    }
    return false;
}

void UNarrativeCapability::RecordMemory(const FString& MemoryEvent)
{
    RecordImportantMemory(MemoryEvent, 1);
}

void UNarrativeCapability::RecordImportantMemory(const FString& MemoryEvent, int32 Importance)
{
    if (MemoryEvent.IsEmpty())
    {
        return;
    }
    
    // 检查是否需要清理旧记忆
    if (TrackedMemories.Num() >= MaxMemoryCount)
    {
        CleanupOldMemories();
    }
    
    TrackedMemories.Add(MemoryEvent);
    MemoryImportance.Add(MemoryEvent, Importance);
    
    UE_LOG(LogTemp, Verbose, TEXT("NarrativeCapability: Recorded memory - %s (Importance: %d)"), 
        *MemoryEvent, Importance);
}

TArray<FString> UNarrativeCapability::GetRelevantMemories(const FString& Context) const
{
    TArray<FString> RelevantMemories;
    
    // 简单的关键词匹配
    for (const FString& Memory : TrackedMemories)
    {
        if (Memory.Contains(Context))
        {
            RelevantMemories.Add(Memory);
        }
    }
    
    // 按重要性排序
    RelevantMemories.Sort([this](const FString& A, const FString& B)
    {
        int32 ImportanceA = MemoryImportance.Contains(A) ? MemoryImportance[A] : 0;
        int32 ImportanceB = MemoryImportance.Contains(B) ? MemoryImportance[B] : 0;
        return ImportanceA > ImportanceB;
    });
    
    return RelevantMemories;
}

void UNarrativeCapability::ForgetOldestMemory()
{
    if (TrackedMemories.Num() > 0)
    {
        // 找到重要性最低的记忆
        FString LeastImportantMemory;
        int32 LowestImportance = MAX_int32;
        
        for (const FString& Memory : TrackedMemories)
        {
            int32 Importance = MemoryImportance.Contains(Memory) ? MemoryImportance[Memory] : 0;
            if (Importance < LowestImportance)
            {
                LowestImportance = Importance;
                LeastImportantMemory = Memory;
            }
        }
        
        if (!LeastImportantMemory.IsEmpty())
        {
            TrackedMemories.Remove(LeastImportantMemory);
            MemoryImportance.Remove(LeastImportantMemory);
        }
    }
}

void UNarrativeCapability::LoadNarrativeConfig(const TMap<FString, FString>& Config)
{
    for (const auto& Pair : Config)
    {
        ApplyConfigValue(Pair.Key, Pair.Value);
    }
}

void UNarrativeCapability::ApplyConfigValue(const FString& Key, const FString& Value)
{
    if (Key == TEXT("MaxMemoryCount"))
    {
        MaxMemoryCount = FCString::Atoi(*Value);
    }
    else if (Key == TEXT("MaxCluesPerInteraction"))
    {
        MaxCluesPerInteraction = FCString::Atoi(*Value);
    }
    else if (Key == TEXT("EventTriggerDelay"))
    {
        EventTriggerDelay = FCString::Atof(*Value);
    }
    else if (Key == TEXT("AutoAdvanceStory"))
    {
        bAutoAdvanceStory = Value.ToBool();
    }
    else if (Key == TEXT("RandomizeClueOrder"))
    {
        bRandomizeClueOrder = Value.ToBool();
    }
    else if (Key.StartsWith(TEXT("Story_")))
    {
        FString BeatID = Key.RightChop(6); // 移除 "Story_"
        AddStoryFragment(BeatID, Value);
    }
    else if (Key.StartsWith(TEXT("Event_")))
    {
        FString EventID = Key.RightChop(6); // 移除 "Event_"
        TriggerableEventIDs.Add(EventID);
        EventDescriptions.Add(EventID, Value);
    }
    else if (Key.StartsWith(TEXT("Clue_")))
    {
        FString ClueID = Key.RightChop(5); // 移除 "Clue_"
        AvailableClues.Add(ClueID, Value);
    }
    
    // 保存到配置
    NarrativeConfig.Add(Key, Value);
}

ANodeSystemManager* UNarrativeCapability::GetNodeSystemManager() const
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

ANodeConnection* UNarrativeCapability::CreateSequenceConnection(AInteractiveNode* NextNode)
{
    if (!OwnerItem || !NextNode)
    {
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    // 创建Sequence关系
    FNodeRelationData RelationData;
    RelationData.SourceNodeID = OwnerItem->GetNodeID();
    RelationData.TargetNodeID = NextNode->GetNodeID();
    RelationData.RelationType = ENodeRelationType::Sequence;
    RelationData.Weight = 1.0f;
    RelationData.bBidirectional = false;
    
    return SystemManager->CreateConnection(OwnerItem, NextNode, RelationData);
}

ANodeConnection* UNarrativeCapability::CreateEventTriggerConnection(const FString& EventNodeID)
{
    if (!OwnerItem || EventNodeID.IsEmpty())
    {
        return nullptr;
    }
    
    ANodeSystemManager* SystemManager = GetNodeSystemManager();
    if (!SystemManager)
    {
        return nullptr;
    }
    
    // 查找具有对应事件ID的节点
    TArray<AInteractiveNode*> AllNodes = SystemManager->GetNodesByType(ENodeType::Trigger);
    for (AInteractiveNode* Node : AllNodes)
    {
        if (Node && Node->TriggerEventIDs.Contains(EventNodeID))
        {
            // 创建Trigger关系
            FNodeRelationData RelationData;
            RelationData.SourceNodeID = OwnerItem->GetNodeID();
            RelationData.TargetNodeID = Node->GetNodeID();
            RelationData.RelationType = ENodeRelationType::Trigger;
            RelationData.Weight = 1.0f;
            RelationData.bBidirectional = false;
            
            return SystemManager->CreateConnection(OwnerItem, Node, RelationData);
        }
    }
    
    return nullptr;
}

void UNarrativeCapability::ProcessStoryAdvancement()
{
    if (CurrentStoryIndex < StoryProgressionPath.Num() - 1)
    {
        AdvanceStory(StoryProgressionPath[CurrentStoryIndex + 1]);
    }
}

FString UNarrativeCapability::ReplaceTemplateVariables(const FString& Template, const TMap<FString, FString>& Variables) const
{
    FString Result = Template;
    
    // 替换所有变量
    for (const auto& Var : Variables)
    {
        FString Placeholder = FString::Printf(TEXT("{%s}"), *Var.Key);
        Result = Result.Replace(*Placeholder, *Var.Value);
    }
    
    return Result;
}

bool UNarrativeCapability::MatchesCombination(const TArray<FString>& Required, const TArray<FString>& Provided) const
{
    if (Required.Num() != Provided.Num())
    {
        return false;
    }
    
    if (bOrderMatters)
    {
        // 顺序重要，直接比较
        for (int32 i = 0; i < Required.Num(); i++)
        {
            if (Required[i] != Provided[i])
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        // 顺序不重要，检查所有元素是否存在
        for (const FString& ReqElement : Required)
        {
            if (!Provided.Contains(ReqElement))
            {
                return false;
            }
        }
        return true;
    }
}

void UNarrativeCapability::CleanupOldMemories()
{
    while (TrackedMemories.Num() >= MaxMemoryCount)
    {
        ForgetOldestMemory();
    }
}

void UNarrativeCapability::OnEventTriggerDelay()
{
    TArray<TPair<FString, float>> EventsToProcess;
    
    // 获取所有到期的事件
    TPair<FString, float> Event;
    while (EventQueue.Peek(Event))
    {
        Event.Value -= 0.1f; // 减少延迟时间
        
        if (Event.Value <= 0.0f)
        {
            EventQueue.Dequeue(Event);
            EventsToProcess.Add(Event);
        }
        else
        {
            // 更新延迟时间
            EventQueue.Dequeue(Event);
            EventQueue.Enqueue(Event);
            break;
        }
    }
    
    // 处理到期的事件
    for (const auto& EventPair : EventsToProcess)
    {
        TriggerEvent(EventPair.Key);
    }
    
    // 如果队列为空，停止定时器
    if (EventQueue.IsEmpty() && EventDelayTimerHandle.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(EventDelayTimerHandle);
    }
}