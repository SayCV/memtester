# #############################################################################
# SPDX-License-Identifier: unlicensed for personal use only
# Copyright (c) 2025-present    SayCV
# All rights reserved.
# #############################################################################

project(
    'memtester',
    ['c', 'cpp'],
    version: '4.7.1',
    license: 'GPL-2.0',
    default_options: [
      'warning_level=3',
      'c_std=gnu11',  # 使用 C11 标准
      'buildtype=release',
      'optimization=3',
      'b_lto=true'    # 启用链接时优化
    ],
    meson_version: '>= 0.58',
)

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
add_project_arguments('-DHAVE_CONFIG_H', language: 'cpp')

common_warning_flags = ['-DPOSIX', '-D_POSIX_C_SOURCE=200809L', '-D_FILE_OFFSET_BITS=64', '-DTEST_NARROW_WRITES']
common_warning_flags += cc.get_supported_arguments('-Wno-implicit-function-declaration')

# 配置选项
conf = configuration_data()
conf.set('HAVE_MMAP', 1)
conf.set('HAVE_LINUX', 1)
conf.set('HAVE_STRINGS_H', cc.has_header('strings.h'))
conf.set('HAVE_SYS_MMAN_H', cc.has_header('sys/mman.h'))
conf.set('HAVE_SYS_TYPES_H', cc.has_header('sys/types.h'))

configure_file(output: 'config.h', configuration: conf)

memtester_inc = include_directories('.')

memtester_src = files(
  'memtester.c',
  'tests.c',
  'io_map.c',
  'va_2_pa.c',
  'cache.c',
)


base_cflags = cc.get_supported_arguments(common_warning_flags)

memtester_c_args = cc.get_supported_arguments(base_cflags)
memtester_cxx_args = cxx.get_supported_arguments(base_cflags)

memtester_link_flags = []

memtester_dep = []

# 编译可执行文件
memtester = executable('memtester', memtester_src,
                include_directories: memtester_inc,
                c_args: memtester_c_args,
                cpp_args: memtester_cxx_args,
                link_args: memtester_link_flags,
                dependencies: memtester_dep,
                install: true)
