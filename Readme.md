# plasma.kraut

Procedurally generated trees for PlasmaEngine. Moved out of the engine tree to be built and
distributed as a package.

Published as [`plasma.kraut`](https://plasmaengine.github.io/PlasmaPackages/index.plPackageIndex)
in the Plasma package registry.

## Layout

```
Package.plPackage        the manifest - identity, compatibility, dependencies, mounts, components
Kraut.plPluginBundle     legacy descriptor, kept until the engine reads manifests directly
Source/
  KrautPlugin/           runtime  -> plKrautPlugin
  EnginePluginKraut/     editor engine process -> plEnginePluginKraut
  EditorPluginKraut/     editor   -> plEditorPluginKraut  (asset window, drag&drop, actions)
ThirdParty/
  Kraut/                 KrautFoundation + KrautGenerator, statically linked into plKrautPlugin
Assets/
  Kraut/                 materials and the Kraut shader
Branding/                icon and banner
EditorIcons/             editor icons, by icon id (":/AssetIcons/Kraut_Tree.svg" is AssetIcons/Kraut_Tree.svg)
Localization/en/         editor string tables
```

`Assets/Kraut/` keeps its folder name deliberately. `Branch.plMaterial` and friends reference
`Kraut/Kraut.plShader` — a path relative to the data-directory root — so flattening the folder would
break every material here and in any project already referencing them.

## Dependencies

- `plasma.assets ^1.0` — `EditorPluginKraut` links `EditorPluginAssetsModule`
- The Kraut generator library is vendored in `ThirdParty/` and statically linked, so it needs no
  package of its own.

## Building

Requires an engine SDK — a built engine checkout — named by `PLASMA_SDK_ROOT`:

```bash
PLASMA_SDK_ROOT=/path/to/engine plasmabuild build EditorPluginKraut
```

Output lands in `Bin/Windows_x64_Development/`.

`Configs/PlasmaPackageSdk.pb.cs` is what makes this work outside an engine checkout: it resolves
engine headers and import libraries against the SDK rather than `context.ProjectRoot`, and restates
the engine's feature defines. Those defines are the one thing to watch — they propagate through the
module graph automatically inside the engine tree but not out here, so a mismatch changes the ABI
silently.

## Publishing a new version

Staging and publishing are driven from the engine and registry repositories:

```bash
# engine repo - stage and validate, emitting machine-readable output
python Utilities/PackageInspector.py <staged-package-dir> --json <out>.json

# registry repo - upload blobs and record the release
python tools/publish_package.py <out>.json
```

Versions are immutable. A mistake is fixed by publishing a new patch version, never by replacing an
existing one — every lock file that already trusts `1.2.0` must keep resolving to the same bytes.
