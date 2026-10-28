#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <RendererCore/Material/MaterialResource.h>

using namespace AE_NS_FOUNDATION;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetDocument, 4, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plKrautTreeAssetDocument::plKrautTreeAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plKrautTreeAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

void plKrautTreeAssetDocument::SetWindStrength(plKrautWindStrength::Enum strength)
{
  if (m_WindStrength == strength)
    return;

  m_WindStrength = strength;

  // Send message to engine to update wind
  plSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "SetWindStrength";
  msg.m_sPayload = "";
  msg.m_PayloadValue = (plInt32)strength;
  SendMessageToEngine(&msg);

  // Notify actions to update their checked state
  plKrautTreeAssetEvent e;
  e.m_Type = plKrautTreeAssetEvent::Type::WindStrengthChanged;
  m_Events.Broadcast(e);
}

void plKrautTreeAssetDocument::SetShowFrondsLeaves(bool bShow)
{
  if (m_bShowFrondsLeaves == bShow)
    return;

  m_bShowFrondsLeaves = bShow;

  // Send message to engine to update component
  plSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "SetShowFrondsLeaves";
  msg.m_sPayload = "";
  msg.m_PayloadValue = (plInt32)bShow;
  SendMessageToEngine(&msg);

  // Notify actions to update their checked state
  plKrautTreeAssetEvent e;
  e.m_Type = plKrautTreeAssetEvent::Type::FrondsLeavesVisibilityChanged;
  m_Events.Broadcast(e);
}

//////////////////////////////////////////////////////////////////////////

void CopyConfig(Kraut::SpawnNodeDesc& node, const plKrautAssetBranchType& bt, plDynamicArray<plKrautMaterialDescriptor>& materials, plKrautBranchType branchType);

class KrautStreamIn : public aeStreamIn
{
public:
  plStreamReader* m_pStream = nullptr;

private:
  virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) override { return (aeUInt32)m_pStream->ReadBytes(pData, uiSize); }
};

