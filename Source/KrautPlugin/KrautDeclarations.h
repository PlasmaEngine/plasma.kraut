#pragma once

#include <KrautPlugin/KrautPluginDLL.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

struct plResourceEvent;

class plKrautLodInfo;
class plKrautRenderData;

class plKrautTreeComponent;
class plKrautTreeComponentManager;

enum class plKrautLodType : plUInt8
{
  None = 0xFF,
  Mesh = 0,
  StaticImpostor = 1,
  BillboardImpostor = 2,
};

enum class plKrautMaterialType : plUInt8
{
  None = 0xFF,
  Branch = 0,
  Frond = 1,
  Leaf = 2,
};

enum class plKrautBranchType : plUInt8
{
  None = 0xFF,
  Trunk1 = 0,
  Trunk2 = 1,
  Trunk3 = 2,
  MainBranches1 = 3,
  MainBranches2 = 4,
  MainBranches3 = 5,
  SubBranches1 = 6,
  SubBranches2 = 7,
  SubBranches3 = 8,
  Twigs1 = 9,
  Twigs2 = 10,
  Twigs3 = 11,
};

struct plKrautTreeTypeBits
{
  using StorageType = plUInt32;

  enum Enum : plUInt32
  {
    Trunk1 = PL_BIT(0),
    Trunk2 = PL_BIT(1),
    Trunk3 = PL_BIT(2),
    MainBranches1 = PL_BIT(3),
    MainBranches2 = PL_BIT(4),
    MainBranches3 = PL_BIT(5),
    SubBranches1 = PL_BIT(6),
    SubBranches2 = PL_BIT(7),
    SubBranches3 = PL_BIT(8),
    Twigs1 = PL_BIT(9),
    Twigs2 = PL_BIT(10),
    Twigs3 = PL_BIT(11),

    Default = 0xFFFFFFFF
  };

  struct Bits
  {
    StorageType Trunk1 : 1;
    StorageType Trunk2 : 1;
    StorageType Trunk3 : 1;
    StorageType MainBranches1 : 1;
    StorageType MainBranches2 : 1;
    StorageType MainBranches3 : 1;
    StorageType SubBranches1 : 1;
    StorageType SubBranches2 : 1;
    StorageType SubBranches3 : 1;
    StorageType Twigs1 : 1;
    StorageType Twigs2 : 1;
    StorageType Twigs3 : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plKrautTreeTypeBits);
PL_DECLARE_REFLECTABLE_TYPE(PL_KRAUTPLUGIN_DLL, plKrautTreeTypeBits);
