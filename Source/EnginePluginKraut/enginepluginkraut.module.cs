using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EnginePluginKrautModule : ModuleRules
{
    public EnginePluginKrautModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/EnginePluginKraut", "EnginePluginKrautPCH.h",
            "BUILDSYSTEM_BUILDING_ENGINEPLUGINKRAUT_LIB");

        // This one runs inside the editor's engine process, so it binds to that framework as well.
        PlasmaPackageSdk.AddImportLibrary(this, context, "EditorEngineProcessFramework");

        PublicDependencies.Add("KrautModule");

        // Sibling plugin from this package rather than from the SDK.
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/KrautPlugin", "plKrautPlugin");
    }
}
