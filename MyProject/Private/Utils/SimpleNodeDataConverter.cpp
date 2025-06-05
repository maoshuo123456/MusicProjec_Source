// Fill out your copyright notice in the Description page of Project Settings.

#include "Utils/SimpleNodeDataConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"

bool USimpleNodeDataConverter::ConvertJSONToNodeData(const FString& JSONString, TArray<FNodeGenerateData>& OutNodeData, TArray<FNodeRelationData>& OutRelations)
{
    // 解析JSON
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
        return false;
    }
    
    // 清空输出数组
    OutNodeData.Empty();
    OutRelations.Empty();
    
    // 解析节点数组
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (RootObject->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
        {
            const TSharedPtr<FJsonObject>& NodeObject = NodeValue->AsObject();
            if (NodeObject.IsValid())
            {
                FNodeGenerateData NodeData;
                if (ParseNodeObject(NodeObject, NodeData))
                {
                    OutNodeData.Add(NodeData);
                    UE_LOG(LogTemp, Log, TEXT("Parsed node: %s"), *NodeData.NodeData.NodeID);
                }
            }
        }
    }
    
    // 解析关系数组
    const TArray<TSharedPtr<FJsonValue>>* RelationsArray;
    if (RootObject->TryGetArrayField(TEXT("relations"), RelationsArray))
    {
        for (const TSharedPtr<FJsonValue>& RelationValue : *RelationsArray)
        {
            const TSharedPtr<FJsonObject>& RelationObject = RelationValue->AsObject();
            if (RelationObject.IsValid())
            {
                FNodeRelationData RelationData;
                if (ParseRelationObject(RelationObject, RelationData))
                {
                    OutRelations.Add(RelationData);
                    UE_LOG(LogTemp, Log, TEXT("Parsed relation: %s -> %s"), 
                        *RelationData.SourceNodeID, *RelationData.TargetNodeID);
                }
            }
        }
    }
    
    return OutNodeData.Num() > 0;
}

bool USimpleNodeDataConverter::LoadAndConvertJSONFile(const FString& FilePath, TArray<FNodeGenerateData>& OutNodeData, TArray<FNodeRelationData>& OutRelations)
{
    FString JSONString;
    
    if (!FFileHelper::LoadFileToString(JSONString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *FilePath);
        return false;
    }
    
    return ConvertJSONToNodeData(JSONString, OutNodeData, OutRelations);
}

bool USimpleNodeDataConverter::ParseNodeObject(const TSharedPtr<FJsonObject>& NodeObject, FNodeGenerateData& OutData)
{
    if (!NodeObject.IsValid())
    {
        return false;
    }
    
    // 解析基本属性
    FString ID, Type, Name, State;
    NodeObject->TryGetStringField(TEXT("id"), ID);
    NodeObject->TryGetStringField(TEXT("type"), Type);
    NodeObject->TryGetStringField(TEXT("name"), Name);
    NodeObject->TryGetStringField(TEXT("state"), State);
    
    // 设置节点数据
    OutData.NodeData.NodeID = ID;
    OutData.NodeData.NodeName = Name;
    OutData.NodeData.NodeType = StringToNodeType(Type);
    OutData.NodeData.InitialState = StringToNodeState(State);
    
    // 解析transform
    const TSharedPtr<FJsonObject>* TransformObject;
    if (NodeObject->TryGetObjectField(TEXT("transform"), TransformObject) && TransformObject->IsValid())
    {
        const TSharedPtr<FJsonObject>* LocationObject;
        if ((*TransformObject)->TryGetObjectField(TEXT("location"), LocationObject) && LocationObject->IsValid())
        {
            OutData.SpawnTransform.SetLocation(ParseLocation(*LocationObject));
        }
    }

    // 解析capabilities数组
    const TArray<TSharedPtr<FJsonValue>>* CapabilitiesArray;
    if (NodeObject->TryGetArrayField(TEXT("capabilities"), CapabilitiesArray))
    {
        for (const TSharedPtr<FJsonValue>& CapabilityValue : *CapabilitiesArray)
        {
            const TSharedPtr<FJsonObject>& CapabilityObject = CapabilityValue->AsObject();
            if (CapabilityObject.IsValid())
            {
                FCapabilityData CapabilityData;
                if (ParseCapabilityObject(CapabilityObject, CapabilityData))
                {
                    OutData.Capabilities.Add(CapabilityData);
                    UE_LOG(LogTemp, Log, TEXT("Parsed capability: %s"), *CapabilityData.CapabilityID);
                }
            }
        }
    }
    
    // 根据类型设置默认的NodeClass（这里需要实际的类引用）
    // 在实际使用中，这些类应该通过某种注册机制获取
    OutData.NodeClass = nullptr; // 将在实际使用时设置
    
    return !ID.IsEmpty() && !Name.IsEmpty();
}

