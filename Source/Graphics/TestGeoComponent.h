// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
#include "TestGeoComponent.generated.h"

/**
 * ģ��UBoxComponent��д��, �ص㿼��FPrimitiveSceneProxy
 */
UCLASS(HideCategories = (Mobility, Collision, Physics, LOD, TextureStreaming, Activation ) , meta=(BlueprintSpawnableComponent))
class GRAPHICS_API UTestGeoComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

	UTestGeoComponent();
public:

	//���º�����ʵ��һ����Ҫrender����������ṩ��
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	//~ Begin USceneComponent Interface,�ṩ��Engine��Χ�߽�,���ӿ��޳�ʱҪ�õ�
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface

	//ע���UShapeComponent�̳е�����Ҫʵ�ִ˺�����������ϵͳ
	//virtual void UpdateBodySetup() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;


	/** Used to control the line thickness when rendering */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = Shape)
	float LineThickness;

	//��ʱ����box extent
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = Shape)
	FVector BoxExtent;
};
