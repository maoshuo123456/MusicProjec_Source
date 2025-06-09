// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NodeDataTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "NodeSystemManager.generated.h"

// 前向声明
class AInteractiveNode;
class ASceneNode;
class AItemNode;
class ANodeConnection;
class UItemCapability;

// 系统状态结构
USTRUCT(BlueprintType)
struct FSystemState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Save Data")
    TArray<FNodeData> SavedNodes;

    UPROPERTY(BlueprintReadWrite, Category = "Save Data")
    TArray<FNodeRelationData> SavedConnections;

    UPROPERTY(BlueprintReadWrite, Category = "Save Data")
    FString ActiveSceneID;

    UPROPERTY(BlueprintReadWrite, Category = "Save Data")
    TMap<FString, FString> SystemData;

    UPROPERTY(BlueprintReadWrite, Category = "Save Data")
    FDateTime SaveTime;

    FSystemState()
    {
        SaveTime = FDateTime::Now();
    }
};

// 节点生成结果
USTRUCT(BlueprintType)
struct FNodeGenerationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Generation")
    bool bSuccess;

    UPROPERTY(BlueprintReadOnly, Category = "Generation")
    AInteractiveNode* GeneratedNode;

    UPROPERTY(BlueprintReadOnly, Category = "Generation")
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly, Category = "Generation")
    TArray<ANodeConnection*> GeneratedConnections;

    FNodeGenerationResult()
    {
        bSuccess = false;
        GeneratedNode = nullptr;
    }
};

// 委托声明
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeRegistered, AInteractiveNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeUnregistered, AInteractiveNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionCreated, ANodeConnection*, Connection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionRemoved, ANodeConnection*, Connection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSceneChanged, ASceneNode*, OldScene, ASceneNode*, NewScene);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSystemStateChanged, const FString&, StateDescription);

UCLASS(Blueprintable)
class MYPROJECT_API ANodeSystemManager : public AActor
{
    GENERATED_BODY()

public:
    ANodeSystemManager();

    // 注册表
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|Registry")
    TMap<FString, AInteractiveNode*> NodeRegistry;

    // 使用TArray代替TMultiMap
    TMap<FString, TArray<ANodeConnection*>> ConnectionRegistry;

    TMap<FString, TArray<AInteractiveNode*>> NodeTypeMap;

    TMap<FString, TArray<AInteractiveNode*>> NodeTagMap;

    // 默认类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Classes")
    TSubclassOf<AInteractiveNode> DefaultSceneNodeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Classes")
    TSubclassOf<AInteractiveNode> DefaultItemNodeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Classes")
    TSubclassOf<ANodeConnection> DefaultConnectionClass;

    // 状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|State")
    ASceneNode* ActiveSceneNode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|State")
    TArray<AInteractiveNode*> ActiveNodes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System|State")
    TArray<ANodeConnection*> ActiveConnections;

    // 配置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Config", meta = (ClampMin = "100.0"))
    float NodeSpawnRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Config", meta = (ClampMin = "1"))
    int32 MaxNodesPerScene;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Config")
    bool bAutoRegisterSpawnedNodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Config")
    bool bDebugDrawConnections;

    // 生成队列
    TQueue<FNodeGenerateData> NodeGenerationQueue;
    TQueue<FNodeRelationData> ConnectionGenerationQueue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Generation", meta = (ClampMin = "0.01"))
    float GenerationInterval;

    // 系统数据
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Data")
    FGameplayTagContainer SystemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System|Data")
    TMap<FString, FString> SystemMetadata;

public:
    // 委托
    UPROPERTY(BlueprintAssignable, Category = "System|Events")
    FOnNodeRegistered OnNodeRegistered;

    UPROPERTY(BlueprintAssignable, Category = "System|Events")
    FOnNodeUnregistered OnNodeUnregistered;

    UPROPERTY(BlueprintAssignable, Category = "System|Events")
    FOnConnectionCreated OnConnectionCreated;

    UPROPERTY(BlueprintAssignable, Category = "System|Events")
    FOnConnectionRemoved OnConnectionRemoved;

    UPROPERTY(BlueprintAssignable, Category = "System|Events")
    FOnSceneChanged OnSceneChanged;

    UPROPERTY(BlueprintAssignable, Category = "System|Events")
    FOnSystemStateChanged OnSystemStateChanged;

    


protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // 节点创建
    UFUNCTION(BlueprintCallable, Category = "System|Nodes", meta = (DisplayName = "Create Node"))
    AInteractiveNode* CreateNode(TSubclassOf<AInteractiveNode> NodeClass, const FNodeGenerateData& GenerateData);

    UFUNCTION(BlueprintCallable, Category = "System|Nodes")
    ASceneNode* CreateSceneNode(const FNodeGenerateData& GenerateData);

    UFUNCTION(BlueprintCallable, Category = "System|Nodes")
    AItemNode* CreateItemNode(const FNodeGenerateData& GenerateData);

    UFUNCTION(BlueprintCallable, Category = "System|Nodes")
    AInteractiveNode* SpawnNodeAtLocation(TSubclassOf<AInteractiveNode> NodeClass, const FVector& Location, const FNodeData& NodeData);

    // 节点注册
    UFUNCTION(BlueprintCallable, Category = "System|Nodes")
    bool RegisterNode(AInteractiveNode* Node);

    UFUNCTION(BlueprintCallable, Category = "System|Nodes", meta = (DisplayName = "Unregister Node by ID"))
    bool UnregisterNodeByID(const FString& NodeID);

    UFUNCTION(BlueprintCallable, Category = "System|Nodes", meta = (DisplayName = "Unregister Node"))
    bool UnregisterNode(AInteractiveNode* Node);