bool USimpleNodeDataConverter::ParseRelationObject(const TSharedPtr<FJsonObject>& RelationObject, FNodeRelationData& OutData)
{
    if (!RelationObject.IsValid())
    {
        return false;
    }
    
    FString SourceID, TargetID, RelationType;
    RelationObject->TryGetStringField(TEXT("source_id"), SourceID);
    RelationObject->TryGetStringField(TEXT("target_id"), TargetID);
    RelationObject->TryGetStringField(TEXT("relation_type"), RelationType);
    
    OutData.SourceNodeID = SourceID;
    OutData.TargetNodeID = TargetID;
    OutData.RelationType = StringToRelationType(RelationType);
    
    // 获取权重，默认为1.0
    double Weight = 1.0;
    RelationObject->TryGetNumberField(TEXT("weight"), Weight);
    OutData.Weight = Weight;
    
    // 获取是否双向，默认为false
    RelationObject->TryGetBoolField(TEXT("bidirectional"), OutData.bBidirectional);
    
    return !SourceID.IsEmpty() && !TargetID.IsEmpty();
}

FVector USimpleNodeDataConverter::ParseLocation(const TSharedPtr<FJsonObject>& LocationObject)
{
    double X = 0, Y = 0, Z = 0;
    LocationObject->TryGetNumberField(TEXT("x"), X);
    LocationObject->TryGetNumberField(TEXT("y"), Y);
    LocationObject->TryGetNumberField(TEXT("z"), Z);
    
    return FVector(X, Y, Z);
}

ENodeType USimpleNodeDataConverter::StringToNodeType(const FString& TypeString)
{
    if (TypeString.Equals(TEXT("scene"), ESearchCase::IgnoreCase))
    {
        return ENodeType::Scene;
    }
    else if (TypeString.Equals(TEXT("item"), ESearchCase::IgnoreCase))
    {
        return ENodeType::Item;
    }
    else if (TypeString.Equals(TEXT("trigger"), ESearchCase::IgnoreCase))
    {
        return ENodeType::Trigger;
    }
    else if (TypeString.Equals(TEXT("story"), ESearchCase::IgnoreCase))
    {
        return ENodeType::Story;
    }
    
    return ENodeType::Item; // 默认
}

ENodeState USimpleNodeDataConverter::StringToNodeState(const FString& StateString)
{
    if (StateString.Equals(TEXT("active"), ESearchCase::IgnoreCase))
    {
        return ENodeState::Active;
    }
    else if (StateString.Equals(TEXT("inactive"), ESearchCase::IgnoreCase))
    {
        return ENodeState::Inactive;
    }
    else if (StateString.Equals(TEXT("completed"), ESearchCase::IgnoreCase))
    {
        return ENodeState::Completed;
    }
    else if (StateString.Equals(TEXT("locked"), ESearchCase::IgnoreCase))
    {
        return ENodeState::Locked;
    }
    else if (StateString.Equals(TEXT("hidden"), ESearchCase::IgnoreCase))
    {
        return ENodeState::Hidden;
    }
    
    return ENodeState::Inactive; // 默认
}

ENodeRelationType USimpleNodeDataConverter::StringToRelationType(const FString& RelationString)
{
    if (RelationString.Equals(TEXT("dependency"), ESearchCase::IgnoreCase))
    {
        return ENodeRelationType::Dependency;
    }
    else if (RelationString.Equals(TEXT("prerequisite"), ESearchCase::IgnoreCase))
    {
        return ENodeRelationType::Prerequisite;
    }
    else if (RelationString.Equals(TEXT("trigger"), ESearchCase::IgnoreCase))
    {
        return ENodeRelationType::Trigger;
    }
    else if (RelationString.Equals(TEXT("mutual"), ESearchCase::IgnoreCase))
    {
        return ENodeRelationType::Mutual;
    }
    else if (RelationString.Equals(TEXT("parent"), ESearchCase::IgnoreCase))
    {
        return ENodeRelationType::Parent;
    }
    else if (RelationString.Equals(TEXT("sequence"), ESearchCase::IgnoreCase))
    {
        return ENodeRelationType::Sequence;
    }
    
    return ENodeRelationType::Dependency; // 默认
}

