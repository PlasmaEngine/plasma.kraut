using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class KrautModule : ModuleRules
{
    public KrautModule(BuildContext context) : base(context)
    {
        Type = ModuleType.ThirdParty;
        SourceDirectory = "ThirdParty/Kraut";

        // Kraut has no dllexport annotations, so its symbols are never exported from
        // a DLL. Keep it folded (compiled) into every consumer rather than importing
        // it across a DLL boundary.
        ExportsSymbols = false;

        PublicIncludePaths.Add("..");
        PublicIncludePaths.Add(".");

        PrivateDefinitions.Add("BUILDSYSTEM_BUILDING_KRAUTGENERATOR_LIB");
        PrivateDefinitions.Add("BUILDSYSTEM_BUILDING_KRAUTFOUNDATION_LIB");

        CppStandard = CppStandard.Cpp20;
        TreatWarningsAsErrors = false;
    }
}