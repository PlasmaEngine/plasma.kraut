#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>
#include <KrautGenerator/Serialization/SerializeTree.h>
#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QToolButton>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

using namespace AE_NS_FOUNDATION;

namespace
{
  constexpr const char* s_szBranchTypeMimeType = "application/plEditor.KrautBranchType";
  constexpr const char* s_szLodMimeType = "application/plEditor.KrautLod";

  struct KrautLodPreset
  {
    const char* szName;
    int iMode; // plKrautLodMode
    float fCurvatureThreshold;
    float fThicknessThreshold;
    float fTipDetail;
    float fVertexRingDetail;
    plInt8 iMaxFrondDetail;
    plInt8 iFrondDetailReduction;
    plUInt32 uiLodDistance;
    int iBranchSpikeTipMode; // plKrautBranchSpikeTipMode
  };

  // clang-format off
  static const KrautLodPreset s_LodPresets[] = {
    //  Name         Mode                           Curv   Thick  Tip    Ring   Frond  Redu  Dist  SpikeTip
    { "Ultra",     (int)plKrautLodMode::Full,     0.5f,  0.02f, 0.01f, 0.05f, 32,    0,    8,    (int)plKrautBranchSpikeTipMode::FullDetail     },
    { "Very High", (int)plKrautLodMode::Full,     1.5f,  0.04f, 0.03f, 0.15f, 32,    0,    15,   (int)plKrautBranchSpikeTipMode::FullDetail     },
    { "High",      (int)plKrautLodMode::Full,     2.0f,  0.05f, 0.04f, 0.20f, 32,    0,    20,   (int)plKrautBranchSpikeTipMode::SingleTriangle },
    { "Medium",    (int)plKrautLodMode::Full,     5.0f,  0.10f, 0.10f, 0.40f, 6,     1,    25,   (int)plKrautBranchSpikeTipMode::Hole           },
    { "Low",       (int)plKrautLodMode::Full,     10.0f, 0.15f, 0.20f, 0.60f, 4,     2,    40,   (int)plKrautBranchSpikeTipMode::Hole           },
    { "Very Low",  (int)plKrautLodMode::Full,     15.0f, 0.20f, 0.30f, 0.80f, 2,     3,    50,   (int)plKrautBranchSpikeTipMode::Hole           },
  };
  // clang-format on

  // Default preset index (into s_LodPresets) applied when adding each LOD for the first time.
  // Indices: 0=Ultra,1=VeryHigh,2=High,3=Medium,4=Low,5=VeryLow,6=Disabled
  static const int s_iDefaultPresetForLod[5] = {0, 1, 2, 3, 4};

  static const char* s_szLodProps[5] = {"LOD0", "LOD1", "LOD2", "LOD3", "LOD4"};
  static const char* s_szLodNames[5] = {"LOD 0", "LOD 1", "LOD 2", "LOD 3", "LOD 4"};

  class KrautOSFileStreamIn : public aeStreamIn
  {
  public:
    plOSFile m_File;

  private:
    virtual aeUInt32 ReadFromStream(void* pData, aeUInt32 uiSize) override { return (aeUInt32)m_File.Read(pData, uiSize); }
  };
} // namespace

