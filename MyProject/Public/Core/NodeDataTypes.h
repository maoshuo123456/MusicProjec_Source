// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
// NodeDataTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "NodeDataTypes.generated.h"

// 前向声明
class AInteractiveNode;
class ANodeConnection;
class UItemCapability;
class APlayerController;

// 节点状态枚举
UENUM(BlueprintType)
enum class ENodeState : uint8
{
    Inactive    UMETA(DisplayName = "Inactive"),    // 未激活
    Active      UMETA(DisplayName = "Active"),      // 激活
    Completed   UMETA(DisplayName = "Completed"),   // 已完成
    Locked      UMETA(DisplayName = "Locked"),      // 锁定
    Hidden      UMETA(DisplayName = "Hidden")       // 隐藏
};

// 节点类型枚举
UENUM(BlueprintType)
enum class ENodeType : uint8
{
    Scene       UMETA(DisplayName = "Scene"),       // 场景节点
    Item        UMETA(DisplayName = "Item"),        // 物品节点
    Trigger     UMETA(DisplayName = "Trigger"),     // 触发器节点
    Story       UMETA(DisplayName = "Story"),       // 故事节点
    Custom      UMETA(DisplayName = "Custom")       // 自定义节点
};

// 节点关系类型枚举
UENUM(BlueprintType)
enum class ENodeRelationType : uint8
{
    Dependency      UMETA(DisplayName = "Dependency"),      // 依赖关系
    Prerequisite    UMETA(DisplayName = "Prerequisite"),    // 前置条件
    Trigger         UMETA(DisplayName = "Trigger"),         // 触发关系
    Mutual          UMETA(DisplayName = "Mutual"),          // 相互关联
    Parent          UMETA(DisplayName = "Parent"),          // 父子关系
    Sequence        UMETA(DisplayName = "Sequence"),        // 顺序关系
    Emotional       UMETA(DisplayName = "Emotional")        // 情绪关联
};

// 交互类型枚举
UENUM(BlueprintType)
enum class EInteractionType : uint8
{
    Click           UMETA(DisplayName = "Click"),           // 点击
    Hold            UMETA(DisplayName = "Hold"),            // 长按
    Drag            UMETA(DisplayName = "Drag"),            // 拖拽
    Hover           UMETA(DisplayName = "Hover"),           // 悬停
    MultiTouch      UMETA(DisplayName = "MultiTouch"),      // 多点触摸
    Gesture         UMETA(DisplayName = "Gesture")          // 手势
};

// 情绪类型枚举
UENUM(BlueprintType)
enum class EEmotionType : uint8
{
    Neutral     UMETA(DisplayName = "Neutral"),     // 中性
    Joy         UMETA(DisplayName = "Joy"),         // 快乐
    Sadness     UMETA(DisplayName = "Sadness"),     // 悲伤
    Anger       UMETA(DisplayName = "Anger"),       // 愤怒
    Fear        UMETA(DisplayName = "Fear"),        // 恐惧
    Surprise    UMETA(DisplayName = "Surprise"),    // 惊讶
    Disgust     UMETA(DisplayName = "Disgust"),     // 厌恶
    Trust       UMETA(DisplayName = "Trust"),       // 信任
    Anticipation UMETA(DisplayName = "Anticipation") // 期待
};

// 事件类型枚举
UENUM(BlueprintType)
enum class EGameEventType : uint8
{
    NodeInteraction UMETA(DisplayName = "Node Interaction"), // 节点交互
    StateChange     UMETA(DisplayName = "State Change"),     // 状态变化
    StoryProgress   UMETA(DisplayName = "Story Progress"),   // 故事进展
    MusicTrigger    UMETA(DisplayName = "Music Trigger"),    // 音乐触发
    SystemEvent     UMETA(DisplayName = "System Event")      // 系统事件
};

// 节点基础数据
USTRUCT(BlueprintType)
struct FNodeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data")
    FString NodeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data")
    FString NodeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data", meta = (MultiLine = true))
    FText NodeDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data")
    ENodeType NodeType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data")
    ENodeState InitialState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data")
    FGameplayTagContainer NodeTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node Data")
    TMap<FString, FString> CustomProperties;

    FNodeData()
    {
        NodeID = "";
        NodeName = "New Node";
        NodeDescription = FText::GetEmpty();
        NodeType = ENodeType::Item;
        InitialState = ENodeState::Inactive;
    }
};

// 节点关系数据
USTRUCT(BlueprintType)
struct FNodeRelationData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relation")
    FString SourceNodeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relation")
    FString TargetNodeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relation")
    ENodeRelationType RelationType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relation")
    float Weight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relation")
    bool bBidirectional;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relation")
    FGameplayTagContainer RelationTags;

    FNodeRelationData()
    {
        Weight = 1.0f;
        bBidirectional = false;
        RelationType = ENodeRelationType::Dependency;
    }
};

// 交互数据
USTRUCT(BlueprintType)
struct FInteractionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    EInteractionType InteractionType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    APlayerController* Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    FVector InteractionLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    float InteractionDuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
    TMap<FString, FString> InteractionContext;

    FInteractionData()
    {
        InteractionType = EInteractionType::Click;
        Instigator = nullptr;
        InteractionLocation = FVector::ZeroVector;
        InteractionDuration = 0.0f;
    }
};

