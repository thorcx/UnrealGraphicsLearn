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
		bWillEverBeLit = false; //SceneProxy基类成员,设置此几何体是否提交到光照pass
	}

	//Relevance指定动态Mesh路径后,这里由各个需要的RenderPass调用来完成绘制
	//注意此处调度应该是由RenderThread完成，这里的指令不需要ENQUEUE_RENDER_COMMAND来完成
	//但是如果是在上面的构造函数中进行VB,IB的构建，需要小心，因为是在GameThread
	//注意这里虽然在RenderThread,但是尽量不要在其中进行RenderState切换逻辑,而是根据逻辑状态从GameThread内通过Enqueue_Render_command来做
	//根据文档https://docs.unrealengine.com/en-US/Programming/Rendering/ThreadedRendering/index.html
	//此处的函数可以在一帧内被根据pass需求被多次调用,也可能被丢弃,而RenderState的更新应该是一帧只进行一次,从逻辑上其State切换不应该放在这里
	//应该由GameThread的Tick更新状态并进行ENQUEUE调度RenderThread进行RenderState更新
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_TestGeoProxy_GetDynamicMeshElements);

		const FMatrix& localToWorld = GetLocalToWorld();//Proxy自身保存变换矩阵
		//VisibilityMap(此值是bit位图)决定某个view是否可以被Render
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				const FLinearColor DrawColor = GetViewSelectionColor(GeoColor, *View, IsSelected(), IsHovered(), false, IsIndividuallySelected());
				
				//这里直接使用PDI接口绘制而没有使用MeshBatch
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				DrawOrientedWireBox(PDI, localToWorld.GetOrigin(), localToWorld.GetScaledAxis(EAxis::X), localToWorld.GetScaledAxis(EAxis::Y), localToWorld.GetScaledAxis(EAxis::Z), BoxExtents, DrawColor, SDPG_World, LineThickness);
			}
		}
	}


	//Scene在init时候调用此函数决定当前元素是否显示
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		//const bool bProxyVisible = !bDrawOnlyIfSelected || IsSelected();
		const bool bProxyVisible = IsSelected();
		//是否引擎开启collision drawing并且此Geo有碰撞
		const bool bShowForCollision = View->Family->EngineShowFlags.Collision && IsCollisionEnabled();

		FPrimitiveViewRelevance Result;
		//当前几何体是否在view内显示
		Result.bDrawRelevance = (IsShown(View) && bProxyVisible) || bShowForCollision;
		//是否采用动态meshDraw的renderpath,如果dynamicRelevance,就需要实现GetDynamicMeshElements
		//如果式StaticRelevance,需要实现DrawStaticElements,根据文档FPrimitiveViewRelevance是位图,
		//保存哪个pass当前的Geo会用到,因为一个Primitive可能有多个子元素,而每个元素可能会用到不同的pass来渲染
		//比如元素同时有opaque与translucent，这些都可以通过设置对应的标记位来实现
		Result.bDynamicRelevance = true; 
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View); //此几何体编辑器内是否在postprocess之后render
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
//在Component的RegisterComponent函数中,会调用CreateRenderState_Concurrent函数,内部会通过FScene调度CreateSceneProxy
//完成RenderThread端对Component的初始化，此后GameThread不能以任何形式访问RT端的Proxy数据，当GT端的Component有关数据修改后
//需要使用MarkRenderStateDirty()函数,在此帧结束后由RT端同步所有数据到Proxy,一般RT的数据都要落后GT端1-2帧
//GT端在Tick结束后会阻塞直到RenderThread追到只落后1-2帧差距,一般GT都不会阻塞到RT完全追上当前帧s
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
	//记录一帧之内调用次数
	SCOPE_CYCLE_COUNTER(STAT_TestComponentTick);
	
}

//void UTestGeoComponent::UpdateBodySetup()
//{
//
//}
