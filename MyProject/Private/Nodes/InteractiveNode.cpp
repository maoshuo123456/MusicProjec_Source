// Fill out your copyright notice in the Description page of Project Settings.

#include "Nodes/InteractiveNode.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AInteractiveNode::AInteractiveNode()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建根组件
    NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
    RootComponent = NodeMesh;
    NodeMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    NodeMesh->SetCollisionResponseToAllChannels(ECR_Block);
    NodeMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    // 创建交互体积
    InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
    InteractionVolume->SetupAttachment(RootComponent);
    InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionVolume->SetBoxExtent(FVector(150.0f, 150.0f, 150.0f));

    // 创建UI组件
    InfoWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InfoWidget"));
    InfoWidgetComponent->SetupAttachment(RootComponent);
    InfoWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    InfoWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    InfoWidgetComponent->SetDrawSize(FVector2D(200.0f, 100.0f));
    InfoWidgetComponent->SetVisibility(false);

    // 默认值
    bIsInteractable = true;
    InteractionRange = 100000.0f;
    UIDisplayDistance = 1000.0f;
    bAlwaysShowUI = false;
    CurrentState = ENodeState::Inactive;
}

void AInteractiveNode::BeginPlay()
{
    Super::BeginPlay();

    // 应用初始状态
    if (NodeData.InitialState != CurrentState)
    {
        SetNodeState(NodeData.InitialState);
    }

    // 创建UI
    CreateNodeUI();

    // 开始UI可见性检查
    if (!bAlwaysShowUI)
    {
        GetWorld()->GetTimerManager().SetTimer(
            UIUpdateTimerHandle,
            this,
            &AInteractiveNode::CheckUIVisibility,
            0.2f,
            true
        );
    }
    else
    {
        SetUIVisibility(true);
    }

    UpdateVisuals();
}

void AInteractiveNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理定时器
    if (UIUpdateTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(UIUpdateTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void AInteractiveNode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AInteractiveNode::Initialize(const FNodeData& InNodeData)
{
    NodeData = InNodeData;
    
    // 设置Actor标签
    Tags.Empty();
    for (const FGameplayTag& Tag : NodeData.NodeTags)
    {
        Tags.Add(Tag.GetTagName());
    }

    // 应用初始状态
    SetNodeState(NodeData.InitialState);
    
    // 更新UI
    UpdateNodeUI();
}

void AInteractiveNode::SetNodeState(ENodeState NewState)
{
    if (CurrentState != NewState)
    {
        ENodeState OldState = CurrentState;
        CurrentState = NewState;
        
        OnStateChanged(OldState, NewState);
        BroadcastStateChange(OldState, NewState);
        
        UpdateVisuals();
        UpdateNodeUI();
    }
}

bool AInteractiveNode::CanInteract_Implementation(const FInteractionData& Data) const
{
    // 基础检查
    if (!bIsInteractable || CurrentState == ENodeState::Hidden)
    {
        return false;
    }

    // 验证交互
    return ValidateInteraction(Data);
}

void AInteractiveNode::OnInteract_Implementation(const FInteractionData& Data)
{
    if (!CanInteract(Data))
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Node %s interacted by %s"), 
        *NodeData.NodeName, 
        Data.Instigator ? *Data.Instigator->GetName() : TEXT("Unknown"));

    // 广播交互事件
    BroadcastInteraction(Data);

    // 检查是否触发故事
    if (ShouldTriggerStory())
    {
        OnStoryTriggered();
    }
}

void AInteractiveNode::OnStartInteraction_Implementation(const FInteractionData& Data)
{
    // 子类可以重写此方法处理交互开始
}

void AInteractiveNode::OnEndInteraction_Implementation(const FInteractionData& Data)
{
    // 子类可以重写此方法处理交互结束
}

void AInteractiveNode::UpdateNodeUI_Implementation()
{
    // 子类可以重写此方法更新UI内容
    if (InfoWidgetComponent && InfoWidgetComponent->GetUserWidgetObject())
    {
        // 这里可以调用UI更新接口
    }
}

void AInteractiveNode::SetUIVisibility(bool bVisible)
{
    if (InfoWidgetComponent)
    {
        InfoWidgetComponent->SetVisibility(bVisible);
    }
}

FText AInteractiveNode::GetInteractionPrompt_Implementation() const
{
    return FText::FromString(FString::Printf(TEXT("Interact with %s"), *NodeData.NodeName));
}

bool AInteractiveNode::ShouldShowUI_Implementation(APlayerController* Player) const
{
    if (!Player || !bIsInteractable || CurrentState == ENodeState::Hidden)
    {
        return false;
    }

    if (bAlwaysShowUI)
    {
        return true;
    }

    // 检查距离
    APawn* PlayerPawn = Player->GetPawn();
    if (PlayerPawn)
    {
        float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
        return Distance <= UIDisplayDistance;
    }

    return false;
}

void AInteractiveNode::SetStoryFragment(const FString& FragmentID)
{
    StoryFragmentID = FragmentID;
}

void AInteractiveNode::AddTriggerEvent(const FString& EventID)
{
    if (!EventID.IsEmpty() && !TriggerEventIDs.Contains(EventID))
    {
        TriggerEventIDs.Add(EventID);
    }
}

bool AInteractiveNode::ShouldTriggerStory_Implementation() const
{
    return !StoryFragmentID.IsEmpty() || TriggerEventIDs.Num() > 0;
}

void AInteractiveNode::OnStoryTriggered_Implementation()
{
    BroadcastStoryTrigger();
}

TArray<FNodeGenerateData> AInteractiveNode::GetNodeSpawnData_Implementation() const
{
    // 基类返回空数组，子类可以重写
    return TArray<FNodeGenerateData>();
}

void AInteractiveNode::OnStateChanged_Implementation(ENodeState OldState, ENodeState NewState)
{
    // 子类可以重写此方法处理状态变化
    UE_LOG(LogTemp, Log, TEXT("Node %s state changed from %d to %d"), 
        *NodeData.NodeName, (int32)OldState, (int32)NewState);
}

bool AInteractiveNode::ValidateInteraction_Implementation(const FInteractionData& Data) const
{
    // 检查交互者
    if (!Data.Instigator)
    {
        return false;
    }

    // 检查距离
    if (InteractionRange > 0.0f)
    {
        APawn* PlayerPawn = Data.Instigator->GetPawn();
        if (PlayerPawn)
        {
            float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
            if (Distance > InteractionRange)
            {
                return false;
            }
        }
    }

    return true;
}

void AInteractiveNode::BroadcastStateChange(ENodeState OldState, ENodeState NewState)
{
    OnNodeStateChanged.Broadcast(this, OldState, NewState);
}

void AInteractiveNode::BroadcastInteraction(const FInteractionData& Data)
{
    OnNodeInteracted.Broadcast(this, Data);
}

void AInteractiveNode::BroadcastStoryTrigger()
{
    OnNodeStoryTriggered.Broadcast(this, TriggerEventIDs);
}

void AInteractiveNode::UpdateVisuals_Implementation()
{
    // 基于状态更新视觉效果
    if (NodeMesh)
    {
        switch (CurrentState)
        {
        case ENodeState::Inactive:
            NodeMesh->SetRenderCustomDepth(false);
            break;
        case ENodeState::Active:
            NodeMesh->SetRenderCustomDepth(true);
            NodeMesh->SetCustomDepthStencilValue(1);
            break;
        case ENodeState::Completed:
            NodeMesh->SetRenderCustomDepth(true);
            NodeMesh->SetCustomDepthStencilValue(2);
            break;
        case ENodeState::Locked:
            NodeMesh->SetRenderCustomDepth(true);
            NodeMesh->SetCustomDepthStencilValue(3);
            break;
        case ENodeState::Hidden:
            NodeMesh->SetVisibility(false);
            break;
        default:
            break;
        }
    }
}

void AInteractiveNode::CreateNodeUI()
{
    if (InfoWidgetClass && InfoWidgetComponent)
    {
        InfoWidgetComponent->SetWidgetClass(InfoWidgetClass);
        UpdateNodeUI();
    }
}

void AInteractiveNode::CheckUIVisibility()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        bool bShouldShow = ShouldShowUI(PC);
        SetUIVisibility(bShouldShow);
    }
}