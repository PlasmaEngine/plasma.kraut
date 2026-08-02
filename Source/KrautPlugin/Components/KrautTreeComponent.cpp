#include <KrautPlugin/KrautPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plKrautTreeTypeBits, 1)
  PL_BITFLAGS_CONSTANTS(plKrautTreeTypeBits::Trunk1, plKrautTreeTypeBits::Trunk2, plKrautTreeTypeBits::Trunk3)
  PL_BITFLAGS_CONSTANTS(plKrautTreeTypeBits::MainBranches1, plKrautTreeTypeBits::MainBranches2, plKrautTreeTypeBits::MainBranches3)
  PL_BITFLAGS_CONSTANTS(plKrautTreeTypeBits::SubBranches1, plKrautTreeTypeBits::SubBranches2, plKrautTreeTypeBits::SubBranches3)
  PL_BITFLAGS_CONSTANTS(plKrautTreeTypeBits::Twigs1, plKrautTreeTypeBits::Twigs2, plKrautTreeTypeBits::Twigs3)
PL_END_STATIC_REFLECTED_BITFLAGS

PL_BEGIN_COMPONENT_TYPE(plKrautTreeComponent, 3, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_RESOURCE_ACCESSOR_PROPERTY("KrautTree", GetKrautGeneratorResource, SetKrautGeneratorResource)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Kraut_Tree")),
    PL_ACCESSOR_PROPERTY("VariationIndex", GetVariationIndex, SetVariationIndex)->AddAttributes(new plDefaultValueAttribute(0xFFFF)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgExtractGeometry, OnMsgExtractGeometry),
    PL_MESSAGE_HANDLER(plMsgBuildStaticMesh, OnBuildStaticMesh),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Terrain"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKrautTreeComponent::plKrautTreeComponent() = default;
plKrautTreeComponent::~plKrautTreeComponent() = default;

void plKrautTreeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hKrautGenerator;
  s << m_uiVariationIndex;
  s << m_uiCustomRandomSeed;
  s << m_iLodOverride;
}

void plKrautTreeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  if (uiVersion <= 1)
  {
    s >> m_hKrautTree;
  }
  else
  {
    s >> m_hKrautGenerator;
  }

  s >> m_uiVariationIndex;
  s >> m_uiCustomRandomSeed;

  if (uiVersion == 2)
  {
    plUInt16 m_uiDefaultVariationIndex;
    s >> m_uiDefaultVariationIndex;
  }

  if (uiVersion >= 3)
  {
    s >> m_iLodOverride;
  }

  GetWorld()->GetOrCreateComponentManager<plKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

