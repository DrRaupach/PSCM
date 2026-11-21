# Building and installing

## Prerequisites

1. PixInsight 1.9.3 or newer.
2. The PixInsight Class Library (PCL) SDK compatible with PixInsight 1.9.3
   (or the specific newer host release) and the target operating system.
3. The compiler/toolchain supported by that SDK. Do not mix PCL headers,
   libraries, or build tools from different SDK releases. To claim 1.9.3
   compatibility, build and test with the matching PCL SDK and host version.

Obtain PCL and its platform-specific module build resources from the official
PixInsight developer resources: <https://pixinsight.com/developer/>. Review
the SDK license and installation instructions before use. For multiple host
versions, keep each complete snapshot separate under
`../sdk/PCL/<PixInsight-version>/PCL-<PCL-version>/`, shared by process
projects under `src/processes/`. The downloaded SDK contents are ignored by
version control; keep their provenance in a README beside each snapshot.

### Snapshots currently available locally

| PixInsight | PCL | Provenance and checksum |
| --- | --- | --- |
| 1.8.9-3 | 2.7.0 | [`sdk/PCL/1.8.9-3/README.md`](../../sdk/PCL/1.8.9-3/README.md) |
| 1.9.3 | 2.9.3 | [`sdk/PCL/1.9.3/README.md`](../../sdk/PCL/1.9.3/README.md) |
| 1.9.6 | 2.15.0 | [`sdk/PCL/1.9.6/README.md`](../../sdk/PCL/1.9.6/README.md) |

PCL 2.9.3 includes Windows projects for Visual C++ 2019 (`vc16`) and 2022
(`vc17`) and requires C++17 support. PCL 2.15.0 includes Windows projects for
Visual C++ 2022 (`vc17`) and requires C++20 support. Use the project files and
toolchain requirements documented in the selected snapshot's own README.
PixInsight's Makefile Generator is distributed with the corresponding host
installation, not the PCL source archives.

### Example: resources for PixInsight 1.8.9-3

PCL is distributed as source code, not as a separate installer for each
PixInsight release. The official PCL GitLab history records the matching
release snapshot:

