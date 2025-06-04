// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NodeDataTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"
#include "NodeConnection.generated.h"

// 前向声明
class AInteractiveNode;
class UNiagaraComponent;
class UUserWidget;

// 连接委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionActivated, ANodeConnection*, Connection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionDeactivated, ANodeConnection*, Connection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionPulsed, ANodeConnection*, Connection);

// 连接视觉数据
USTRUCT(BlueprintType)
struct FConnectionVisualData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    UStaticMesh* ConnectionMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    UMaterialInterface* ConnectionMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FVector MeshScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    bool bScaleByDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    float MinScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    float MaxScale;

    FConnectionVisualData()
    {
        ConnectionMesh = nullptr;
        ConnectionMaterial = nullptr;
        MeshScale = FVector(1.0f, 1.0f, 1.0f);
        bScaleByDistance = true;
        MinScale = 0.5f;
        MaxScale = 2.0f;
    }
};

UCLASS(Blueprintable)
class MYPROJECT_API ANodeConnection : public AActor
{
    GENERATED_BODY()

public:
    ANodeConnection();

    // 连接的节点
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connection|Nodes")
    AInteractiveNode* SourceNode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connection|Nodes")
    AInteractiveNode* TargetNode;

    // 连接属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Properties")
    ENodeRelationType RelationType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Properties", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ConnectionWeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Properties")
    bool bIsActive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Properties")
    bool bIsBidirectional;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Properties")
    FGameplayTagContainer ConnectionTags;

    // 视觉组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connection|Components")
    UStaticMeshComponent* ConnectionMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Connection|Components")
    UWidgetComponent* ConnectionInfoWidget;

    // 视觉属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Visual")
    FLinearColor BaseColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Visual")
    FLinearColor ActiveColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Visual")
    float ConnectionThickness;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Visual")
    bool bAnimateConnection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Visual")
    FConnectionVisualData VisualData;

    // 连接数据
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Data")
    FString ConnectionID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Data")
    TMap<FString, FString> ConnectionMetadata;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Data", meta = (ClampMin = "0.0"))
    float ActivationDelay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection|Data", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ConnectionStrength;

public:
    // 委托
    UPROPERTY(BlueprintAssignable, Category = "Connection|Events")
    FOnConnectionActivated OnConnectionActivated;

    UPROPERTY(BlueprintAssignable, Category = "Connection|Events")
    FOnConnectionDeactivated OnConnectionDeactivated;

    UPROPERTY(BlueprintAssignable, Category = "Connection|Events")
    FOnConnectionPulsed OnConnectionPulsed;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void Tick(float DeltaTime) override;

    // 初始化和设置
    UFUNCTION(BlueprintCallable, Category = "Connection|Setup")
    void Initialize(AInteractiveNode* Source, AInteractiveNode* Target, ENodeRelationType Type);

    UFUNCTION(BlueprintCallable, Category = "Connection|Setup")
    void SetConnectionWeight(float Weight);

    UFUNCTION(BlueprintCallable, Category = "Connection|Setup")
    void SetBidirectional(bool bBidirectional);

    // 查询方法
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Connection|Query")
    bool IsValid() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Connection|Query")
    bool IsConnecting(AInteractiveNode* NodeA, AInteractiveNode* NodeB) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Connection|Query")
    AInteractiveNode* GetOppositeNode(AInteractiveNode* FromNode) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Connection|Query")
    float GetConnectionDistance() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Connection|Query")
    AInteractiveNode* GetSourceNode() const { return SourceNode; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Connection|Query")
    AInteractiveNode* GetTargetNode() const { return TargetNode; }

    // 状态管理
    UFUNCTION(BlueprintCallable, Category = "Connection|State")
    void Activate();

    UFUNCTION(BlueprintCallable, Category = "Connection|State")
    void Deactivate();

    UFUNCTION(BlueprintCallable, Category = "Connection|State")
    void UpdateConnection();

    // 传播功能
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection|Propagation")
    bool CanPropagateState(ENodeState State) const;
    virtual bool CanPropagateState_Implementation(ENodeState State) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection|Propagation")
    bool CanPropagateInteraction() const;
    virtual bool CanPropagateInteraction_Implementation() const;

    UFUNCTION(BlueprintCallable, Category = "Connection|Propagation")
    void PropagateState(AInteractiveNode* FromNode, ENodeState NewState);

    UFUNCTION(BlueprintCallable, Category = "Connection|Propagation")
    void PropagateInteraction(AInteractiveNode* FromNode, const FInteractionData& Data);

    // 视觉相关
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Connection|Visual")
    FLinearColor GetConnectionColor() const;
    virtual FLinearColor GetConnectionColor_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Connection|Visual")
    FText GetConnectionDescription() const;
    virtual FText GetConnectionDescription_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Connection|Visual")
    bool ShouldShowConnection() const;
    virtual bool ShouldShowConnection_Implementation() const;

protected:
    // 内部方法
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection|Internal")
    void UpdateVisuals();
    virtual void UpdateVisuals_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection|Internal")
    void UpdateEffects();
    virtual void UpdateEffects_Implementation();

    void CalculateConnectionPoints(FVector& OutStart, FVector& OutEnd) const;

    // 事件处理
    UFUNCTION()
    void OnSourceNodeStateChanged(AInteractiveNode* Node, ENodeState OldState, ENodeState NewState);

    UFUNCTION()
    void OnTargetNodeStateChanged(AInteractiveNode* Node, ENodeState OldState, ENodeState NewState);

    UFUNCTION()
    void OnNodeInteracted(AInteractiveNode* Node, const FInteractionData& Data);

    UFUNCTION()
    void OnNodeDestroyed(AActor* DestroyedActor);

    // 辅助方法
    void RegisterNodeEvents();
    void UnregisterNodeEvents();
    bool ValidateConnection() const;
    void HandleNodeStateChange(AInteractiveNode* ChangedNode, ENodeState NewState);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Connection|Internal")
    void ApplyRelationTypeRules();
    virtual void ApplyRelationTypeRules_Implementation();

private:
    // 动画相关
    float CurrentAnimationTime;
    bool bIsAnimating;
};