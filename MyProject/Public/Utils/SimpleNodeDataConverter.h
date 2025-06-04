// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/NodeDataTypes.h"
#include "Dom/JsonObject.h"
#include "SimpleNodeDataConverter.generated.h"

UCLASS(BlueprintType)
class MYPROJECT_API USimpleNodeDataConverter : public UObject
{
	GENERATED_BODY()

public:
	// 主要转换方法
	UFUNCTION(BlueprintCallable, Category = "Converter")
	static bool ConvertJSONToNodeData(const FString& JSONString, TArray<FNodeGenerateData>& OutNodeData, TArray<FNodeRelationData>& OutRelations);

	UFUNCTION(BlueprintCallable, Category = "Converter")
	static bool LoadAndConvertJSONFile(const FString& FilePath, TArray<FNodeGenerateData>& OutNodeData, TArray<FNodeRelationData>& OutRelations);

private:
	// 解析方法
	static bool ParseNodeObject(const TSharedPtr<FJsonObject>& NodeObject, FNodeGenerateData& OutData);
	static bool ParseRelationObject(const TSharedPtr<FJsonObject>& RelationObject, FNodeRelationData& OutData);
	static FVector ParseLocation(const TSharedPtr<FJsonObject>& LocationObject);
    
	// 类型转换
	static ENodeType StringToNodeType(const FString& TypeString);
	static ENodeState StringToNodeState(const FString& StateString);
	static ENodeRelationType StringToRelationType(const FString& RelationString);
};