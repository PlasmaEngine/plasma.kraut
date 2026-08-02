using System.IO;
using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class KrautPluginModule : ModuleRules
{
    public KrautPluginModule(BuildContext context) : base(context)
    {
        PlasmaBuildPluginModule.Configure(this, context, "Source/KrautPlugin", "KrautPluginPCH.h", "BUILDSYSTEM_BUILDING_KRAUTPLUGIN_LIB",
            "GameEngineModule",
            "UtilitiesModule",
            "KrautModule");

        PrivateIncludePaths.Add(Path.Combine(context.ProjectRoot, "Data/Base"));
    }
}