- Commit: [`fd570d584a7f3799f435482c27afde5625e568b9`](https://gitlab.com/pixinsight/PCL/-/commit/fd570d584a7f3799f435482c27afde5625e568b9),
  titled "Update to PixInsight core version 1.8.9-3" (June 21, 2024).
- That commit updates PCL from 2.6.11 to **PCL 2.7.0**.
- Download the source archive for that exact commit:
  [PCL 2.7.0 snapshot ZIP](https://gitlab.com/pixinsight/PCL/-/archive/fd570d584a7f3799f435482c27afde5625e568b9/PCL-fd570d584a7f3799f435482c27afde5625e568b9.zip).

The snapshot is extracted under
`../sdk/PCL/1.8.9-3/PCL-2.7.0/`. The version-specific
[`README.md`](../../sdk/PCL/1.8.9-3/README.md) records the source commit and
archive checksum. The snapshot contains the PCL headers, sources, third-party
libraries, standard modules, and their platform build resources. For Windows,
its README documents Visual C++ 2019 (`vc16`) and 2022 (`vc17`) project files
and requires C++17 support.

The Makefile Generator is supplied with PixInsight core installations rather
than the PCL source archive; use the copy from the matching host if needed.
Alternatively, to recreate the PCL snapshot with Git, clone
<https://gitlab.com/pixinsight/PCL.git> and check out the full commit hash
above.

**Compatibility note:** the project's declared minimum remains PixInsight
1.9.3. A local Windows x64 build against PixInsight 1.8.9-3 and PCL 2.7.0 is
recorded below at the user's request; it does not change the declared minimum
or establish runtime compatibility. The resulting module has not yet been
installed or smoke-tested in PixInsight 1.8.9-3.

## Local build record: PixInsight 1.8.9-3 (Windows x64)

- Host: PixInsight Core 1.8.9-3 Ripley, x64, build 1612.
- PCL: 2.7.0 from the pinned 1.8.9-3 snapshot above.
- Toolchain: Visual Studio 2022 Build Tools, MSBuild 17.14.60, MSVC 14.44.35207
  (v143), Windows x64.
- The host's Makefile Generator v1.140 generated the PCL project at
  `../sdk/PCL/1.8.9-3/PCL-2.7.0/src/pcl/windows/vc17/PCL.vcxproj`.
- PCL's `lz4`, `zstd`, `zlib`, `RFC6234`, `lcms`, and `cminpack` static
  libraries and `PCL-pxi.lib` were built in Release x64.
- The PSCM project is
  [`build/1.8.9-3/windows/vc17/PlanckStarColorMapping.vcxproj`](../build/1.8.9-3/windows/vc17/PlanckStarColorMapping.vcxproj).
  It uses the matching PCL 2.7.0 vc17 module settings and builds with C++17.
  `PSCM_EMBED_ICON_XPM` selects the exact PNG-derived XPM data embedded in the
  module for both the process and interface icons. The small icon is resampled
  by PixInsight. This target does not compile the workstation-specific raster
  file paths.
- Output:
  `../build/1.8.9-3/bin/PlanckStarColorMapping-pxm.dll`
  (2,768,384 bytes; SHA-256
  `B6015FA734B5463F6A95A5B49345D919D1651618854453059FCEC4FBA5B230D7`).
  `dumpbin /headers` confirms an x64 PE32+ DLL. The build completed with zero
  warnings and zero errors; the embedded XPM dimensions and every pixel were
  verified against the supplied PNG.
- The module was not copied into the PixInsight installation and has not been
  loaded or run in the host. Do not treat this compile result as a release or
  compatibility claim.
- The temporary user-scope PCL environment variables were restored after the
  build. If configuring them for Makefile Generator, restart PixInsight so the
  application process inherits the updated environment.

To reproduce the build in an x64 Native Tools Command Prompt for Visual Studio
2022, set the SDK paths and build the PCL dependencies before PSCM:

```bat
set "PCLDIR=D:\_User\Rainer\Documents\_Software\GitHub\PSCM\src\processes\sdk\PCL\1.8.9-3\PCL-2.7.0"
set "PCLSRCDIR=%PCLDIR%\src"
set "PCLINCDIR=%PCLDIR%\include"
set "PCLLIBDIR64=%PCLDIR%\lib\windows\x64"
if not exist "%PCLLIBDIR64%" mkdir "%PCLLIBDIR64%"

for %L in (lz4 zstd zlib RFC6234 lcms cminpack) do msbuild "%PCLSRCDIR%\3rdparty\%L\windows\vc17\%L.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /m
msbuild "%PCLSRCDIR%\pcl\windows\vc17\PCL.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /m
msbuild "D:\_User\Rainer\Documents\_Software\GitHub\PSCM\src\processes\PlanckStarColorMapping\build\1.8.9-3\windows\vc17\PlanckStarColorMapping.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

The PSCM project file is a project-specific adaptation of the PCL 2.7.0
`NoOperation` module project settings. The host Makefile Generator was used to
generate the PCL project; its external-module UI was not used for PSCM.

## Local build record: PixInsight 1.9.6 (Windows x64)

- PCL: 2.15.0 from the pinned 1.9.6 snapshot above.
- Toolchain: Visual Studio 2022 Build Tools, MSBuild 17.14.60, MSVC 14.44.35207
  (v143), Windows x64, C++20.
- The downloaded snapshot did not include a generated Windows PCL core project.
  [`build/1.9.6/windows/vc17/PCL.vcxproj`](../build/1.9.6/windows/vc17/PCL.vcxproj)
  builds the 212 PCL source files with the PCL 2.15.0 Windows settings from its
  `NoOperation` project. The PSCM module project uses the corresponding C++20,
  AVX2, and FMA settings. It also defines `PSCM_EMBED_ICON_SVG`, embedding the
  process and interface SVG icons in the module instead of requiring a
  per-install module-icon resource file. The 1.8.9-3 project does not define
  this option and is unchanged.
- The `lz4`, `zstd`, `zlib`, and `lcms` static libraries, `PCL-pxi.lib`, and
  PSCM module were built in Release x64.
- Project files:
  [`PCL.vcxproj`](../build/1.9.6/windows/vc17/PCL.vcxproj) and
  [`PlanckStarColorMapping.vcxproj`](../build/1.9.6/windows/vc17/PlanckStarColorMapping.vcxproj).
- Output:
  `../build/1.9.6/bin/PlanckStarColorMapping-pxm.dll`
  (2,857,472 bytes; SHA-256
  `A1D81BE6229DAB09D28C56A4250B9C09878F6A98AE5F598C409FA0A907FA0D44`).
  `dumpbin /headers` confirms an x64 PE32+ DLL. The clean PSCM build completed
  with zero warnings and zero errors.
- The existing 701-byte `.xsgn` sidecar was moved with the DLL. It is preserved
  from the previous build; the documented build commands do not regenerate or
  validate the signature.
- The module has not been installed, loaded, or smoke-tested in PixInsight
  1.9.6. Do not treat this compile result as a runtime compatibility claim.

To reproduce the build in an x64 Native Tools Command Prompt for Visual Studio
2022, set the SDK paths and build the PCL dependencies, PCL, and PSCM:

```bat
set "PCLDIR=D:\_User\Rainer\Documents\_Software\GitHub\PSCM\src\processes\sdk\PCL\1.9.6\PCL-2.15.0"
set "PCLSRCDIR=%PCLDIR%\src"
set "PCLINCDIR=%PCLDIR%\include"
set "PCLLIBDIR64=%PCLDIR%\lib\windows\x64"
if not exist "%PCLLIBDIR64%" mkdir "%PCLLIBDIR64%"

for %L in (lz4 zstd zlib lcms) do msbuild "%PCLSRCDIR%\3rdparty\%L\windows\vc17\%L.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /m
msbuild "D:\_User\Rainer\Documents\_Software\GitHub\PSCM\src\processes\PlanckStarColorMapping\build\1.9.6\windows\vc17\PCL.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /m
msbuild "D:\_User\Rainer\Documents\_Software\GitHub\PSCM\src\processes\PlanckStarColorMapping\build\1.9.6\windows\vc17\PlanckStarColorMapping.vcxproj" /t:Build /p:Configuration=Release /p:Platform=x64 /m
```

## macOS build preparation

The 1.8.9-3 PCL 2.7.0 snapshot includes macOS `g++` makefiles for x86_64 and
the matching third-party libraries. This Windows environment cannot compile
Mach-O `.dylib` files or verify macOS code signing, so no macOS binary is
claimed here.

### PixInsight 1.9.6 GitHub Actions build

The repository workflow
[`build-macos.yml`](../../../../.github/workflows/build-macos.yml) builds only the
PixInsight 1.9.6 module against the pinned PCL 2.15.0 snapshot. It downloads
and verifies that snapshot, then uses the matching x64 or arm64 PCL makefiles
for the architecture reported by `macos-latest`. The resulting
`PlanckStarColorMapping-pxm.dylib` is written to
`build/1.9.6/bin/` and uploaded as a workflow artifact. The module is not
signed with PixInsight's distribution certificate; a successful CI build
does not establish that it loads in PixInsight.

Run it from the Actions tab with **Build PixInsight 1.9.6 macOS module** or let
it run on pushes and pull requests that change the workflow or process project.

Project-owned macOS build files are provided at:

- [`build/1.8.9-3/macos/build.sh`](../build/1.8.9-3/macos/build.sh)
- [`build/1.8.9-3/macos/g++/makefile-x64`](../build/1.8.9-3/macos/g++/makefile-x64)

On an Intel macOS host with Xcode 15-compatible command-line tools and the
matching PixInsight 1.8.9-3 PCL 2.7.0 snapshot:

```sh
cd /path/to/PSCM/src/processes/PlanckStarColorMapping/build/1.8.9-3/macos
chmod +x build.sh
PCLDIR=/path/to/PCL-2.7.0 ./build.sh
```

The script builds PCL's macOS static dependencies and PCL itself before
building `PlanckStarColorMapping-pxm.dylib`. The generated output is placed in
`build/1.8.9-3/bin/PlanckStarColorMapping-pxm.dylib`. Install it by copying the
signed module into the PixInsight
`/Applications/PixInsight/bin` module directory and installing the SVG or PNG
icon in `rsc/icons/module/PlanckStarColorMapping/`, then restart PixInsight.

The macOS build remains pending host verification. Apple Silicon support also
requires the matching arm64 PCL build and host resources; the PCL 2.7.0 files
available locally generate x86_64 macOS modules.

An arm64 build workflow is prepared at:

- [`build/1.8.9-3/macos/build-arm64.sh`](../build/1.8.9-3/macos/build-arm64.sh)
- [`build/1.8.9-3/macos/g++/makefile-arm64`](../build/1.8.9-3/macos/g++/makefile-arm64)

Run it on Apple Silicon macOS:

```sh
cd /path/to/PSCM/src/processes/PlanckStarColorMapping/build/1.8.9-3/macos
chmod +x build-arm64.sh
PCLDIR=/path/to/PCL-2.7.0 ./build-arm64.sh
```

This script derives arm64 PCL makefiles from the x64 makefiles, removes
x86-only SSE flags, selects `-arch arm64`, and writes static libraries to
`$PCLDIR/lib/macos/arm64`. The arm64 `.dylib` has not been produced or tested
in this Windows environment.

## Step-by-step local integration

The process-list icon is the SVG wrapper
[`icons/PlanckStarColorMapping.svg`](../icons/PlanckStarColorMapping.svg),
which embeds `src/icons/PlanckColorBar_Q.png`. The 1.8.9-3 Windows target
embeds the PNG as XPM, and the 1.9.6 Windows target embeds the SVG in the PXM;
neither requires a separate icon resource file. For other builds that use
`@module_icons_dir/PlanckStarColorMapping.svg`, copy the SVG into
`<PixInsight>/rsc/icons/module/PlanckStarColorMapping/PlanckStarColorMapping.svg`.
Then copy the compiled PXM into the matching module directory and restart
PixInsight.

The files in this directory are the module's source code. PixInsight cannot
load the C++ files directly; compile them into a platform-specific PXM module
first. Version-specific build projects live under `build/`; the available
1.8.9-3 x64 project and output are documented above. Use the project/build
mechanism supplied with your PCL SDK rather than guessing SDK-specific
compiler or linker options.

### 1. Prepare the local environment

1. Install PixInsight for the target operating system and architecture.
2. Select the matching source snapshot under
   `../sdk/PCL/<PixInsight-version>/` and follow its license and setup
   instructions. For a new host release, download the corresponding commit
   from the official PCL repository and record its provenance in that version
   folder.
3. Use a PCL version supported by the installed host. To claim PixInsight
   1.9.3 compatibility, use the matching 1.9.3-compatible PCL and toolchain,
   then test the resulting module in PixInsight 1.9.3.
4. Configure the project with the selected snapshot's local path, or install
   PCL elsewhere and configure that path instead. Do not commit SDK files or
   machine-specific absolute paths.

### 2. Create/configure the module build project

1. Start from the module project/template or build files distributed with the
   selected PCL SDK.
2. Add these four implementation files to the module target:
   - `PSCMModule.cpp` — PXM entry point and module metadata.
   - `PSCMProcess.cpp` — process registration and parameter metadata.
   - `PSCMInstance.cpp` — stored parameters and image processing.
   - `PSCMInterface.cpp` — process interface and controls.
3. Make the corresponding headers available to the compiler:
   `PSCMProcess.h`, `PSCMInstance.h`, and `PSCMInterface.h`.
4. Configure PCL include/library directories and all platform, architecture,
   compiler, and linker options as required by that SDK. Build a module (shared
   library), not a standalone executable.
5. Set the module output name to match `PSCMModule.cpp`:
   `PlanckStarColorMapping-pxm.dll` on Windows,
   `PlanckStarColorMapping-pxm.dylib` on macOS, or
   `PlanckStarColorMapping-pxm.so` on Linux/FreeBSD.
6. Build in the SDK-supported configuration. Keep IDE projects, build scripts,
   and platform-specific settings in a project-owned `build/` or `project/`
   subdirectory. Record the SDK, compiler, host, and architecture versions.

The exact IDE steps and commands depend on the PCL release and operating
system. The source package intentionally does not invent a project format or
compiler command. If the build reports missing PCL symbols or incompatible
headers, verify that every PCL component comes from the same SDK release.

### 3. Install and load the PXM

1. Locate the output PXM produced by the successful build and verify that its
   name and architecture match the target PixInsight installation.
2. Install/copy it to the process-module location designated by the PCL SDK
   and PixInsight installation instructions. Use the appropriate permissions;
   do not overwrite a different module with the same name.
3. Restart PixInsight (or use the host's supported module reload procedure).
   Check the startup/module log for load errors.
4. Open the process list and confirm **Planck Star Color Mapping** appears in
   the `ColorSpaces` category. Open its interface and confirm the controls are
   available.

The PXM is a native module, not a script package: do not place the C++ source
files in the PixInsight scripts directory. The original JavaScript script
continues to work through the script system independently of the PXM.

### 4. Smoke-test the local integration

Use duplicate/test images; the process modifies image pixels in place.

1. Open a non-complex linear RGB image with at least three channels. Execute
   the default HOO process and confirm it completes without a Console/module
   error.
2. Confirm undo restores the input and that a process instance can be saved
   and reapplied.
3. Try HSO, SHO, RGB, and `RGB (ignore G)`. For modes using two wavelength
   pairs, confirm execution completes.
4. Change each numeric control and use its reset icon. Confirm the factory
   values are restored: saturation `1.00`, background protection `1.5`, and
   spectral spread `1.00`.
5. Enable spectral spread and verify the configured value is applied; disable
   it and verify behavior returns to the unspread mapping.
6. Activate the standard real-time preview control. Confirm the preview
   updates when parameters change and matches the mapping applied to a duplicate
   image.
7. If testing an image with more than three channels, confirm channels beyond
   the first three remain unchanged.
8. Try a monochrome and a complex image; confirm PixInsight rejects them with
   the process validation message.
9. For a 1.9.3 compatibility claim, repeat the build and smoke test in that
   exact PixInsight/PCL environment.

If the process does not appear, verify the module file name, target
architecture, install location, and PixInsight's module-load log before
changing source code.

## Build artifacts

Do not commit generated object files, SDK copies, or built modules unless the
project's distribution policy explicitly requires them. Record the exact SDK
release, compiler version, target architecture, build configuration, module
output name, and tested PixInsight version in the release notes.
