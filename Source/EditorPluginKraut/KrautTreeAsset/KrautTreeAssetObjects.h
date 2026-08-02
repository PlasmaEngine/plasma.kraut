#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct plPropertyMetaStateEvent;

namespace plKrautBranchTypeNames
{
  constexpr const char* Trunk = "Trunk";
  constexpr const char* MainBranch1 = "Main Branch 1";
  constexpr const char* MainBranch2 = "Main Branch 2";
  constexpr const char* MainBranch3 = "Main Branch 3";
  constexpr const char* SubBranch1 = "Sub Branch 1";
  constexpr const char* SubBranch2 = "Sub Branch 2";
  constexpr const char* SubBranch3 = "Sub Branch 3";
  constexpr const char* Twig1 = "Twig 1";
  constexpr const char* Twig2 = "Twig 2";
  constexpr const char* Twig3 = "Twig 3";
} // namespace plKrautBranchTypeNames

struct plKrautAssetMaterial
{
  plString m_sLabel;
  plString m_sMaterial;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautAssetMaterial);

struct plKrautBranchTypeMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Regular,
    Umbrella,

    Default = Regular,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautBranchTypeMode);

struct plKrautBranchTargetDir
{
  using StorageType = plUInt8;

  enum Enum
  {
    Straight, // along the start direction
    Upwards,  // to the sky!
    Degree22,
    Degree45,
    Degree67,
    Degree90,
    Degree112,
    Degree135,
    Degree157,
    Downwards, // to the ground

    Default = Straight
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautBranchTargetDir);

struct plKrautLeafOrientation
{
  using StorageType = plUInt8;

  enum Enum
  {
    Upwards,
    AlongBranch,
    OrthogonalToBranch,

    Default = Upwards,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautLeafOrientation);

struct plKrautFrondContourMode
{
  using StorageType = plInt8;

  enum Enum
  {
    Off = -1,
    Full,
    Symetric,
    InverseSymetric,

    Default = Full
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautFrondContourMode);

struct plKrautBranchTargetDir2Usage
{
  using StorageType = plUInt8;

  enum Enum
  {
    Off,
    Relative,
    Absolute,

    Default = Off
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautBranchTargetDir2Usage);

struct plKrautLodMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Full,
    FourQuads,
    TwoQuads,
    Billboard,
    Disabled,

    Default = Full
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautLodMode);

struct plKrautBranchSpikeTipMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    FullDetail,
    SingleTriangle,
    Hole,

    Default = FullDetail
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautBranchSpikeTipMode);

struct plKrautAssetBranchType
{
  // === Administrative ===

  //bool m_bVisible = true;

  bool m_bGrowSubBranchType1 = false;
  bool m_bGrowSubBranchType2 = false;
  bool m_bGrowSubBranchType3 = false;

  // === Branch Type ===

  // General

  plUInt8 m_uiSegmentLengthCM = 5;
  plEnum<plKrautBranchTypeMode> m_BranchTypeMode = plKrautBranchTypeMode::Regular;
  float m_fBranchlessPartABS = 0.0f;
  float m_fBranchlessPartEndABS = 0.0f;
  plUInt8 m_uiLowerBound = 0;
  plUInt8 m_uiUpperBound = 100;
  plUInt16 m_uiMinBranchThicknessInCM = 20;
  plUInt16 m_uiMaxBranchThicknessInCM = 20;

  // Spawn Nodes

  plUInt8 m_uiMinBranches = 4;
  plUInt8 m_uiMaxBranches = 4;
  float m_fNodeSpacingBefore = 0.5f;
  float m_fNodeSpacingAfter = 0.0f;
  float m_fNodeHeight = 0.0f;


  // === Growth ===

  // Start Direction

  plAngle m_MaxRotationalDeviation = {};
  plAngle m_BranchAngle = plAngle::MakeFromDegree(90);
  plAngle m_MaxBranchAngleDeviation = plAngle::MakeFromDegree(10);

  // Target Direction

  plEnum<plKrautBranchTargetDir> m_TargetDirection = plKrautBranchTargetDir::Straight;
  bool m_bTargetDirRelative = false;
  plEnum<plKrautBranchTargetDir2Usage> m_TargetDir2Usage = plKrautBranchTargetDir2Usage::Off;
  float m_fTargetDir2Usage = 2.5f;
  plEnum<plKrautBranchTargetDir> m_TargetDirection2 = plKrautBranchTargetDir::Upwards;
  plAngle m_MaxTargetDirDeviation = plAngle::MakeFromDegree(20);

  // Growth

  plUInt16 m_uiMinBranchLengthInCM = 100;
  plUInt16 m_uiMaxBranchLengthInCM = 100;
  plSingleCurveData m_MaxBranchLengthParentScale;
  plAngle m_GrowMaxTargetDirDeviation = {};
  plAngle m_GrowMaxDirChangePerSegment = plAngle::MakeFromDegree(5);
  bool m_bRestrictGrowthToFrondPlane = false;

