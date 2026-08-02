#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plKrautActions::s_hCategory;
plActionDescriptorHandle plKrautActions::s_hWindStrengthMenu;
plActionDescriptorHandle plKrautActions::s_hWindStrength[4];
plActionDescriptorHandle plKrautActions::s_hToggleFrondsLeaves;

void plKrautActions::RegisterActions()
{
  s_hCategory = PL_REGISTER_CATEGORY("KrautCategory");

  s_hWindStrengthMenu = PL_REGISTER_MENU_WITH_ICON("Kraut.Wind.Menu", ":/EditorPluginKraut/Wind.svg");
  s_hWindStrength[0] = PL_REGISTER_ACTION_2(
    "Kraut.Wind.Off", plActionScope::Document, "Kraut Tree", "", plKrautAction, plKrautAction::ActionType::WindStrength, plKrautWindStrength::Off);
  s_hWindStrength[1] = PL_REGISTER_ACTION_2(
    "Kraut.Wind.Light", plActionScope::Document, "Kraut Tree", "", plKrautAction, plKrautAction::ActionType::WindStrength, plKrautWindStrength::Light);
  s_hWindStrength[2] = PL_REGISTER_ACTION_2(
    "Kraut.Wind.Moderate", plActionScope::Document, "Kraut Tree", "", plKrautAction, plKrautAction::ActionType::WindStrength, plKrautWindStrength::Moderate);
  s_hWindStrength[3] = PL_REGISTER_ACTION_2(
    "Kraut.Wind.Strong", plActionScope::Document, "Kraut Tree", "", plKrautAction, plKrautAction::ActionType::WindStrength, plKrautWindStrength::Strong);

  s_hToggleFrondsLeaves = PL_REGISTER_ACTION_1("Kraut.ToggleFrondsLeaves", plActionScope::Document, "Kraut Tree", "", plKrautAction, plKrautAction::ActionType::ToggleFrondsLeaves);
}

void plKrautActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hWindStrengthMenu);

  for (int i = 0; i < PL_ARRAY_SIZE(s_hWindStrength); ++i)
    plActionManager::UnregisterAction(s_hWindStrength[i]);

  plActionManager::UnregisterAction(s_hToggleFrondsLeaves);
}

void plKrautActions::MapActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "KrautCategory";

  pMap->MapAction(s_hWindStrengthMenu, szSubPath, 1.0f);

  plStringBuilder sSubPath(szSubPath, "/Kraut.Wind.Menu");

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(s_hWindStrength); ++i)
    pMap->MapAction(s_hWindStrength[i], sSubPath, i + 1.0f);

  pMap->MapAction(s_hToggleFrondsLeaves, szSubPath, 2.0f);
}

plKrautAction::plKrautAction(const plActionContext& context, const char* szName, plKrautAction::ActionType type, plKrautWindStrength::Enum windStrength)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_WindStrength = windStrength;

  m_pDocument = const_cast<plKrautTreeAssetDocument*>(static_cast<const plKrautTreeAssetDocument*>(context.m_pDocument));
  m_pDocument->m_Events.AddEventHandler(plMakeDelegate(&plKrautAction::KrautEventHandler, this));

  UpdateState();

  if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    SetIconPath(":/EditorPluginKraut/Leaf.svg");
  }
}

plKrautAction::~plKrautAction()
{
  m_pDocument->m_Events.RemoveEventHandler(plMakeDelegate(&plKrautAction::KrautEventHandler, this));
}

void plKrautAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::WindStrength)
  {
    m_pDocument->SetWindStrength(m_WindStrength);
  }
  else if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    m_pDocument->SetShowFrondsLeaves(!m_pDocument->GetShowFrondsLeaves());
  }
}

void plKrautAction::KrautEventHandler(const plKrautTreeAssetEvent& e)
{
  switch (e.m_Type)
  {
    case plKrautTreeAssetEvent::Type::WindStrengthChanged:
    case plKrautTreeAssetEvent::Type::FrondsLeavesVisibilityChanged:
      UpdateState();
      break;

    default:
      break;
  }
}

void plKrautAction::UpdateState()
{
  if (m_Type == ActionType::WindStrength)
  {
    SetCheckable(true);
    SetChecked(m_pDocument->GetWindStrength() == m_WindStrength);
  }
  else if (m_Type == ActionType::ToggleFrondsLeaves)
  {
    SetCheckable(true);
    SetChecked(m_pDocument->GetShowFrondsLeaves());
  }
}
