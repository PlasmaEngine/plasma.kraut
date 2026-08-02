#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

struct plKrautTreeResourceDescriptor;
struct plKrautGeneratorResourceDescriptor;

namespace plModelImporter2
{
  enum class TextureSemantic : plInt8;
}

struct plKrautTreeAssetEvent
{
  enum class Type
  {
    WindStrengthChanged,
    FrondsLeavesVisibilityChanged,
  };

  Type m_Type;
};

class plKrautTreeAssetDocument : public plSimpleAssetDocument<plKrautTreeAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetDocument, plSimpleAssetDocument<plKrautTreeAssetProperties>);

public:
  plKrautTreeAssetDocument(plStringView sDocumentPath);

  plStatus WriteKrautAsset(plStreamWriter& stream) const;

  plKrautWindStrength::Enum GetWindStrength() const { return m_WindStrength; }
  void SetWindStrength(plKrautWindStrength::Enum strength);

  bool GetShowFrondsLeaves() const { return m_bShowFrondsLeaves; }
  void SetShowFrondsLeaves(bool bShow);

  plEvent<const plKrautTreeAssetEvent&> m_Events;

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void AssignMaterials(plKrautGeneratorResourceDescriptor& desc, const plKrautTreeAssetProperties* pProp) const;

  void SyncBackAssetProperties(plKrautTreeAssetProperties*& pProp, const plKrautGeneratorResourceDescriptor& desc);

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

private:
  plKrautWindStrength::Enum m_WindStrength = plKrautWindStrength::Light;
  bool m_bShowFrondsLeaves = true;
};

//////////////////////////////////////////////////////////////////////////


class plKrautTreeAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plKrautTreeAssetDocumentGenerator();
  ~plKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plKrautTreeAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "KrautTrees"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments) override;
};
