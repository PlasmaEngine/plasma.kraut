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
  EditorPluginKraut/     editor   -> plEditorPluginKraut  (Qt: asset window, drag&drop, actions)
ThirdParty/
  Kraut/                 KrautFoundation + KrautGenerator, statically linked into plKrautPlugin
Assets/
  Kraut/                 materials and the Kraut shader
Branding/                icon and banner
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

**This repository cannot be built standalone yet.**

PlasmaBuild resolves every path from `context.ProjectRoot` and has no concept of an installed engine
SDK: `AddPluginImport` links against `<ProjectRoot>/Binaries/<Platform>_<Config>`, which only exists
inside the engine tree. Three things are needed before this builds on its own:

1. An installed-SDK layout — engine headers, import libraries and the shared build rules, laid out
   somewhere discoverable outside the engine repository.
2. A way to point PlasmaBuild at a module directory outside `ProjectRoot`.
3. An `AddPluginImport` equivalent that resolves against that SDK rather than `ProjectRoot`.

Until then, building means copying `Source/` and `ThirdParty/` back into an engine checkout. The
published `plasma.kraut@1.2.0` package was built that way; its binaries are already in the registry,
so consuming the plugin does not require building it.

The module and target files here have been repointed at this layout, so they are internally
consistent and ready for whichever of the above lands first.

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
