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
    InteractionRange = 10000.0f;
    HoldDuration = 0.8f;
    DragThreshold = 10.0f;
    bEnableDebugTrace = false;
    TraceChannel = ECC_Visibility;
    bUseScreenCenterForInteraction = true; // 默认使用屏幕中心
    ItemDistance = 300.f;

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

    // 如果使用屏幕中心模式，创建并显示准星UI
    if (bUseScreenCenterForInteraction && CrosshairWidgetClass && CachedPlayerController)
    {
        CrosshairWidget = CreateWidget<UUserWidget>(CachedPlayerController, CrosshairWidgetClass);
        if (CrosshairWidget)
        {
            CrosshairWidget->AddToViewport(50); 
            UE_LOG(LogTemp, Log, TEXT("Crosshair UI created and added to viewport"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create Crosshair UI widget"));
        }
    }
    
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

FVector2D UPlayerInteractionManager::GetScreenCenter() const
{
    if (!CachedPlayerController)
    {
        return FVector2D::ZeroVector;
    }
    
    int32 ViewportSizeX, ViewportSizeY;
    CachedPlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    
    return FVector2D(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
}

void UPlayerInteractionManager::OnMouseButtonDown(bool bIsLeftButton)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Mouse Button Down ==="));
    
    if (!bIsLeftButton || bIsMouseButtonDown)
    {
        return;
    }

    bIsMouseButtonDown = true;
    
    // 根据配置选择检测位置
    FVector2D InteractionPosition;
    AItemNode* HitNode = nullptr;
    
    if (bUseScreenCenterForInteraction)
    {
        InteractionPosition = GetScreenCenter();
        HitNode = TraceFromScreenCenter();
    }
    else
    {
        InteractionPosition = LastMousePosition;
        HitNode = TraceForInteractiveNode(LastMousePosition);
    }
    
    MouseDownPosition = InteractionPosition; // 更新记录的位置
    
    if (HitNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("Found HitNode: %s"), *HitNode->GetNodeName());
        
        // 检查是否可以选择
        bool bCanInteract = CanNodeBeInteracted(HitNode, EInteractionType::Click);
        
        if (bCanInteract)
        {
            FVector WorldLocation;
            FVector WorldDirection;
            if (ScreenToWorldTrace(InteractionPosition, WorldLocation, WorldDirection))
            {
                StartInteraction(HitNode, EInteractionType::Click);
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

AItemNode* UPlayerInteractionManager::TraceFromScreenCenter() const
{
    FVector2D ScreenCenter = GetScreenCenter();
    return TraceForInteractiveNode(ScreenCenter);
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

    UE_LOG(LogTemp, Warning, TEXT("Trace from %s to %s"), *TraceStart.ToString(), *TraceEnd.ToString());
    UE_LOG(LogTemp, Warning, TEXT("Hit result: %s"), bHit ? TEXT("TRUE") : TEXT("FALSE"));

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

        AActor* HitActor = HitResult.GetActor();
        UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));
        UE_LOG(LogTemp, Warning, TEXT("Hit Actor Class: %s"), HitActor ? *HitActor->GetClass()->GetName() : TEXT("None"));
      
        // 检查命中的Actor是否是InteractiveNode
        AItemNode* HitNode = Cast<AItemNode>(HitResult.GetActor());
        
        if (HitNode && HitNode->bIsInteractable)
        {
            UE_LOG(LogTemp, Warning, TEXT("Found InteractiveNode: %s"), *HitNode->GetNodeName());
            return HitNode;
        }else
        {
            UE_LOG(LogTemp, Warning, TEXT("No InteractiveNode found"));
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

    bool bSuccess = UGameplayStatics::DeprojectScreenToWorld(
        CachedPlayerController,
        ScreenPosition,
        WorldLocation,
        WorldDirection
    );

    // UE_LOG(LogTemp, Warning, TEXT("Screen Position: %s"), *ScreenPosition.ToString());
    // UE_LOG(LogTemp, Warning, TEXT("World Location: %s"), *WorldLocation.ToString());
    // UE_LOG(LogTemp, Warning, TEXT("World Direction: %s"), *WorldDirection.ToString());
    // UE_LOG(LogTemp, Warning, TEXT("Deproject Success: %s"), bSuccess ? TEXT("TRUE") : TEXT("FALSE"));
    return bSuccess;
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
    AItemNode* HoveredNode = nullptr;
    FVector2D HoverPosition;
    
    if (bUseScreenCenterForInteraction)
    {
        HoverPosition = GetScreenCenter();
        HoveredNode = TraceFromScreenCenter();
    }
    else
    {
        HoverPosition = LastMousePosition;
        HoveredNode = TraceForInteractiveNode(LastMousePosition);
    }

    if (HoveredNode != CurrentHoveredNode)
    {
        if (CurrentHoveredNode)
        {
            HandleNodeUnhover();
        }

        if (HoveredNode && CanNodeBeInteracted(HoveredNode, EInteractionType::Hover))
        {
            HandleNodeHover(HoveredNode, HoverPosition);
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

    // 如果使用屏幕中心模式，直接将节点放在相机前方
    if (bUseScreenCenterForInteraction)
    {
        FVector CameraLocation = CachedPlayerController->PlayerCameraManager->GetCameraLocation();
        FVector CameraForward = CachedPlayerController->PlayerCameraManager->GetCameraRotation().Vector();
        FVector TargetLocation = CameraLocation + CameraForward * ItemDistance;
        
        Node->SetActorLocation(TargetLocation);
        LastDragLocation = TargetLocation;
        
        UE_LOG(LogTemp, Verbose, TEXT("Dragging node to camera center position: %s"), *TargetLocation.ToString());
    }
    else
    {
        // 使用原来的鼠标位置计算逻辑
        FVector CameraForward = CachedPlayerController->PlayerCameraManager->GetCameraRotation().Vector();
        FVector ProjectedLocation = NewLocation + CameraForward * ItemDistance;
        
        Node->SetActorLocation(ProjectedLocation);
        LastDragLocation = ProjectedLocation;
    }

    // 处理拖拽交互
    ProcessInteraction(Node, EInteractionType::Drag, Node->GetActorLocation());
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