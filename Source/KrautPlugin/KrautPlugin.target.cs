using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class KrautPluginTarget : TargetRules
{
    public KrautPluginTarget(BuildContext context) : base(context)
    {
        PlasmaBuildPlugin.ConfigurePluginTargetForHost(this, context, "plKrautPlugin", "KrautPluginModule", new[] { "KrautModule" }, "EditorEngineProcess", "Kraut.plPluginBundle");
    }
}