plResult plKrautTreeComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_hKrautTree.IsValid())
  {
    plResourceLock<plKrautTreeResource> pTree(m_hKrautTree, plResourceAcquireMode::AllowLoadingFallback);
    // TODO: handle fallback case properly

    bounds = pTree->GetDetails().m_Bounds;

    {
      // this is a work around to make shadows and LODing work better
      // shadows do not affect the maximum LOD of a tree that is being rendered,
      // otherwise moving/rotating light-sources would cause LOD popping artifacts
      // and would generally result in more detailed tree rendering than typically necessary
      // however, that means when one is facing away from a tree, but can see its shadow,
      // the shadow may disappear entirely, because no view is setting a decent LOD level
      //
      // by artificially increasing its bbox the main camera will affect the LOD much longer,
      // even when not looking at the tree, thus resulting in decent shadows

      bounds.m_fSphereRadius *= s_iLocalBoundsScale;
      bounds.m_vBoxHalfExtends *= (float)s_iLocalBoundsScale;
    }

    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plKrautTreeComponent::SetVariationIndex(plUInt16 uiIndex)
{
  if (m_uiVariationIndex == uiIndex)
    return;

  m_uiVariationIndex = uiIndex;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<plKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

plUInt16 plKrautTreeComponent::GetVariationIndex() const
{
  return m_uiVariationIndex;
}

void plKrautTreeComponent::SetCustomRandomSeed(plUInt16 uiSeed)
{
  if (m_uiCustomRandomSeed == uiSeed)
    return;

  m_uiCustomRandomSeed = uiSeed;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<plKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

plUInt16 plKrautTreeComponent::GetCustomRandomSeed() const
{
  return m_uiCustomRandomSeed;
}

void plKrautTreeComponent::SetKrautGeneratorResource(const plKrautGeneratorResourceHandle& hTree)
{
  if (m_hKrautGenerator == hTree)
    return;

  m_hKrautGenerator = hTree;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateComponentManager<plKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void plKrautTreeComponent::OnActivated()
{
  SUPER::OnActivated();

  m_hKrautTree.Invalidate();
  m_vWindSpringPos.SetZero();
  m_vWindSpringVel.SetZero();

  if (m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<plKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void plKrautTreeComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!m_hKrautTree.IsValid())
    return;

  plResourceLock<plKrautTreeResource> pTree(m_hKrautTree, m_bForceGenerateImmediate ? plResourceAcquireMode::BlockTillLoaded_NeverFail : plResourceAcquireMode::AllowLoadingFallback);

  if (m_bForceGenerateImmediate)
  {
    if (pTree.GetAcquireResult() != plResourceAcquireResult::Final)
      return;
  }

  const plGameObject* pOwner = GetOwner();
  const plTransform tOwner = pOwner->GetGlobalTransform();
  const float fGlobalUniformScale = pOwner->GetGlobalScalingSimd().HorizontalSum<3>() * plSimdFloat(1.0f / 3.0f);

  const plVec3 vLodCamPos = msg.m_pView->GetLodCamera()->GetPosition();
  const bool bIsShadowView = msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow;
  const float fDistanceSQR = (tOwner.m_vPosition - vLodCamPos).GetLengthSquared();

  // ignore scale, the shader expects the wind strength in the global 0-20 m/sec range
  const plVec3 vLocalWind = pOwner->GetGlobalRotation().GetInverse() * m_vWindSpringPos;

  // changes every frame
  const bool bDynamic = true;
  const plColor color = plColor(vLocalWind.x, vLocalWind.y, vLocalWind.z, vLocalWind.GetLength());
  const plVec4 customData = pTree->GetDetails().m_vLeafCenter.GetAsVec4(0.0f);
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, tOwner, m_InstanceDataOffset, GetUniqueIdForRendering(), color, customData);

  const plUInt8 uiMaxLods = static_cast<plUInt8>(pTree->GetTreeLODs().GetCount());

  // Determine which single LOD to render
  plUInt8 uiActiveLod = 0;

  if (m_iLodOverride >= 0 && m_iLodOverride < uiMaxLods)
  {
    // Force specific LOD
    uiActiveLod = m_iLodOverride;
  }
  else
  {
    // Select LOD based on distance
    for (plUInt8 uiCurLod = 0; uiCurLod < uiMaxLods; ++uiCurLod)
    {
      const auto& lodData = pTree->GetTreeLODs()[uiCurLod];

      if (!lodData.m_hMesh.IsValid())
        continue;

      const float fMinDistSQR = plMath::Square(fGlobalUniformScale * lodData.m_fMinLodDistance);
      const float fMaxDistSQR = plMath::Square(fGlobalUniformScale * lodData.m_fMaxLodDistance);

      if (fDistanceSQR >= fMinDistSQR && fDistanceSQR < fMaxDistSQR)
      {
        uiActiveLod = uiCurLod;
        break;
      }
    }
  }

  // Render only the selected LOD
  {
    const auto& lodData = pTree->GetTreeLODs()[uiActiveLod];

    if (!lodData.m_hMesh.IsValid())
      return;

    if (bIsShadowView && lodData.m_LodType != plKrautLodType::Mesh)
      return;

    plResourceLock<plMeshResource> pMesh(lodData.m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
    plArrayPtr<const plMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();

    plArrayPtr<const plKrautTreeResourceDescriptor::MaterialData> materials = pTree->GetMaterials();

    for (plUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
    {
      const plUInt32 uiMaterialIndex = subMeshes[subMeshIdx].m_uiMaterialIndex;

      // Filter fronds and leaves based on render flags
      if (uiMaterialIndex < materials.GetCount())
      {
        const auto& matInfo = materials[uiMaterialIndex];

        // Only filter if branch type is valid (not None)
        // This ensures backward compatibility with old assets
        if (matInfo.m_BranchType != plKrautBranchType::None)
        {
          // Check if this is a frond and if fronds for this branch type are disabled
          if (m_bHideFrondsAndLeafs && (matInfo.m_MaterialType == plKrautMaterialType::Frond || matInfo.m_MaterialType == plKrautMaterialType::Leaf))
          {
            continue; // Skip this submesh
          }
        }
      }

      plMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

      plMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<plMeshRenderData>(pOwner);
      pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
      pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, lodData.m_hMesh, uiMaterialIndex, subMeshIdx);

      msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitOpaque, plRenderData::Caching::Never);
    }
  }
}

plResult plKrautTreeComponent::CreateGeometry(plGeometry& geo, plWorldGeoExtractionUtil::ExtractionMode mode) const
{
  if (GetOwner()->IsDynamic())
    return PL_FAILURE;

  // EnsureTreeIsGenerated(); // not const

  if (!m_hKrautTree.IsValid())
    return PL_FAILURE;

  plResourceLock<plKrautTreeResource> pTree(m_hKrautTree, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTree.GetAcquireResult() != plResourceAcquireResult::Final)
    return PL_FAILURE;

  const auto& details = pTree->GetDetails();

  if (mode == plWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
  {
    // TODO: support to load the actual tree mesh and return it
  }
  // else
  {
    const float fHeightScale = GetOwner()->GetGlobalScalingSimd().z();
    const float fMaxScale = GetOwner()->GetGlobalScalingSimd().HorizontalMax<3>();

    if (details.m_fStaticColliderRadius * fMaxScale <= 0.0f)
      return PL_FAILURE;

    const float fTreeHeight = (details.m_Bounds.m_vCenter.z + details.m_Bounds.m_vBoxHalfExtends.z) * 0.9f;

    if (fHeightScale * fTreeHeight <= 0.0f)
      return PL_FAILURE;

    // using a cone or even a cylinder with a thinner top results in the character controller getting stuck while sliding along the geometry
    // TODO: instead of triangle geometry it would maybe be better to use actual physics capsules

    // due to 'transform' this will already include the tree scale
    geo.AddCylinderOnePiece(details.m_fStaticColliderRadius, details.m_fStaticColliderRadius, fTreeHeight, 0.0f, 8);

    geo.TriangulatePolygons();
  }

  return PL_SUCCESS;
}

void plKrautTreeComponent::EnsureTreeIsGenerated()
{
  if (!m_hKrautGenerator.IsValid())
    return;

  plResourceLock<plKrautGeneratorResource> pResource(m_hKrautGenerator, plResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pResource.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  auto genDesc = pResource->GetDescriptor();
  if (genDesc == nullptr)
    return;

  plKrautTreeResourceHandle hNewTree;

  if (m_uiCustomRandomSeed != 0xFFFF)
  {
    hNewTree = pResource->GenerateTree(genDesc, m_uiCustomRandomSeed);
  }
  else
  {
    if (m_uiVariationIndex == 0xFFFF)
    {
      hNewTree = pResource->GenerateTreeWithGoodSeed(genDesc, GetOwner()->GetStableRandomSeed() & 0xFFFF);
    }
    else
    {
      hNewTree = pResource->GenerateTreeWithGoodSeed(genDesc, m_uiVariationIndex);
    }
  }

  if (m_hKrautTree != hNewTree)
  {
    m_hKrautTree = hNewTree;
    TriggerLocalBoundsUpdate();
  }
}

void plKrautTreeComponent::ComputeWind()
{
  if (!IsActiveAndSimulating() || GetOwner()->GetVisibilityState() == plVisibilityState::Invisible)
    return;

  const plWindWorldModuleInterface* pWindInterface = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>();

  if (!pWindInterface)
    return;

  auto pOwnder = GetOwner();

  const plVec3 vOwnerPos = pOwnder->GetGlobalPosition();
  const plVec3 vSampleWindPos = vOwnerPos + plVec3(0, 0, 2);
  const plVec3 vWindForce = pWindInterface->GetWindAt(vSampleWindPos);

  const float realTimeStep = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // springy wind force
  {
    const float fOverallStrength = 4.0f;

    const float fSpringConstant = 1.0f;
    const float fSpringDamping = 0.5f;
    const float fTreeMass = 1.0f;

    const plVec3 vSpringForce = -(fSpringConstant * m_vWindSpringPos + fSpringDamping * m_vWindSpringVel);

    const plVec3 vTotalForce = vWindForce + vSpringForce;

    // F = mass*acc
    // acc = F / mass
    const plVec3 vTreeAcceleration = vTotalForce / fTreeMass;

    m_vWindSpringVel += vTreeAcceleration * realTimeStep * fOverallStrength;
    m_vWindSpringPos += m_vWindSpringVel * realTimeStep * fOverallStrength;
  }

  // debug draw wind vectors
  if (false)
  {
    const plVec3 offset = GetOwner()->GetGlobalPosition() + plVec3(2, 0, 1);

    plHybridArray<plDebugRenderer::Line, 2> lines;

    // actual wind
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + vWindForce;
      l.m_startColor = plColor::BlueViolet;
      l.m_endColor = plColor::PowderBlue;
    }

    // springy wind
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + m_vWindSpringPos;
      l.m_startColor = plColor::BlueViolet;
      l.m_endColor = plColor::MediumVioletRed;
    }

    // springy wind 2
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + m_vWindSpringPos;
      l.m_startColor = plColor::LightGoldenRodYellow;
      l.m_endColor = plColor::MediumVioletRed;
    }

    plDebugRenderer::DrawLines(GetWorld(), lines, plColor::White);

    plStringBuilder tmp;
    tmp.SetFormat("Wind: {}m/s", m_vWindSpringPos.GetLength());

    plDebugRenderer::Draw3DText(GetWorld(), tmp, GetOwner()->GetGlobalPosition() + plVec3(0, 0, 1), plColor::DeepSkyBlue);
  }
}

void plKrautTreeComponent::OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg) const
{
  plStringBuilder sResourceName;
  sResourceName.SetFormat("KrautTreeCpu:{}", m_hKrautGenerator.GetResourceID());

  plCpuMeshResourceHandle hMesh = plResourceManager::GetExistingResource<plCpuMeshResource>(sResourceName);
  if (!hMesh.IsValid())
  {
    plGeometry geo;
    if (CreateGeometry(geo, ref_msg.m_Mode).Failed())
      return;

    plMeshResourceDescriptor desc;

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geo, plGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hMesh = plResourceManager::GetOrCreateResource<plCpuMeshResource>(sResourceName, std::move(desc), sResourceName);
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMesh);
}

void plKrautTreeComponent::OnBuildStaticMesh(plMsgBuildStaticMesh& ref_msg) const
{
  plGeometry geo;
  if (CreateGeometry(geo, plWorldGeoExtractionUtil::ExtractionMode::CollisionMesh).Failed())
    return;

  auto& desc = *ref_msg.m_pStaticMeshDescription;
  auto& subMesh = ref_msg.m_pStaticMeshDescription->m_SubMeshes.ExpandAndGetRef();

  {
    plResourceLock<plKrautTreeResource> pTree(m_hKrautTree, plResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pTree.GetAcquireResult() != plResourceAcquireResult::Final)
      return;

    const auto& details = pTree->GetDetails();

    if (!details.m_sSurfaceResource.IsEmpty())
    {
      const plUInt32 uiSurfIdx = desc.m_Surfaces.IndexOf(details.m_sSurfaceResource);
      if (uiSurfIdx == plInvalidIndex)
      {
        subMesh.m_uiSurfaceIndex = static_cast<plUInt16>(desc.m_Surfaces.GetCount());
        desc.m_Surfaces.PushBack(details.m_sSurfaceResource);
      }
      else
      {
        subMesh.m_uiSurfaceIndex = static_cast<plUInt16>(uiSurfIdx);
      }
    }
  }

  const plTransform transform = GetOwner()->GetGlobalTransform();

  subMesh.m_uiFirstTriangle = desc.m_Triangles.GetCount();
  subMesh.m_uiNumTriangles = geo.GetPolygons().GetCount();

  const plUInt32 uiFirstVertex = desc.m_Vertices.GetCount();

  for (const auto& vtx : geo.GetVertices())
  {
    desc.m_Vertices.ExpandAndGetRef() = transform.TransformPosition(vtx.m_vPosition);
  }

  for (const auto& tri : geo.GetPolygons())
  {
    auto& t = desc.m_Triangles.ExpandAndGetRef();
    t.m_uiVertexIndices[0] = uiFirstVertex + tri.m_Vertices[0];
    t.m_uiVertexIndices[1] = uiFirstVertex + tri.m_Vertices[1];
    t.m_uiVertexIndices[2] = uiFirstVertex + tri.m_Vertices[2];
  }
}

//////////////////////////////////////////////////////////////////////////

void plKrautTreeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plKrautTreeComponentManager::Update, this);
    desc.m_Phase = plWorldUpdatePhase::PreAsync;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plKrautTreeComponentManager::UpdateWind, this);
    desc.m_Phase = plWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiGranularity = 16;

    RegisterUpdateFunction(desc);
  }

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plKrautTreeComponentManager::ResourceEventHandler, this));
}

