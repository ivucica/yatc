# Bazel 9 migration notes

This repository keeps Bazel 6.5.x `WORKSPACE` support while adding Bazel 9 `MODULE.bazel` support for `//:yatc`.

## Patch files

Apply these patches before building with Bazel 9:

- `patches/bazel/rules_tibia-bazel9.patch`
- `patches/bazel/rules_libsdl12-bazel9.patch`
- `patches/bazel/glict-bazel9.patch`

## Exact porting steps

1. Initialize submodules:
   - `git submodule update --init --recursive`
2. Apply the vendored repo patches from the repository root:
   - `git -C vendor/github.com/ivucica/rules_tibia apply patches/bazel/rules_tibia-bazel9.patch`
   - `git -C vendor/github.com/ivucica/rules_libsdl12 apply patches/bazel/rules_libsdl12-bazel9.patch`
   - `git -C vendor/github.com/ivucica/glict apply patches/bazel/glict-bazel9.patch`
3. After step 2 has added the vendored `MODULE.bazel` files, use Bazel 9 for the module-based build:
   - `bazelisk build //:yatc --define=libsdl12_linux_deps_bin=true`
4. Use Bazel 6.5.x for the legacy workspace-based build:
   - `USE_BAZEL_VERSION=6.5.0 bazelisk build --enable_bzlmod=false //:yatc --define=libsdl12_linux_deps_bin=true`

## What changed

- Added a root `MODULE.bazel` for YATC.
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