    // 节点查询
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    AInteractiveNode* GetNode(const FString& NodeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> GetNodesByType(ENodeType Type) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> GetNodesByState(ENodeState State) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> GetNodesByTag(const FGameplayTag& Tag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> GetNodesInRadius(const FVector& Center, float Radius) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AItemNode*> FindNodesWithCapability(TSubclassOf<UItemCapability> CapabilityClass) const;

    //玩家与节点交互
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void BindPlayerInteractionEvents();

    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnPlayerNodeInteractionEvent(AItemNode* Node, EInteractionType Type, bool bIsStarting);

    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnInteractionStarted(AItemNode* Node, EInteractionType Type);

    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnInteractionEnded(AItemNode* Node, EInteractionType Type);
    
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnNodeSelected(AItemNode* Node);
    
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnNodeDeselected(AItemNode* Node);
    
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnNodeDragStarted(AItemNode* Node);
    
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnNodeDragEnded(AItemNode* Node);
    
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnNodeHoverStarted(AItemNode* Node);
    
    UFUNCTION(BlueprintCallable, Category = "System|Interaction")
    void OnNodeHoverEnded(AItemNode* Node);
    
    // 连接管理
    UFUNCTION(BlueprintCallable, Category = "System|Connections")
    ANodeConnection* CreateConnection(AInteractiveNode* Source, AInteractiveNode* Target, const FNodeRelationData& RelationData);

    UFUNCTION(BlueprintCallable, Category = "System|Connections")
    ANodeConnection* CreateConnectionBetween(const FString& SourceID, const FString& TargetID, ENodeRelationType Type);

    UFUNCTION(BlueprintCallable, Category = "System|Connections")
    bool RemoveConnection(ANodeConnection* Connection);

    UFUNCTION(BlueprintCallable, Category = "System|Connections")
    int32 RemoveConnectionsBetween(const FString& NodeA, const FString& NodeB);

    UFUNCTION(BlueprintCallable, Category = "System|Connections")
    int32 RemoveAllConnectionsForNode(const FString& NodeID);

    // 连接查询
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    ANodeConnection* GetConnection(const FString& SourceID, const FString& TargetID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<ANodeConnection*> GetConnectionsForNode(const FString& NodeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<ANodeConnection*> GetOutgoingConnections(const FString& NodeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<ANodeConnection*> GetIncomingConnections(const FString& NodeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> GetConnectedNodes(const FString& NodeID, ENodeRelationType RelationType = ENodeRelationType::Dependency) const;

    // 场景管理
    UFUNCTION(BlueprintCallable, Category = "System|Scene")
    bool SetActiveScene(ASceneNode* Scene);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Scene")
    ASceneNode* GetActiveScene() const { return ActiveSceneNode; }

    UFUNCTION(BlueprintCallable, Category = "System|Scene")
    void TransitionToScene(ASceneNode* NewScene, float TransitionDuration = 1.0f);

    // 生成队列
    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    void QueueNodeGeneration(const FNodeGenerateData& GenerateData);

    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    void QueueConnectionGeneration(const FNodeRelationData& RelationData);

    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    void ProcessGenerationQueue();

    UFUNCTION(BlueprintCallable, Category = "System|Generation")
    void ClearGenerationQueues();

    // 高级查询
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> ExecuteNodeQuery(const FNodeQueryParams& QueryParams) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> FindPath(AInteractiveNode* Start, AInteractiveNode* End) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "System|Query")
    TArray<AInteractiveNode*> GetNodeHierarchy(AInteractiveNode* RootNode) const;

    // 系统管理
    UFUNCTION(BlueprintCallable, Category = "System|Management")
    bool SaveSystemState(const FString& SaveName);

    UFUNCTION(BlueprintCallable, Category = "System|Management")
    bool LoadSystemState(const FString& SaveName);

    UFUNCTION(BlueprintCallable, Category = "System|Management")
    void ResetSystem();

    UFUNCTION(BlueprintCallable, Category = "System|Management")
    bool ValidateSystem();

protected:
    // 事件处理
    UFUNCTION()
    void OnNodeStateChanged(AInteractiveNode* Node, ENodeState OldState, ENodeState NewState);

    UFUNCTION()
    void OnNodeInteracted(AInteractiveNode* Node, const FInteractionData& Data);

    UFUNCTION()
    void OnNodeDestroyed(AActor* DestroyedActor);

    UFUNCTION()
    void OnConnectionStateChanged(ANodeConnection* Connection);

    // 内部方法
    void ProcessNodeGeneration();
    void ProcessConnectionGeneration();
    void UpdateNodeIndices();
    void CleanupInvalidReferences();
    void PropagateSystemEvent(const FGameEventData& EventData);
    bool CheckPrerequisites(AInteractiveNode* Node) const;
    void ActivateDependentNodes(AInteractiveNode* CompletedNode);

    // 辅助方法
    void RegisterNodeEvents(AInteractiveNode* Node);
    void UnregisterNodeEvents(AInteractiveNode* Node);
    void RegisterConnectionEvents(ANodeConnection* Connection);
    void UnregisterConnectionEvents(ANodeConnection* Connection);
    void UpdateNodeTypeMap(AInteractiveNode* Node, bool bAdd);
    void UpdateNodeTagMap(AInteractiveNode* Node, bool bAdd);
    
    FVector CalculateNodeSpawnLocation(const FVector& BaseLocation) const;
    
private:
    // 定时器
    FTimerHandle GenerationTimerHandle;
    FTimerHandle ValidationTimerHandle;

    // 场景过渡
    bool bIsTransitioning;
    float TransitionProgress;
    ASceneNode* TransitionTargetScene;
};