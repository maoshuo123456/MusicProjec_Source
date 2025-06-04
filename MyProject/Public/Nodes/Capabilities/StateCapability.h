// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Core/NodeDataTypes.h"
#include "StateCapability.generated.h"

// 前向声明
class AInteractiveNode;
class ANodeConnection;
class ANodeSystemManager;
class UStaticMesh;
class UMaterialInterface;

/**
 * 状态能力类
 * 管理节点的状态变化和状态传播
 * 可以改变自身或其他节点的状态，并通过Dependency关系传播状态变化
 * 支持状态条件检查和外观变形
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API UStateCapability : public UItemCapability
{
    GENERATED_BODY()

public:
    UStateCapability();

    // ========== 状态管理 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|States")
    TArray<ENodeState> PossibleStates;              // 可能的状态列表

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|States")
    ENodeState CurrentInternalState;                // 当前内部状态

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|States", meta = (ClampMin = "0.0"))
    float StateTransitionDuration;                   // 状态转换持续时间

    // ========== 状态影响 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Influence")
    TMap<FString, ENodeState> TargetNodeStates;     // 目标节点ID和要改变的状态

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Influence", meta = (ClampMin = "0.0"))
    float StateChangeRadius;                         // 状态改变影响范围

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Influence")
    TSubclassOf<AInteractiveNode> AffectedNodeClass; // 受影响的节点类型

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Influence")
    bool bPropagateThoughDependency;                // 是否通过依赖关系传播

    // ========== 状态条件 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Conditions")
    TMap<FString, FString> StateConditions;         // 状态检查条件（键值对）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Conditions")
    bool bAutoCheckState;                            // 是否自动检查状态

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Conditions", meta = (ClampMin = "0.1"))
    float StateCheckInterval;                        // 状态检查间隔

    // ========== 外观变形 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Transform")
    TArray<TSubclassOf<UStaticMesh>> StateMeshClasses; // 不同状态的网格类

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Transform")
    TArray<TSubclassOf<UMaterialInterface>> StateMaterialClasses; // 不同状态的材质类

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Transform")
    bool bChangeAppearanceOnStateChange;             // 状态改变时是否改变外观

    // ========== 配置数据 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Config")
    TMap<FString, FString> StateConfig;              // 动态配置参数

public:
    // ========== 重写父类方法 ==========
    virtual void Initialize_Implementation(AItemNode* Owner) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const override;
    virtual bool Use_Implementation(const FInteractionData& Data) override;
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState) override;

    // ========== 核心方法 ==========
    UFUNCTION(BlueprintCallable, Category = "State|Control")
    void ChangeOwnState(ENodeState NewState);       // 改变自身状态

    UFUNCTION(BlueprintCallable, Category = "State|Control")
    void ChangeTargetNodeState(const FString& TargetNodeID, ENodeState NewState); // 改变目标节点状态

    UFUNCTION(BlueprintCallable, Category = "State|Control")
    void ChangeNodesInRadius(ENodeState NewState);  // 改变范围内节点状态

    UFUNCTION(BlueprintCallable, Category = "State|Control")
    void ApplyStateToConnectedNodes(ENodeState NewState); // 应用状态到连接的节点

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State|Query")
    ENodeState GetCurrentInternalState() const { return CurrentInternalState; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State|Query")
    bool IsStateAvailable(ENodeState State) const;  // 检查状态是否可用

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "State|Conditions")
    bool CheckStateConditions() const;              // 检查状态条件

    UFUNCTION(BlueprintCallable, Category = "State|Conditions")
    void AddStateCondition(const FString& Key, const FString& Value); // 添加状态条件

    UFUNCTION(BlueprintCallable, Category = "State|Conditions")
    void RemoveStateCondition(const FString& Key);  // 移除状态条件

    UFUNCTION(BlueprintCallable, Category = "State|Transform")
    void TransformAppearance(int32 FormIndex);      // 改变外观形态

    UFUNCTION(BlueprintCallable, Category = "State|Transform")
    void RestoreOriginalAppearance();               // 恢复原始外观

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "State|Control")
    void PropagateStateChange(ENodeState NewState); // 通过Dependency关系传播状态
    virtual void PropagateStateChange_Implementation(ENodeState NewState);

    // ========== 配置方法 ==========
    UFUNCTION(BlueprintCallable, Category = "State|Config")
    void LoadStateConfig(const TMap<FString, FString>& Config);

    UFUNCTION(BlueprintCallable, Category = "State|Config")
    void ApplyConfigValue(const FString& Key, const FString& Value);

protected:
    // 内部辅助方法
    ANodeSystemManager* GetNodeSystemManager() const;
    void ProcessStateTransition(ENodeState FromState, ENodeState ToState);
    bool ValidateStateTransition(ENodeState FromState, ENodeState ToState) const;
    void UpdateNodeAppearance(ENodeState State);
    TArray<AInteractiveNode*> GetNodesInRadius() const;
    TArray<ANodeConnection*> GetDependencyConnections() const;

    UFUNCTION()
    void OnStateCheckTimer();

private:
    UPROPERTY()
    ANodeSystemManager* CachedSystemManager;

    UPROPERTY()
    UStaticMesh* OriginalMesh;

    UPROPERTY()
    UMaterialInterface* OriginalMaterial;

    FTimerHandle StateCheckTimerHandle;
    FTimerHandle StateTransitionTimerHandle;

    bool bIsTransitioning;
    ENodeState TransitionTargetState;
};