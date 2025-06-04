// Fill out your copyright notice in the Description page of Project Settings.

// NodeConnection.cpp
#include "Nodes/NodeConnection.h"
#include "Nodes/InteractiveNode.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

ANodeConnection::ANodeConnection()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // 创建根组件
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // 创建连接网格
    ConnectionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConnectionMesh"));
    ConnectionMesh->SetupAttachment(RootComponent);
    ConnectionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ConnectionMesh->SetCastShadow(false);

    // 创建特效组件


    // 创建信息UI组件
    ConnectionInfoWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConnectionInfoWidget"));
    ConnectionInfoWidget->SetupAttachment(RootComponent);
    ConnectionInfoWidget->SetWidgetSpace(EWidgetSpace::Screen);
    ConnectionInfoWidget->SetDrawSize(FVector2D(150.0f, 50.0f));
    ConnectionInfoWidget->SetVisibility(false);

    // 默认值
    RelationType = ENodeRelationType::Dependency;
    ConnectionWeight = 1.0f;
    bIsActive = true;
    bIsBidirectional = false;
    ConnectionStrength = 1.0f;
    ActivationDelay = 0.0f;

    // 视觉默认值
    BaseColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
    ActiveColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f);
    ConnectionThickness = 0.2f;
    bAnimateConnection = false;
    CurrentAnimationTime = 0.0f;
    bIsAnimating = false;

    // 设置默认网格
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    if (DefaultMesh.Succeeded())
    {
        VisualData.ConnectionMesh = DefaultMesh.Object;
        ConnectionMesh->SetStaticMesh(VisualData.ConnectionMesh);
    }
}

void ANodeConnection::BeginPlay()
{
    Super::BeginPlay();

    // 如果已经有源节点和目标节点，进行初始化
    if (SourceNode && TargetNode)
    {
        // 验证连接
        if (!ValidateConnection())
        {
            UE_LOG(LogTemp, Error, TEXT("NodeConnection %s: Invalid connection setup"), *ConnectionID);
            Destroy();
            return;
        }

        // 注册事件
        RegisterNodeEvents();

        // 初始更新
        UpdateConnection();
        UpdateVisuals();

        // 根据关系类型应用规则
        ApplyRelationTypeRules();
    }
}

void ANodeConnection::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 注销事件
    UnregisterNodeEvents();

    Super::EndPlay(EndPlayReason);
}

void ANodeConnection::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新连接位置
    UpdateConnection();

    // 更新动画
    if (bIsAnimating && bAnimateConnection)
    {
        CurrentAnimationTime += DeltaTime;
        UpdateVisuals();
    }
}

void ANodeConnection::Initialize(AInteractiveNode* Source, AInteractiveNode* Target, ENodeRelationType Type)
{
    if (!Source || !Target)
    {
        UE_LOG(LogTemp, Error, TEXT("NodeConnection::Initialize - Invalid nodes provided"));
        return;
    }

    SourceNode = Source;
    TargetNode = Target;
    RelationType = Type;

    // 生成连接ID
    ConnectionID = FString::Printf(TEXT("%s_to_%s_%s"), 
        *Source->GetNodeID(), 
        *Target->GetNodeID(),
        *UEnum::GetValueAsString(Type));

    // 注册事件（如果已经BeginPlay）
    if (HasActorBegunPlay())
    {
        RegisterNodeEvents();
        UpdateConnection();
        UpdateVisuals();
        ApplyRelationTypeRules();
    }

    UE_LOG(LogTemp, Log, TEXT("NodeConnection initialized: %s"), *ConnectionID);
}

void ANodeConnection::SetConnectionWeight(float Weight)
{
    ConnectionWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
    UpdateVisuals();
}

void ANodeConnection::SetBidirectional(bool bBidirectional)
{
    bIsBidirectional = bBidirectional;
    ApplyRelationTypeRules();
}

bool ANodeConnection::IsValid() const
{
    return SourceNode != nullptr && TargetNode != nullptr && bIsActive;
}

bool ANodeConnection::IsConnecting(AInteractiveNode* NodeA, AInteractiveNode* NodeB) const
{
    if (!NodeA || !NodeB || !IsValid())
    {
        return false;
    }

    return (SourceNode == NodeA && TargetNode == NodeB) || 
           (bIsBidirectional && SourceNode == NodeB && TargetNode == NodeA);
}

AInteractiveNode* ANodeConnection::GetOppositeNode(AInteractiveNode* FromNode) const
{
    if (!FromNode || !IsValid())
    {
        return nullptr;
    }

    if (FromNode == SourceNode)
    {
        return TargetNode;
    }
    else if (FromNode == TargetNode)
    {
        return SourceNode;
    }

    return nullptr;
}

