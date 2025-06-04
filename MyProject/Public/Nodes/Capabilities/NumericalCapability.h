// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Core/NodeDataTypes.h"
#include "NumericalCapability.generated.h"

// 前向声明
class AInteractiveNode;
class ANodeConnection;
class ANodeSystemManager;

// 感知类型枚举
UENUM(BlueprintType)
enum class EPerceptionType : uint8
{
    Vision      UMETA(DisplayName = "Vision"),      // 视觉
    Hearing     UMETA(DisplayName = "Hearing"),     // 听觉
    Danger      UMETA(DisplayName = "Danger"),      // 危险感知
    All         UMETA(DisplayName = "All")          // 所有类型
};

/**
 * 数值能力类
 * 管理游戏中的各种数值系统，提供通用的数值管理框架
 * 支持动态添加新的数值类型，包括血量、精神值等
 * 通过Emotional关系处理好感度等情感数值
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API UNumericalCapability : public UItemCapability
{
    GENERATED_BODY()

public:
    UNumericalCapability();

    // ========== 通用数值系统 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Values")
    TMap<FString, float> NumericalValues;           // 数值映射（键->值）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Values")
    TMap<FString, float> MaxValues;                 // 最大值映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Values")
    TMap<FString, float> MinValues;                 // 最小值映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Values")
    TMap<FString, float> RegenerationRates;         // 回复速率映射

    // ========== 预定义数值类型 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Predefined", meta = (ClampMin = "0.0"))
    float PlayerHealth;                             // 玩家血量

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Predefined", meta = (ClampMin = "0.0"))
    float PlayerMaxHealth;                          // 玩家最大血量

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Predefined", meta = (ClampMin = "0.0"))
    float MentalState;                              // 精神状态值

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Predefined", meta = (ClampMin = "0.0"))
    float MaxMentalState;                           // 最大精神状态值

    // ========== 资源管理 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Resources")
    TMap<FString, float> ResourcePools;             // 资源池映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Resources")
    TMap<FString, float> ConsumptionRates;          // 消耗率映射

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Resources")
    bool bAutoConsume;                              // 是否自动消耗

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Resources", meta = (ClampMin = "0.1"))
    float ResourceUpdateInterval;                    // 资源更新间隔

    // ========== 进度系统 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Progress")
    TMap<FString, float> ProgressTrackers;          // 进度追踪器

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Progress")
    TMap<FString, float> ProgressMilestones;        // 进度里程碑

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Progress")
    TMap<FString, FString> MilestoneRewards;        // 里程碑奖励

    // ========== 感知系统 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Perception", meta = (ClampMin = "0.0"))
    float VisionRadius;                             // 视野半径

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Perception", meta = (ClampMin = "0.0"))
    float HearingRadius;                            // 听力半径

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Perception", meta = (ClampMin = "0.0"))
    float DangerSenseRadius;                        // 危险感知半径

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Perception")
    TSubclassOf<AInteractiveNode> PerceptibleNodeClass; // 可感知的节点类型

    // ========== 好感度系统（预留） ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Affinity")
    TMap<FString, float> AffinityValues;            // 好感度数值

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Affinity", meta = (ClampMin = "-100.0", ClampMax = "100.0"))
    float AffinityMin;                              // 最小好感度

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Affinity", meta = (ClampMin = "-100.0", ClampMax = "100.0"))
    float AffinityMax;                              // 最大好感度

    // ========== 配置数据 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Numerical|Config")
    TMap<FString, FString> NumericalConfig;         // 动态配置参数

public:
    // ========== 重写父类方法 ==========
    virtual void Initialize_Implementation(AItemNode* Owner) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const override;
    virtual bool Use_Implementation(const FInteractionData& Data) override;
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ========== 核心方法 ==========
    
    // 通用数值管理
    UFUNCTION(BlueprintCallable, Category = "Numerical|Values")
    void SetValue(const FString& ValueID, float Value); // 设置数值

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Values")
    float GetValue(const FString& ValueID) const;   // 获取数值

    UFUNCTION(BlueprintCallable, Category = "Numerical|Values")
    void ModifyValue(const FString& ValueID, float Delta); // 修改数值（增减）

    UFUNCTION(BlueprintCallable, Category = "Numerical|Values")
    void RegisterNewValue(const FString& ValueID, float InitialValue, float MinVal, float MaxVal); // 注册新数值类型

    UFUNCTION(BlueprintCallable, Category = "Numerical|Values")
    void SetValueRange(const FString& ValueID, float MinVal, float MaxVal); // 设置数值范围

    UFUNCTION(BlueprintCallable, Category = "Numerical|Values")
    void SetRegenerationRate(const FString& ValueID, float Rate); // 设置回复速率

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Values")
    float GetValuePercentage(const FString& ValueID) const; // 获取数值百分比

    // 预定义数值操作
    UFUNCTION(BlueprintCallable, Category = "Numerical|Health")
    void ModifyHealth(float Delta);                 // 修改血量

    UFUNCTION(BlueprintCallable, Category = "Numerical|Health")
    void ModifyMentalState(float Delta);            // 修改精神状态

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Health")
    float GetHealthPercentage() const;              // 获取血量百分比

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Health")
    float GetMentalStatePercentage() const;         // 获取精神状态百分比

    // 资源管理
    UFUNCTION(BlueprintCallable, Category = "Numerical|Resources")
    bool ConsumeResource(const FString& ResourceID, float Amount); // 消耗资源

    UFUNCTION(BlueprintCallable, Category = "Numerical|Resources")
    void ReplenishResource(const FString& ResourceID, float Amount); // 补充资源

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Resources")
    float GetResourceAmount(const FString& ResourceID) const; // 获取资源数量

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Resources")
    bool HasEnoughResource(const FString& ResourceID, float Amount) const; // 检查资源是否足够

    UFUNCTION(BlueprintCallable, Category = "Numerical|Resources")
    void SetConsumptionRate(const FString& ResourceID, float Rate); // 设置消耗率

    // 进度系统
    UFUNCTION(BlueprintCallable, Category = "Numerical|Progress")
    void UpdateProgress(const FString& ProgressID, float Delta); // 更新进度

    UFUNCTION(BlueprintCallable, Category = "Numerical|Progress")
    void SetProgress(const FString& ProgressID, float Value); // 设置进度

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Progress")
    float GetProgress(const FString& ProgressID) const; // 获取进度

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Progress")
    bool HasReachedMilestone(const FString& ProgressID) const; // 检查是否达到里程碑

    UFUNCTION(BlueprintCallable, Category = "Numerical|Progress")
    void RegisterMilestone(const FString& ProgressID, float MilestoneValue, const FString& Reward); // 注册里程碑

    // 感知系统
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Perception")
    TArray<AInteractiveNode*> PerceiveNodesInRadius(float Radius, TSubclassOf<AInteractiveNode> NodeClass) const; // 感知范围内节点

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Perception")
    TArray<AInteractiveNode*> PerceiveNodesByType(EPerceptionType Type) const; // 按类型感知节点

    UFUNCTION(BlueprintCallable, Category = "Numerical|Perception")
    void SetPerceptionRadius(EPerceptionType Type, float Radius); // 设置感知半径

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Perception")
    float GetPerceptionRadius(EPerceptionType Type) const; // 获取感知半径

    // ========== 预留接口 ==========
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Numerical|Affinity")
    void ModifyAffinity(const FString& TargetID, float Delta); // 修改好感度（预留）
    virtual void ModifyAffinity_Implementation(const FString& TargetID, float Delta);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Numerical|Affinity")
    float GetAffinity(const FString& TargetID) const; // 获取好感度

    // ========== 配置方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Numerical|Config")
    void LoadNumericalConfig(const TMap<FString, FString>& Config);

    UFUNCTION(BlueprintCallable, Category = "Numerical|Config")
    void ApplyConfigValue(const FString& Key, const FString& Value);

protected:
    // 内部辅助方法
    ANodeSystemManager* GetNodeSystemManager() const;
    void ProcessRegeneration(float DeltaTime);
    void ProcessResourceConsumption(float DeltaTime);
    void CheckAndTriggerMilestones(const FString& ProgressID);
    void ClampValue(const FString& ValueID);
    void InitializePredefinedValues();
    void UpdatePredefinedValues();
    ANodeConnection* CreateEmotionalConnection(const FString& TargetNodeID);

    UFUNCTION()
    void OnResourceUpdateTimer();

private:
    UPROPERTY()
    ANodeSystemManager* CachedSystemManager;

    // 定时器句柄
    FTimerHandle ResourceUpdateTimerHandle;

    // 内部状态
    float AccumulatedDeltaTime;
    TMap<FString, float> LastMilestoneChecked;
};