// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/NodeDataTypes.h"
#include "GameplayTagContainer.h"
#include "ItemCapability.generated.h"

// 前向声明
class AItemNode;

UCLASS(Abstract, Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UItemCapability : public UActorComponent
{
    GENERATED_BODY()

public:
    UItemCapability();

    // 拥有者
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    AItemNode* OwnerItem;

    // 能力状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    bool bCapabilityIsActive;

    // 能力标识
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    FString CapabilityID;

    // 能力标签
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    FGameplayTagContainer CapabilityTags;

    // 能力描述
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability", meta = (MultiLine = true))
    FText CapabilityDescription;

    // 使用提示
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability")
    FString UsagePrompt;

    // 冷却时间
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capability", meta = (ClampMin = "0.0"))
    float CooldownDuration;

    // 当前冷却剩余时间
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    float CurrentCooldown;

public:
    // 生命周期
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 初始化
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability")
    void Initialize(AItemNode* Owner);
    virtual void Initialize_Implementation(AItemNode* Owner);

    // 激活/停用
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability")
    void CapabilityActivate();
    virtual void CapabilityActivate_Implementation();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability")
    void CapabilityDeactivate();
    virtual void CapabilityDeactivate_Implementation();

    // 使用能力
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability")
    bool CanUse(const FInteractionData& Data) const;
    virtual bool CanUse_Implementation(const FInteractionData& Data) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability")
    bool Use(const FInteractionData& Data);
    virtual bool Use_Implementation(const FInteractionData& Data);

    // 获取信息
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    bool CapabilityIsActive() const { return bCapabilityIsActive; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    bool IsOnCooldown() const { return CurrentCooldown > 0.0f; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    float GetCooldownRemaining() const { return CurrentCooldown; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    float GetCooldownProgress() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    FString GetCapabilityID() const { return CapabilityID; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    FString GetUsagePrompt() const { return UsagePrompt; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    FText GetCapabilityDescription() const { return CapabilityDescription; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Capability")
    AItemNode* GetOwnerItem() const { return OwnerItem; }

    // 事件响应
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability")
    void OnOwnerStateChanged(ENodeState NewState);
    virtual void OnOwnerStateChanged_Implementation(ENodeState NewState);

    // 能力信息
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Capability")
    FCapabilityData GetCapabilityInfo() const;
    virtual FCapabilityData GetCapabilityInfo_Implementation() const;

protected:
    // 内部方法
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability|Internal")
    void OnUseSuccess(const FInteractionData& Data);
    virtual void OnUseSuccess_Implementation(const FInteractionData& Data);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability|Internal")
    void OnUseFailed(const FInteractionData& Data);
    virtual void OnUseFailed_Implementation(const FInteractionData& Data);

    UFUNCTION(BlueprintCallable, Category = "Capability|Internal")
    void StartCooldown();

    UFUNCTION(BlueprintCallable, Category = "Capability|Internal")
    void ResetCooldown();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability|Internal")
    bool ValidateOwner() const;
    virtual bool ValidateOwner_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Capability|Internal")
    bool CheckPrerequisites(const FInteractionData& Data) const;
    virtual bool CheckPrerequisites_Implementation(const FInteractionData& Data) const;

private:
    // 定时器句柄
    FTimerHandle CooldownTimerHandle;
    
    // 冷却完成回调
    void OnCooldownComplete();
};