# -*-Python-*-
workspace(name="yatc")

load(":vendoring.bzl", "vendored_git_repository", "new_vendored_git_repository")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "rules_tibia",
    path = __workspace_dir__ + "/vendor/github.com/ivucica/rules_tibia",
)

load("@rules_tibia//:tibia_data.bzl", "tibia_data_repositories")
tibia_data_repositories()

local_repository(
    name = "glict",
    path = __workspace_dir__ + "/vendor/github.com/ivucica/glict",
)

local_repository(
    name = "rules_libsdl12",
    path = __workspace_dir__ + "/vendor/github.com/ivucica/rules_libsdl12",
)

load("@rules_libsdl12//:libsdl12.bzl", "libsdl12_repositories")
libsdl12_repositories()

http_archive(
    name = "libsdlgfx",
    url = "http://www.ferzkopp.net/Software/SDL_gfx-2.0/SDL_gfx-2.0.24.tar.gz",
    sha256 = "30ad38c3e17586e5212ce4a43955adf26463e69a24bb241f152493da28d59118",
    type = "tar.gz",
    build_file = "@//:BUILD.libsdlgfx",
    strip_prefix = "SDL_gfx-2.0.24",
)

# libtommath
#new_git_repository(
#    remote = "https://github.com/libtom/libtommath",
#    tag = "v1.0",
new_local_repository(
    path = __workspace_dir__ + "/vendor/github.com/libtom/libtommath",
    name = "tommath",
    build_file_content = "\n".join([
        "config_setting(",
        "    name = 'windows',",
        "    values = {'host_cpu': 'x64_windows'},",
        ")",
        "config_setting(",
        "    name = 'windows_msys',",
        "    values = {'host_cpu': 'x64_windows_msys'},",
        ")",
        "config_setting(",
        "    name = 'windows_msvc',",
        "    values = {'host_cpu': 'x64_windows_msvc'},",
        ")",
        "cc_library(",
        "  name='tommath',",
        "  srcs=glob(['*.c']),",
        "  hdrs=glob(['*.h']),",
        "  visibility=['//visibility:public'],",
        "  defines = select({",
        "    '//conditions:default': [],",
        "    # https://github.com/libtom/libtommath/issues/87 -- no __int128 on MSVC, so we need to build with 32bit math.",
        "    '//:windows': ['MP_32BIT'],",
        "    '//:windows_msvc': ['MP_32BIT'],",
        "  }),",
        "  copts = select({",
        "    '//conditions:default': ['-isystem external/tommath'],",
        "    '//:windows': ['-I external/tommath'],",
        "    '//:windows_msvc': ['-I external/tommath'],",
        "  }),",
        ")",
    ]),
)

local_repository(
    name = "bazelregistry_sdl2",
    path = __workspace_dir__ + "/vendor/github.com/bazelregistry/sdl2",
)