float ANodeConnection::GetConnectionDistance() const
{
    if (!IsValid())
    {
        return 0.0f;
    }

    return FVector::Dist(SourceNode->GetActorLocation(), TargetNode->GetActorLocation());
}

void ANodeConnection::Activate()
{
    if (bIsActive)
    {
        return;
    }

    bIsActive = true;
    bIsAnimating = true;
    CurrentAnimationTime = 0.0f;

    // 启用Tick进行动画
    if (bAnimateConnection)
    {
        SetActorTickEnabled(true);
    }

    UpdateVisuals();
    OnConnectionActivated.Broadcast(this);

    UE_LOG(LogTemp, Log, TEXT("NodeConnection %s activated"), *ConnectionID);
}

void ANodeConnection::Deactivate()
{
    if (!bIsActive)
    {
        return;
    }

    bIsActive = false;
    bIsAnimating = false;

    // 禁用Tick
    SetActorTickEnabled(false);

    UpdateVisuals();
    OnConnectionDeactivated.Broadcast(this);

    UE_LOG(LogTemp, Log, TEXT("NodeConnection %s deactivated"), *ConnectionID);
}

void ANodeConnection::UpdateConnection()
{
    if (!IsValid())
    {
        return;
    }

    FVector StartPoint, EndPoint;
    CalculateConnectionPoints(StartPoint, EndPoint);

    // 设置位置为中点
    FVector MidPoint = (StartPoint + EndPoint) * 0.5f;
    SetActorLocation(MidPoint);

    // 计算旋转
    FVector Direction = (EndPoint - StartPoint).GetSafeNormal();
    FRotator Rotation = UKismetMathLibrary::MakeRotFromX(Direction);
    SetActorRotation(Rotation);

    // 计算缩放
    float Distance = FVector::Dist(StartPoint, EndPoint);
    if (ConnectionMesh && VisualData.bScaleByDistance)
    {
        float ScaleFactor = FMath::GetMappedRangeValueClamped(
            FVector2D(100.0f, 2000.0f),
            FVector2D(VisualData.MinScale, VisualData.MaxScale),
            Distance
        );
        
        FVector NewScale = VisualData.MeshScale;
        NewScale.X *= Distance / 100.0f; // 假设默认网格长度为100
        NewScale.Y *= ConnectionThickness;//ScaleFactor * ConnectionThickness;
        NewScale.Z *= ConnectionThickness;//ScaleFactor * ConnectionThickness;
        
        ConnectionMesh->SetRelativeScale3D(NewScale);
    }
}

bool ANodeConnection::CanPropagateState_Implementation(ENodeState State) const
{
    if (!IsValid() || !bIsActive)
    {
        return false;
    }

    // 根据关系类型决定是否传播状态
    switch (RelationType)
    {
    case ENodeRelationType::Dependency:
        // 依赖关系只在源完成时传播
        return State == ENodeState::Completed;
        
    case ENodeRelationType::Trigger:
        // 触发关系在激活时传播
        return State == ENodeState::Active || State == ENodeState::Completed;
        
    case ENodeRelationType::Parent:
        // 父子关系传播所有状态
        return true;
        
    case ENodeRelationType::Prerequisite:
        // 前置条件在完成时传播
        return State == ENodeState::Completed;
        
    case ENodeRelationType::Sequence:
        // 序列关系在完成时传播
        return State == ENodeState::Completed;
        
    case ENodeRelationType::Mutual:
        // 相互关系传播激活和完成
        return State == ENodeState::Active || State == ENodeState::Completed;
        
    default:
        return false;
    }
}

bool ANodeConnection::CanPropagateInteraction_Implementation() const
{
    if (!IsValid() || !bIsActive)
    {
        return false;
    }

    // 某些关系类型可能传播交互
    return RelationType == ENodeRelationType::Parent || 
           RelationType == ENodeRelationType::Mutual;
}

void ANodeConnection::PropagateState(AInteractiveNode* FromNode, ENodeState NewState)
{
    if (!CanPropagateState(NewState))
    {
        return;
    }

    AInteractiveNode* ToNode = GetOppositeNode(FromNode);
    if (!ToNode)
    {
        return;
    }

    // 应用连接权重和强度
    float PropagationStrength = ConnectionWeight * ConnectionStrength;
    
    // 根据关系类型处理传播
    switch (RelationType)
    {
    case ENodeRelationType::Dependency:
    case ENodeRelationType::Prerequisite:
        if (FromNode == SourceNode && NewState == ENodeState::Completed)
        {
            // 检查目标节点的所有前置条件
            // 这里简化处理，实际应该由NodeSystemManager统一管理
            if (ToNode->GetNodeState() == ENodeState::Locked)
            {
                ToNode->SetNodeState(ENodeState::Active);
            }
        }
        break;
        
    case ENodeRelationType::Trigger:
        if (FromNode == SourceNode && NewState == ENodeState::Active)
        {
            ToNode->SetNodeState(ENodeState::Active);
        }
        break;
        
    case ENodeRelationType::Parent:
        // 父子关系可能需要特殊处理
        if (PropagationStrength >= 0.5f) // 只有足够强的连接才传播
        {
            ToNode->SetNodeState(NewState);
        }
        break;
        
    case ENodeRelationType::Sequence:
        if (FromNode == SourceNode && NewState == ENodeState::Completed)
        {
            ToNode->SetNodeState(ENodeState::Active);
        }
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("NodeConnection %s: Propagated state %d from %s to %s"), 
        *ConnectionID, (int32)NewState, *FromNode->GetNodeName(), *ToNode->GetNodeName());
}

