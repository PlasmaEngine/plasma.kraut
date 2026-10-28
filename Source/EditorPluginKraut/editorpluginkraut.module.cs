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

        // The side panel's pages in the engine shell are editor UI contents.
        PlasmaPackageSdk.AddImportLibrary(this, context, "plEditorUi");

        PublicDependencies.Add("KrautModule");

        // Siblings from this package.
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/KrautPlugin", "plKrautPlugin");
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/EnginePluginKraut", "plEnginePluginKraut");
    }
}
