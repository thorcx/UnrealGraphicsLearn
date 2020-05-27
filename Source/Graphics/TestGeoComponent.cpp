// Fill out your copyright notice in the Description page of Project Settings.


#include "TestGeoComponent.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"
#include "ComponentUtils.h"

//DECLARE_STATS_GROUP(TEXT("TestComponent Tick"), STATGROUP_TestComponent, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("TestGeo - TickCount"), STAT_TestComponentTick, STATGROUP_Component);

class FTestGeoSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FTestGeoSceneProxy(const UTestGeoComponent* InComponent)
		:FPrimitiveSceneProxy(InComponent)
		//,bDrawOnlyIfSelected(InComponent->bDrawOnlyIfSelected)
		//,GeoColor(InComponent->ShapeColor)
		,GeoColor(FColor::Cyan)
		,LineThickness(InComponent->LineThickness)
		,BoxExtents(InComponent->BoxExtent)
	{
		bWillEverBeLit = false; //SceneProxy�����Ա,���ô˼������Ƿ��ύ������pass
	}

	//Relevanceָ����̬Mesh·����,�����ɸ�����Ҫ��RenderPass��������ɻ���
	//ע��˴�����Ӧ������RenderThread��ɣ������ָ���ҪENQUEUE_RENDER_COMMAND�����
	//���������������Ĺ��캯���н���VB,IB�Ĺ�������ҪС�ģ���Ϊ����GameThread
	//ע��������Ȼ��RenderThread,���Ǿ�����Ҫ�����н���RenderState�л��߼�,���Ǹ����߼�״̬��GameThread��ͨ��Enqueue_Render_command����
	//�����ĵ�https://docs.unrealengine.com/en-US/Programming/Rendering/ThreadedRendering/index.html
	//�˴��ĺ���������һ֡�ڱ�����pass���󱻶�ε���,Ҳ���ܱ�����,��RenderState�ĸ���Ӧ����һֻ֡����һ��,���߼�����State�л���Ӧ�÷�������
	//Ӧ����GameThread��Tick����״̬������ENQUEUE����RenderThread����RenderState����
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_TestGeoProxy_GetDynamicMeshElements);

		const FMatrix& localToWorld = GetLocalToWorld();//Proxy������任����
		//VisibilityMap(��ֵ��bitλͼ)����ĳ��view�Ƿ���Ա�Render
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				const FLinearColor DrawColor = GetViewSelectionColor(GeoColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected());
				
				//����ֱ��ʹ��PDI�ӿڻ��ƶ�û��ʹ��MeshBatch
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				DrawOrientedWireBox(PDI, localToWorld.GetOrigin(), localToWorld.GetScaledAxis(EAxis::X), localToWorld.GetScaledAxis(EAxis::Y), localToWorld.GetScaledAxis(EAxis::Z), BoxExtents, DrawColor, SDPG_World, LineThickness);
			}
		}
	}


	//Scene��initʱ����ô˺���������ǰԪ���Ƿ���ʾ
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		//const bool bProxyVisible = !bDrawOnlyIfSelected || IsSelected();
		const bool bProxyVisible = IsSelected();
		//�Ƿ����濪��collision drawing���Ҵ�Geo����ײ
		const bool bShowForCollision = View->Family->EngineShowFlags.Collision && IsCollisionEnabled();

		FPrimitiveViewRelevance Result;
		//��ǰ�������Ƿ���view����ʾ
		Result.bDrawRelevance = (IsShown(View) && bProxyVisible) || bShowForCollision;
		//�Ƿ���ö�̬meshDraw��renderpath,���dynamicRelevance,����Ҫʵ��GetDynamicMeshElements
		//���ʽStaticRelevance,��Ҫʵ��DrawStaticElements,�����ĵ�FPrimitiveViewRelevance��λͼ,
		//�����ĸ�pass��ǰ��Geo���õ�,��Ϊһ��Primitive�����ж����Ԫ��,��ÿ��Ԫ�ؿ��ܻ��õ���ͬ��pass����Ⱦ
		//����Ԫ��ͬʱ��opaque��translucent����Щ������ͨ�����ö�Ӧ�ı��λ��ʵ��
		Result.bDynamicRelevance = true; 
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View); //�˼�����༭�����Ƿ���postprocess֮��render
		return Result;
	}

	virtual uint32 GetMemoryFootprint(void) const override
	{
		return (sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return FPrimitiveSceneProxy::GetAllocatedSize();
	}

	//const uint32	bDrawOnlyIfSelected : 1;
	FColor GeoColor;
	const float LineThickness;
	const FVector BoxExtents;
};


UTestGeoComponent::UTestGeoComponent()
{
	LineThickness = 2.0f;
	BoxExtent = FVector(16.0f, 16.0f, 16.0f);
	bUseEditorCompositing = true;
	PrimaryComponentTick.bCanEverTick = true;
}
//��Component��RegisterComponent������,�����CreateRenderState_Concurrent����,�ڲ���ͨ��FScene����CreateSceneProxy
//���RenderThread�˶�Component�ĳ�ʼ�����˺�GameThread�������κ���ʽ����RT�˵�Proxy���ݣ���GT�˵�Component�й������޸ĺ�
//��Ҫʹ��MarkRenderStateDirty()����,�ڴ�֡��������RT��ͬ���������ݵ�Proxy,һ��RT�����ݶ�Ҫ���GT��1-2֡
//GT����Tick�����������ֱ��RenderThread׷��ֻ���1-2֡���,һ��GT������������RT��ȫ׷�ϵ�ǰ֡s
FPrimitiveSceneProxy* UTestGeoComponent::CreateSceneProxy()
{
	return new FTestGeoSceneProxy(this);
}

FBoxSphereBounds UTestGeoComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox boundBox(-BoxExtent, BoxExtent);
	return FBoxSphereBounds(boundBox).TransformBy(LocalToWorld);
}

void UTestGeoComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	//��¼һ֮֡�ڵ��ô���
	SCOPE_CYCLE_COUNTER(STAT_TestComponentTick);
	
}

//void UTestGeoComponent::UpdateBodySetup()
//{
//
//}
