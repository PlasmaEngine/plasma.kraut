using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class KrautPluginTarget : TargetRules
{
    public KrautPluginTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plKrautPlugin";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);

        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;

        // No TargetDependencies on engine targets: they are not in this workspace. The engine is
        // bound through its import libraries instead - see PlasmaPackageSdk.ConfigureSdkConsumer.
        ExtraModules.Add("KrautPluginModule");

        // Kraut exports no symbols, so it is compiled into this plugin rather than imported.
        ExtraModules.Add("KrautModule");
    }
}
