#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plQtPropertyGridWidget;
class QCheckBox;
class QComboBox;
class QPushButton;

class plQtKrautTreeAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtKrautTreeAssetDocumentWindow(plAssetDocument* pDocument);
  ~plQtKrautTreeAssetDocumentWindow();

  plKrautTreeAssetDocument* GetKrautDocument() const
  {
    return static_cast<plKrautTreeAssetDocument*>(GetDocument());
  }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);

private Q_SLOTS:
  void onBranchTypeSelected(int index);
  void onLodSelected(int index);
  void onAutoLodToggled(bool bChecked);
  void onAddLod();
  void onDeleteLod();
  void onImportKrautFile();
  void onCopyBranchType();
  void onPasteBranchType();
  void onCopyLod();
  void onPasteLod();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);
  void UpdatePreview();
  void RestoreResource();
  void ImportKrautFile();
  void RebuildBranchTypeCombo();
  void RebuildLodCombo();
  void ApplyLodPreset(int iPresetIndex);
  plDocumentObject* GetCurrentBranchTypeObject() const;
  plDocumentObject* GetCurrentLodObject() const;
  void CopyObjectToClipboard(const plDocumentObject* pObject, const char* szMimeType);
  void PasteObjectFromClipboard(plDocumentObject* pObject, const char* szMimeType, const char* szTransactionName);

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  plKrautTreeAssetDocument* m_pAssetDoc = nullptr;
  plQtPropertyGridWidget* m_pBranchProps = nullptr;
  plQtPropertyGridWidget* m_pLodProps = nullptr;
  QComboBox* m_pBranchTypeCombo = nullptr;
  QComboBox* m_pLodCombo = nullptr;
  QPushButton* m_pAddLodButton = nullptr;
  QPushButton* m_pDeleteLodButton = nullptr;
  QCheckBox* m_pAutoLodCheckbox = nullptr;
  int m_iCurrentLodIndex = 0; ///< -1 = "Full Detail" (no LOD reduction), 0-4 = forced LOD index
};
