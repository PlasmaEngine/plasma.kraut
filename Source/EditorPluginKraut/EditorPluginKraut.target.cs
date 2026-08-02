using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginKrautTarget : TargetRules
{
    public EditorPluginKrautTarget(BuildContext context) : base(context)
    {
        PlasmaBuildPlugin.ConfigurePluginTarget(this, context, "plEditorPluginKraut", "EditorPluginKrautModule", new[] { "KrautPluginModule", "EnginePluginKrautModule", "EditorPluginAssetsModule", "SharedPluginAssetsModule", "KrautModule" }, "Kraut.plPluginBundle");
        PlasmaBuildQt.AddQtRuntimeDeployment(this, context);
    }
}