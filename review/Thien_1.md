Things that need to be improved/fixed/implemented:

- Fix run_tests.sh since git merge causes some conflicts.
- Write CMakeLists.txt
- generate_data.py only accepts --size-mb -> fix it to also support --size-gb and write a standard <file>.sha256 sidecar file alongside the binary.
- adds verify_checksum.py features to allow verifying a single file against its .sha256 sidecar or expected hex digest
- fix the mismatch between Ubuntu VM version between document and cmake.
- add cmake to the single-line setup command.
