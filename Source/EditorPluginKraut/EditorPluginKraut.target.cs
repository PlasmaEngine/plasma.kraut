using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginKrautTarget : TargetRules
{
    public EditorPluginKrautTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plEditorPluginKraut";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);

        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;

        // Both siblings are linked by import library, so both must exist first.
        TargetDependencies.Add("KrautPlugin");
        TargetDependencies.Add("EnginePluginKraut");

        ExtraModules.Add("EditorPluginKrautModule");
        ExtraModules.Add("KrautModule");
    }
}