plQtKrautTreeAssetDocumentWindow::plQtKrautTreeAssetDocumentWindow(plAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "KrautTreeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "KrautTreeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("KrautTreeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  plQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(plVec3(0, 0, 2), plVec3(10.0f), plVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, "MeshAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }


  // Property Grid
  {
    plDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

    plDeque<const plDocumentObject*> sel;
    sel.PushBack(pRootObject);

    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("KrautTreeAssetDockWidget");
    pPropertyPanel->setWindowTitle("Kraut Tree Properties");
    pPropertyPanel->show();

    QTabWidget* pTabWidget = new QTabWidget(pPropertyPanel);

    QWidget* pAssetTab = new QWidget(pTabWidget);
    pAssetTab->setLayout(new QVBoxLayout);
    pAssetTab->layout()->setContentsMargins(0, 0, 0, 0);

    QPushButton* pImportKrautButton = new QPushButton("Import Kraut File", pAssetTab);
    pAssetTab->layout()->addWidget(pImportKrautButton);

    plQtPropertyGridWidget* pAssetProps = new plQtPropertyGridWidget(pAssetTab, pDocument, false);
    pAssetProps->SetSelectionIncludeExcludeProperties(nullptr, "Materials;BT_Trunk1;BT_Trunk2;BT_Trunk3;BT_MainBranch1;BT_MainBranch2;BT_MainBranch3;BT_SubBranch1;BT_SubBranch2;BT_SubBranch3;BT_Twig1;BT_Twig2;BT_Twig3;LOD0;LOD1;LOD2;LOD3;LOD4");
    pAssetProps->SetSelection(sel);
    pAssetTab->layout()->addWidget(pAssetProps);

    pTabWidget->addTab(pAssetTab, "Asset");

    // plQtPropertyGridWidget* pMaterialProps = new plQtPropertyGridWidget(pTabWidget, pDocument, false);
    // pMaterialProps->SetSelectionIncludeExcludeProperties("Materials");
    // pMaterialProps->SetSelection(sel);
    // pTabWidget->addTab(pMaterialProps, "Materials");

    // Branch Type Tab with combo box
    QWidget* pBranchTypeTab = new QWidget(pTabWidget);
    pBranchTypeTab->setLayout(new QVBoxLayout);
    pBranchTypeTab->layout()->setContentsMargins(0, 0, 0, 0);

    QComboBox* pBranchTypeCombo = new QComboBox(pBranchTypeTab);
    pBranchTypeTab->layout()->addWidget(new QLabel("Branch Type to Edit:"));
    pBranchTypeTab->layout()->addWidget(pBranchTypeCombo);

    {
      QWidget* pCopyPasteRow = new QWidget(pBranchTypeTab);
      auto* pRowLayout = new QHBoxLayout(pCopyPasteRow);
      pRowLayout->setContentsMargins(0, 0, 0, 0);
      QPushButton* pCopyButton = new QPushButton("Copy", pCopyPasteRow);
      QPushButton* pPasteButton = new QPushButton("Paste", pCopyPasteRow);
      pRowLayout->addWidget(pCopyButton);
      pRowLayout->addWidget(pPasteButton);
      pBranchTypeTab->layout()->addWidget(pCopyPasteRow);
      connect(pCopyButton, SIGNAL(clicked()), this, SLOT(onCopyBranchType()));
      connect(pPasteButton, SIGNAL(clicked()), this, SLOT(onPasteBranchType()));
    }

    m_pBranchProps = new plQtPropertyGridWidget(pBranchTypeTab, pDocument, false);
    // m_pBranchProps->SetSelectionIncludeExcludeProperties("BT_Trunk1;BT_MainBranch1;BT_MainBranch2");
    // m_pBranchProps->SetSelection(sel);
    pBranchTypeTab->layout()->addWidget(m_pBranchProps);

    pTabWidget->addTab(pBranchTypeTab, "Branch Type");

    // LOD Tab with combo box
    QWidget* pLodTab = new QWidget(pTabWidget);
    pLodTab->setLayout(new QVBoxLayout);
    pLodTab->layout()->setContentsMargins(0, 0, 0, 0);

    QCheckBox* pAutoLodCheckbox = new QCheckBox("Automatic LOD (by distance)", pLodTab);
    pAutoLodCheckbox->setToolTip("When checked, the engine selects the LOD to render based on camera distance.\nWhen unchecked, the LOD selected below is always used for rendering.");
    pAutoLodCheckbox->setChecked(false);
    pLodTab->layout()->addWidget(pAutoLodCheckbox);

    QComboBox* pLodCombo = new QComboBox(pLodTab);
    QPushButton* pAddLodButton = new QPushButton("+", pLodTab);
    pAddLodButton->setToolTip("Add a new LOD");
    pAddLodButton->setFixedWidth(28);
    QPushButton* pDeleteLodButton = new QPushButton("-", pLodTab);
    pDeleteLodButton->setToolTip("Remove the last LOD");
    pDeleteLodButton->setFixedWidth(28);
    pLodTab->layout()->addWidget(new QLabel("LOD Level to Edit:"));
    {
      QWidget* pLodRow = new QWidget(pLodTab);
      auto* pLodRowLayout = new QHBoxLayout(pLodRow);
      pLodRowLayout->setContentsMargins(0, 0, 0, 0);
      pLodRowLayout->addWidget(pLodCombo, 1);
      pLodRowLayout->addWidget(pAddLodButton);
      pLodRowLayout->addWidget(pDeleteLodButton);
      pLodTab->layout()->addWidget(pLodRow);
    }

    QToolButton* pPresetButton = new QToolButton(pLodTab);
    pPresetButton->setText("Apply Preset");
    pPresetButton->setPopupMode(QToolButton::InstantPopup);
    {
      QMenu* pPresetMenu = new QMenu(pPresetButton);
      for (int i = 0; i < (int)PL_ARRAY_SIZE(s_LodPresets); ++i)
      {
        if (i == (int)PL_ARRAY_SIZE(s_LodPresets) - 1)
          pPresetMenu->addSeparator();
        QAction* pAction = pPresetMenu->addAction(s_LodPresets[i].szName);
        connect(pAction, &QAction::triggered, this, [this, i]()
          { ApplyLodPreset(i); });
      }
      pPresetButton->setMenu(pPresetMenu);
    }
    pLodTab->layout()->addWidget(pPresetButton);

    {
      QWidget* pCopyPasteRow = new QWidget(pLodTab);
      auto* pRowLayout = new QHBoxLayout(pCopyPasteRow);
      pRowLayout->setContentsMargins(0, 0, 0, 0);
      QPushButton* pCopyLodButton = new QPushButton("Copy", pCopyPasteRow);
      QPushButton* pPasteLodButton = new QPushButton("Paste", pCopyPasteRow);
      pRowLayout->addWidget(pCopyLodButton);
      pRowLayout->addWidget(pPasteLodButton);
      pLodTab->layout()->addWidget(pCopyPasteRow);
      connect(pCopyLodButton, SIGNAL(clicked()), this, SLOT(onCopyLod()));
      connect(pPasteLodButton, SIGNAL(clicked()), this, SLOT(onPasteLod()));
    }

    m_pLodProps = new plQtPropertyGridWidget(pLodTab, pDocument, false);
    pLodTab->layout()->addWidget(m_pLodProps);

    pTabWidget->addTab(pLodTab, "LOD");

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new plQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pTabWidget);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

    connect(pImportKrautButton, SIGNAL(clicked()), this, SLOT(onImportKrautFile()));

    {
      m_pBranchTypeCombo = pBranchTypeCombo;
      connect(pBranchTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onBranchTypeSelected(int)));
      RebuildBranchTypeCombo();
    }

    {
      m_pLodCombo = pLodCombo;
      m_pAddLodButton = pAddLodButton;
      m_pDeleteLodButton = pDeleteLodButton;
      m_pAutoLodCheckbox = pAutoLodCheckbox;
      connect(pLodCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onLodSelected(int)));
      connect(pAddLodButton, SIGNAL(clicked()), this, SLOT(onAddLod()));
      connect(pDeleteLodButton, SIGNAL(clicked()), this, SLOT(onDeleteLod()));
      connect(pAutoLodCheckbox, SIGNAL(toggled(bool)), this, SLOT(onAutoLodToggled(bool)));
      RebuildLodCombo();
    }
  }

  m_pAssetDoc = static_cast<plKrautTreeAssetDocument*>(pDocument);

  FinishWindowCreation();

  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtKrautTreeAssetDocumentWindow::PropertyEventHandler, this));
}

