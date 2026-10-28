#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plOrbitCamViewWidget;
class plEditorUiPropertyGrid;
class plEditorUiControlStrip;

class plKrautTreeAssetDocumentWindow : public plEngineDocumentWindow
{
public:
  plKrautTreeAssetDocumentWindow(plAssetDocument* pDocument);
  ~plKrautTreeAssetDocumentWindow();

  plKrautTreeAssetDocument* GetKrautDocument() const
  {
    return static_cast<plKrautTreeAssetDocument*>(GetDocument());
  }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);

private:
  void CreatePages();
  void CreateAssetPage();
  void CreateBranchTypePage();
  void CreateLodPage();

  void OnBranchTypeSelected(int index);
  void OnLodSelected(int index);
  void OnAutoLodToggled(bool bChecked);
  void OnAddLod();
  void OnDeleteLod();
  void OnCopyBranchType();
  void OnPasteBranchType();
  void OnCopyLod();
  void OnPasteLod();

  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);
  void UpdatePreview();
  void RestoreResource();
  void ImportKrautFile();
  void RebuildBranchTypeCombo();
  void RebuildLodCombo();
  void OnAssetControl(plUInt32 uiControl, plInt32 iValue);
  void OnBranchControl(plUInt32 uiControl, plInt32 iValue);
  void OnLodControl(plUInt32 uiControl, plInt32 iValue);
  void ApplyLodPreset(int iPresetIndex);
  plDocumentObject* GetCurrentBranchTypeObject() const;
  plDocumentObject* GetCurrentLodObject() const;
  void CopyObjectToClipboard(const plDocumentObject* pObject, const char* szMimeType);
  void PasteObjectFromClipboard(plDocumentObject* pObject, const char* szMimeType, const char* szTransactionName);

  plEngineViewConfig m_ViewConfig;
  plOrbitCamViewWidget* m_pViewWidget = nullptr;
  plKrautTreeAssetDocument* m_pAssetDoc = nullptr;
  plEditorUiPropertyGrid* m_pBranchProps = nullptr;
  plEditorUiPropertyGrid* m_pLodProps = nullptr;
  // The side panel's pages show a control strip above a property grid; the plUInt32 members are control ids.
  plEditorUiControlStrip* m_pAssetControls = nullptr;
  plUInt32 m_uiImportButton = 0;

  plEditorUiControlStrip* m_pBranchControls = nullptr;
  plUInt32 m_uiBranchTypeCombo = 0;
  plUInt32 m_uiCopyBranchButton = 0;
  plUInt32 m_uiPasteBranchButton = 0;
  plHybridArray<plString, 10> m_BranchTypeProps; ///< property of each branch type choice

  plEditorUiControlStrip* m_pLodControls = nullptr;
  plUInt32 m_uiAutoLodCheckbox = 0;
  plUInt32 m_uiLodCombo = 0;
  plUInt32 m_uiAddLodButton = 0;
  plUInt32 m_uiDeleteLodButton = 0;
  plUInt32 m_uiPresetButton = 0;
  plUInt32 m_uiCopyLodButton = 0;
  plUInt32 m_uiPasteLodButton = 0;
  plHybridArray<int, 6> m_LodChoices; ///< LOD index of each LOD choice, -1 = "Full Detail"

  int m_iCurrentLodIndex = 0; ///< -1 = "Full Detail" (no LOD reduction), 0-4 = forced LOD index
};
