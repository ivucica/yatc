def _symlink_local_repository_impl(ctx):
    source = ctx.path(ctx.attr.path)

    for entry in source.readdir():
        if entry.basename == ".git":
            continue
        ctx.symlink(entry, entry.basename)

    if not ctx.path("MODULE.bazel").exists() and not ctx.path("WORKSPACE").exists() and not ctx.path("WORKSPACE.bazel").exists():
        ctx.file("WORKSPACE.bazel", "workspace(name = "%s")\n" % ctx.name)

    if ctx.attr.build_file_content and not ctx.path("BUILD").exists() and not ctx.path("BUILD.bazel").exists():
        ctx.file("BUILD.bazel", ctx.attr.build_file_content)


symlink_local_repository = repository_rule(
    implementation = _symlink_local_repository_impl,
    attrs = {
        "path": attr.string(mandatory = True),
        "build_file_content": attr.string(default = ""),
    },
)


def _yatc_local_repositories_impl(module_ctx):
    del module_ctx

    symlink_local_repository(
        name = "bazelregistry_sdl2",
        path = "vendor/github.com/bazelregistry/sdl2",
    )

    symlink_local_repository(
        name = "tommath",
        path = "vendor/github.com/libtom/libtommath",
        build_file_content = """
config_setting(
    name = 'windows',
    values = {'host_cpu': 'x64_windows'},
)
config_setting(
    name = 'windows_msys',
    values = {'host_cpu': 'x64_windows_msys'},
)
config_setting(
    name = 'windows_msvc',
    values = {'host_cpu': 'x64_windows_msvc'},
)
cc_library(
  name='tommath',
  srcs=glob(['*.c']),
  hdrs=glob(['*.h']),
  visibility=['//visibility:public'],
  defines = select({
    '//conditions:default': [],
    # https://github.com/libtom/libtommath/issues/87 -- no __int128 on MSVC, so we need to build with 32bit math.
    ':windows': ['MP_32BIT'],
    ':windows_msvc': ['MP_32BIT'],
  }),
  copts = select({
    '//conditions:default': ['-isystem external/tommath'],
    ':windows': ['-I external/tommath'],
    ':windows_msvc': ['-I external/tommath'],
  }),
)
""",
    )


yatc_local_repositories = module_extension(
    implementation = _yatc_local_repositories_impl,
)