static void GetMaterialLabel(plStringBuilder& ref_sOut, plKrautBranchType branchType, plKrautMaterialType materialType)
{
  ref_sOut.Clear();

  switch (branchType)
  {
    case plKrautBranchType::Trunk1:
    case plKrautBranchType::Trunk2:
    case plKrautBranchType::Trunk3:
      ref_sOut.SetFormat("Trunk {}", (int)branchType - (int)plKrautBranchType::Trunk1 + 1);
      break;

    case plKrautBranchType::MainBranches1:
    case plKrautBranchType::MainBranches2:
    case plKrautBranchType::MainBranches3:
      ref_sOut.SetFormat("Branch {}", (int)branchType - (int)plKrautBranchType::MainBranches1 + 1);
      break;

    case plKrautBranchType::SubBranches1:
    case plKrautBranchType::SubBranches2:
    case plKrautBranchType::SubBranches3:
      ref_sOut.SetFormat("Twig {}", (int)branchType - (int)plKrautBranchType::SubBranches1 + 1);
      break;

    case plKrautBranchType::Twigs1:
    case plKrautBranchType::Twigs2:
    case plKrautBranchType::Twigs3:
      ref_sOut.SetFormat("Twigy {}", (int)branchType - (int)plKrautBranchType::Twigs1 + 1);
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  switch (materialType)
  {
    case plKrautMaterialType::Branch:
      ref_sOut.Append(" - Stem");
      break;
    case plKrautMaterialType::Frond:
      ref_sOut.Append(" - Frond");
      break;
    case plKrautMaterialType::Leaf:
      ref_sOut.Append(" - Leaf");
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}


plStatus plKrautTreeAssetDocument::WriteKrautAsset(plStreamWriter& stream) const
{
  const plKrautTreeAssetProperties* pProp = GetProperties();

  plKrautGeneratorResourceDescriptor desc;
  desc.m_uiDefaultDisplaySeed = 0;
  desc.m_GoodRandomSeeds.PushBack(0);

  auto& ts = desc.m_TreeStructureDesc;

  const plKrautAssetBranchType* pBts[12] =
    {
      &pProp->m_BT_Trunk1,
      nullptr,
      nullptr,
      &pProp->m_BT_MainBranch1,
      &pProp->m_BT_MainBranch2,
      &pProp->m_BT_MainBranch3,
      &pProp->m_BT_SubBranch1,
      &pProp->m_BT_SubBranch2,
      &pProp->m_BT_SubBranch3,
      &pProp->m_BT_Twig1,
      &pProp->m_BT_Twig2,
      &pProp->m_BT_Twig3};

  {
    plInt32 iBaseBranch = -3;
    for (plUInt32 n = 0; n < Kraut::BranchType::ENUM_COUNT; ++n)
    {
      ts.m_BranchTypes[n].m_Type = (Kraut::BranchType::Enum)n;
      ts.m_BranchTypes[n].Reset();
      ts.m_BranchTypes[n].m_bUsed = false;
      ts.m_BranchTypes[n].m_bAllowSubType[0] = false;
      ts.m_BranchTypes[n].m_bAllowSubType[1] = false;
      ts.m_BranchTypes[n].m_bAllowSubType[2] = false;

      if (pBts[n] != nullptr)
      {
        if (iBaseBranch < 0 ||
            (ts.m_BranchTypes[iBaseBranch + 0].m_bAllowSubType[n % 3]) ||
            (ts.m_BranchTypes[iBaseBranch + 1].m_bAllowSubType[n % 3]) ||
            (ts.m_BranchTypes[iBaseBranch + 2].m_bAllowSubType[n % 3]))
        {
          ts.m_BranchTypes[n].m_bUsed = true;
          CopyConfig(ts.m_BranchTypes[n], *pBts[n], desc.m_Materials, (plKrautBranchType)n);
        }
      }

      if (n % 3 == 2)
        iBaseBranch += 3;
    }

    const plKrautAssetLod* pLods[5] = {
      &pProp->m_Lod0,
      &pProp->m_Lod1,
      &pProp->m_Lod2,
      &pProp->m_Lod3,
      &pProp->m_Lod4};

    for (plUInt32 n = 0; n < 5; ++n)
    {
      const plKrautAssetLod& lod = *pLods[n];

      desc.m_LodDesc[n].m_fTipDetail = lod.m_fTipDetail;
      desc.m_LodDesc[n].m_fCurvatureThreshold = lod.m_fCurvatureThreshold;
      desc.m_LodDesc[n].m_fThicknessThreshold = lod.m_fThicknessThreshold;
      desc.m_LodDesc[n].m_fVertexRingDetail = lod.m_fVertexRingDetail;

      desc.m_LodDesc[n].m_AllowTypes[Kraut::BranchGeometryType::Branch] = lod.m_AllowBranch.GetValue();
      desc.m_LodDesc[n].m_AllowTypes[Kraut::BranchGeometryType::Frond] = lod.m_AllowFrond.GetValue();
      desc.m_LodDesc[n].m_AllowTypes[Kraut::BranchGeometryType::Leaf] = lod.m_AllowLeaf.GetValue();

      desc.m_LodDesc[n].m_iMaxFrondDetail = lod.m_iMaxFrondDetail;
      desc.m_LodDesc[n].m_iFrondDetailReduction = lod.m_iFrondDetailReduction;
      desc.m_LodDesc[n].m_uiLodDistance = lod.m_uiLodDistance;
      desc.m_LodDesc[n].m_BranchSpikeTipMode = (Kraut::BranchSpikeTipMode::Enum)lod.m_BranchSpikeTipMode.GetValue();
    }
  }

  // AssignMaterials(desc, pProp);

  // write the output data
  {
    desc.m_sSurfaceResource = pProp->m_sSurface;
    desc.m_fStaticColliderRadius = pProp->m_fStaticColliderRadius;
    desc.m_fUniformScaling = 1.0f;   // TODO pProp->m_fUniformScaling;
    desc.m_fLodDistanceScale = 1.0f; // TODO pProp->m_fLodDistanceScale;
    desc.m_GoodRandomSeeds = pProp->m_GoodRandomSeeds;
    desc.m_uiDefaultDisplaySeed = pProp->m_uiRandomSeedForDisplay;
    desc.m_fTreeStiffness = pProp->m_fTreeStiffness;
    desc.m_fMinAmbientOcclusion = pProp->m_fMinAmbientOcclusion;

    if (desc.Serialize(stream).Failed())
    {
      return plStatus("Writing KrautGenerator resource descriptor failed.");
    }
  }

  return plStatus(PL_SUCCESS);
}

plTransformStatus plKrautTreeAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plProgressRange range("Transforming Asset", 2, false);

  if (WriteKrautAsset(stream).Failed())
  {
    return plStatus("Writing KrautGenerator resource descriptor failed.");
  }

  return plStatus(PL_SUCCESS);
}

void plKrautTreeAssetDocument::AssignMaterials(plKrautGeneratorResourceDescriptor& desc, const plKrautTreeAssetProperties* pProp) const
{
  desc.m_Materials.Clear();

  plStringBuilder materialLabel;

  for (plUInt32 bt = 0; bt < Kraut::BranchType::ENUM_COUNT; ++bt)
  {
    const auto& type = desc.m_TreeStructureDesc.m_BranchTypes[bt];

    if (!type.m_bUsed)
      continue;

    for (plUInt32 gt = 0; gt < Kraut::BranchGeometryType::ENUM_COUNT; ++gt)
    {
      if (!type.m_bEnable[gt])
        continue;

      auto& m = desc.m_Materials.ExpandAndGetRef();

      m.m_MaterialType = static_cast<plKrautMaterialType>((int)plKrautMaterialType::Branch + gt);
      m.m_BranchType = static_cast<plKrautBranchType>((int)plKrautBranchType::Trunk1 + bt);

      GetMaterialLabel(materialLabel, m.m_BranchType, m.m_MaterialType);

      // find the matching material from the user input (don't want to guess an index, in case the list size changed)
      for (const auto& mat : pProp->m_Materials)
      {
        if (mat.m_sLabel == materialLabel)
        {
          m.m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(mat.m_sMaterial);
          break;
        }
      }
    }
  }
}

void plKrautTreeAssetDocument::SyncBackAssetProperties(plKrautTreeAssetProperties*& pProp, const plKrautGeneratorResourceDescriptor& desc)
{
  bool bModified = pProp->m_Materials.GetCount() != desc.m_Materials.GetCount();

  pProp->m_Materials.SetCount(desc.m_Materials.GetCount());

  plStringBuilder newLabel;

  // TODO: match up old and new materials by label name

  for (plUInt32 m = 0; m < pProp->m_Materials.GetCount(); ++m)
  {
    auto& mat = pProp->m_Materials[m];

    GetMaterialLabel(newLabel, desc.m_Materials[m].m_BranchType, desc.m_Materials[m].m_MaterialType);

    if (newLabel != mat.m_sLabel)
    {
      mat.m_sLabel = newLabel;
      bModified = true;
    }
  }

  if (bModified)
  {
    GetObjectAccessor()->StartTransaction("Update Kraut Material Info");
    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }
}

plTransformStatus plKrautTreeAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plKrautTreeAssetDocumentGenerator>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plKrautTreeAssetDocumentGenerator::plKrautTreeAssetDocumentGenerator()
{
  AddSupportedFileType("tree");
}

plKrautTreeAssetDocumentGenerator::~plKrautTreeAssetDocumentGenerator() = default;

void plKrautTreeAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "KrautTreeImport.Tree";
    info.m_sIcon = ":/AssetIcons/Kraut_Tree.svg";
  }
}

plStatus plKrautTreeAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (plOSFile::ExistsFile(sOutFile))
  {
    plLog::Info("Skipping Kraut tree import, file has been imported before: '{}'", sOutFile);
    return plStatus(PL_SUCCESS);
  }

  auto pApp = plEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  plDocument* pDoc = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
  if (pDoc == nullptr)
    return plStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  plKrautTreeAssetDocument* pAssetDoc = plDynamicCast<plKrautTreeAssetDocument*>(pDoc);

  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plKrautTreeAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("KrautFile", sInputFileRel.GetView());

  plLog::Success("Imported Kraut tree: '{}'", sOutFile);

  return plStatus(PL_SUCCESS);
}
