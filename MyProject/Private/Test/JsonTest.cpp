// Fill out your copyright notice in the Description page of Project Settings.


// JSONTestActor.cpp
#include "Test/JsonTest.h"
#include "Utils/SimpleNodeDataConverter.h"
#include "Nodes/NodeSystemManager.h"
#include "Nodes/SceneNode.h"
#include "Nodes/ItemNode.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AJSONTest::AJSONTest()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // 默认JSON文件路径
    JSONFilePath = TEXT("Content/Data/test_simple_scene.json");
    bAutoLoadOnBeginPlay = false;
}

void AJSONTest::BeginPlay()
{
    Super::BeginPlay();
    
    // 查找NodeSystemManager
    if (!NodeSystemManager)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANodeSystemManager::StaticClass(), FoundActors);
        
        if (FoundActors.Num() > 0)
        {
            NodeSystemManager = Cast<ANodeSystemManager>(FoundActors[0]);
        }
        else
        {
            NodeSystemManager = GetWorld()->SpawnActor<ANodeSystemManager>();
        }
    }
    
    if (bAutoLoadOnBeginPlay)
    {
        LoadJSONAndGenerateNodes();
    }
}

void AJSONTest::LoadJSONAndGenerateNodes()
{
    if (!NodeSystemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("No NodeSystemManager found!"));
        return;
    }
    
    // 清理之前的节点
    ClearGeneratedNodes();
    
    // 转换JSON
    TArray<FNodeGenerateData> NodeData;
    TArray<FNodeRelationData> Relations;
    
    FString FullPath = FPaths::ProjectDir() + JSONFilePath;
    if (USimpleNodeDataConverter::LoadAndConvertJSONFile(FullPath, NodeData, Relations))
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully loaded JSON with %d nodes and %d relations"), 
            NodeData.Num(), Relations.Num());
        
        ProcessNodeData(NodeData, Relations);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON from: %s"), *FullPath);
    }
}

void AJSONTest::TestWithHardcodedJSON()
{
    if (!NodeSystemManager)
    {
        UE_LOG(LogTemp, Error, TEXT("No NodeSystemManager found!"));
        return;
    }
    
    // 硬编码的测试JSON
    FString TestJSON = TEXT(R"(
    {
      "scene_id": "test_scene_001",
      "scene_name": "测试场景",
      "nodes": [
        {
          "id": "scene_main",
          "type": "scene",
          "name": "主场景",
          "state": "active",
          "transform": {
            "location": {"x": 0, "y": 0, "z": 0}
          }
        },
        {
          "id": "item_a",
          "type": "item",
          "name": "物品A",
          "state": "active",
          "transform": {
            "location": {"x": -200, "y": 0, "z": 50}
          }
        },
        {
          "id": "item_b",
          "type": "item",
          "name": "物品B",
          "state": "locked",
          "transform": {
            "location": {"x": 200, "y": 0, "z": 50}
          }
        }
      ],
      "relations": [
        {
          "source_id": "item_a",
          "target_id": "item_b",
          "relation_type": "prerequisite",
          "weight": 1.0
        }
      ]
    }
    )");
    
    // 清理之前的节点
    ClearGeneratedNodes();
    
    // 转换JSON
    TArray<FNodeGenerateData> NodeData;
    TArray<FNodeRelationData> Relations;
    
    if (USimpleNodeDataConverter::ConvertJSONToNodeData(TestJSON, NodeData, Relations))
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully parsed hardcoded JSON with %d nodes and %d relations"), 
            NodeData.Num(), Relations.Num());
        
        ProcessNodeData(NodeData, Relations);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse hardcoded JSON"));
    }
}

void AJSONTest::ClearGeneratedNodes()
{
    if (NodeSystemManager)
    {
        for (AInteractiveNode* Node : GeneratedNodes)
        {
            if (Node)
            {
                NodeSystemManager->UnregisterNode(Node);
                Node->Destroy();
            }
        }
    }
    
    GeneratedNodes.Empty();
    NodeIDMap.Empty();
}

void AJSONTest::ProcessNodeData(const TArray<FNodeGenerateData>& NodeData, const TArray<FNodeRelationData>& Relations)
{
    ASceneNode* MainScene = nullptr;
    
    // 第一步：创建所有节点
    for (const FNodeGenerateData& Data : NodeData)
    {
        // 设置正确的节点类
        FNodeGenerateData ModifiedData = Data;
        ModifiedData.NodeClass = GetNodeClassForType(Data.NodeData.NodeType);
        
        // 调整生成位置（相对于测试Actor）
        ModifiedData.SpawnTransform.SetLocation(GetActorLocation() + Data.SpawnTransform.GetLocation());
        
        // 创建节点
        AInteractiveNode* NewNode = NodeSystemManager->CreateNode(ModifiedData.NodeClass, ModifiedData);
        if (NewNode)
        {
            GeneratedNodes.Add(NewNode);
            NodeIDMap.Add(NewNode->GetNodeID(), NewNode);
            
            UE_LOG(LogTemp, Log, TEXT("Created node: %s at %s"), 
                *NewNode->GetNodeName(), 
                *NewNode->GetActorLocation().ToString());
            
            // 记录场景节点
            if (NewNode->IsA<ASceneNode>() && !MainScene)
            {
                MainScene = Cast<ASceneNode>(NewNode);
            }
        }
    }
    
    // 第二步：如果有场景节点，将物品节点添加为子节点
    if (MainScene)
    {
        for (AInteractiveNode* Node : GeneratedNodes)
        {
            if (Node && Node != MainScene && Node->IsA<AItemNode>())
            {
                MainScene->AddChildNode(Node);
            }
        }
        
        // 设置为活动场景
        NodeSystemManager->SetActiveScene(MainScene);
    }
    
    // 第三步：创建关系
    for (const FNodeRelationData& Relation : Relations)
    {
        AInteractiveNode* SourceNode = NodeIDMap.FindRef(Relation.SourceNodeID);
        AInteractiveNode* TargetNode = NodeIDMap.FindRef(Relation.TargetNodeID);
        
        if (SourceNode && TargetNode)
        {
            ANodeConnection* Connection = NodeSystemManager->CreateConnection(SourceNode, TargetNode, Relation);
            if (Connection)
            {
                UE_LOG(LogTemp, Log, TEXT("Created connection: %s -> %s (%s)"), 
                    *Relation.SourceNodeID, 
                    *Relation.TargetNodeID,
                    *UEnum::GetValueAsString(Relation.RelationType));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create connection: source or target not found"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("JSON processing complete: %d nodes, scene: %s"), 
        GeneratedNodes.Num(), 
        MainScene ? *MainScene->GetNodeName() : TEXT("None"));
}

TSubclassOf<AInteractiveNode> AJSONTest::GetNodeClassForType(ENodeType Type)
{
    switch (Type)
    {
    case ENodeType::Scene:
        return SceneNodeClass ? SceneNodeClass : ASceneNode::StaticClass();
        
    case ENodeType::Item:
    case ENodeType::Trigger:
    case ENodeType::Story:
    default:
        return ItemNodeClass ? ItemNodeClass : AItemNode::StaticClass();
    }
}