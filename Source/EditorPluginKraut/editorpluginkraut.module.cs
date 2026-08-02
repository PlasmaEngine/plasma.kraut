using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginKrautModule : ModuleRules
{
    public EditorPluginKrautModule(BuildContext context) : base(context)
    {
        PlasmaBuildPluginModule.Configure(this, context, "Source/EditorPluginKraut", "EditorPluginKrautPCH.h", "BUILDSYSTEM_BUILDING_EDITORPLUGINKRAUT_LIB",
            "EditorFrameworkModule",
            "GameEngineModule",
            "EditorPluginAssetsModule",
            "KrautModule");

        PlasmaBuildPluginModule.AddPluginImport(this, context, "Source/KrautPlugin", "plKrautPlugin");
        PlasmaBuildPluginModule.AddPluginImport(this, context, "Source/EnginePluginKraut", "plEnginePluginKraut");
        PlasmaBuildPluginModule.ConfigureQt(this, context, "EditorPluginKrautModule", "Source/EditorPluginKraut", "Intermediate/PlasmaBuild/Generated/Source/EditorPluginKraut", "Core", "Gui", "Widgets");
    }
}
