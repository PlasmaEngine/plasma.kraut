using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EnginePluginKrautTarget : TargetRules
{
    public EnginePluginKrautTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plEnginePluginKraut";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);

        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;

        // Links plKrautPlugin's import library, so it has to be built first.
        TargetDependencies.Add("KrautPlugin");

        ExtraModules.Add("EnginePluginKrautModule");
        ExtraModules.Add("KrautModule");
    }
}
