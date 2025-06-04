// Fill out your copyright notice in the Description page of Project Settings.

// ItemNode.h
#pragma once

#include "CoreMinimal.h"
#include "Nodes/InteractiveNode.h"
#include "Core/NodeDataTypes.h"
#include "ItemNode.generated.h"

// 前向声明
class UItemCapability;

UCLASS(Blueprintable)
class MYPROJECT_API AItemNode : public AInteractiveNode
{
    GENERATED_BODY()

public:
    AItemNode();

    // 能力系统
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Capabilities")
    TArray<UItemCapability*> Capabilities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Capabilities")
    TMap<TSubclassOf<UItemCapability>, UItemCapability*> CapabilityMap;

    // 物品属性
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Properties")
    bool bIsCarryable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Properties")
    bool bAutoActivateCapabilities;

    // 故事相关
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Story")
    TArray<FString> UnlockStoryIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Story")
    TMap<FString, FNodeGenerateData> ConditionalSpawns;

    // 能力管理
    UFUNCTION(BlueprintCallable, Category = "Item|Capabilities", meta = (CallInEditor = "true"))
    UItemCapability* AddCapability(TSubclassOf<UItemCapability> CapabilityClass);

    UFUNCTION(BlueprintCallable, Category = "Item|Capabilities")
    void AddCapabilityInstance(UItemCapability* Capability);

    UFUNCTION(BlueprintCallable, Category = "Item|Capabilities")
    bool RemoveCapability(TSubclassOf<UItemCapability> CapabilityClass);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Capabilities")
    UItemCapability* GetCapability(TSubclassOf<UItemCapability> CapabilityClass) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Capabilities")
    TArray<UItemCapability*> GetAllCapabilities() const { return Capabilities; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Capabilities")
    bool HasCapability(TSubclassOf<UItemCapability> CapabilityClass) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Capabilities")
    int32 GetCapabilityCount() const { return Capabilities.Num(); }

    // 能力使用
    UFUNCTION(BlueprintCallable, Category = "Item|Capabilities")
    bool UseCapability(TSubclassOf<UItemCapability> CapabilityClass, const FInteractionData& Data);

    UFUNCTION(BlueprintCallable, Category = "Item|Capabilities")
    int32 UseAllCapabilities(const FInteractionData& Data);

    // 故事相关
    UFUNCTION(BlueprintCallable, Category = "Item|Story")
    void AddUnlockStory(const FString& StoryID);

    UFUNCTION(BlueprintCallable, Category = "Item|Story")
    void AddConditionalSpawn(const FString& Condition, const FNodeGenerateData& SpawnData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Story")
    TArray<FNodeGenerateData> GetConditionalSpawns(const FString& Condition) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Story")
    bool HasUnlockStories() const { return UnlockStoryIDs.Num() > 0; }

    // 物品状态
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Properties")
    bool IsCarryable() const { return bIsCarryable; }

    UFUNCTION(BlueprintCallable, Category = "Item|Properties")
    void SetCarryable(bool bNewCarryable) { bIsCarryable = bNewCarryable; }

    // 重写父类方法
    virtual void OnInteract_Implementation(const FInteractionData& Data) override;
    virtual bool CanInteract_Implementation(const FInteractionData& Data) const override;
    virtual void OnStateChanged_Implementation(ENodeState OldState, ENodeState NewState) override;
    virtual bool ShouldTriggerStory_Implementation() const override;
    virtual TArray<FNodeGenerateData> GetNodeSpawnData_Implementation() const override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual void UpdateVisuals_Implementation() override;

protected:
    // BeginPlay
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 内部方法
    void RegisterCapability(UItemCapability* Capability);
    void UnregisterCapability(UItemCapability* Capability);
    void NotifyCapabilitiesStateChange();
    
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item|Internal")
    void CheckStoryConditions();
    virtual void CheckStoryConditions_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item|Internal")
    void OnCapabilityUsed(UItemCapability* Capability, bool bSuccess);
    virtual void OnCapabilityUsed_Implementation(UItemCapability* Capability, bool bSuccess);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item|Internal")
    FString GetBestCapabilityPrompt() const;
    virtual FString GetBestCapabilityPrompt_Implementation() const;

private:
    // 辅助方法
    void InitializeDefaultCapabilities();
    void CleanupCapabilities();
};