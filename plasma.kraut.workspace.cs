// #pl-version 1
//
// Makes this package directory buildable on its own. Without a workspace there is no entry point
// for plasmabuild outside the engine checkout.

using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class PlasmaKrautWorkspace : WorkspaceRules
{
    public PlasmaKrautWorkspace(BuildContext context) : base(context)
    {
        TargetNames.Add("KrautPlugin");
        TargetNames.Add("EnginePluginKraut");
        TargetNames.Add("EditorPluginKraut");
    }
}
