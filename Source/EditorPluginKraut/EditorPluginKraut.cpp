#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeShellPanels.h>
#include <EditorUi/Widgets.h>
#include <Foundation/Configuration/Plugin.h>

/// The package's icon folder: the icons under their ids (":/AssetIcons/Kraut_Tree.svg" is
/// "EditorIcons/AssetIcons/Kraut_Tree.svg"). The plugin binary lives in <package>/Bin/<configuration>.
static plString s_sIconFolder;

static void RegisterIconFolder()
{
  plStringBuilder sPlugin, sCopy;
  plPlugin::GetPluginPaths("plEditorPluginKraut", sPlugin, sCopy, 0);

  plStringBuilder sFolder = sPlugin.GetFileDirectory();
  sFolder.AppendPath("..", "..", "EditorIcons");
  sFolder.MakeCleanPath();

  s_sIconFolder = sFolder;
  plEditorUi::AddIconFolder(s_sIconFolder);
}

static void UnregisterIconFolder()
{
  plEditorUi::RemoveIconFolder(s_sIconFolder);
  s_sIconFolder.Clear();
}

PL_PLUGIN_ON_LOADED()
{
  RegisterIconFolder();

  plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plKrautTreeAssetProperties::PropertyMetaStateEventHandler);

  plKrautActions::RegisterActions();

  // Menu Bar
  {
    plActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar").IgnoreResult();
  }

  // Tool Bar
  {
    plActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
    plKrautActions::MapActions("KrautTreeAssetToolBar");
  }

  plKrautTreeShellPanels::Register();
}

PL_PLUGIN_ON_UNLOADED()
{
  plKrautTreeShellPanels::Unregister();
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plKrautTreeAssetProperties::PropertyMetaStateEventHandler);
  plKrautActions::UnregisterActions();

  UnregisterIconFolder();
}
