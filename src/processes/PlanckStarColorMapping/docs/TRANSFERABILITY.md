# Transferability and maintenance

## Moving the project

Copy this entire `PlanckStarColorMapping` directory while preserving its
relative structure. Keep the JavaScript reference script in the parent
`src/scripts/DrRaupach/PSCM/` tree. The native source does not depend on that
script at runtime.

The PCL snapshots are shared outside this project at
`src/processes/sdk/PCL/`; copy or recreate them separately when transferring
the process project.

For a new workstation:

1. Install PixInsight and obtain the PCL snapshot matching that host version.
2. Recreate `src/processes/sdk/PCL/<PixInsight-version>/PCL-<PCL-version>/`
   as described in the [shared SDK index](../../sdk/PCL/README.md) and the
   version-specific README; do not copy binaries built for another platform.
3. Import the listed C++ files into the supported PCL module project format.
4. Build and complete the smoke tests in [BUILDING.md](BUILDING.md).
5. Record SDK, compiler, host version, and test results with the release.

## Source-of-truth map

- UI defaults/ranges and original processing semantics:
  [PlanckStarColorMapping.js](../../../scripts/DrRaupach/PSCM/PlanckStarColorMapping.js)
- Native parameter IDs and exposed process metadata:
  [PSCMProcess.h](../PSCMProcess.h), [PSCMProcess.cpp](../PSCMProcess.cpp)
- Native image processing and channel/wavelength mapping:
  [PSCMInstance.h](../PSCMInstance.h), [PSCMInstance.cpp](../PSCMInstance.cpp)
- Native interface controls:
  [PSCMInterface.h](../PSCMInterface.h), [PSCMInterface.cpp](../PSCMInterface.cpp)
- Process-module registration:
  [PSCMModule.cpp](../PSCMModule.cpp)

When modifying behavior, change the relevant native parameter metadata and its
interface control together. Keep the JavaScript reference and this
specification synchronized. Add focused tests for numerical changes, especially
zero signal, zero MAD, one-pair/two-pair mappings, and extra-channel
preservation.

## Suggested skill storage

Project-specific PixInsight build and review skills belong under `skills/`.
Keep skills narrowly scoped and reproducible. A skill should state:

- its purpose and when to apply it;
- required local SDK/host/toolchain versions;
- exact source files and validation steps;
- known limitations and the date last verified.

Do not store SDK binaries, credentials, machine-specific paths, or unsigned
release artifacts in skills. See [the skills directory README](../skills/README.md).
