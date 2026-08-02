using System.IO;
using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class KrautPluginModule : ModuleRules
{
    public KrautPluginModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/KrautPlugin", "KrautPluginPCH.h",
            "BUILDSYSTEM_BUILDING_KRAUTPLUGIN_LIB");

        // Kraut's own generator, which ships in this package rather than in the SDK.
        PublicDependencies.Add("KrautModule");

        // Resolves against the SDK, not the package. In-tree this read context.ProjectRoot, which
        // now means the package directory and would silently find nothing.
        PrivateIncludePaths.Add(Path.Combine(PlasmaPackageSdk.Root, "Data", "Base"));
    }
}
