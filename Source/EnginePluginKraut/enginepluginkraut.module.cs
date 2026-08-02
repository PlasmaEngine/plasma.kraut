using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EnginePluginKrautModule : ModuleRules
{
    public EnginePluginKrautModule(BuildContext context) : base(context)
    {
        PlasmaBuildPluginModule.Configure(this, context, "Source/EnginePluginKraut", "EnginePluginKrautPCH.h", "BUILDSYSTEM_BUILDING_ENGINEPLUGINKRAUT_LIB",
            "EditorEngineProcessFrameworkModule",
            "GameEngineModule",
            "KrautModule");

        PlasmaBuildPluginModule.AddPluginImport(this, context, "Source/KrautPlugin", "plKrautPlugin");
    }
}
