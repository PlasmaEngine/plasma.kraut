#pragma once

#include <EditorUi/Content.h>

class plActionWindow;
class plDocument;
class plEditorUiControlStrip;
class plEditorUiPropertyGrid;

/// \brief A page of the Kraut tree window's side panel (Asset, Branch Type, LOD): a control strip above a property grid.
/// The window owns the page and its logic (import, branch type and LOD choice, presets, copy and paste) serves it.
class plEditorUiKrautTreePage : public plEditorUiContent
{
public:
  /// The grid edits the document's objects the window hands it (SetSelection); it does not follow the selection.
  plEditorUiKrautTreePage(plDocument* pDocument, plActionWindow* pContextWindow);
  ~plEditorUiKrautTreePage() override;

  plEditorUiControlStrip& GetStrip() const { return *m_pStrip; }
  plEditorUiPropertyGrid& GetGrid() const { return *m_pGrid; }

protected:
  void OnUiUpdate() override;

private:
  plEditorUiControlStrip* m_pStrip = nullptr; ///< a child content
  plEditorUiPropertyGrid* m_pGrid = nullptr;  ///< a child content
};

/// \brief The Kraut tree's panels in the engine shell.
///
/// The shell shows a tree's preview and properties by itself (plEngineAssetDocumentContentProvider) and the window's
/// pages (plEditorUiKrautTreePage) with them; this provider puts the pages as tabs after the properties, with their
/// icons, the first page current.
namespace plKrautTreeShellPanels
{
  /// The panel ids of the side panel's pages, in tab order.
  constexpr const char* s_szAssetPageId = "KrautAsset";
  constexpr const char* s_szBranchTypePageId = "KrautBranchType";
  constexpr const char* s_szLodPageId = "KrautLod";

  void Register();
  void Unregister();
} // namespace plKrautTreeShellPanels