plQtKrautTreeAssetDocumentWindow::~plQtKrautTreeAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtKrautTreeAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

void plQtKrautTreeAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void plQtKrautTreeAssetDocumentWindow::QueryObjectBBox(plInt32 iPurpose /*= 0*/)
{
  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void plQtKrautTreeAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Kraut Tree";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtKrautTreeAssetDocumentWindow::onImportKrautFile()
{
  ImportKrautFile();
}

void plQtKrautTreeAssetDocumentWindow::ImportKrautFile()
{
  const QString sSelectedFile = QFileDialog::getOpenFileName(this, "Import Kraut Tree File", "", "Kraut Tree (*.tree)", nullptr, QFileDialog::Option::DontResolveSymlinks);
  if (sSelectedFile.isEmpty())
    return;

  KrautOSFileStreamIn kstream;
  if (kstream.m_File.Open(sSelectedFile.toUtf8().constData(), plFileOpenMode::Read).Failed())
  {
    QMessageBox::warning(this, "Import Failed", QString("Could not open file:\n%1").arg(sSelectedFile));
    return;
  }

  plUInt32 uiKrautEditorVersion = 0;
  kstream.Read(&uiKrautEditorVersion, sizeof(uiKrautEditorVersion));

  Kraut::TreeStructureDesc treeStructure;
  Kraut::LodDesc lodDescs[5];

  Kraut::Deserializer ts;
  ts.m_pTreeStructure = &treeStructure;
  for (int i = 0; i < 5; ++i)
    ts.m_LODs[i] = &lodDescs[i];

  if (!ts.Deserialize(kstream))
  {
    QMessageBox::warning(this, "Import Failed", "Failed to parse the Kraut tree file. The file may be corrupt or in an unsupported format.");
    return;
  }

  auto pHistory = GetDocument()->GetCommandHistory();
  pHistory->StartTransaction("Import Kraut File");

  plDocumentObject* pRootObject = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  const plUuid rootGuid = pRootObject->GetGuid();

  auto SetProperty = [&](const plUuid& objectGuid, const char* szProperty, const plVariant& value)
  {
    plSetObjectPropertyCommand cmd;
    cmd.m_Object = objectGuid;
    cmd.m_sProperty = szProperty;
    cmd.m_NewValue = value;
    pHistory->AddCommand(cmd).AssertSuccess();
  };

  // Set the random seed from the tree structure
  SetProperty(rootGuid, "DisplayRandomSeed", (plUInt16)treeStructure.m_uiRandomSeed);

  // Branch types: map editor property name -> Kraut SpawnNodeDesc index
  const char* szBranchTypeProps[12] = {
    "BT_Trunk1", nullptr, nullptr,
    "BT_MainBranch1", "BT_MainBranch2", "BT_MainBranch3",
    "BT_SubBranch1", "BT_SubBranch2", "BT_SubBranch3",
    "BT_Twig1", "BT_Twig2", "BT_Twig3"};

  for (int n = 0; n < 12; ++n)
  {
    if (szBranchTypeProps[n] == nullptr)
      continue;

    plDocumentObject* pBtObject = nullptr;
    for (plDocumentObject* pChild : pRootObject->GetChildren())
    {
      if (pChild->GetParentProperty() == szBranchTypeProps[n])
      {
        pBtObject = pChild;
        break;
      }
    }

    if (pBtObject == nullptr)
      continue;

    const Kraut::SpawnNodeDesc& nd = treeStructure.m_BranchTypes[n];
    const plUuid guid = pBtObject->GetGuid();

    // Administrative
    SetProperty(guid, "GrowSubBranchType1", nd.m_bAllowSubType[0]);
    SetProperty(guid, "GrowSubBranchType2", nd.m_bAllowSubType[1]);
    SetProperty(guid, "GrowSubBranchType3", nd.m_bAllowSubType[2]);

    // Geometry enables
    SetProperty(guid, "EnableMesh", nd.m_bEnable[Kraut::BranchGeometryType::Branch]);
    SetProperty(guid, "EnableFronds", nd.m_bEnable[Kraut::BranchGeometryType::Frond]);
    SetProperty(guid, "EnableLeaves", nd.m_bEnable[Kraut::BranchGeometryType::Leaf]);

    // General
    SetProperty(guid, "SegmentLength", (plUInt8)plMath::Clamp<int>(nd.m_iSegmentLengthCM, 1, 50));
    SetProperty(guid, "BranchType", (int)nd.m_BranchTypeMode);
    SetProperty(guid, "BranchlessPartABS", nd.m_fBranchlessPartABS);
    SetProperty(guid, "BranchlessPartEndABS", nd.m_fBranchlessPartEndABS);
    SetProperty(guid, "LowerBound", (plUInt8)nd.m_uiLowerBound);
    SetProperty(guid, "UpperBound", (plUInt8)nd.m_uiUpperBound);
    SetProperty(guid, "MinBranchThickness", (plUInt16)nd.m_uiMinBranchThicknessInCM);
    SetProperty(guid, "MaxBranchThickness", (plUInt16)nd.m_uiMaxBranchThicknessInCM);

    // Spawn Nodes
    SetProperty(guid, "MinBranchesPerNode", (plUInt8)nd.m_uiMinBranches);
    SetProperty(guid, "MaxBranchesPerNode", (plUInt8)nd.m_uiMaxBranches);
    SetProperty(guid, "NodeSpacingBefore", nd.m_fNodeSpacingBefore);
    SetProperty(guid, "NodeSpacingAfter", nd.m_fNodeSpacingAfter);
    SetProperty(guid, "NodeHeight", nd.m_fNodeHeight);

    // Growth - Start Direction
    SetProperty(guid, "MaxRotationalDeviation", plAngle::MakeFromDegree(nd.m_fMaxRotationalDeviation));
    SetProperty(guid, "BranchAngle", plAngle::MakeFromDegree(nd.m_fBranchAngle));
    SetProperty(guid, "MaxBranchAngleDeviation", plAngle::MakeFromDegree(nd.m_fMaxBranchAngleDeviation));

    // Growth - Target Direction
    SetProperty(guid, "TargetDirection", (int)nd.m_TargetDirection);
    SetProperty(guid, "TargetDirRelative", nd.m_bTargetDirRelative);
    SetProperty(guid, "TargetDir2Usage", (int)nd.m_TargetDir2Usage);
    SetProperty(guid, "TargetDir2Offset", nd.m_fTargetDir2Usage);
    SetProperty(guid, "TargetDirection2", (int)nd.m_TargetDirection2);
    SetProperty(guid, "MaxTargetDirDeviation", plAngle::MakeFromDegree(nd.m_fMaxTargetDirDeviation));

    // Growth - Branch Behavior
    SetProperty(guid, "MinBranchLength", (plUInt16)nd.m_uiMinBranchLengthInCM);
    SetProperty(guid, "MaxBranchLength", (plUInt16)nd.m_uiMaxBranchLengthInCM);
    SetProperty(guid, "TargetDirDeviation", plAngle::MakeFromDegree(nd.m_fGrowMaxTargetDirDeviation));
    SetProperty(guid, "DirChangePerSegment", plAngle::MakeFromDegree(nd.m_fGrowMaxDirChangePerSegment));
    SetProperty(guid, "OnlyGrowUpAndDown", nd.m_bRestrictGrowthToFrondPlane);

    // Appearance - Branch Mesh
    SetProperty(guid, "Roundness", nd.m_fRoundnessFactor);
    SetProperty(guid, "Flares", (plUInt8)nd.m_uiFlares);
    SetProperty(guid, "FlareWidth", nd.m_fFlareWidth);
    SetProperty(guid, "FlareRotation", plAngle::MakeFromDegree(nd.m_fFlareRotation));
    SetProperty(guid, "RotateTexCoords", nd.m_bRotateTexCoords);

    // Appearance - Fronds
    SetProperty(guid, "TextureRepeat", nd.m_fTextureRepeat);
    SetProperty(guid, "FrondUpOrientation", (int)nd.m_FrondUpOrientation);
    SetProperty(guid, "FrondOrientationDeviation", plAngle::MakeFromDegree((float)nd.m_uiMaxFrondOrientationDeviation));
    SetProperty(guid, "NumFronds", (plUInt8)nd.m_uiNumFronds);
    SetProperty(guid, "AlignFrondsOnSurface", nd.m_bAlignFrondsOnSurface);
    SetProperty(guid, "FrondDetail", (plUInt8)nd.m_uiFrondDetail);
    SetProperty(guid, "FrondContourMode", (int)nd.m_FrondContourMode);
    SetProperty(guid, "FrondHeight", nd.m_fFrondHeight);
    SetProperty(guid, "FrondWidth", nd.m_fFrondWidth);

    // Appearance - Leaves
    SetProperty(guid, "BillboardLeaves", nd.m_bBillboardLeaves);
    SetProperty(guid, "LeafSize", nd.m_fLeafSize);
    SetProperty(guid, "LeafInterval", nd.m_fLeafInterval);
  }

  // LODs
  const char* szLodProps[5] = {"LOD0", "LOD1", "LOD2", "LOD3", "LOD4"};
  for (int n = 0; n < 5; ++n)
  {
    plDocumentObject* pLodObject = nullptr;
    for (plDocumentObject* pChild : pRootObject->GetChildren())
    {
      if (pChild->GetParentProperty() == szLodProps[n])
      {
        pLodObject = pChild;
        break;
      }
    }

    if (pLodObject == nullptr)
      continue;

    const Kraut::LodDesc& lod = lodDescs[n];
    const plUuid guid = pLodObject->GetGuid();

    SetProperty(guid, "Mode", (int)lod.m_Mode);
    SetProperty(guid, "TipDetail", lod.m_fTipDetail);
    SetProperty(guid, "CurvatureThreshold", lod.m_fCurvatureThreshold);
    SetProperty(guid, "ThicknessThreshold", lod.m_fThicknessThreshold);
    SetProperty(guid, "VertexRingDetail", lod.m_fVertexRingDetail);
    SetProperty(guid, "AllowBranch", (int)lod.m_AllowTypes[Kraut::BranchGeometryType::Branch]);
    SetProperty(guid, "AllowFrond", (int)lod.m_AllowTypes[Kraut::BranchGeometryType::Frond]);
    SetProperty(guid, "AllowLeaf", (int)lod.m_AllowTypes[Kraut::BranchGeometryType::Leaf]);
    SetProperty(guid, "MaxFrondDetail", (plInt8)lod.m_iMaxFrondDetail);
    SetProperty(guid, "FrondDetailReduction", (plInt8)lod.m_iFrondDetailReduction);
    SetProperty(guid, "LodDistance", lod.m_uiLodDistance);
    SetProperty(guid, "BranchSpikeTipMode", (int)lod.m_BranchSpikeTipMode);
  }

  pHistory->FinishTransaction();
}

void plQtKrautTreeAssetDocumentWindow::UpdatePreview()
{
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Kraut Tree";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  plStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plKrautTree");

  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetTransformHash(GetDocument()->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).AssertSuccess();

  // Write Asset Data
  GetKrautDocument()->WriteKrautAsset(memoryWriter).IgnoreResult();
  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtKrautTreeAssetDocumentWindow::InternalRedraw()
{
  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtKrautTreeAssetDocumentWindow::ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plQuerySelectionBBoxResultMsgToEditor>())
  {
    const plQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const plQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(plVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  plQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}

void plQtKrautTreeAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "DisplayRandomSeed")
  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "UpdateTree";
    msg.m_sPayload = "DisplayRandomSeed";
    msg.m_PayloadValue = static_cast<plKrautTreeAssetDocument*>(GetDocument())->GetProperties()->m_uiRandomSeedForDisplay;

    GetDocument()->SendMessageToEngine(&msg);
  }
  else
  {
    if (e.m_sProperty == "GrowSubBranchType1" || e.m_sProperty == "GrowSubBranchType2" || e.m_sProperty == "GrowSubBranchType3")
    {
      RebuildBranchTypeCombo();
    }

    if (e.m_sProperty == "Mode")
    {
      RebuildLodCombo();
    }

    UpdatePreview();
  }
}

