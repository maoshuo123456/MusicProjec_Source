// Fill out your copyright notice in the Description page of Project Settings.

// JSONTestActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/NodeDataTypes.h"
#include "JSONTest.generated.h"

class ANodeSystemManager;
class ASceneNode;
class AItemNode;

UCLASS()
class MYPROJECT_API AJSONTest : public AActor
{
	GENERATED_BODY()

public:
	AJSONTest();

protected:
	// 配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON Test")
	FString JSONFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON Test")
	ANodeSystemManager* NodeSystemManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON Test")
	TSubclassOf<ASceneNode> SceneNodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON Test")
	TSubclassOf<AItemNode> ItemNodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON Test")
	bool bAutoLoadOnBeginPlay;

public:
	// 主要功能
	UFUNCTION(BlueprintCallable, Category = "JSON Test")
	void LoadJSONAndGenerateNodes();

	UFUNCTION(BlueprintCallable, Category = "JSON Test")
	void TestWithHardcodedJSON();

	UFUNCTION(BlueprintCallable, Category = "JSON Test")
	void ClearGeneratedNodes();

protected:
	virtual void BeginPlay() override;

private:
	// 辅助方法
	void ProcessNodeData(const TArray<FNodeGenerateData>& NodeData, const TArray<FNodeRelationData>& Relations);
	TSubclassOf<AInteractiveNode> GetNodeClassForType(ENodeType Type);
    
	// 存储生成的节点
	UPROPERTY()
	TArray<AInteractiveNode*> GeneratedNodes;

	UPROPERTY()
	TMap<FString, AInteractiveNode*> NodeIDMap;
};