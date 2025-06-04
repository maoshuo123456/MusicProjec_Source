// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/Capabilities/ItemCapability.h"
#include "Core/NodeDataTypes.h"
#include "SpatialCapability.generated.h"

// 前向声明
class AInteractiveNode;
class ANodeConnection;
class ANodeSystemManager;

/**
 * 空间能力类
 * 管理节点的空间属性和空间关系，包括容纳其他节点、控制访问权限、传送功能等
 * 主要通过Parent关系管理容纳的节点
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API USpatialCapability : public UItemCapability
{
    GENERATED_BODY()

public:
    USpatialCapability();

    // ========== 容纳功能 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Container")
    bool bCanContainNodes;                          // 是否可以容纳其他节点

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Container", meta = (ClampMin = "0"))
    int32 MaxContainedNodes;                        // 最大容纳节点数

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spatial|Container")
    TArray<AInteractiveNode*> ContainedNodes;      // 已容纳的节点列表

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Container")
    TSubclassOf<AInteractiveNode> AllowedNodeClass; // 允许容纳的节点类型

    // ========== 访问控制 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Access")
    bool bAllowsEntry;                              // 是否允许进入

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Access")
    bool bAllowsExit;                               // 是否允许离开

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Access")
    TMap<FString, bool> AccessPermissions;          // 特定节点ID的访问权限

    // ========== 传送功能 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Teleport")
    FVector TeleportDestination;                    // 传送目标位置

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Teleport")
    FString TeleportTargetNodeID;                   // 传送目标节点ID

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Teleport")
    bool bTeleportToNode;                           // 是否传送到节点位置

    // ========== 预留功能（暂不实现） ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Reserved", meta = (ClampMin = "0.0"))
    float LightingRadius;                           // 照明范围

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Reserved")
    TArray<FVector> GuidePath;                      // 引导路径点

    // ========== 配置数据 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spatial|Config")
    TMap<FString, FString> SpatialConfig;           // 动态配置参数

public:
    // ========== 重写父类方法 ==========
    virtual void Initialize_Implementation(AItemNode* Owner) override;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const override;
    virtual bool Use_Implementation(const FInteractionData& Data) override;
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState) override;

    // ========== 核心方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Spatial|Container")
    bool ContainNode(AInteractiveNode* Node);       // 容纳节点（创建Parent关系）

    UFUNCTION(BlueprintCallable, Category = "Spatial|Container")
    bool ReleaseNode(AInteractiveNode* Node);       // 释放节点（移除Parent关系）

    UFUNCTION(BlueprintCallable, Category = "Spatial|Container")
    bool ReleaseAllNodes();                         // 释放所有节点

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spatial|Container")
    bool IsNodeContained(AInteractiveNode* Node) const; // 检查节点是否被容纳

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spatial|Container")
    int32 GetContainedNodeCount() const { return ContainedNodes.Num(); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spatial|Access")
    bool CanNodeEnter(AInteractiveNode* RequestingNode) const; // 检查节点是否可以进入

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Spatial|Access")
    bool CanNodeExit(AInteractiveNode* RequestingNode) const; // 检查节点是否可以离开

    UFUNCTION(BlueprintCallable, Category = "Spatial|Access")
    void SetNodeAccess(const FString& NodeID, bool bAllowAccess); // 设置特定节点访问权限

    UFUNCTION(BlueprintCallable, Category = "Spatial|Teleport")
    bool TeleportPlayer(APlayerController* Player); // 传送玩家到目标位置

    UFUNCTION(BlueprintCallable, Category = "Spatial|Teleport")
    void SetTeleportTarget(const FVector& Location); // 设置传送目标位置

    UFUNCTION(BlueprintCallable, Category = "Spatial|Teleport")
    void SetTeleportTargetNode(const FString& NodeID); // 设置传送目标节点

    // ========== 预留接口 ==========
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Spatial|Reserved")
    void UpdateLighting();                          // 更新照明效果（预留）
    virtual void UpdateLighting_Implementation() {}

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Spatial|Reserved")
    FVector GetNextGuidePoint();                    // 获取下一个引导点（预留）
    virtual FVector GetNextGuidePoint_Implementation() { return FVector::ZeroVector; }

    // ========== 配置方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Spatial|Config")
    void LoadSpatialConfig(const TMap<FString, FString>& Config);

    UFUNCTION(BlueprintCallable, Category = "Spatial|Config")
    void ApplyConfigValue(const FString& Key, const FString& Value);

protected:
    // 内部辅助方法
    ANodeSystemManager* GetNodeSystemManager() const;
    ANodeConnection* CreateParentConnection(AInteractiveNode* ChildNode);
    void RemoveParentConnection(AInteractiveNode* ChildNode);
    bool ValidateNodeForContainment(AInteractiveNode* Node) const;

    UFUNCTION()
    void OnContainedNodeDestroyed(AActor* DestroyedActor);

private:
    UPROPERTY()
    ANodeSystemManager* CachedSystemManager;

    TMap<AInteractiveNode*, ANodeConnection*> NodeConnectionMap;
};