void plKrautTreeComponentManager::Deinitialize()
{
  PL_LOCK(m_Mutex);

  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plKrautTreeComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void plKrautTreeComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  plDeque<plComponentHandle> requireUpdate;

  {
    PL_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
    m_RequireUpdateSet.Clear();
  }

  for (const auto& hComp : requireUpdate)
  {
    plKrautTreeComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    // TODO: this could be wrapped into a task
    pComp->EnsureTreeIsGenerated();
  }
}

void plKrautTreeComponentManager::UpdateWind(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    plKrautTreeComponent* pComponent = it;
    pComponent->ComputeWind();
  }
}

void plKrautTreeComponentManager::EnqueueUpdate(plComponentHandle hComponent)
{
  PL_LOCK(m_Mutex);

  if (m_RequireUpdateSet.Insert(hComponent))
  {
    m_RequireUpdate.PushBack(hComponent);
  }
}

void plKrautTreeComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if ((e.m_Type == plResourceEvent::Type::ResourceContentUnloading || e.m_Type == plResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plKrautGeneratorResource>())
  {
    PL_LOCK(m_Mutex);

    plKrautGeneratorResourceHandle hResource((plKrautGeneratorResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const plKrautTreeComponent* pComponent = static_cast<plKrautTreeComponent*>(it.Value());

      if (pComponent->GetKrautGeneratorResource() == hResource)
      {
        EnqueueUpdate(pComponent->GetHandle());
      }
    }
  }
}


PL_STATICLINK_FILE(KrautPlugin, KrautPlugin_Components_KrautTreeComponent);
