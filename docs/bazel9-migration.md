# Bazel 9 migration notes

This repository keeps Bazel 6.5.x `WORKSPACE` support while adding Bazel 9 `MODULE.bazel` support for `//:yatc`.

## Patch files

Apply these patches before building with Bazel 9:

- `patches/bazel/rules_tibia-bazel9.patch`
- `patches/bazel/rules_libsdl12-bazel9.patch`
- `patches/bazel/glict-bazel9.patch`

## Exact porting steps

1. Initialize submodules:
   - `git submodule update --init`
2. Apply the vendored repo patches from the repository root. These commands modify the checked-out submodules in place, so rerun them after any submodule reset or update:
   - `git -C vendor/github.com/ivucica/rules_tibia apply patches/bazel/rules_tibia-bazel9.patch`
   - `git -C vendor/github.com/ivucica/rules_libsdl12 apply patches/bazel/rules_libsdl12-bazel9.patch`
   - `git -C vendor/github.com/ivucica/glict apply patches/bazel/glict-bazel9.patch`
3. After step 2 has added the vendored `MODULE.bazel` files, and while running from a checkout whose `vendor/` submodules are populated, ensure `www.ferzkopp.net` is reachable so the root `MODULE.bazel` can fetch `libsdlgfx` using the checked-in `/home/runner/work/yatc/yatc/BUILD.libsdlgfx`, then use Bazel 9 for the module-based build:
   - `USE_BAZEL_VERSION=9.0.0 bazelisk build --enable_bzlmod=true //:yatc --define=libsdl12_linux_deps_bin=true`
4. Use Bazel 6.5.x for the legacy workspace-based build:
   - `USE_BAZEL_VERSION=6.5.0 bazelisk build --enable_bzlmod=false //:yatc --define=libsdl12_linux_deps_bin=true`

## What changed

- Added a root `MODULE.bazel` plus `module_extensions.bzl` helpers for YATC.
- Added Bazel module files for `rules_tibia`, `rules_libsdl12`, and `glict`.
- Added module extensions for `rules_tibia` and `rules_libsdl12` so their repository setup can be reused from `MODULE.bazel`.
- Replaced `native.new_local_repository` in `rules_libsdl12`'s `xcb_repository()` helper with a repository rule that works in module resolution.

## Notes for sandboxed CI or coding-agent environments

If Bazelisk cannot download Bazel or external archives due to DNS failures, allowlist these hosts for the coding agent:

- `releases.bazel.build`
- `github.com`
- `www.libsdl.org`
- `www.ferzkopp.net`

The first host is required to download the Bazel binary itself. The others are required by the SDL- and Tibia-related external repositories during fetch.