  // Obstacles

  //bool m_bActAsObstacle = false;
  //bool m_bDoPhysicalSimulation = false;
  //float m_fPhysicsLookAhead = 1.5f;
  //plAngle m_PhysicsEvasionAngle = plAngle::MakeFromDegree(30);


  // === Appearance ===

  // Branch Mesh

  bool m_bEnableMesh = true;
  plString m_sBranchMaterial;
  plSingleCurveData m_BranchContour;
  float m_fRoundnessFactor = 0.5f;
  plUInt8 m_uiFlares = 0;
  float m_fFlareWidth = 2.0f;
  plSingleCurveData m_FlareWidthCurve;
  plAngle m_FlareRotation = {};
  bool m_bRotateTexCoords = true;

  // Fronds

  bool m_bEnableFronds = true;
  plString m_sFrondMaterial;
  float m_fTextureRepeat = 0.0f;
  plEnum<plKrautLeafOrientation> m_FrondUpOrientation = plKrautLeafOrientation::Upwards;
  plAngle m_MaxFrondOrientationDeviation = {};
  plUInt8 m_uiNumFronds = 1;
  bool m_bAlignFrondsOnSurface = false;
  plUInt8 m_uiFrondDetail = 1;
  plSingleCurveData m_FrondContour;
  plEnum<plKrautFrondContourMode> m_FrondContourMode = plKrautFrondContourMode::Full;
  float m_fFrondHeight = 0.5f;
  plSingleCurveData m_FrondHeight;
  float m_fFrondWidth = 0.5f;
  plSingleCurveData m_FrondWidth;
  //plColor m_FrondVariationColor;// currently done through the material

  // Leaves

  bool m_bEnableLeaves = true;
  plString m_sLeafMaterial;
  bool m_bBillboardLeaves = true;
  float m_fLeafSize = 0.25f;
  plSingleCurveData m_LeafScale;
  float m_fLeafInterval = 0;
  //plColor m_LeafVariationColor;// currently done through the material

  // Shared

  //plUInt8 m_uiTextureTilingX[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
  //plUInt8 m_uiTextureTilingY[Kraut::BranchGeometryType::ENUM_COUNT] = {1, 1, 1};
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautAssetBranchType);


struct plKrautAssetLod
{
  plEnum<plKrautLodMode> m_Mode = plKrautLodMode::Full;
  float m_fTipDetail = 0.04f;
  float m_fCurvatureThreshold = 5.0f;
  float m_fThicknessThreshold = 0.2f;
  float m_fVertexRingDetail = 0.2f;
  plBitflags<plKrautTreeTypeBits> m_AllowBranch = plKrautTreeTypeBits::Default;
  plBitflags<plKrautTreeTypeBits> m_AllowFrond = plKrautTreeTypeBits::Default;
  plBitflags<plKrautTreeTypeBits> m_AllowLeaf = plKrautTreeTypeBits::Default;
  plInt8 m_iMaxFrondDetail = 32;
  plInt8 m_iFrondDetailReduction = 0;
  plUInt32 m_uiLodDistance = 0;
  plEnum<plKrautBranchSpikeTipMode> m_BranchSpikeTipMode = plKrautBranchSpikeTipMode::FullDetail;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautAssetLod);

class plKrautTreeAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetProperties, plReflectedClass);

public:
  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  plKrautTreeAssetProperties();
  ~plKrautTreeAssetProperties();

  //float m_fUniformScaling = 1.0f;
  //float m_fLodDistanceScale = 1.0f;
  float m_fStaticColliderRadius = 0.4f;
  float m_fTreeStiffness = 10.0f;
  float m_fMinAmbientOcclusion = 0.7f;
  plString m_sSurface;

  plHybridArray<plKrautAssetMaterial, 8> m_Materials;
  plKrautAssetBranchType m_BT_Trunk1;
  //plKrautAssetBranchType m_BT_Trunk2;
  //plKrautAssetBranchType m_BT_Trunk3;
  plKrautAssetBranchType m_BT_MainBranch1;
  plKrautAssetBranchType m_BT_MainBranch2;
  plKrautAssetBranchType m_BT_MainBranch3;
  plKrautAssetBranchType m_BT_SubBranch1;
  plKrautAssetBranchType m_BT_SubBranch2;
  plKrautAssetBranchType m_BT_SubBranch3;
  plKrautAssetBranchType m_BT_Twig1;
  plKrautAssetBranchType m_BT_Twig2;
  plKrautAssetBranchType m_BT_Twig3;

  plKrautAssetLod m_Lod0;
  plKrautAssetLod m_Lod1;
  plKrautAssetLod m_Lod2;
  plKrautAssetLod m_Lod3;
  plKrautAssetLod m_Lod4;

  plUInt16 m_uiRandomSeedForDisplay = 0;

  plHybridArray<plUInt16, 16> m_GoodRandomSeeds;
};