void plQtKrautTreeAssetDocumentWindow::RebuildBranchTypeCombo()
{
  if (m_pBranchTypeCombo == nullptr)
    return;

  plDocumentObject* pRootObject = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  auto GetBoolProp = [&](const char* szBranchTypeProp, const char* szBoolProp) -> bool
  {
    for (plDocumentObject* pChild : pRootObject->GetChildren())
    {
      if (pChild->GetParentProperty() == szBranchTypeProp)
        return pChild->GetTypeAccessor().GetValue(szBoolProp).ConvertTo<bool>();
    }
    return false;
  };

  const bool bMainBranch1 = GetBoolProp("BT_Trunk1", "GrowSubBranchType1");
  const bool bMainBranch2 = GetBoolProp("BT_Trunk1", "GrowSubBranchType2");
  const bool bMainBranch3 = GetBoolProp("BT_Trunk1", "GrowSubBranchType3");

  const bool bSubBranch1 = (bMainBranch1 && GetBoolProp("BT_MainBranch1", "GrowSubBranchType1")) ||
                           (bMainBranch2 && GetBoolProp("BT_MainBranch2", "GrowSubBranchType1")) ||
                           (bMainBranch3 && GetBoolProp("BT_MainBranch3", "GrowSubBranchType1"));
  const bool bSubBranch2 = (bMainBranch1 && GetBoolProp("BT_MainBranch1", "GrowSubBranchType2")) ||
                           (bMainBranch2 && GetBoolProp("BT_MainBranch2", "GrowSubBranchType2")) ||
                           (bMainBranch3 && GetBoolProp("BT_MainBranch3", "GrowSubBranchType2"));
  const bool bSubBranch3 = (bMainBranch1 && GetBoolProp("BT_MainBranch1", "GrowSubBranchType3")) ||
                           (bMainBranch2 && GetBoolProp("BT_MainBranch2", "GrowSubBranchType3")) ||
                           (bMainBranch3 && GetBoolProp("BT_MainBranch3", "GrowSubBranchType3"));

  const bool bTwig1 = (bSubBranch1 && GetBoolProp("BT_SubBranch1", "GrowSubBranchType1")) ||
                      (bSubBranch2 && GetBoolProp("BT_SubBranch2", "GrowSubBranchType1")) ||
                      (bSubBranch3 && GetBoolProp("BT_SubBranch3", "GrowSubBranchType1"));
  const bool bTwig2 = (bSubBranch1 && GetBoolProp("BT_SubBranch1", "GrowSubBranchType2")) ||
                      (bSubBranch2 && GetBoolProp("BT_SubBranch2", "GrowSubBranchType2")) ||
                      (bSubBranch3 && GetBoolProp("BT_SubBranch3", "GrowSubBranchType2"));
  const bool bTwig3 = (bSubBranch1 && GetBoolProp("BT_SubBranch1", "GrowSubBranchType3")) ||
                      (bSubBranch2 && GetBoolProp("BT_SubBranch2", "GrowSubBranchType3")) ||
                      (bSubBranch3 && GetBoolProp("BT_SubBranch3", "GrowSubBranchType3"));

  const QString sPrevData = m_pBranchTypeCombo->currentData().toString();

  m_pBranchTypeCombo->blockSignals(true);
  m_pBranchTypeCombo->clear();

  m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::Trunk, "BT_Trunk1");
  if (bMainBranch1)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::MainBranch1, "BT_MainBranch1");
  if (bMainBranch2)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::MainBranch2, "BT_MainBranch2");
  if (bMainBranch3)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::MainBranch3, "BT_MainBranch3");
  if (bSubBranch1)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::SubBranch1, "BT_SubBranch1");
  if (bSubBranch2)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::SubBranch2, "BT_SubBranch2");
  if (bSubBranch3)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::SubBranch3, "BT_SubBranch3");
  if (bTwig1)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::Twig1, "BT_Twig1");
  if (bTwig2)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::Twig2, "BT_Twig2");
  if (bTwig3)
    m_pBranchTypeCombo->addItem(plKrautBranchTypeNames::Twig3, "BT_Twig3");

  // Restore previous selection, fall back to Trunk if it is no longer present
  const int iNewIndex = m_pBranchTypeCombo->findData(sPrevData);
  const int iSelectIndex = iNewIndex >= 0 ? iNewIndex : 0;
  m_pBranchTypeCombo->setCurrentIndex(iSelectIndex);
  m_pBranchTypeCombo->blockSignals(false);

  // Always explicitly sync the property grid, since setCurrentIndex may not emit
  // a signal when the index did not change (e.g. on first population of the combo)
  onBranchTypeSelected(iSelectIndex);
}

