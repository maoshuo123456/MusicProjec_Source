// Fill out your copyright notice in the Description page of Project Settings.

// InteractiveCapability.h
#pragma once

#include "CoreMinimal.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Core/NodeDataTypes.h"
#include "InteractiveCapability.generated.h"

// 前向声明
class AInteractiveNode;
class AItemNode;
class ANodeConnection;
class ANodeSystemManager;

/**
 * 交互能力类
 * 处理玩家与节点的直接交互，包括观察、对话、物品交换、答案验证等
 * 通过Trigger关系处理物品交换，通过Mutual关系处理节点比对
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API UInteractiveCapability : public UItemCapability
{
    GENERATED_BODY()

public:
    UInteractiveCapability();

    // ========== 观察功能 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Observe")
    TMap<FString, FString> ObservableInfo;          // 可观察的信息键值对

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Observe", meta = (ClampMin = "0.0"))
    float ObservationDistance;                       // 观察距离

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Observe")
    bool bDistanceAffectsDetail;                    // 距离是否影响细节

    // ========== 对话功能 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Dialogue")
    TMap<FString, FString> DialogueOptions;         // 对话选项（ID->文本）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Dialogue")
    TMap<FString, FString> DialogueResponses;      // 对话响应（选项ID->响应）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactive|Dialogue")
    TArray<FString> DialogueHistory;                // 对话历史记录

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Dialogue")
    FString CurrentDialogueState;                   // 当前对话状态

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Dialogue")
    bool bEmotionAffectsDialogue;                   // 情绪是否影响对话

    // ========== 物品交换 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Items")
    TMap<FString, FString> GivableItems;            // 可给予的物品（ID->描述）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Items")
    TMap<FString, FString> AcceptableItems;         // 可接收的物品（ID->描述）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Items")
    TSubclassOf<AItemNode> ItemNodeClass;           // 物品节点类

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactive|Items")
    TArray<FString> ReceivedItems;                  // 已接收的物品列表

    // ========== 答案验证 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Validation")
    TMap<FString, FString> CorrectAnswers;          // 正确答案映射（问题ID->答案）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Validation", meta = (ClampMin = "1"))
    int32 MaxAttempts;                              // 最大尝试次数

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactive|Validation")
    TMap<FString, int32> AttemptCounts;             // 各问题的尝试次数

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Validation")
    bool bCaseSensitive;                            // 答案是否区分大小写

    // ========== 比对功能 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Compare")
    TArray<FString> ComparisonKeys;                 // 比对的属性键

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Compare")
    float ComparisonThreshold;                      // 比对相似度阈值

    // ========== 配置数据 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactive|Config")
    TMap<FString, FString> InteractionConfig;       // 动态配置参数

    public:
    // ========== 重写父类方法 ==========
    virtual void Initialize_Implementation(AItemNode* Owner) override;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const override;
    virtual bool Use_Implementation(const FInteractionData& Data) override;
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState) override;

    // ========== 核心方法 ==========
    
    // 观察功能
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Observe")
    FString GetObservationText(float Distance) const; // 获取观察文本

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Observe")
    TArray<FString> GetAvailableObservationKeys() const; // 获取可观察的键

    UFUNCTION(BlueprintCallable, Category = "Interactive|Observe")
    void AddObservableInfo(const FString& Key, const FString& Info); // 添加可观察信息

    // 对话功能
    UFUNCTION(BlueprintCallable, Category = "Interactive|Dialogue")
    FString ProcessDialogue(const FString& OptionID); // 处理对话选择

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Dialogue")
    TArray<FString> GetAvailableDialogueOptions() const; // 获取当前可用对话选项

    UFUNCTION(BlueprintCallable, Category = "Interactive|Dialogue")
    void AddDialogueOption(const FString& OptionID, const FString& Text, const FString& Response);

    UFUNCTION(BlueprintCallable, Category = "Interactive|Dialogue")
    void ClearDialogueHistory(); // 清空对话历史

    // 物品交换
    UFUNCTION(BlueprintCallable, Category = "Interactive|Items")
    bool GiveItem(const FString& ItemID);           // 给予物品（创建Trigger关系）

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Items")
    bool CanReceiveItem(const FString& ItemID) const; // 检查是否可接收物品

    UFUNCTION(BlueprintCallable, Category = "Interactive|Items")
    bool ReceiveItem(const FString& ItemID);        // 接收物品

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Items")
    TArray<FString> GetGivableItems() const;        // 获取可给予的物品列表

    // 答案验证
    UFUNCTION(BlueprintCallable, Category = "Interactive|Validation")
    bool ValidateAnswer(const FString& QuestionID, const FString& Answer); // 验证答案

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Validation")
    int32 GetRemainingAttempts(const FString& QuestionID) const; // 获取剩余尝试次数

    UFUNCTION(BlueprintCallable, Category = "Interactive|Validation")
    void ResetAttempts(const FString& QuestionID);  // 重置尝试次数

    // 比对功能
    UFUNCTION(BlueprintCallable, Category = "Interactive|Compare")
    bool CompareWithNode(AInteractiveNode* OtherNode); // 与其他节点比对（使用Mutual关系）

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interactive|Compare")
    float CalculateSimilarity(AInteractiveNode* OtherNode) const; // 计算相似度

    // 情绪相关
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interactive|Dialogue")
    void UpdateDialogueBranch(const FEmotionData& PlayerEmotion); // 根据情绪更新对话
    virtual void UpdateDialogueBranch_Implementation(const FEmotionData& PlayerEmotion);

    // ========== 配置方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Interactive|Config")
    void LoadInteractionConfig(const TMap<FString, FString>& Config);

    UFUNCTION(BlueprintCallable, Category = "Interactive|Config")
    void ApplyConfigValue(const FString& Key, const FString& Value);

protected:
    // 内部辅助方法
    ANodeSystemManager* GetNodeSystemManager() const;
    ANodeConnection* CreateTriggerConnection(AInteractiveNode* TargetNode);
    ANodeConnection* CreateMutualConnection(AInteractiveNode* OtherNode);
    FString GetDetailLevelText(const FString& FullText, float Distance) const;
    bool CompareNodeProperties(AInteractiveNode* NodeA, AInteractiveNode* NodeB, const FString& PropertyKey) const;
    void RecordDialogueEvent(const FString& EventType, const FString& EventData);

private:
    UPROPERTY()
    ANodeSystemManager* CachedSystemManager;

    // 内部状态
    TMap<FString, ANodeConnection*> ItemConnectionMap;
};