#pragma once

#include <EditorPluginKraut/EditorPluginKrautDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plKrautTreeAssetDocument;
struct plKrautTreeAssetEvent;

/// Wind strength presets for Kraut tree preview
struct plKrautWindStrength
{
  enum Enum
  {
    Off,
    Light,
    Moderate,
    Strong,

    Default = Light
  };
};

class plKrautActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hWindStrengthMenu;
  static plActionDescriptorHandle s_hWindStrength[4];
  static plActionDescriptorHandle s_hToggleFrondsLeaves;
};

class plKrautAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautAction, plButtonAction);

public:
  enum class ActionType
  {
    WindStrength,
    ToggleFrondsLeaves,
  };

  plKrautAction(const plActionContext& context, const char* szName, ActionType type, plKrautWindStrength::Enum windStrength = plKrautWindStrength::Light);
  ~plKrautAction();

  virtual void Execute(const plVariant& value) override;

private:
  void KrautEventHandler(const plKrautTreeAssetEvent& e);
  void UpdateState();

  plKrautTreeAssetDocument* m_pDocument;
  ActionType m_Type;
  plKrautWindStrength::Enum m_WindStrength;
};
