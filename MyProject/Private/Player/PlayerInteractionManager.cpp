// Fill out your copyright notice in the Description page of Project Settings.

// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/PlayerInteractionManager.h"
#include "Nodes/InteractiveNode.h"
#include "Nodes/Capabilities/InteractiveCapability.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Nodes/ItemNode.h"

UPlayerInteractionManager::UPlayerInteractionManager()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // 默认配置
    InteractionRange = 2000.0f;
    HoldDuration = 0.8f;
    DragThreshold = 10.0f;
    bEnableDebugTrace = false;
    TraceChannel = ECC_Visibility;

    // 初始状态
    CurrentState = EInteractionState::None;
    CurrentSelectedNode = nullptr;
    CurrentHoveredNode = nullptr;
    LastMousePosition = FVector2D::ZeroVector;
    MouseDownPosition = FVector2D::ZeroVector;
    HoldTimer = 0.0f;

    // 内部状态
    bIsMouseButtonDown = false;
    bHoldTimerStarted = false;
    DragStartLocation = FVector::ZeroVector;
    LastDragLocation = FVector::ZeroVector;

    CachedPlayerController = nullptr;
    CachedCamera = nullptr;
}

void UPlayerInteractionManager::BeginPlay()
{
    Super::BeginPlay();
    
    // 缓存组件引用
    CacheComponents();
    
    UE_LOG(LogTemp, Log, TEXT("PlayerInteractionManager initialized"));
}

void UPlayerInteractionManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CachedPlayerController)
    {
        CacheComponents();
        return;
    }

    // 获取当前鼠标位置
    FVector2D CurrentMousePosition;
    if (CachedPlayerController->GetMousePosition(CurrentMousePosition.X, CurrentMousePosition.Y))
    {
        LastMousePosition = CurrentMousePosition;
        
        // 更新不同状态
        UpdateHoverState(DeltaTime);
        UpdateHoldState(DeltaTime);
        UpdateDragState(DeltaTime);
    }
}

void UPlayerInteractionManager::OnMouseButtonDown(bool bIsLeftButton)
{
    if (!bIsLeftButton || bIsMouseButtonDown)
    {
        return;
    }

    bIsMouseButtonDown = true;
    MouseDownPosition = LastMousePosition;
    
    // 检测鼠标下的节点
    AItemNode* HitNode = TraceForInteractiveNode(LastMousePosition);
    
    if (HitNode)
    {
        // 检查是否可以选择
        if (CanNodeBeInteracted(HitNode, EInteractionType::Click))
        {
            // 开始点击交互
            FVector WorldLocation;
            FVector WorldDirection;
            if (ScreenToWorldTrace(LastMousePosition, WorldLocation, WorldDirection))
            {
                StartInteraction(HitNode, EInteractionType::Click);
                
                // 选择节点
                SelectNode(HitNode);
                
                // 开始长按计时
                if (CanNodeBeInteracted(HitNode, EInteractionType::Hold))
                {
                    bHoldTimerStarted = true;
                    HoldTimer = 0.0f;
                    SetInteractionState(EInteractionState::Selecting);
                }
            }
        }
    }
    else
    {
        // 点击空白区域，取消选择
        DeselectCurrentNode();
    }
}

void UPlayerInteractionManager::OnMouseButtonUp(bool bIsLeftButton)
{
    if (!bIsLeftButton || !bIsMouseButtonDown)
    {
        return;
    }

    bIsMouseButtonDown = false;
    bHoldTimerStarted = false;
    
    // 计算鼠标移动距离
    float MouseMoveDistance = GetDistanceFromMouseDown();
    
    // 根据当前状态处理
    if (CurrentState == EInteractionState::Dragging)
    {
        // 结束拖拽
        if (CurrentSelectedNode)
        {
            EndDragging(CurrentSelectedNode);
            EndInteraction(CurrentSelectedNode, EInteractionType::Drag);
        }
        SetInteractionState(EInteractionState::Selected);
    }
    else if (CurrentState == EInteractionState::Holding)
    {
        // 结束长按
        if (CurrentSelectedNode)
        {
            EndInteraction(CurrentSelectedNode, EInteractionType::Hold);
        }
        SetInteractionState(EInteractionState::Selected);
    }
    else if (CurrentState == EInteractionState::Selecting)
    {
        // 完成点击
        if (CurrentSelectedNode && MouseMoveDistance < DragThreshold)
        {
            // 处理点击交互
            FVector WorldLocation;
            FVector WorldDirection;
            if (ScreenToWorldTrace(LastMousePosition, WorldLocation, WorldDirection))
            {
                ProcessInteraction(CurrentSelectedNode, EInteractionType::Click, WorldLocation);
                EndInteraction(CurrentSelectedNode, EInteractionType::Click);
            }
        }
        SetInteractionState(EInteractionState::Selected);
    }
    
    HoldTimer = 0.0f;
}

