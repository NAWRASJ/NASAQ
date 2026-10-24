# Windows build — NASAQ 0.3.10

Requirements: Visual Studio Build Tools 2022 (MSVC x64, ATL, Windows SDK), CMake >=3.24,
Ninja and a compatible Strawberry dependency prefix. The tested environment used
CMake 3.31.6, Qt 6.12.0 and official strawberry-msvc-dependencies 0.1.321.

Dependency archive SHA256:
`02f48098e876deabb6770bea60d4059ee7c92f0f768d8d03eff78d5cec9ab069`.
Obtain it from https://github.com/strawberrymusicplayer/strawberry-msvc-dependencies .
Build Qt ShaderTools, Multimedia and Speech v6.12.0 into the same prefix; they are
required in addition to the base dependency archive. The original dependency build
recipes are at https://github.com/strawberrymusicplayer/strawberry-msvc .

From a PowerShell development environment, with CMake and Ninja on PATH:

```powershell
./scripts/build-strawberry.ps1 -StrawberrySource $PWD -DependencyPrefix C:/deps/strawberry -VisualStudioPath C:/BuildTools -BuildDirectory C:/build/nasaq
```

Use a short build directory to avoid generated shader path limits. The script
relocates the dependency archive's pkg-config prefixes, configures the application,
builds it, and runs the two NASAQ test suites. It does not provision all dependencies
or sign the output. This command has not been verified on a second clean machine.

Package the complete Qt/GStreamer runtime with plugins and notices. The executable
is built as strawberry.exe and delivered as NASAQ.exe. Inno Setup input is
packaging/NASAQ.iss with StageDir set to the complete runtime folder. Check compiler
commercial licensing before commercial release. Sign the payload before building
and signing a production installer.

The smoke script requires tests/cam_tests.exe, tests/cam_audio_tests.exe and a silent
tests/smoke.wav in a staged package. It uses a separate legacy-named test profile.
The inherited GitHub workflow is an upstream workflow, not validated NASAQ Windows CI.

## Microsoft Store target

See [MSIX.md](MSIX.md) for identity configuration, runtime normalization, Store builds and the installed-package acceptance checklist.

### Radio runtime acceptance (0.3.12+)
Before normalizing or packaging the runtime, run in the Visual Studio developer shell:
```powershell
./scripts/stage-radio-runtime.ps1 -StageDir $stage -DependencyPrefix $prefix
./scripts/normalize-runtime.ps1 -StageDir $stage
./scripts/test-radio-runtime.ps1 -StageDir $stage -ProbePath "$build/nasaq_codec_probe.exe"
```
Use the exact dependency prefix used to build the player. The probe uses a private
registry and a clean PATH, and decodes audio into a silent sink. A successful
probe does not replace installed-app checks of prayer pauses and file activation.
