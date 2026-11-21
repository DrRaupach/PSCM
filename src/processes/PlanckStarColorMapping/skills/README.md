# Skill storage

Store portable, project-specific PixInsight skills in this directory (for
example, a PCL module build skill or a PSCM algorithm review skill). Keep
machine-specific instructions out of shared skill files.

Recommended contents for each skill:

1. A clear name, scope, and trigger conditions.
2. Prerequisites, especially PixInsight/PCL SDK and compiler versions.
3. A step-by-step workflow referencing project files by relative path.
4. Validation steps and expected results.
5. Limitations, ownership, and a last-verified date.

This README is only a storage convention; it does not install or activate an
agent skill. SDKs and generated binaries belong outside this directory.
