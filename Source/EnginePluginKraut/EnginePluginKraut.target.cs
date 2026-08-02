using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EnginePluginKrautTarget : TargetRules
{
    public EnginePluginKrautTarget(BuildContext context) : base(context)
    {
        PlasmaBuildPlugin.ConfigurePluginTargetForHost(this, context, "plEnginePluginKraut", "EnginePluginKrautModule", new[] { "KrautPluginModule", "KrautModule" }, "EditorEngineProcess");
    }
}