void UPlayerInteractionManager::OnMouseMove(const FVector2D& MousePosition)
{
    LastMousePosition = MousePosition;
    
    // 如果鼠标按下且移动距离足够，开始拖拽
    if (bIsMouseButtonDown && CurrentSelectedNode && CurrentState == EInteractionState::Selecting)
    {
        float MouseMoveDistance = GetDistanceFromMouseDown();
        
        if (MouseMoveDistance > DragThreshold)
        {
            if (CanNodeBeInteracted(CurrentSelectedNode, EInteractionType::Drag))
            {
                // 开始拖拽
                StartDragging(CurrentSelectedNode);
                StartInteraction(CurrentSelectedNode, EInteractionType::Drag);
                SetInteractionState(EInteractionState::Dragging);
                bHoldTimerStarted = false; // 停止长按计时
            }
        }
    }
}

bool UPlayerInteractionManager::SelectNode(AItemNode* Node)
{
    if (!Node || Node == CurrentSelectedNode)
    {
        return false;
    }

    // 取消当前选择
    DeselectCurrentNode();

    // 选择新节点
    CurrentSelectedNode = Node;
    SetInteractionState(EInteractionState::Selected);

    // 广播事件
    OnNodeSelected.Broadcast(Node);

    UE_LOG(LogTemp, Log, TEXT("Selected node: %s"), *Node->GetNodeName());
    return true;
}

void UPlayerInteractionManager::DeselectCurrentNode()
{
    if (!CurrentSelectedNode)
    {
        return;
    }

    AItemNode* PreviousNode = CurrentSelectedNode;
    CurrentSelectedNode = nullptr;
    SetInteractionState(EInteractionState::None);

    // 广播事件
    OnNodeDeselected.Broadcast(PreviousNode);

    UE_LOG(LogTemp, Log, TEXT("Deselected node: %s"), *PreviousNode->GetNodeName());
}

AItemNode* UPlayerInteractionManager::TraceForInteractiveNode(const FVector2D& ScreenPosition) const
{
    if (!CachedPlayerController)
    {
        return nullptr;
    }

    FVector WorldLocation, WorldDirection;
    if (!ScreenToWorldTrace(ScreenPosition, WorldLocation, WorldDirection))
    {
        return nullptr;
    }

    // 执行射线检测
    FVector TraceStart = WorldLocation;
    FVector TraceEnd = WorldLocation + (WorldDirection * InteractionRange);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(GetOwner());

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        TraceChannel,
        QueryParams
    );

    // 调试绘制
    if (bEnableDebugTrace)
    {
        FColor TraceColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, TraceColor, false, 0.1f, 0, 2.0f);
        
        if (bHit)
        {
            DrawDebugSphere(GetWorld(), HitResult.Location, 20.0f, 12, FColor::Yellow, false, 0.1f);
        }
    }

    if (bHit)
    {
        // 检查命中的Actor是否是InteractiveNode
        AItemNode* HitNode = Cast<AItemNode>(HitResult.GetActor());
        if (HitNode && HitNode->bIsInteractable)
        {
            return HitNode;
        }
    }

    return nullptr;
}

bool UPlayerInteractionManager::ScreenToWorldTrace(const FVector2D& ScreenPosition, FVector& WorldLocation, FVector& WorldDirection) const
{
    if (!CachedPlayerController)
    {
        return false;
    }

    return UGameplayStatics::DeprojectScreenToWorld(
        CachedPlayerController,
        ScreenPosition,
        WorldLocation,
        WorldDirection
    );
}