void plQtKrautTreeAssetDocumentWindow::RebuildLodCombo()
{
  if (m_pLodCombo == nullptr)
    return;

  plDocumentObject* pRoot = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  // LOD 0 is always active; check others by reading their Mode property
  bool bLodActive[5] = {true, false, false, false, false};
  for (int i = 1; i < 5; ++i)
  {
    for (plDocumentObject* pChild : pRoot->GetChildren())
    {
      if (pChild->GetParentProperty() == s_szLodProps[i])
      {
        bLodActive[i] = (pChild->GetTypeAccessor().GetValue("Mode").ConvertTo<plInt32>() != plKrautLodMode::Disabled);
        break;
      }
    }
  }

  int iNumActiveLods = 0;
  int iLastActiveLodIndex = 0;
  for (int i = 0; i < 5; ++i)
  {
    if (bLodActive[i])
    {
      ++iNumActiveLods;
      iLastActiveLodIndex = i;
    }
  }

  m_pLodCombo->blockSignals(true);
  m_pLodCombo->clear();

  // "Full Detail" is always the first entry and maps to LOD override -1 (distance-based selection)
  m_pLodCombo->addItem("Full Detail", -1);

  // iSelectIndex defaults to 0 ("Full Detail"), overridden below if m_iCurrentLodIndex matches a LOD
  int iSelectIndex = 0;
  for (int i = 0; i < 5; ++i)
  {
    if (bLodActive[i])
    {
      m_pLodCombo->addItem(s_szLodNames[i], i);
      if (i == m_iCurrentLodIndex)
        iSelectIndex = m_pLodCombo->count() - 1;
    }
  }

  iSelectIndex = plMath::Clamp(iSelectIndex, 0, m_pLodCombo->count() - 1);
  m_pLodCombo->setCurrentIndex(iSelectIndex);
  m_pLodCombo->blockSignals(false);

  if (m_pAddLodButton)
    m_pAddLodButton->setEnabled(iNumActiveLods < 5);
  if (m_pDeleteLodButton)
  {
    const int iSelectedLodIndex = m_pLodCombo->itemData(iSelectIndex).toInt();
    m_pDeleteLodButton->setEnabled(iSelectedLodIndex == iLastActiveLodIndex && iLastActiveLodIndex > 0);
  }

  onLodSelected(iSelectIndex);
}

