# Local PCL SDK snapshots

Keep each PixInsight-compatible PCL source snapshot in its own versioned
directory:

```text
sdk/PCL/
  <PixInsight-version>/
    README.md
    PCL-<PCL-version>/
```

Available local snapshots:

| PixInsight | PCL | Snapshot directory | Provenance |
| --- | --- | --- | --- |
| 1.8.9-3 | 2.7.0 | [`1.8.9-3/PCL-2.7.0/`](1.8.9-3/PCL-2.7.0/) | [`1.8.9-3/README.md`](1.8.9-3/README.md) |
| 1.9.3 | 2.9.3 | [`1.9.3/PCL-2.9.3/`](1.9.3/PCL-2.9.3/) | [`1.9.3/README.md`](1.9.3/README.md) |
| 1.9.6 | 2.15.0 | [`1.9.6/PCL-2.15.0/`](1.9.6/PCL-2.15.0/) | [`1.9.6/README.md`](1.9.6/README.md) |

PCL versions must match the PixInsight host version. Download future snapshots
from the official [PCL repository](https://gitlab.com/pixinsight/PCL) at a
commit that identifies the corresponding PixInsight core version. Keep the
complete source archive contents together; do not mix headers, libraries, or
build resources from different snapshots.

Downloaded PCL source directories are ignored by this project's version
control. Commit the version README and project-owned build configuration, not
the SDK copy. A new workstation can reproduce a snapshot using the provenance
and download URL documented beside it.

See [the build guide](../../PlanckStarColorMapping/docs/BUILDING.md).