void ANodeConnection::PropagateInteraction(AInteractiveNode* FromNode, const FInteractionData& Data)
{
    if (!CanPropagateInteraction())
    {
        return;
    }

    AInteractiveNode* ToNode = GetOppositeNode(FromNode);
    if (!ToNode)
    {
        return;
    }

    // 创建传播的交互数据
    FInteractionData PropagatedData = Data;
    PropagatedData.InteractionContext.Add(TEXT("PropagatedFrom"), FromNode->GetNodeID());
    PropagatedData.InteractionContext.Add(TEXT("ConnectionType"), UEnum::GetValueAsString(RelationType));

    // 传播交互
    ToNode->OnInteract(PropagatedData);

    UE_LOG(LogTemp, Log, TEXT("NodeConnection %s: Propagated interaction from %s to %s"), 
        *ConnectionID, *FromNode->GetNodeName(), *ToNode->GetNodeName());
}

FLinearColor ANodeConnection::GetConnectionColor_Implementation() const
{
    if (!bIsActive)
    {
        return FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);
    }

    // 根据关系类型返回不同颜色
    switch (RelationType)
    {
    case ENodeRelationType::Dependency:
        return FLinearColor(0.8f, 0.4f, 0.0f, 1.0f); // 橙色
    case ENodeRelationType::Trigger:
        return FLinearColor(0.0f, 0.8f, 0.2f, 1.0f); // 绿色
    case ENodeRelationType::Parent:
        return FLinearColor(0.2f, 0.4f, 1.0f, 1.0f); // 蓝色
    case ENodeRelationType::Prerequisite:
        return FLinearColor(1.0f, 0.2f, 0.2f, 1.0f); // 红色
    case ENodeRelationType::Sequence:
        return FLinearColor(0.8f, 0.8f, 0.0f, 1.0f); // 黄色
    case ENodeRelationType::Mutual:
        return FLinearColor(0.8f, 0.2f, 0.8f, 1.0f); // 紫色
    default:
        return BaseColor;
    }
}

FText ANodeConnection::GetConnectionDescription_Implementation() const
{
    FString TypeStr = UEnum::GetValueAsString(RelationType);
    FString Description = FString::Printf(TEXT("%s -> %s (%s)"),
        SourceNode ? *SourceNode->GetNodeName() : TEXT("None"),
        TargetNode ? *TargetNode->GetNodeName() : TEXT("None"),
        *TypeStr);

    return FText::FromString(Description);
}

bool ANodeConnection::ShouldShowConnection_Implementation() const
{
    return IsValid() && bIsActive;
}

void ANodeConnection::UpdateVisuals_Implementation()
{
    if (!ConnectionMesh)
    {
        return;
    }

    // 设置颜色
    FLinearColor CurrentColor = GetConnectionColor();
    
    // 如果正在动画，插值颜色
    if (bIsAnimating && bAnimateConnection)
    {
        float Alpha = FMath::Clamp(CurrentAnimationTime / 1.0f, 0.0f, 1.0f);
        CurrentColor = FLinearColor::LerpUsingHSV(BaseColor, CurrentColor, Alpha);
    }

    // 创建动态材质实例
    UMaterialInstanceDynamic* DynMaterial = ConnectionMesh->CreateAndSetMaterialInstanceDynamic(0);
    if (DynMaterial)
    {
        DynMaterial->SetVectorParameterValue(TEXT("BaseColor"), CurrentColor);
        DynMaterial->SetScalarParameterValue(TEXT("Opacity"), CurrentColor.A);
    }

    // 更新可见性
    ConnectionMesh->SetVisibility(ShouldShowConnection());
}

void ANodeConnection::UpdateEffects_Implementation()
{



}