void plQtKrautTreeAssetDocumentWindow::onBranchTypeSelected(int index)
{
  if (m_pBranchProps == nullptr || m_pBranchTypeCombo == nullptr)
    return;

  const QByteArray sParentProp = m_pBranchTypeCombo->itemData(index).toString().toUtf8();
  if (sParentProp.isEmpty())
    return;

  plDocumentObject* pRootObject = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  plDocumentObject* pSelected = nullptr;

  for (plDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (pChild->GetParentProperty() == sParentProp.constData())
    {
      pSelected = pChild;
      break;
    }
  }

  if (pSelected == nullptr)
    return;

  plDeque<const plDocumentObject*> sel;
  sel.PushBack(pSelected);
  GetDocument()->GetSelectionManager()->SetSelection(pSelected);
  m_pBranchProps->SetSelection(sel);
}

plDocumentObject* plQtKrautTreeAssetDocumentWindow::GetCurrentBranchTypeObject() const
{
  if (m_pBranchTypeCombo == nullptr)
    return nullptr;

  const QByteArray sProp = m_pBranchTypeCombo->currentData().toString().toUtf8();
  plDocumentObject* pRoot = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  for (plDocumentObject* pChild : pRoot->GetChildren())
  {
    if (pChild->GetParentProperty() == sProp.constData())
      return pChild;
  }
  return nullptr;
}

plDocumentObject* plQtKrautTreeAssetDocumentWindow::GetCurrentLodObject() const
{
  if (m_iCurrentLodIndex < 0)
    return nullptr;
  plDocumentObject* pRoot = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  for (plDocumentObject* pChild : pRoot->GetChildren())
  {
    if (pChild->GetParentProperty() == s_szLodProps[m_iCurrentLodIndex])
      return pChild;
  }
  return nullptr;
}