FInteractionResult UPlayerInteractionManager::ProcessInteraction(AItemNode* Node, EInteractionType Type, const FVector& Location)
{
    FInteractionResult Result;
    Result.TargetNode = Node;
    Result.InteractionType = Type;
    Result.InteractionLocation = Location;

    if (!Node)
    {
        Result.bSuccess = false;
        return Result;
    }

    // 创建交互数据
    FInteractionData InteractionData = CreateInteractionData(Type, Location);

    // 检查节点是否可以交互
    if (!Node->CanInteract(InteractionData))
    {
        Result.bSuccess = false;
        return Result;
    }

    // 执行交互
    Node->OnInteract(InteractionData);
    Result.bSuccess = true;

    UE_LOG(LogTemp, Log, TEXT("Processed interaction: %s on %s"), 
        *UEnum::GetValueAsString(Type), *Node->GetNodeName());

    return Result;
}

bool UPlayerInteractionManager::CanNodeBeInteracted(AItemNode* Node, EInteractionType Type) const
{
    if (!Node || !Node->bIsInteractable)
    {
        return false;
    }

    // 检查节点是否有InteractiveCapability
    bool CanInteractive = Node->HasCapability(UInteractiveCapability::StaticClass()) ;

    return CanInteractive;
}

FVector2D UPlayerInteractionManager::GetMouseDelta() const
{
    return LastMousePosition - MouseDownPosition;
}

float UPlayerInteractionManager::GetDistanceFromMouseDown() const
{
    return FVector2D::Distance(LastMousePosition, MouseDownPosition);
}

void UPlayerInteractionManager::UpdateHoverState(float DeltaTime)
{
    // 检测当前鼠标下的节点
    AItemNode* HoveredNode = TraceForInteractiveNode(LastMousePosition);

    if (HoveredNode != CurrentHoveredNode)
    {
        // 悬停状态发生变化
        if (CurrentHoveredNode)
        {
            HandleNodeUnhover();
        }

        if (HoveredNode && CanNodeBeInteracted(HoveredNode, EInteractionType::Hover))
        {
            HandleNodeHover(HoveredNode, LastMousePosition);
        }
    }
}

void UPlayerInteractionManager::UpdateHoldState(float DeltaTime)
{
    if (!bHoldTimerStarted || !CurrentSelectedNode)
    {
        return;
    }

    HoldTimer += DeltaTime;

    if (HoldTimer >= HoldDuration && CurrentState == EInteractionState::Selecting)
    {
        // 触发长按
        if (CanNodeBeInteracted(CurrentSelectedNode, EInteractionType::Hold))
        {
            FVector WorldLocation;
            FVector WorldDirection;
            if (ScreenToWorldTrace(LastMousePosition, WorldLocation, WorldDirection))
            {
                StartInteraction(CurrentSelectedNode, EInteractionType::Hold);
                ProcessInteraction(CurrentSelectedNode, EInteractionType::Hold, WorldLocation);
                SetInteractionState(EInteractionState::Holding);
            }
        }
        bHoldTimerStarted = false;
    }
}

void UPlayerInteractionManager::UpdateDragState(float DeltaTime)
{
    if (CurrentState != EInteractionState::Dragging || !CurrentSelectedNode)
    {
        return;
    }

    // 更新拖拽位置
    FVector WorldLocation;
    FVector WorldDirection;
    if (ScreenToWorldTrace(LastMousePosition, WorldLocation, WorldDirection))
    {
        UpdateDragging(CurrentSelectedNode, WorldLocation);
    }
}

void UPlayerInteractionManager::StartInteraction(AItemNode* Node, EInteractionType Type)
{
    if (!Node)
    {
        return;
    }

    OnNodeInteractionStarted.Broadcast(Node, Type);
    UE_LOG(LogTemp, Log, TEXT("Started interaction: %s on %s"), 
        *UEnum::GetValueAsString(Type), *Node->GetNodeName());
}

void UPlayerInteractionManager::EndInteraction(AItemNode* Node, EInteractionType Type)
{
    if (!Node)
    {
        return;
    }

    OnNodeInteractionEnded.Broadcast(Node, Type);
    UE_LOG(LogTemp, Log, TEXT("Ended interaction: %s on %s"), 
        *UEnum::GetValueAsString(Type), *Node->GetNodeName());
}

