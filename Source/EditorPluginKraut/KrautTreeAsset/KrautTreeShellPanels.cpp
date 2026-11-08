#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/EngineShell/AssetDocumentContent.h>
#include <EditorFramework/EngineShell/DocumentFrame.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeShellPanels.h>
#include <EditorUi/DockManager.h>
#include <EditorUi/Surface.h>
#include <EditorUi/Widgets.h>
#include <GuiFoundation/EngineUi/EngineUiControlStrip.h>
#include <GuiFoundation/PropertyGrid/EnginePropertyGridWidget.h>

//////////////////////////////////////////////////////////////////////////
// plEditorUiKrautTreePage

plEditorUiKrautTreePage::plEditorUiKrautTreePage(plDocument* pDocument, plActionWindow* pContextWindow)
  : plEditorUiContent("KrautTreePage")
{
  m_pStrip = static_cast<plEditorUiControlStrip*>(AddChild(plUniquePtr<plEditorUiContent>(PL_DEFAULT_NEW(plEditorUiControlStrip))));

  plUniquePtr<plEditorUiPropertyGrid> pGrid = PL_DEFAULT_NEW(plEditorUiPropertyGrid);
  pGrid->SetContextWindow(pContextWindow);
  pGrid->SetSourceDocument(pDocument);
  m_pGrid = static_cast<plEditorUiPropertyGrid*>(AddChild(std::move(pGrid)));
}

plEditorUiKrautTreePage::~plEditorUiKrautTreePage() = default;

void plEditorUiKrautTreePage::OnUiUpdate()
{
  if (!IsReady() || m_pStrip == nullptr || m_pGrid == nullptr)
    return;

  // the strip is as high as its rows, the grid takes the rest
  const plVec2 vSize = GetSurface()->GetLogicalSize();
  const float fStrip = plMath::Min(m_pStrip->GetHeight(), vSize.y);

  SetChildRect(m_pStrip, plRectFloat(0, 0, vSize.x, fStrip));
  SetChildRect(m_pGrid, plRectFloat(0, fStrip, vSize.x, vSize.y - fStrip));
}

//////////////////////////////////////////////////////////////////////////
// plKrautTreeShellPanels

namespace
{
  constexpr const char* s_szPageIds[] = {plKrautTreeShellPanels::s_szAssetPageId, plKrautTreeShellPanels::s_szBranchTypePageId, plKrautTreeShellPanels::s_szLodPageId};
  const plEditorUi::Icon s_PageIcons[] = {plEditorUi::Icon::Properties, plEditorUi::Icon::Tree, plEditorUi::Icon::Layers};
  static_assert(PL_ARRAY_SIZE(s_szPageIds) == PL_ARRAY_SIZE(s_PageIcons));

  void ArrangeKrautTreePanels(plEditorUiDocumentFrame& ref_frame)
  {
    // the window's pages as tabs after the properties, which keep the asset's status banner; the first page is current
    plEditorUiDockManager& panels = ref_frame.GetPanels();
    const char* szPrevious = "Properties";
    const char* szFirst = nullptr;

    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(s_szPageIds); ++i)
    {
      if (panels.GetPanelSurface(s_szPageIds[i]) == nullptr)
        continue;

      panels.SetPanelIcon(s_szPageIds[i], plEditorUi::IconPath(s_PageIcons[i]));
      panels.DockPanel(s_szPageIds[i], szPrevious, plEditorUiDockSide::Tab);
      szPrevious = s_szPageIds[i];

      if (szFirst == nullptr)
        szFirst = s_szPageIds[i];
    }

    if (szFirst != nullptr)
      panels.SetCurrentPanel(szFirst);
  }

  plEngineAssetPanelsProvider s_KrautTreePanelsProvider(plGetStaticRTTI<plKrautTreeAssetDocument>(), &ArrangeKrautTreePanels);
} // namespace

void plKrautTreeShellPanels::Register()
{
  plEditorUiDocumentContentProvider::Register(&s_KrautTreePanelsProvider);
}

void plKrautTreeShellPanels::Unregister()
{
  plEditorUiDocumentContentProvider::Unregister(&s_KrautTreePanelsProvider);
}
