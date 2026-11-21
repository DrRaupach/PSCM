# PixInsight 1.8.9-3 PCL snapshot

- PCL version: 2.7.0
- Official repository: <https://gitlab.com/pixinsight/PCL>
- Snapshot commit: [`fd570d584a7f3799f435482c27afde5625e568b9`](https://gitlab.com/pixinsight/PCL/-/commit/fd570d584a7f3799f435482c27afde5625e568b9)
- Commit title/date: "Update to PixInsight core version 1.8.9-3", 2024-06-21
- Archive: [PCL source ZIP at this commit](https://gitlab.com/pixinsight/PCL/-/archive/fd570d584a7f3799f435482c27afde5625e568b9/PCL-fd570d584a7f3799f435482c27afde5625e568b9.zip)
- Downloaded ZIP SHA-256: `2C49FD0C65F98AD3B3994206A5727F4F04C2BB53A93404CB5FAC6404721237B7`
- Local extracted path: `sdk/PCL/1.8.9-3/PCL-2.7.0/`

The extracted source snapshot includes the PCL headers and sources, required
third-party library sources, open-source standard modules, and their platform
build files. Its README documents Windows project files for Visual C++ 2019
(`vc16`) and Visual C++ 2022 (`vc17`), and requires C++17 support. Read the
snapshot's `README.md`, `COPYING.md`, and `LICENSE.txt` before using or
redistributing it.

The PixInsight Makefile Generator is distributed with PixInsight core
installations, not in this PCL source archive. Use the generator from the
matching PixInsight installation if needed; do not mix it with another host
release.

**Compatibility status:** the PSCM project currently specifies PixInsight
1.9.3 as its minimum. A local Windows x64 PSCM build has been compiled and
linked against this 1.8.9-3/PCL 2.7.0 snapshot; it has not been loaded or
smoke-tested in the host. This does not establish runtime support or change the
declared minimum. See the [build record](../../../PlanckStarColorMapping/docs/BUILDING.md#local-build-record-pixinsight-189-3-windows-x64).
The project still needs a successful build and test on PixInsight 1.9.3 before
claiming compatibility with its declared minimum.
