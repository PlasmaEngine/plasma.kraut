using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginKrautModule : ModuleRules
{
    public EditorPluginKrautModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/EditorPluginKraut", "EditorPluginKrautPCH.h",
            "BUILDSYSTEM_BUILDING_EDITORPLUGINKRAUT_LIB");

        PlasmaPackageSdk.ConfigureEditorSdkConsumer(this, context);

        // The asset-document framework this plugin's tree asset builds on.
        PlasmaPackageSdk.AddImportLibrary(this, context, "plEditorPluginAssets");
        PlasmaPackageSdk.AddImportLibrary(this, context, "plSharedPluginAssets");

        PublicDependencies.Add("KrautModule");

        // Siblings from this package.
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/KrautPlugin", "plKrautPlugin");
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/EnginePluginKraut", "plEnginePluginKraut");

        PlasmaPackageQt.ConfigureQtModule(this, context, "EditorPluginKrautModule", "Source/EditorPluginKraut",
            "Core", "Gui", "Widgets");
    }
}
