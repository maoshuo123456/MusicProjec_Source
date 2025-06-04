// Fill out your copyright notice in the Description page of Project Settings.

// InteractiveNode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NodeDataTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "InteractiveNode.generated.h"

// 前向声明
class UUserWidget;
class APlayerController;

// 故事触发委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeStoryTriggered, AInteractiveNode*, Node, const TArray<FString>&, EventIDs);

UCLASS(Abstract, Blueprintable)
class MYPROJECT_API AInteractiveNode : public AActor
{
    GENERATED_BODY()

public:
    AInteractiveNode();

    // 节点基础数据
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|Data")
    FNodeData NodeData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node|State")
    ENodeState CurrentState;

    // 组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node|Components")
    UStaticMeshComponent* NodeMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node|Components")
    UBoxComponent* InteractionVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node|Components")
    UWidgetComponent* InfoWidgetComponent;

    // 交互属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|Interaction")
    bool bIsInteractable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|Interaction", meta = (ClampMin = "0.0"))
    float InteractionRange;

    // UI属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|UI")
    TSubclassOf<UUserWidget> InfoWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|UI", meta = (ClampMin = "0.0"))
    float UIDisplayDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|UI")
    bool bAlwaysShowUI;

    // 故事相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|Story")
    FString StoryFragmentID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|Story")
    TArray<FString> TriggerEventIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node|Story")
    TMap<FString, FString> StoryContext;

public:
    // 委托
    UPROPERTY(BlueprintAssignable, Category = "Node|Events")
    FOnNodeStateChanged OnNodeStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Node|Events")
    FOnNodeInteracted OnNodeInteracted;

    UPROPERTY(BlueprintAssignable, Category = "Node|Events")
    FOnNodeStoryTriggered OnNodeStoryTriggered;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // 初始化
    UFUNCTION(BlueprintCallable, Category = "Node|Core")
    virtual void Initialize(const FNodeData& InNodeData);

    // 状态管理
    UFUNCTION(BlueprintCallable, Category = "Node|State")
    virtual void SetNodeState(ENodeState NewState);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Node|State")
    ENodeState GetNodeState() const { return CurrentState; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Node|Data")
    FNodeData GetNodeData() const { return NodeData; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Node|Data")
    FString GetNodeID() const { return NodeData.NodeID; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Node|Data")
    FString GetNodeName() const { return NodeData.NodeName; }

    // 交互接口
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Interaction")
    bool CanInteract(const FInteractionData& Data) const;
    virtual bool CanInteract_Implementation(const FInteractionData& Data) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Interaction")
    void OnInteract(const FInteractionData& Data);
    virtual void OnInteract_Implementation(const FInteractionData& Data);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Interaction")
    void OnStartInteraction(const FInteractionData& Data);
    virtual void OnStartInteraction_Implementation(const FInteractionData& Data);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Interaction")
    void OnEndInteraction(const FInteractionData& Data);
    virtual void OnEndInteraction_Implementation(const FInteractionData& Data);

    // UI管理
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|UI")
    void UpdateNodeUI();
    virtual void UpdateNodeUI_Implementation();

    UFUNCTION(BlueprintCallable, Category = "Node|UI")
    void SetUIVisibility(bool bVisible);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Node|UI")
    FText GetInteractionPrompt() const;
    virtual FText GetInteractionPrompt_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|UI")
    bool ShouldShowUI(APlayerController* Player) const;
    virtual bool ShouldShowUI_Implementation(APlayerController* Player) const;

    // 故事接口
    UFUNCTION(BlueprintCallable, Category = "Node|Story")
    void SetStoryFragment(const FString& FragmentID);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Node|Story")
    FString GetStoryFragment() const { return StoryFragmentID; }

    UFUNCTION(BlueprintCallable, Category = "Node|Story")
    void AddTriggerEvent(const FString& EventID);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Node|Story")
    TArray<FString> GetTriggeredEvents() const { return TriggerEventIDs; }

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Node|Story")
    bool ShouldTriggerStory() const;
    virtual bool ShouldTriggerStory_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Story")
    void OnStoryTriggered();
    virtual void OnStoryTriggered_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Story")
    TArray<FNodeGenerateData> GetNodeSpawnData() const;
    virtual TArray<FNodeGenerateData> GetNodeSpawnData_Implementation() const;

protected:
    // 内部方法
    UFUNCTION(BlueprintNativeEvent, Category = "Node|Internal")
    void OnStateChanged(ENodeState OldState, ENodeState NewState);
    virtual void OnStateChanged_Implementation(ENodeState OldState, ENodeState NewState);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Internal")
    bool ValidateInteraction(const FInteractionData& Data) const;
    virtual bool ValidateInteraction_Implementation(const FInteractionData& Data) const;

    void BroadcastStateChange(ENodeState OldState, ENodeState NewState);
    void BroadcastInteraction(const FInteractionData& Data);
    void BroadcastStoryTrigger();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Node|Visuals")
    void UpdateVisuals();
    virtual void UpdateVisuals_Implementation();

    void CreateNodeUI();

private:
    // UI更新定时器
    FTimerHandle UIUpdateTimerHandle;
    void CheckUIVisibility();
};