bool USimpleNodeDataConverter::ParseCapabilityObject(const TSharedPtr<FJsonObject>& CapabilityObject, FCapabilityData& OutData)
{
    if (!CapabilityObject.IsValid())
    {
        return false;
    }
    
    // 解析能力类型
    FString TypeString;
    CapabilityObject->TryGetStringField(TEXT("type"), TypeString);
    
    if (TypeString.Equals(TEXT("interactive"), ESearchCase::IgnoreCase))
    {
        OutData.CapabilityType = ECapabilityType::Interactive;
        
        // 解析配置
        const TSharedPtr<FJsonObject>* ConfigObject;
        if (CapabilityObject->TryGetObjectField(TEXT("config"), ConfigObject) && ConfigObject->IsValid())
        {
            ParseInteractiveConfig(*ConfigObject, OutData.InteractiveConfig);
        }
    }
    else if (TypeString.Equals(TEXT("spatial"), ESearchCase::IgnoreCase))
    {
        OutData.CapabilityType = ECapabilityType::Spatial;
        // 可以继续添加其他能力类型的解析
    }
    // 添加其他能力类型...
    
    // 设置通用属性
    OutData.CapabilityID = TypeString + TEXT("_capability");
    OutData.bAutoActivate = true;
    
    return OutData.CapabilityType != ECapabilityType::None;
}

void USimpleNodeDataConverter::ParseInteractiveConfig(const TSharedPtr<FJsonObject>& ConfigObject, FInteractiveCapabilityConfig& OutConfig)
{
    if (!ConfigObject.IsValid())
    {
        return;
    }
    
    // 解析允许的交互类型
    const TArray<TSharedPtr<FJsonValue>>* InteractionsArray;
    if (ConfigObject->TryGetArrayField(TEXT("allowed_interactions"), InteractionsArray))
    {
        OutConfig.AllowedInteractions = ParseInteractionTypes(*InteractionsArray);
    }
    
    // 解析对话选项
    const TSharedPtr<FJsonObject>* DialogueObject;
    if (ConfigObject->TryGetObjectField(TEXT("dialogue_options"), DialogueObject) && DialogueObject->IsValid())
    {
        for (const auto& Pair : (*DialogueObject)->Values)
        {
            FString Value;
            if (Pair.Value->TryGetString(Value))
            {
                OutConfig.DialogueOptions.Add(Pair.Key, Value);
            }
        }
    }
    
    // 解析观察信息
    const TSharedPtr<FJsonObject>* ObservableObject;
    if (ConfigObject->TryGetObjectField(TEXT("observable_info"), ObservableObject) && ObservableObject->IsValid())
    {
        for (const auto& Pair : (*ObservableObject)->Values)
        {
            FString Value;
            if (Pair.Value->TryGetString(Value))
            {
                OutConfig.ObservableInfo.Add(Pair.Key, Value);
            }
        }
    }
    
    // 解析最大尝试次数
    int32 MaxAttempts;
    if (ConfigObject->TryGetNumberField(TEXT("max_attempts"), MaxAttempts))
    {
        OutConfig.MaxAttempts = MaxAttempts;
    }
}

TArray<EInteractionType> USimpleNodeDataConverter::ParseInteractionTypes(const TArray<TSharedPtr<FJsonValue>>& JsonArray)
{
    TArray<EInteractionType> Result;
    
    for (const TSharedPtr<FJsonValue>& Value : JsonArray)
    {
        FString TypeString;
        if (Value->TryGetString(TypeString))
        {
            EInteractionType Type = StringToInteractionType(TypeString);
            if (Type != EInteractionType::Click || TypeString.Equals(TEXT("click"), ESearchCase::IgnoreCase))
            {
                Result.AddUnique(Type);
            }
        }
    }
    
    return Result;
}

EInteractionType USimpleNodeDataConverter::StringToInteractionType(const FString& TypeString)
{
    if (TypeString.Equals(TEXT("click"), ESearchCase::IgnoreCase))
    {
        return EInteractionType::Click;
    }
    else if (TypeString.Equals(TEXT("hold"), ESearchCase::IgnoreCase))
    {
        return EInteractionType::Hold;
    }
    else if (TypeString.Equals(TEXT("drag"), ESearchCase::IgnoreCase))
    {
        return EInteractionType::Drag;
    }
    else if (TypeString.Equals(TEXT("hover"), ESearchCase::IgnoreCase))
    {
        return EInteractionType::Hover;
    }
    else if (TypeString.Equals(TEXT("multitouch"), ESearchCase::IgnoreCase))
    {
        return EInteractionType::MultiTouch;
    }
    else if (TypeString.Equals(TEXT("gesture"), ESearchCase::IgnoreCase))
    {
        return EInteractionType::Gesture;
    }
    
    return EInteractionType::Click; // 默认
}