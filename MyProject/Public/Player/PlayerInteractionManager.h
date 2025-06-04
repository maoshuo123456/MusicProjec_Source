// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/NodeDataTypes.h"
#include "Engine/World.h"
#include "PlayerInteractionManager.generated.h"

// 前向声明
class AInteractiveNode;
class AItemNode;
class UInteractiveCapability;
class APlayerController;
class UCameraComponent;

// 交互状态枚举
UENUM(BlueprintType)
enum class EInteractionState : uint8
{
	None        UMETA(DisplayName = "None"),        // 无交互
	Hovering    UMETA(DisplayName = "Hovering"),    // 悬停
	Selecting   UMETA(DisplayName = "Selecting"),   // 选择中
	Selected    UMETA(DisplayName = "Selected"),    // 已选中
	Dragging    UMETA(DisplayName = "Dragging"),    // 拖拽中
	Holding     UMETA(DisplayName = "Holding")      // 长按中
};

// 交互结果结构
USTRUCT(BlueprintType)
struct FInteractionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess;

	UPROPERTY(BlueprintReadOnly)
	AItemNode* TargetNode;

	UPROPERTY(BlueprintReadOnly)
	EInteractionType InteractionType;

	UPROPERTY(BlueprintReadOnly)
	FVector InteractionLocation;

	FInteractionResult()
	{
		bSuccess = false;
		TargetNode = nullptr;
		InteractionType = EInteractionType::Click;
		InteractionLocation = FVector::ZeroVector;
	}
};

// 委托声明
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeInteractionStarted, AItemNode*, Node, EInteractionType, Type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeInteractionEnded, AItemNode*, Node, EInteractionType, Type);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeSelected, AItemNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeDeselected, AItemNode*, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeHoverStarted, AItemNode*, Node, const FVector&, MouseLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeHoverEnded, AItemNode*, Node);


UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT_API UPlayerInteractionManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerInteractionManager();

	    // ========== 配置参数 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
    float InteractionRange;                         // 交互范围

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
    float HoldDuration;                             // 长按持续时间

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
    float DragThreshold;                            // 拖拽阈值

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
    bool bEnableDebugTrace;                         // 启用调试射线

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Settings")
    TEnumAsByte<ECollisionChannel> TraceChannel;    // 射线检测通道

    // ========== 当前状态 ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|State")
    EInteractionState CurrentState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|State")
    AItemNode* CurrentSelectedNode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|State")
    AItemNode* CurrentHoveredNode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|State")
    FVector2D LastMousePosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|State")
    FVector2D MouseDownPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|State")
    float HoldTimer;

    // ========== 委托 ==========
    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnNodeInteractionStarted OnNodeInteractionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnNodeInteractionEnded OnNodeInteractionEnded;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnNodeSelected OnNodeSelected;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnNodeDeselected OnNodeDeselected;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnNodeHoverStarted OnNodeHoverStarted;

    UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
    FOnNodeHoverEnded OnNodeHoverEnded;

	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== 主要接口 ==========
	
    UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
    void OnMouseButtonDown(bool bIsLeftButton);

    UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
    void OnMouseButtonUp(bool bIsLeftButton);

    UFUNCTION(BlueprintCallable, Category = "Interaction|Input")
    void OnMouseMove(const FVector2D& MousePosition);

    // ========== 节点选择 ==========
    UFUNCTION(BlueprintCallable, Category = "Interaction|Selection")
    bool SelectNode(AItemNode* Node);

    UFUNCTION(BlueprintCallable, Category = "Interaction|Selection")
    void DeselectCurrentNode();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|Selection")
    AItemNode* GetSelectedNode() const { return CurrentSelectedNode; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|Selection")
    bool HasSelectedNode() const { return CurrentSelectedNode != nullptr; }

    // ========== 射线检测 ==========
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|Trace")
    AItemNode* TraceForInteractiveNode(const FVector2D& ScreenPosition) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|Trace")
    bool ScreenToWorldTrace(const FVector2D& ScreenPosition, FVector& WorldLocation, FVector& WorldDirection) const;

    // ========== 交互处理 ==========
    UFUNCTION(BlueprintCallable, Category = "Interaction|Process")
    FInteractionResult ProcessInteraction(AItemNode* Node, EInteractionType Type, const FVector& Location);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|Process")
    bool CanNodeBeInteracted(AItemNode* Node, EInteractionType Type) const;

    // ========== 状态查询 ==========
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|State")
    bool IsInState(EInteractionState State) const { return CurrentState == State; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|State")
    FVector2D GetMouseDelta() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction|State")
    float GetDistanceFromMouseDown() const;

protected:
    // ========== 内部方法 ==========
    void UpdateHoverState(float DeltaTime);
    void UpdateHoldState(float DeltaTime);
    void UpdateDragState(float DeltaTime);
    
    void StartInteraction(AItemNode* Node, EInteractionType Type);
    void EndInteraction(AItemNode* Node, EInteractionType Type);
    
    void SetInteractionState(EInteractionState NewState);
    void HandleNodeHover(AItemNode* Node, const FVector2D& MousePosition);
    void HandleNodeUnhover();
    
    bool CheckInteractionPermission(AItemNode* Node, EInteractionType Type) const;
    FInteractionData CreateInteractionData(EInteractionType Type, const FVector& Location) const;

    // ========== 拖拽相关 ==========
    void StartDragging(AItemNode* Node);
    void UpdateDragging(AItemNode* Node, const FVector& NewLocation);
    void EndDragging(AItemNode* Node);

private:
    // 缓存的组件引用
    UPROPERTY()
    APlayerController* CachedPlayerController;

    UPROPERTY()
    UCameraComponent* CachedCamera;

    // 内部状态
    bool bIsMouseButtonDown;
    bool bHoldTimerStarted;
    FVector DragStartLocation;
    FVector LastDragLocation;

    // 辅助方法
    APlayerController* GetPlayerController() const;
    UCameraComponent* GetPlayerCamera() const;
    void CacheComponents();
		
};
