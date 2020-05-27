// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
#include "TestGeoComponent.generated.h"

/**
 * 模拟UBoxComponent的写法, 重点考察FPrimitiveSceneProxy
 */
UCLASS(HideCategories = (Mobility, Collision, Physics, LOD, TextureStreaming, Activation ) , meta=(BlueprintSpawnableComponent))
class GRAPHICS_API UTestGeoComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

	UTestGeoComponent();
public:

	//以下函数是实现一个需要render的组件必须提供的
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	//~ Begin USceneComponent Interface,提供给Engine包围边界,做视口剔除时要用到
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface

	//注意从UShapeComponent继承的类需要实现此函数更新物理系统
	//virtual void UpdateBodySetup() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;


	/** Used to control the line thickness when rendering */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = Shape)
	float LineThickness;

	//暂时先用box extent
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = Shape)
	FVector BoxExtent;
};