void plQtKrautTreeAssetDocumentWindow::CopyObjectToClipboard(const plDocumentObject* pObject, const char* szMimeType)
{
  if (pObject == nullptr)
    return;

  plAbstractObjectGraph graph;
  plDocumentObjectConverterWriter writer(&graph, GetDocument()->GetObjectManager());
  writer.AddObjectToGraph(pObject, "root");

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memWriter(&streamStorage);
  plAbstractGraphDdlSerializer::Write(memWriter, &graph, nullptr, false);

  QMimeData* pMimeData = new QMimeData();
  pMimeData->setData(szMimeType, QByteArray((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32()));
  QApplication::clipboard()->setMimeData(pMimeData);
}

void plQtKrautTreeAssetDocumentWindow::PasteObjectFromClipboard(plDocumentObject* pObject, const char* szMimeType, const char* szTransactionName)
{
  if (pObject == nullptr)
    return;

  const QMimeData* pMimeData = QApplication::clipboard()->mimeData();
  if (!pMimeData->hasFormat(szMimeType))
    return;

  const QByteArray data = pMimeData->data(szMimeType);

  plAbstractObjectGraph clipboardGraph;
  {
    plRawMemoryStreamReader memReader(data.constData(), data.size());
    if (plAbstractGraphDdlSerializer::Read(memReader, &clipboardGraph).Failed())
      return;
  }

  plAbstractObjectGraph baseGraph;
  {
    plDocumentObjectConverterWriter writer(&baseGraph, GetDocument()->GetObjectManager());
    writer.AddObjectToGraph(pObject, "root");
  }

  plAbstractObjectNode* pClipRoot = clipboardGraph.GetNodeByName("root");
  const plAbstractObjectNode* pBaseRoot = baseGraph.GetNodeByName("root");
  if (pClipRoot == nullptr || pBaseRoot == nullptr)
    return;

  // Remap clipboard GUIDs to match the target object's GUIDs so the diff can be applied
  clipboardGraph.ReMapNodeGuidsToMatchGraph(pClipRoot, baseGraph, pBaseRoot);

  plDeque<plAbstractGraphDiffOperation> diff;
  clipboardGraph.CreateDiffWithBaseGraph(baseGraph, diff);

  if (diff.IsEmpty())
    return;

  plObjectCommandAccessor accessor(GetDocument()->GetCommandHistory());
  accessor.StartTransaction(szTransactionName);
  plDocumentObjectConverterReader::ApplyDiffToObject(&accessor, pObject, diff);
  accessor.FinishTransaction();
}

void plQtKrautTreeAssetDocumentWindow::onCopyBranchType()
{
  CopyObjectToClipboard(GetCurrentBranchTypeObject(), s_szBranchTypeMimeType);
}

void plQtKrautTreeAssetDocumentWindow::onPasteBranchType()
{
  PasteObjectFromClipboard(GetCurrentBranchTypeObject(), s_szBranchTypeMimeType, "Paste Branch Type");
}

void plQtKrautTreeAssetDocumentWindow::onCopyLod()
{
  CopyObjectToClipboard(GetCurrentLodObject(), s_szLodMimeType);
}

void plQtKrautTreeAssetDocumentWindow::onPasteLod()
{
  PasteObjectFromClipboard(GetCurrentLodObject(), s_szLodMimeType, "Paste LOD");
}

void plQtKrautTreeAssetDocumentWindow::onLodSelected(int index)
{
  if (m_pLodProps == nullptr || m_pLodCombo == nullptr)
    return;

  m_iCurrentLodIndex = m_pLodCombo->itemData(index).toInt();

  // Update delete button: only enabled when the last active LOD is currently selected
  if (m_pDeleteLodButton)
  {
    plDocumentObject* pRoot = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
    int iLastActiveLodIndex = 0;
    for (int i = 1; i < 5; ++i)
    {
      for (plDocumentObject* pChild : pRoot->GetChildren())
      {
        if (pChild->GetParentProperty() == s_szLodProps[i])
        {
          if (pChild->GetTypeAccessor().GetValue("Mode").ConvertTo<plInt32>() != plKrautLodMode::Disabled)
            iLastActiveLodIndex = i;
          break;
        }
      }
    }
    m_pDeleteLodButton->setEnabled(m_iCurrentLodIndex == iLastActiveLodIndex && iLastActiveLodIndex > 0);
  }

  // Update property grid: show the selected LOD's properties, or clear it for "Full Detail"
  plDocumentObject* pSelected = GetCurrentLodObject(); // returns nullptr for Full Detail (-1)
  {
    plDeque<const plDocumentObject*> sel;
    if (pSelected != nullptr)
    {
      sel.PushBack(pSelected);
      GetDocument()->GetSelectionManager()->SetSelection(pSelected);
    }
    m_pLodProps->SetSelection(sel);
  }

  // In auto mode the engine selects the LOD by distance; only send an override in fixed mode
  if (m_pAutoLodCheckbox == nullptr || !m_pAutoLodCheckbox->isChecked())
  {
    // LOD index 0 = full-detail, 1..N = regular LODs (m_iCurrentLodIndex + 1)
    // m_iCurrentLodIndex == -1 (Full Detail) maps to override index 0
    // m_iCurrentLodIndex == 0..4 (regular LODs) map to override indices 1..5
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "SetLodOverride";
    msg.m_sPayload = "";
    msg.m_PayloadValue = m_iCurrentLodIndex + 1;
    GetDocument()->SendMessageToEngine(&msg);
  }
}

void plQtKrautTreeAssetDocumentWindow::onAutoLodToggled(bool bChecked)
{
  plSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "SetLodOverride";
  msg.m_sPayload = "";

  if (bChecked)
  {
    // -1 tells the component to select the LOD by camera distance
    msg.m_PayloadValue = -1;
  }
  else
  {
    // Restore the fixed override for the currently selected LOD
    msg.m_PayloadValue = m_iCurrentLodIndex + 1;
  }

  GetDocument()->SendMessageToEngine(&msg);
}

void plQtKrautTreeAssetDocumentWindow::onAddLod()
{
  plDocumentObject* pRoot = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  // Find the first disabled LOD (LOD 0 is always active, skip it)
  plDocumentObject* pFirstDisabledLod = nullptr;
  int iLodIndex = -1;
  for (int i = 1; i < 5; ++i)
  {
    for (plDocumentObject* pChild : pRoot->GetChildren())
    {
      if (pChild->GetParentProperty() == s_szLodProps[i])
      {
        if (pChild->GetTypeAccessor().GetValue("Mode").ConvertTo<plInt32>() == plKrautLodMode::Disabled)
        {
          pFirstDisabledLod = pChild;
          iLodIndex = i;
        }
        break;
      }
    }
    if (pFirstDisabledLod != nullptr)
      break;
  }

  if (pFirstDisabledLod == nullptr)
    return;

  const KrautLodPreset& preset = s_LodPresets[s_iDefaultPresetForLod[iLodIndex]];

  // Set m_iCurrentLodIndex before the transaction so RebuildLodCombo (triggered by the Mode property
  // change event) selects the newly added LOD.
  m_iCurrentLodIndex = iLodIndex;

  auto pHistory = GetDocument()->GetCommandHistory();
  pHistory->StartTransaction("Add LOD");

  const plUuid objectGuid = pFirstDisabledLod->GetGuid();
  auto SetProperty = [&](const char* szProperty, const plVariant& value)
  {
    plSetObjectPropertyCommand cmd;
    cmd.m_Object = objectGuid;
    cmd.m_sProperty = szProperty;
    cmd.m_NewValue = value;
    pHistory->AddCommand(cmd).AssertSuccess();
  };

  SetProperty("Mode", (int)plKrautLodMode::Full);
  SetProperty("CurvatureThreshold", preset.fCurvatureThreshold);
  SetProperty("ThicknessThreshold", preset.fThicknessThreshold);
  SetProperty("TipDetail", preset.fTipDetail);
  SetProperty("VertexRingDetail", preset.fVertexRingDetail);
  SetProperty("AllowBranch", (int)plKrautTreeTypeBits::Default);
  SetProperty("AllowFrond", (int)plKrautTreeTypeBits::Default);
  SetProperty("AllowLeaf", (int)plKrautTreeTypeBits::Default);
  SetProperty("MaxFrondDetail", preset.iMaxFrondDetail);
  SetProperty("FrondDetailReduction", preset.iFrondDetailReduction);
  SetProperty("LodDistance", preset.uiLodDistance);
  SetProperty("BranchSpikeTipMode", preset.iBranchSpikeTipMode);

  pHistory->FinishTransaction();
}

void plQtKrautTreeAssetDocumentWindow::onDeleteLod()
{
  // The delete button is only enabled when the last active LOD is currently selected.
  plDocumentObject* pLodObject = GetCurrentLodObject();
  if (pLodObject == nullptr)
    return;

  // Move selection to the previous LOD before the transaction so RebuildLodCombo picks the right item.
  m_iCurrentLodIndex = plMath::Max(0, m_iCurrentLodIndex - 1);

  auto pHistory = GetDocument()->GetCommandHistory();
  pHistory->StartTransaction("Delete LOD");

  plSetObjectPropertyCommand cmd;
  cmd.m_Object = pLodObject->GetGuid();
  cmd.m_sProperty = "Mode";
  cmd.m_NewValue = (int)plKrautLodMode::Disabled;
  pHistory->AddCommand(cmd).AssertSuccess();

  pHistory->FinishTransaction();
}

void plQtKrautTreeAssetDocumentWindow::ApplyLodPreset(int iPresetIndex)
{
  PL_ASSERT_DEV(iPresetIndex >= 0 && iPresetIndex < (int)PL_ARRAY_SIZE(s_LodPresets), "Invalid preset index");

  plDocumentObject* pLodObject = GetCurrentLodObject();
  if (pLodObject == nullptr)
    return;

  const KrautLodPreset& preset = s_LodPresets[iPresetIndex];
  auto pHistory = GetDocument()->GetCommandHistory();
  pHistory->StartTransaction("Apply LOD Preset");

  const plUuid objectGuid = pLodObject->GetGuid();
  auto SetProperty = [&](const char* szProperty, const plVariant& value)
  {
    plSetObjectPropertyCommand cmd;
    cmd.m_Object = objectGuid;
    cmd.m_sProperty = szProperty;
    cmd.m_NewValue = value;
    pHistory->AddCommand(cmd).AssertSuccess();
  };

  SetProperty("Mode", preset.iMode);
  SetProperty("CurvatureThreshold", preset.fCurvatureThreshold);
  SetProperty("ThicknessThreshold", preset.fThicknessThreshold);
  SetProperty("TipDetail", preset.fTipDetail);
  SetProperty("VertexRingDetail", preset.fVertexRingDetail);
  SetProperty("AllowBranch", (int)plKrautTreeTypeBits::Default);
  SetProperty("AllowFrond", (int)plKrautTreeTypeBits::Default);
  SetProperty("AllowLeaf", (int)plKrautTreeTypeBits::Default);
  SetProperty("MaxFrondDetail", preset.iMaxFrondDetail);
  SetProperty("FrondDetailReduction", preset.iFrondDetailReduction);
  SetProperty("LodDistance", preset.uiLodDistance);
  SetProperty("BranchSpikeTipMode", preset.iBranchSpikeTipMode);

  pHistory->FinishTransaction();
}