// 情绪数据
USTRUCT(BlueprintType)
struct FEmotionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Intensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion")
    EEmotionType PrimaryEmotion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion")
    EEmotionType SecondaryEmotion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BlendFactor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emotion")
    FLinearColor EmotionColor;

    FEmotionData()
    {
        Intensity = 0.5f;
        PrimaryEmotion = EEmotionType::Neutral;
        SecondaryEmotion = EEmotionType::Neutral;
        BlendFactor = 0.0f;
        EmotionColor = FLinearColor::White;
    }
};

// 游戏事件数据
USTRUCT(BlueprintType)
struct FGameEventData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FString EventID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    EGameEventType EventType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FString SourceNodeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FString TargetNodeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    FGameplayTagContainer EventTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    TMap<FString, FString> EventParameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
    float EventDelay;

    FGameEventData()
    {
        EventType = EGameEventType::NodeInteraction;
        EventDelay = 0.0f;
    }
};

//能力枚举
UENUM(BlueprintType)
enum class ECapabilityType : uint8
{
    None        = 0 UMETA(DisplayName = "None"),
    Spatial     = 1 UMETA(DisplayName = "Spatial Capability"),     // 空间能力
    State       = 2 UMETA(DisplayName = "State Capability"),       // 状态能力
    Interactive = 3 UMETA(DisplayName = "Interactive Capability"), // 交互能力
    Narrative   = 4 UMETA(DisplayName = "Narrative Capability"),   // 叙事能力
    System      = 5 UMETA(DisplayName = "System Capability"),      // 系统能力
    Numerical   = 6 UMETA(DisplayName = "Numerical Capability")    // 数值能力
};

// 空间能力配置
USTRUCT(BlueprintType)
struct FSpatialCapabilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanContainNodes = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxContainedNodes = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TeleportDestination = FVector::ZeroVector;
};

// 状态能力配置
USTRUCT(BlueprintType)
struct FStateCapabilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ENodeState> PossibleStates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StateChangeRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPropagateThoughDependency = true;
};

// 交互能力配置
USTRUCT(BlueprintType)
struct FInteractiveCapabilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> DialogueOptions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> ObservableInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxAttempts = 3;
};

// 叙事能力配置
USTRUCT(BlueprintType)
struct FNarrativeCapabilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> StoryProgressionPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> AvailableClues;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxMemoryCount = 10;
};

// 系统能力配置
USTRUCT(BlueprintType)
struct FSystemCapabilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TimeScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> ConditionRules;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRelationships = 10;
};

// 数值能力配置
USTRUCT(BlueprintType)
struct FNumericalCapabilityConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PlayerMaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxMentalState = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, float> ResourcePools;
};

// 能力数据（用于动态创建能力）
USTRUCT(BlueprintType)
struct FCapabilityData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    TSubclassOf<UItemCapability> CapabilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    FString CapabilityID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    ECapabilityType CapabilityType = ECapabilityType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    TMap<FString, FString> CapabilityParameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    bool bAutoActivate = true;

    // ========== 六大能力配置 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability|Spatial", 
        meta = (EditCondition = "CapabilityType == ECapabilityType::Spatial"))
    FSpatialCapabilityConfig SpatialConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability|State", 
        meta = (EditCondition = "CapabilityType == ECapabilityType::State"))
    FStateCapabilityConfig StateConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability|Interactive", 
        meta = (EditCondition = "CapabilityType == ECapabilityType::Interactive"))
    FInteractiveCapabilityConfig InteractiveConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability|Narrative", 
        meta = (EditCondition = "CapabilityType == ECapabilityType::Narrative"))
    FNarrativeCapabilityConfig NarrativeConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability|System", 
        meta = (EditCondition = "CapabilityType == ECapabilityType::System"))
    FSystemCapabilityConfig SystemConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability|Numerical", 
        meta = (EditCondition = "CapabilityType == ECapabilityType::Numerical"))
    FNumericalCapabilityConfig NumericalConfig;

    // 辅助函数：获取枚举类型
    ECapabilityType GetCapabilityType() const
    {
        return CapabilityType;
    }

    // 辅助函数：设置枚举类型
    void SetCapabilityType(ECapabilityType Type)
    {
        CapabilityType = Type;
    }

    FCapabilityData()
    {
        CapabilityClass = nullptr;
        bAutoActivate = true;
        CapabilityType = ECapabilityType::None;
    }
};

// 节点生成数据（用于动态生成）
USTRUCT(BlueprintType)
struct FNodeGenerateData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generate")
    FNodeData NodeData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generate")
    TSubclassOf<AInteractiveNode> NodeClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generate")
    FTransform SpawnTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generate")
    TArray<FCapabilityData> Capabilities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generate")
    TArray<FNodeRelationData> Relations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generate")
    FEmotionData EmotionContext;

    FNodeGenerateData()
    {
        NodeClass = nullptr;
        SpawnTransform = FTransform::Identity;
    }
};

// 委托声明
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNodeStateChanged, AInteractiveNode*, Node, ENodeState, OldState, ENodeState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeInteracted, AInteractiveNode*, Node, const FInteractionData&, InteractionData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeRelationChanged, ANodeConnection*, Connection, bool, bAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEmotionChanged, const FEmotionData&, OldEmotion, const FEmotionData&, NewEmotion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameEvent, const FGameEventData&, EventData);

// 节点查询参数
USTRUCT(BlueprintType)
struct FNodeQueryParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    TArray<ENodeType> NodeTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    TArray<ENodeState> NodeStates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    FGameplayTagQuery TagQuery;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    float MaxDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query")
    bool bIncludeInactive;

    FNodeQueryParams()
    {
        MaxDistance = 0.0f; // 0表示不限制距离
        bIncludeInactive = false;
    }
};