void UPlayerInteractionManager::SetInteractionState(EInteractionState NewState)
{
    if (CurrentState != NewState)
    {
        EInteractionState OldState = CurrentState;
        CurrentState = NewState;
        
        UE_LOG(LogTemp, Verbose, TEXT("Interaction state changed: %d -> %d"), 
            (int32)OldState, (int32)NewState);
    }
}

void UPlayerInteractionManager::HandleNodeHover(AItemNode* Node, const FVector2D& MousePosition)
{
    CurrentHoveredNode = Node;
    
    FVector WorldLocation;
    FVector WorldDirection;
    ScreenToWorldTrace(MousePosition, WorldLocation, WorldDirection);
    
    OnNodeHoverStarted.Broadcast(Node, WorldLocation);
    UE_LOG(LogTemp, Verbose, TEXT("Started hovering over node: %s"), *Node->GetNodeName());
}

void UPlayerInteractionManager::HandleNodeUnhover()
{
    if (CurrentHoveredNode)
    {
        OnNodeHoverEnded.Broadcast(CurrentHoveredNode);
        UE_LOG(LogTemp, Verbose, TEXT("Stopped hovering over node: %s"), *CurrentHoveredNode->GetNodeName());
        CurrentHoveredNode = nullptr;
    }
}

bool UPlayerInteractionManager::CheckInteractionPermission(AItemNode* Node, EInteractionType Type) const
{
    // 基础检查
    if (!Node || !Node->bIsInteractable)
    {
        return false;
    }

    // 状态检查
    ENodeState NodeState = Node->GetNodeState();
    if (NodeState == ENodeState::Hidden || NodeState == ENodeState::Locked)
    {
        return false;
    }

    return true;
}

FInteractionData UPlayerInteractionManager::CreateInteractionData(EInteractionType Type, const FVector& Location) const
{
    FInteractionData Data;
    Data.InteractionType = Type;
    Data.Instigator = CachedPlayerController;
    Data.InteractionLocation = Location;
    Data.InteractionDuration = (Type == EInteractionType::Hold) ? HoldTimer : 0.0f;
    
    // 添加上下文信息
    Data.InteractionContext.Add(TEXT("Source"), TEXT("PlayerInteractionManager"));
    Data.InteractionContext.Add(TEXT("MousePosition"), LastMousePosition.ToString());
    
    return Data;
}

void UPlayerInteractionManager::StartDragging(AItemNode* Node)
{
    if (!Node)
    {
        return;
    }

    DragStartLocation = Node->GetActorLocation();
    LastDragLocation = DragStartLocation;
    
    UE_LOG(LogTemp, Log, TEXT("Started dragging node: %s"), *Node->GetNodeName());
}

void UPlayerInteractionManager::UpdateDragging(AItemNode* Node, const FVector& NewLocation)
{
    if (!Node)
    {
        return;
    }

    // 计算拖拽偏移
    FVector CameraForward = CachedPlayerController->PlayerCameraManager->GetCameraRotation().Vector();
    FVector ProjectedLocation = NewLocation + CameraForward * 500.0f; // 投射到相机前方500单位

    // 更新节点位置
    Node->SetActorLocation(ProjectedLocation);
    LastDragLocation = ProjectedLocation;

    // 处理拖拽交互
    ProcessInteraction(Node, EInteractionType::Drag, ProjectedLocation);
}

void UPlayerInteractionManager::EndDragging(AItemNode* Node)
{
    if (!Node)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Ended dragging node: %s at %s"), 
        *Node->GetNodeName(), *Node->GetActorLocation().ToString());
}

APlayerController* UPlayerInteractionManager::GetPlayerController() const
{
    if (CachedPlayerController)
    {
        return CachedPlayerController;
    }

    return UGameplayStatics::GetPlayerController(GetWorld(), 0);
}

UCameraComponent* UPlayerInteractionManager::GetPlayerCamera() const
{
    if (CachedCamera)
    {
        return CachedCamera;
    }

    APlayerController* PC = GetPlayerController();
    if (PC && PC->GetPawn())
    {
        return PC->GetPawn()->FindComponentByClass<UCameraComponent>();
    }

    return nullptr;
}

void UPlayerInteractionManager::CacheComponents()
{
    CachedPlayerController = GetPlayerController();
    CachedCamera = GetPlayerCamera();
}