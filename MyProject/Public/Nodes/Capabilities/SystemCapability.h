// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Core/NodeDataTypes.h"
#include "SystemCapability.generated.h"

// 前向声明
class AInteractiveNode;
class ANodeConnection;
class ANodeSystemManager;

/**
 * 系统能力类
 * 控制游戏系统层面的功能，包括时间控制、条件判断、关系建立、规则修改、AI威胁管理等
 * 可以动态创建节点和建立各种类型的关系
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API USystemCapability : public UItemCapability
{
    GENERATED_BODY()

public:
    USystemCapability();

    // ========== 时间控制 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Time", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float TimeScale;                                // 时间缩放比例

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Time")
    bool bCanPauseTime;                             // 是否可以暂停时间

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Time", meta = (ClampMin = "0.0"))
    float TimeDuration;                             // 时间控制持续时间

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|Time")
    float RemainingTimeDuration;                    // 剩余时间控制时间

    // ========== 条件系统 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Conditions")
    TMap<FString, FString> ConditionRules;          // 条件规则映射

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|Conditions")
    TMap<FString, bool> ConditionStates;           // 条件状态

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Conditions")
    bool bAutoEvaluateConditions;                   // 是否自动评估条件

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Conditions", meta = (ClampMin = "0.1"))
    float ConditionCheckInterval;                   // 条件检查间隔

    // ========== 关系管理 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Relations")
    TMap<FString, ENodeRelationType> RelationTemplates; // 关系模板

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Relations", meta = (ClampMin = "1"))
    int32 MaxRelationships;                         // 最大关系数量

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Relations")
    TSubclassOf<ANodeConnection> ConnectionClass;   // 连接类

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|Relations")
    TArray<ANodeConnection*> CreatedConnections;    // 创建的连接列表

    // ========== 规则修改 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Rules")
    TMap<FString, FString> WorldRules;              // 世界规则映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Rules")
    TMap<FString, FString> OriginalRules;           // 原始规则备份

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Rules")
    bool bCanModifyPermanently;                     // 是否可以永久修改规则

    // ========== 威胁管理（简化版） ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Threat")
    TMap<FString, float> ThreatLevels;              // 威胁等级映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Threat")
    TArray<FString> ActiveThreatIDs;                // 活跃的威胁ID列表

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Threat", meta = (ClampMin = "0.0"))
    float ThreatUpdateInterval;                     // 威胁更新间隔

    // ========== 概率控制 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Probability")
    TMap<FString, float> EventProbabilities;        // 事件概率映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Probability")
    float GlobalProbabilityModifier;                // 全局概率修正

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Probability")
    int32 RandomSeed;                               // 随机种子

    // ========== 节点生成 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Generation")
    TMap<FString, TSubclassOf<AInteractiveNode>> NodeTemplates; // 节点模板

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Generation", meta = (ClampMin = "100.0"))
    float SpawnRadius;                              // 生成半径

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Generation", meta = (ClampMin = "1"))
    int32 MaxGeneratedNodes;                        // 最大生成节点数

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|Generation")
    TArray<AInteractiveNode*> GeneratedNodes;      // 生成的节点列表

    // ========== 配置数据 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Config")
    TMap<FString, FString> SystemConfig;            // 动态配置参数

public:
    // ========== 重写父类方法 ==========
    virtual void Initialize_Implementation(AItemNode* Owner) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const override;
    virtual bool Use_Implementation(const FInteractionData& Data) override;
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState) override;

    // ========== 核心方法 ==========
    
    // 时间控制
    UFUNCTION(BlueprintCallable, Category = "System|Time")
    void SetTimeScale(float Scale, float Duration = 0.0f); // 设置时间缩放

    UFUNCTION(BlueprintCallable, Category = "System|Time")
    void PauseTime(bool bPause);                   // 暂停/恢复时间

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Time")
    float GetCurrentTimeScale() const;              // 获取当前时间缩放

    UFUNCTION(BlueprintCallable, Category = "System|Time")
    void RestoreNormalTime();                       // 恢复正常时间

    // 条件系统
    UFUNCTION(BlueprintCallable, Category = "System|Conditions")
    bool EvaluateCondition(const FString& ConditionID); // 评估条件

    UFUNCTION(BlueprintCallable, Category = "System|Conditions")
    void AddCondition(const FString& ConditionID, const FString& Rule); // 添加条件

    UFUNCTION(BlueprintCallable, Category = "System|Conditions")
    void SetConditionState(const FString& ConditionID, bool bState); // 设置条件状态

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Conditions")
    bool GetConditionState(const FString& ConditionID) const; // 获取条件状态

    UFUNCTION(BlueprintCallable, Category = "System|Conditions")
    void EvaluateAllConditions();                   // 评估所有条件

    // 关系管理
    UFUNCTION(BlueprintCallable, Category = "System|Relations")
    ANodeConnection* EstablishRelation(const FString& NodeA_ID, const FString& NodeB_ID, ENodeRelationType Type); // 建立关系

    UFUNCTION(BlueprintCallable, Category = "System|Relations")
    bool RemoveRelation(const FString& NodeA_ID, const FString& NodeB_ID); // 移除关系

    UFUNCTION(BlueprintCallable, Category = "System|Relations")
    void AddRelationTemplate(const FString& TemplateID, ENodeRelationType Type); // 添加关系模板

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Relations")
    int32 GetActiveRelationCount() const;           // 获取活跃关系数量

    // 规则修改
    UFUNCTION(BlueprintCallable, Category = "System|Rules")
    void ModifyWorldRule(const FString& RuleID, const FString& NewValue); // 修改规则

    UFUNCTION(BlueprintCallable, Category = "System|Rules")
    void RestoreOriginalRule(const FString& RuleID); // 恢复原始规则

    UFUNCTION(BlueprintCallable, Category = "System|Rules")
    void RestoreAllRules();                         // 恢复所有规则

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Rules")
    FString GetRuleValue(const FString& RuleID) const; // 获取规则值

    // 威胁管理（简化版）
    UFUNCTION(BlueprintCallable, Category = "System|Threat")
    void SetThreatLevel(const FString& ThreatID, float Level); // 设置威胁等级

    UFUNCTION(BlueprintCallable, Category = "System|Threat")
    void RegisterThreat(const FString& ThreatID);   // 注册威胁

    UFUNCTION(BlueprintCallable, Category = "System|Threat")
    void UpdateThreatBehavior(const FString& ThreatID); // 更新威胁行为

    UFUNCTION(BlueprintCallable, Category = "System|Threat")
    void RemoveAllThreats();                        // 移除所有威胁

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Threat")
    float GetThreatLevel(const FString& ThreatID) const; // 获取威胁等级

    // 概率控制
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Probability")
    float GetEventProbability(const FString& EventID) const; // 获取事件概率

    UFUNCTION(BlueprintCallable, Category = "System|Probability")
    void SetEventProbability(const FString& EventID, float Probability); // 设置事件概率

    UFUNCTION(BlueprintCallable, Category = "System|Probability")
    bool RollProbability(const FString& EventID) const; // 概率检定

    UFUNCTION(BlueprintCallable, Category = "System|Probability")
    void SetRandomSeed(int32 Seed);                 // 设置随机种子

    // 节点生成
    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    AInteractiveNode* GenerateNode(const FString& TemplateID, const FVector& Location); // 生成节点

    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    TArray<AInteractiveNode*> GenerateNodeCluster(const FString& TemplateID, int32 Count); // 生成节点群

    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    void RegisterNodeTemplate(const FString& TemplateID, TSubclassOf<AInteractiveNode> NodeClass); // 注册节点模板

    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    void ClearGeneratedNodes();                     // 清理生成的节点

    // ========== 配置方法 ==========
    UFUNCTION(BlueprintCallable, Category = "System|Config")
    void LoadSystemConfig(const TMap<FString, FString>& Config);

    UFUNCTION(BlueprintCallable, Category = "System|Config")
    void ApplyConfigValue(const FString& Key, const FString& Value);

protected:
    // 内部辅助方法
    ANodeSystemManager* GetNodeSystemManager() const;
    void UpdateTimeControl(float DeltaTime);
    bool EvaluateConditionRule(const FString& Rule) const;
    FVector GenerateRandomLocation() const;
    void CleanupInvalidConnections();
    void ProcessThreatUpdate(const FString& ThreatID, float ThreatLevel);

    UFUNCTION()
    void OnConditionCheckTimer();

    UFUNCTION()
    void OnTimeControlEnd();

    UFUNCTION()
    void OnThreatUpdateTimer();

private:
    UPROPERTY()
    ANodeSystemManager* CachedSystemManager;

    // 定时器句柄
    FTimerHandle ConditionCheckTimerHandle;
    FTimerHandle TimeControlTimerHandle;
    FTimerHandle ThreatUpdateTimerHandle;

    // 原始时间缩放
    float OriginalTimeScale;
    bool bTimeControlActive;

    // 随机数生成器
    FRandomStream RandomStream;
};