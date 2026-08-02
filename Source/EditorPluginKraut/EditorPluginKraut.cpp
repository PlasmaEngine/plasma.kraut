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

PL_PLUGIN_ON_LOADED()
{
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
}

PL_PLUGIN_ON_UNLOADED()
{
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plKrautTreeAssetProperties::PropertyMetaStateEventHandler);
  plKrautActions::UnregisterActions();
}
