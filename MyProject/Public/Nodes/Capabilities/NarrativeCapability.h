// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Core/NodeDataTypes.h"
#include "NarrativeCapability.generated.h"

// 前向声明
class AInteractiveNode;
class ANodeConnection;
class ANodeSystemManager;

/**
 * 叙事能力类
 * 管理故事进展和叙事元素，包括故事推进、事件触发、文本生成、线索提供等
 * 通过Sequence关系推进故事，通过Trigger关系触发事件
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API UNarrativeCapability : public UItemCapability
{
    GENERATED_BODY()

public:
    UNarrativeCapability();

    // ========== 故事管理 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Story")
    FString CurrentStoryBeat;                       // 当前故事节拍

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Story")
    TArray<FString> StoryProgressionPath;           // 故事进展路径

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Story")
    TMap<FString, FString> StoryFragments;          // 故事片段映射（ID->内容）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Story")
    int32 CurrentStoryIndex;                        // 当前故事索引

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Story")
    bool bAutoAdvanceStory;                         // 是否自动推进故事

    // ========== 事件系统 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Events")
    TArray<FString> TriggerableEventIDs;            // 可触发的事件ID列表

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Events")
    TMap<FString, FString> EventDescriptions;       // 事件描述

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Events")
    TArray<FString> TriggeredEvents;                // 已触发的事件列表

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Events")
    float EventTriggerDelay;                        // 事件触发延迟

    // ========== 文本生成 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Text")
    TMap<FString, FString> TextTemplates;           // 文本模板（键->模板）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Text")
    TArray<FString> ContextualKeywords;             // 上下文关键词

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Text")
    FString TextGenerationPrefix;                   // 文本生成前缀

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Text")
    FString TextGenerationSuffix;                   // 文本生成后缀

    // ========== 线索系统 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Clues")
    TMap<FString, FString> AvailableClues;          // 可用线索（ID->内容）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Clues")
    TArray<FString> ProvidedClues;                  // 已提供的线索ID

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Clues")
    int32 MaxCluesPerInteraction;                   // 每次交互最多提供的线索数

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Clues")
    bool bRandomizeClueOrder;                       // 是否随机化线索顺序

    // ========== 组合验证 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Combination")
    TMap<FString, FString> RequiredCombinations;    // 需要的元素组合（组合ID->元素列表，用逗号分隔）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Combination")
    TMap<FString, bool> CombinationStatus;          // 组合验证状态

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Combination")
    bool bOrderMatters;                             // 组合顺序是否重要

    // ========== 记忆追踪 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Memory", meta = (ClampMin = "1"))
    int32 MaxMemoryCount;                           // 最大记忆数量

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Memory")
    TArray<FString> TrackedMemories;                // 追踪的记忆事件

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Memory")
    TMap<FString, int32> MemoryImportance;          // 记忆重要性（用于优先级）

    // ========== 配置数据 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config")
    TMap<FString, FString> NarrativeConfig;         // 动态配置参数

public:
    // ========== 重写父类方法 ==========
    virtual void Initialize_Implementation(AItemNode* Owner) override;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const override;
    virtual bool Use_Implementation(const FInteractionData& Data) override;
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState) override;

    // ========== 核心方法 ==========
    
    // 故事管理
    UFUNCTION(BlueprintCallable, Category = "Narrative|Story")
    void AdvanceStory(const FString& NextBeat);    // 推进故事（通过Sequence关系）

    UFUNCTION(BlueprintCallable, Category = "Narrative|Story")
    void JumpToStoryBeat(const FString& BeatID);   // 跳转到特定故事节拍

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Story")
    FString GetCurrentStoryFragment() const;        // 获取当前故事片段

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Story")
    float GetStoryProgress() const;                 // 获取故事进度（0-1）

    UFUNCTION(BlueprintCallable, Category = "Narrative|Story")
    void AddStoryFragment(const FString& BeatID, const FString& Fragment);

    // 事件系统
    UFUNCTION(BlueprintCallable, Category = "Narrative|Events")
    bool TriggerEvent(const FString& EventID);     // 触发事件（通过Trigger关系）

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events")
    void QueueEvent(const FString& EventID, float Delay); // 延迟触发事件

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Events")
    bool HasEventBeenTriggered(const FString& EventID) const; // 检查事件是否已触发

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events")
    void ResetEvents();                             // 重置所有事件

    // 文本生成
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Text")
    FString GenerateContextualText(const TMap<FString, FString>& Context); // 生成文本

    UFUNCTION(BlueprintCallable, Category = "Narrative|Text")
    void AddTextTemplate(const FString& TemplateID, const FString& Template);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Text")
    FString ProcessTemplate(const FString& TemplateID, const TMap<FString, FString>& Variables) const;

    // 线索系统
    UFUNCTION(BlueprintCallable, Category = "Narrative|Clues")
    bool ProvideClue(const FString& ClueID);       // 提供线索

    UFUNCTION(BlueprintCallable, Category = "Narrative|Clues")
    TArray<FString> GetNextClues(int32 Count = 1); // 获取下一批线索

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Clues")
    bool HasClueBeenProvided(const FString& ClueID) const; // 检查线索是否已提供

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Clues")
    int32 GetRemainingClueCount() const;            // 获取剩余线索数量

    // 组合验证
    UFUNCTION(BlueprintCallable, Category = "Narrative|Combination")
    bool ValidateCombination(const FString& ComboID, const TArray<FString>& Elements); // 验证组合

    UFUNCTION(BlueprintCallable, Category = "Narrative|Combination")
    void RegisterCombination(const FString& ComboID, const TArray<FString>& RequiredElements);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Combination")
    bool IsCombinationComplete(const FString& ComboID) const; // 检查组合是否完成

    // 记忆系统
    UFUNCTION(BlueprintCallable, Category = "Narrative|Memory")
    void RecordMemory(const FString& MemoryEvent); // 记录记忆

    UFUNCTION(BlueprintCallable, Category = "Narrative|Memory")
    void RecordImportantMemory(const FString& MemoryEvent, int32 Importance); // 记录重要记忆

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Narrative|Memory")
    TArray<FString> GetRelevantMemories(const FString& Context) const; // 获取相关记忆

    UFUNCTION(BlueprintCallable, Category = "Narrative|Memory")
    void ForgetOldestMemory();                      // 遗忘最旧的记忆

    // ========== 配置方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Narrative|Config")
    void LoadNarrativeConfig(const TMap<FString, FString>& Config);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Config")
    void ApplyConfigValue(const FString& Key, const FString& Value);

protected:
    // 内部辅助方法
    ANodeSystemManager* GetNodeSystemManager() const;
    ANodeConnection* CreateSequenceConnection(AInteractiveNode* NextNode);
    ANodeConnection* CreateEventTriggerConnection(const FString& EventNodeID);
    void ProcessStoryAdvancement();
    FString ReplaceTemplateVariables(const FString& Template, const TMap<FString, FString>& Variables) const;
    bool MatchesCombination(const TArray<FString>& Required, const TArray<FString>& Provided) const;
    void CleanupOldMemories();

    UFUNCTION()
    void OnEventTriggerDelay();

private:
    UPROPERTY()
    ANodeSystemManager* CachedSystemManager;

    // 内部状态
    TQueue<TPair<FString, float>> EventQueue;
    FTimerHandle EventDelayTimerHandle;
    TArray<FString> ShuffledClues;
    int32 CurrentClueIndex;
};