void ANodeConnection::CalculateConnectionPoints(FVector& OutStart, FVector& OutEnd) const
{
    if (!IsValid())
    {
        OutStart = GetActorLocation();
        OutEnd = GetActorLocation();
        return;
    }

    // 获取节点位置
    OutStart = SourceNode->GetActorLocation();
    OutEnd = TargetNode->GetActorLocation();

    // 可以在这里添加偏移，避免连接线穿过节点
    FVector Direction = (OutEnd - OutStart).GetSafeNormal();
    OutStart += Direction * 50.0f; // 偏移50单位
    OutEnd -= Direction * 50.0f;
}

void ANodeConnection::OnSourceNodeStateChanged(AInteractiveNode* Node, ENodeState OldState, ENodeState NewState)
{
    if (Node != SourceNode)
    {
        return;
    }

    HandleNodeStateChange(Node, NewState);
}

void ANodeConnection::OnTargetNodeStateChanged(AInteractiveNode* Node, ENodeState OldState, ENodeState NewState)
{
    if (Node != TargetNode)
    {
        return;
    }

    HandleNodeStateChange(Node, NewState);
}

void ANodeConnection::OnNodeInteracted(AInteractiveNode* Node, const FInteractionData& Data)
{
    if (!IsValid() || !bIsActive)
    {
        return;
    }

    // 触发视觉反馈
    OnConnectionPulsed.Broadcast(this);

    // 可能传播交互
    if (CanPropagateInteraction())
    {
        PropagateInteraction(Node, Data);
    }
}

void ANodeConnection::OnNodeDestroyed(AActor* DestroyedActor)
{
    if (DestroyedActor == SourceNode || DestroyedActor == TargetNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("NodeConnection %s: Connected node destroyed, destroying connection"), *ConnectionID);
        Destroy();
    }
}

void ANodeConnection::RegisterNodeEvents()
{
    if (SourceNode)
    {
        SourceNode->OnNodeStateChanged.AddDynamic(this, &ANodeConnection::OnSourceNodeStateChanged);
        SourceNode->OnNodeInteracted.AddDynamic(this, &ANodeConnection::OnNodeInteracted);
        SourceNode->OnDestroyed.AddDynamic(this, &ANodeConnection::OnNodeDestroyed);
    }

    if (TargetNode)
    {
        TargetNode->OnNodeStateChanged.AddDynamic(this, &ANodeConnection::OnTargetNodeStateChanged);
        if (bIsBidirectional)
        {
            TargetNode->OnNodeInteracted.AddDynamic(this, &ANodeConnection::OnNodeInteracted);
        }
        TargetNode->OnDestroyed.AddDynamic(this, &ANodeConnection::OnNodeDestroyed);
    }
}

void ANodeConnection::UnregisterNodeEvents()
{
    if (SourceNode)
    {
        SourceNode->OnNodeStateChanged.RemoveDynamic(this, &ANodeConnection::OnSourceNodeStateChanged);
        SourceNode->OnNodeInteracted.RemoveDynamic(this, &ANodeConnection::OnNodeInteracted);
        SourceNode->OnDestroyed.RemoveDynamic(this, &ANodeConnection::OnNodeDestroyed);
    }

    if (TargetNode)
    {
        TargetNode->OnNodeStateChanged.RemoveDynamic(this, &ANodeConnection::OnTargetNodeStateChanged);
        if (bIsBidirectional)
        {
            TargetNode->OnNodeInteracted.RemoveDynamic(this, &ANodeConnection::OnNodeInteracted);
        }
        TargetNode->OnDestroyed.RemoveDynamic(this, &ANodeConnection::OnNodeDestroyed);
    }
}

bool ANodeConnection::ValidateConnection() const
{
    return SourceNode != nullptr && TargetNode != nullptr && SourceNode != TargetNode;
}

void ANodeConnection::HandleNodeStateChange(AInteractiveNode* ChangedNode, ENodeState NewState)
{
    // 更新视觉效果
    UpdateVisuals();

    // 可能需要传播状态
    if (CanPropagateState(NewState))
    {
        // 延迟传播
        if (ActivationDelay > 0.0f)
        {
            FTimerHandle DelayHandle;
            GetWorld()->GetTimerManager().SetTimer(DelayHandle, 
                [this, ChangedNode, NewState]()
                {
                    PropagateState(ChangedNode, NewState);
                }, 
                ActivationDelay, false);
        }
        else
        {
            PropagateState(ChangedNode, NewState);
        }
    }
}

void ANodeConnection::ApplyRelationTypeRules_Implementation()
{
    // 根据关系类型设置默认行为
    switch (RelationType)
    {
    case ENodeRelationType::Parent:
        // 父子关系通常是双向的
        bIsBidirectional = true;
        break;
        
    case ENodeRelationType::Mutual:
        // 相互关系必须是双向的
        bIsBidirectional = true;
        break;
        
    case ENodeRelationType::Sequence:
        // 序列关系是单向的
        bIsBidirectional = false;
        break;
    }

    // 更新视觉效果以反映关系类型
    UpdateVisuals();
}