# Hedron's Compile Commands Extractor for Bazel
# https://github.com/hedronvision/bazel-compile-commands-extractor
http_archive(
    name = "hedron_compile_commands",

    # Replace the commit hash in both places (below) with the latest, rather than using the stale one here.
    # Even better, set up Renovate and let it do the work for you (see "Suggestion: Updates" in the README).
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/ed994039a951b736091776d677f324b3903ef939.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-ed994039a951b736091776d677f324b3903ef939",
    # When you first run this tool, it'll recommend a sha256 hash to put here with a message like: "DEBUG: Rule 'hedron_compile_commands' indicated that a canonical reproducible form can be obtained by modifying arguments sha256 = ..."
    sha256 = "085bde6c5212c8c1603595341ffe7133108034808d8c819f8978b2b303afc9e7",
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()
# To refresh C++ flags, run bazel run @hedron_compile_commands//:refresh_all

# To move to a newer googletest, we need to pass in --std=c++14, like so:
# https://stackoverflow.com/a/43388168
http_archive(
  name = "com_google_googletest",
  urls = ["https://github.com/google/googletest/archive/4219e7254cb8c473f57f6065bd13d1520d7b708f.zip"],
  strip_prefix = "googletest-4219e7254cb8c473f57f6065bd13d1520d7b708f",
  sha256 = "21bf1f707ef089d576396cdb39d1c4069bce3df69cfab04f7e196ccf130a4884",
)

# Toolchains for Resource Compilation (.rc files on Windows).
#load("@bazel_tools//src/main/res:local_config_winsdk.bzl", "local_config_winsdk")
#local_config_winsdk()

# Toolchains for remote execution via buildbuddy.
http_archive(
    name = "io_buildbuddy_buildbuddy_toolchain",
    sha256 = "1cab6ef3ae9b4211ab9d57826edd4bbc34e5b9e5cb1927c97f0788d8e7ad0442",
    strip_prefix = "buildbuddy-toolchain-b043878a82f266fd78369b794a105b57dc0b2600",
    urls = ["https://github.com/buildbuddy-io/buildbuddy-toolchain/archive/b043878a82f266fd78369b794a105b57dc0b2600.tar.gz"],
)

load("@io_buildbuddy_buildbuddy_toolchain//:deps.bzl", "buildbuddy_deps")

buildbuddy_deps()

load("@io_buildbuddy_buildbuddy_toolchain//:rules.bzl", "buildbuddy", "UBUNTU20_04_IMAGE")

buildbuddy(name = "buildbuddy_toolchain", container_image = UBUNTU20_04_IMAGE)

## Begin rules license deps
http_archive(
    name = "rules_license",
    # sha256 = ...,
    strip_prefix = "rules_license-f27beb61ec306f5466941a1a993249281d05e4be",  # post-1.0.0 commit
    urls = ["https://github.com/bazelbuild/rules_license/archive/f27beb61ec306f5466941a1a993249281d05e4be.tar.gz"],

    # Error: 'TransitiveMetadataInfo' value has no field or method 'other_metadata'
    # Available attributes: deps, licenses, target_under_license, traces
    # Trying older
    # bcffeb0c481d178cbee69bdc7e23ef22d3a087b1, 0.0.8, rules_python 0.19 in WORKSPACE (but newer in MODULE.bazel)
    #sha256 = "8c1155797cb5f5697ea8c6eac6c154cf51aa020e368813d9d9b949558c84f2da"
    #strip_prefix = "rules_license-0.0.8",
    #urls = ["https://github.com/bazelbuild/rules_license/archive/0.0.8.tar.gz"],

    # 0.0.6 acee90188bd0f33f6645374e6f438ede05034804
    # 0.0.5 ... does not matter, also depends on other_metadata.
    #strip_prefix = "rules_license-0.0.5",
    #urls = ["https://github.com/bazelbuild/rules_license/archive/0.0.5.tar.gz"],

)

http_archive(
    name = "rules_python",
    #sha256 = "be04b635c7be4604be1ef20542e9870af3c49778ce841ee2d92fcb42f9d9516a",
    #strip_prefix = "rules_python-0.35.0",
    #url = "https://github.com/bazelbuild/rules_python/releases/download/0.35.0/rules_python-0.35.0.tar.gz",

    # ERROR: Traceback (most recent call last):
    #    File "/.../_bazel_xyz/fcbe4622f304082123c08ec520eac28d/external/rules_python/python/private/common/providers.bzl", line 16, column 33, in <toplevel>
    #            load("@rules_cc//cc:defs.bzl", "CcInfo")
    # Error: file '@rules_cc//cc:defs.bzl' does not contain symbol 'CcInfo'
    # Trying older
    sha256 = "3b8b4cdc991bc9def8833d118e4c850f1b7498b3d65d5698eea92c3528b8cf2c",
    strip_prefix = "rules_python-0.30.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.30.0/rules_python-0.30.0.tar.gz",
)

http_archive(
    name = "rules_pkg",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/1.0.1/rules_pkg-1.0.1.tar.gz",
        "https://github.com/bazelbuild/rules_pkg/releases/download/1.0.1/rules_pkg-1.0.1.tar.gz",
    ],
    sha256 = "d20c951960ed77cb7b341c2a59488534e494d5ad1d30c4818c736d57772a9fef",
)

http_archive(
    name = "bazel_skylib",
    sha256 = "bc283cdfcd526a52c3201279cda4bc298652efa898b10b4db0837dc51652756f",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.7.1/bazel-skylib-1.7.1.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.7.1/bazel-skylib-1.7.1.tar.gz",
    ],
)

http_archive(
    name = "bazel_stardoc",
    sha256 = "c9794dcc8026a30ff67cf7cf91ebe245ca294b20b071845d12c192afe243ad72",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/stardoc/releases/download/0.5.0/stardoc-0.5.0.tar.gz",
        "https://github.com/bazelbuild/stardoc/releases/download/0.5.0/stardoc-0.5.0.tar.gz",
    ],
)

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

load("@bazel_stardoc//:setup.bzl", "stardoc_repositories")

stardoc_repositories()
## End rules license deps
