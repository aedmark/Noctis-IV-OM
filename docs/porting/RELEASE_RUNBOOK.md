# Release and Rollback Runbook (M8-W05)

This document specifies the standard procedure for rehearsing, producing,
verifying, publishing, and rolling back release artifacts for Noctis IV OM.

---

## 1. Pre-Release Quality Gates

Before initiating a release, verify that all mandatory quality gates pass:

1. **Working Tree Cleanliness:** Ensure `git status` is clean on `master`.
2. **Local Multi-Compiler Test Suite:**
   ```sh
   # Clang Release (40/40 tests)
   cmake --preset linux-clang-release
   cmake --build --preset linux-clang-release --parallel
   ctest --preset linux-clang-release --output-on-failure

   # GCC Debug (40/40 tests)
   cmake --preset linux-gcc-debug
   cmake --build --preset linux-gcc-debug --parallel
   ctest --preset linux-gcc-debug --output-on-failure

   # AddressSanitizer & UBSan (zero leaks, zero UB)
   cmake --preset linux-clang-sanitized
   cmake --build --preset linux-clang-sanitized --parallel
   ctest --preset linux-clang-sanitized --output-on-failure

   # MinGW Windows Cross-Compilation
   cmake --preset windows-mingw-release
   cmake --build --preset windows-mingw-release --parallel
   ```
3. **Automated CI Pipelines:** Ensure all GitHub Actions workflows pass:
   - `linux.yml`: Clang/GCC/Sanitizers.
   - `linux-package.yml`: Clean Ubuntu 24.04 packaging and verification.
   - `windows.yml`: Hosted Windows 2022 MSVC.
   - `windows-package.yml`: Portable MSVC Release ZIP verification.
4. **Documentation and Licensing:** Ensure `README.md`, `KNOWN_ISSUES.md`,
   `TROUBLESHOOTING.md`, `THIRD_PARTY_NOTICES.md`, `LICENSE`, and `WTOF-LICENSE.md`
   are up to date.

---

## 2. Release Artifact Generation

### Step 1: Linux Archive (`.tar.gz`)

```sh
cpack --config build/linux-clang-release/CPackConfig.cmake
cmake -DPACKAGE="$PWD/build/linux-clang-release/Noctis-IV-OM-linux-x86_64.tar.gz" \
  -P cmake/VerifyLinuxPackage.cmake
```

### Step 2: Windows Archive (`.zip`)

```sh
cpack --config build/windows-mingw-release/CPackConfig.cmake
cmake -DPACKAGE="$PWD/build/windows-mingw-release/Noctis-IV-OM-windows-x86_64.zip" \
  -P cmake/VerifyWindowsPackage.cmake
```

### Step 3: Checksum Validation

```sh
cd build/linux-clang-release
sha256sum -c Noctis-IV-OM-linux-x86_64.tar.gz.sha256

cd ../windows-mingw-release
sha256sum -c Noctis-IV-OM-windows-x86_64.zip.sha256
```

---

## 3. Extracted Smoke Test

Test the packages in an isolated directory to confirm that runtime resource
discovery, default catalogs, and graphical presentation succeed without
dependencies on the build tree:

```sh
# Test extracted Linux package
mkdir -p /tmp/noctis-release-check
tar -xzf build/linux-clang-release/Noctis-IV-OM-linux-x86_64.tar.gz -C /tmp/noctis-release-check
cd /tmp/noctis-release-check/*

# Run headless diagnostics
./nivlr --diagnostics

# Run graphical smoke test
./nivlr --graphical-smoke

# Clean up
cd /home/gordonk/PycharmProjects/Noctis-IV-OM
rm -rf /tmp/noctis-release-check
```

---

## 4. Tagging and Publishing Procedure

1. **Tag the Release:**
   ```sh
   git tag -a v1.0.0 -m "Release v1.0.0"
   ```
2. **Push to Remote:**
   ```sh
   git push origin master --tags
   ```
3. **Publish GitHub Release:**
   Attach the built `.tar.gz`, `.zip`, and `.sha256` checksum files with
   release notes detailing changes and fixed issues.

---

## 5. Rollback and Disaster Recovery Procedure

If a critical release-blocking regression or packaging defect is detected after
tagging or publication:

1. **Immediate Quarantine:**
   Edit the GitHub Release to mark it as **Pre-release** or delete the release
   assets to prevent further downloads.
2. **Remove the Remote Tag:**
   ```sh
   git push origin :refs/tags/v1.0.0
   ```
3. **Delete the Local Tag:**
   ```sh
   git tag -d v1.0.0
   ```
4. **Revert or Fix the Defect:**
   Commit the fix or revert to `master`, re-run all quality gates, and prepare a
   corrected patch release (e.g. `v1.0.1`).
5. **Publish Post-Mortem / Known Issue:**
   Document the defect in `KNOWN_ISSUES.md` if user-facing data was affected.

---

## 6. M8-W05 Rehearsal Verification Record

A complete release and rollback rehearsal was executed on 2026-10-01:

1. Temporary rehearsal tag `v0.0.1-rehearsal-test` created and verified.
2. CPack produced both `Noctis-IV-OM-linux-x86_64-preview.tar.gz` and
   `Noctis-IV-OM-windows-x86_64-preview.zip`.
3. Checksums verified cleanly (`sha256sum -c`).
4. Extracted Linux package passed `VerifyLinuxPackage.cmake`, `--diagnostics`, and
   `--graphical-smoke` (three frames presented, clean exit code 0).
5. Extracted Windows package passed `VerifyWindowsPackage.cmake`.
6. Temporary tag deleted and working directory restored to clean state.
7. Rehearsal confirmed complete with zero defects.
