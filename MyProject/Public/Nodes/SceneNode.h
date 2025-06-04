// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nodes/InteractiveNode.h"
#include "Core/NodeDataTypes.h"
#include "SceneNode.generated.h"

// 前向声明
class AInteractiveNode;

UCLASS(Blueprintable)
class MYPROJECT_API ASceneNode : public AInteractiveNode
{
    GENERATED_BODY()

public:
    ASceneNode();

protected:
    // 子节点管理
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Children")
    TArray<AInteractiveNode*> ChildNodes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Children")
    TMap<FString, AInteractiveNode*> ChildNodeMap;

    // 场景属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Properties")
    FEmotionData SceneEmotion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Properties")
    bool bIsActiveScene;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Properties", meta = (ClampMin = "0.0"))
    float SceneRadius;

    // 故事相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene|Story")
    FString SceneStoryChapterID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scene|Story")
    TArray<FNodeGenerateData> PendingNodeSpawns;

public:
    // 子节点管理方法
    UFUNCTION(BlueprintCallable, Category = "Scene|Children")
    virtual void AddChildNode(AInteractiveNode* Node);

    UFUNCTION(BlueprintCallable, Category = "Scene|Children")
    virtual void RemoveChildNode(AInteractiveNode* Node);

    UFUNCTION(BlueprintCallable, Category = "Scene|Children")
    virtual void RemoveChildNodeByID(const FString& NodeID);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Children")
    AInteractiveNode* GetChildNode(const FString& NodeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Children")
    TArray<AInteractiveNode*> GetAllChildNodes() const { return ChildNodes; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Children")
    TArray<AInteractiveNode*> GetChildNodesByType(ENodeType Type) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Children")
    TArray<AInteractiveNode*> GetChildNodesByState(ENodeState State) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Children")
    int32 GetChildNodeCount() const { return ChildNodes.Num(); }

    // 场景管理
    UFUNCTION(BlueprintCallable, Category = "Scene|Management")
    virtual void ActivateScene();

    UFUNCTION(BlueprintCallable, Category = "Scene|Management")
    virtual void DeactivateScene();

    UFUNCTION(BlueprintCallable, Category = "Scene|Management")
    void SetSceneEmotion(const FEmotionData& Emotion);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Management")
    FEmotionData GetSceneEmotion() const { return SceneEmotion; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene|Management")
    bool IsActiveScene() const { return bIsActiveScene; }

    // 节点生成
    UFUNCTION(BlueprintCallable, Category = "Scene|Spawning")
    void SpawnNodesFromData(const TArray<FNodeGenerateData>& SpawnDataArray);

    UFUNCTION(BlueprintCallable, Category = "Scene|Spawning")
    void QueueNodeSpawn(const FNodeGenerateData& SpawnData);

    UFUNCTION(BlueprintCallable, Category = "Scene|Spawning")
    void ProcessPendingSpawns();

    UFUNCTION(BlueprintCallable, Category = "Scene|Spawning")
    void ClearPendingSpawns() { PendingNodeSpawns.Empty(); }

    // 重写父类方法
    virtual void OnInteract_Implementation(const FInteractionData& Data) override;
    virtual void OnStateChanged_Implementation(ENodeState OldState, ENodeState NewState) override;
    virtual bool ShouldTriggerStory_Implementation() const override;
    virtual TArray<FNodeGenerateData> GetNodeSpawnData_Implementation() const override;
    virtual void UpdateVisuals_Implementation() override;

protected:
    // BeginPlay
    virtual void BeginPlay() override;

    // 内部方法
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Scene|Internal")
    void UpdateChildrenStates();
    virtual void UpdateChildrenStates_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Scene|Internal")
    void PropagateEmotionToChildren();
    virtual void PropagateEmotionToChildren_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Scene|Internal")
    void ArrangeChildNodes();
    virtual void ArrangeChildNodes_Implementation();

    UFUNCTION()
    void OnChildNodeStateChanged(AInteractiveNode* ChildNode, ENodeState OldState, ENodeState NewState);

    UFUNCTION()
    void OnChildNodeInteracted(AInteractiveNode* ChildNode, const FInteractionData& InteractionData);

    UFUNCTION()
    void OnChildNodeStoryTriggered(AInteractiveNode* ChildNode, const TArray<FString>& EventIDs);

private:
    // 辅助方法
    void RegisterChildNode(AInteractiveNode* Node);
    void UnregisterChildNode(AInteractiveNode* Node);
    AInteractiveNode* SpawnNodeFromData(const FNodeGenerateData& SpawnData);
};