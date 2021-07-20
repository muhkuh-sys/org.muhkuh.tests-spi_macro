# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------#
#   Copyright (C) 2011 by Christoph Thelen                                #
#   doc_bacardi@users.sourceforge.net                                     #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
#-------------------------------------------------------------------------#


import os.path


#----------------------------------------------------------------------------
#
# Set up the Muhkuh Build System.
#
SConscript('mbs/SConscript')
Import('atEnv')

# Create a build environment for the ARM9 based netX chips.
env_arm9 = atEnv.DEFAULT.CreateEnvironment(['gcc-arm-none-eabi-4.7', 'asciidoc'])
env_arm9.CreateCompilerEnv('NETX500', ['arch=armv5te'])
env_arm9.CreateCompilerEnv('NETX56', ['arch=armv5te'])
env_arm9.CreateCompilerEnv('NETX50', ['arch=armv5te'])
env_arm9.CreateCompilerEnv('NETX10', ['arch=armv5te'])

# Create a build environment for the Cortex-R7 and Cortex-A9 based netX chips.
env_cortexR7 = atEnv.DEFAULT.CreateEnvironment(['gcc-arm-none-eabi-4.9', 'asciidoc'])
env_cortexR7.CreateCompilerEnv('NETX4000', ['arch=armv7', 'thumb'], ['arch=armv7-r', 'thumb'])

# Create a build environment for the Cortex-M4 based netX chips.
env_cortexM4 = atEnv.DEFAULT.CreateEnvironment(['gcc-arm-none-eabi-4.9', 'asciidoc'])
env_cortexM4.CreateCompilerEnv('NETX90_MPW', ['arch=armv7', 'thumb'], ['arch=armv7e-m', 'thumb'])
env_cortexM4.CreateCompilerEnv('NETX90', ['arch=armv7', 'thumb'], ['arch=armv7e-m', 'thumb'])

# Build the platform libraries.
SConscript('platform/SConscript')


#----------------------------------------------------------------------------
#
# Get the source code version from the VCS.
#
atEnv.DEFAULT.Version('/tmp/targets/version/version.h', 'templates/version.h')


#----------------------------------------------------------------------------
# This is the list of sources. The elements must be separated with whitespace
# (i.e. spaces, tabs, newlines). The amount of whitespace does not matter.
sources_common = """
    src/header.c
    src/init_muhkuh.S
    src/main.c
    src/parameter_placeholder.c
    src/spi_macro_player.c
"""

sources_netx4000 = """
    src/boot_spi.c
    src/boot_drv_sqi.c
    src/boot_drv_spi_v2.c
    src/portcontrol.c
"""

sources_netx500 = """
    src/boot_drv_spi_v1.c
"""

sources_netx90 = """
    src/boot_spi.c
    src/boot_drv_sqi.c
"""

sources_netx56 = """
    src/boot_spi.c
    src/boot_drv_sqi.c
    src/boot_drv_spi_v2.c
"""

sources_netx50 = """
    src/boot_spi.c
    src/boot_drv_spi_v2.c
"""

sources_netx10 = """
"""


#----------------------------------------------------------------------------
#
# Build all files.
#

# The list of include folders. Here it is used for all files.
astrIncludePaths = ['src', '#platform/src', '#platform/src/lib', '/tmp/targets/version', 'targets/flasher/includes']


atBuildConfigurations = {
    'netX4000': {
        'ENV': atEnv.NETX4000,
        'LD': 'src/netx4000/netx4000_intram.ld',
        'SRC': sources_common + sources_netx4000,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx4000.bin',
        'FILTER': {}
    },

    'netX500': {
        'ENV': atEnv.NETX500,
        'LD': 'src/netx500/netx500.ld',
        'SRC': sources_common + sources_netx500,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx500.bin',
        'FILTER': {}
    },

    'netX90_MPW': {
        'ENV': atEnv.NETX90_MPW,
        'LD': 'src/netx90/netx90_intram.ld',
        'SRC': sources_common + sources_netx90,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx90_mpw.bin',
        'FILTER': {}
    },

    'netX90': {
        'ENV': atEnv.NETX90,
        'LD': 'src/netx90/netx90_intram.ld',
        'SRC': sources_common + sources_netx90,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx90.bin',
        'FILTER': {}
    },

    'netX90B': {
        'ENV': atEnv.NETX90,
        'LD': 'src/netx90/netx90_intram.ld',
        'SRC': sources_common + sources_netx90,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx90b.bin',
        'FILTER': {}
    },

    'netX56': {
        'ENV': atEnv.NETX56,
        'LD': 'src/netx56/netx56_intram.ld',
        'SRC': sources_common + sources_netx56,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx56.bin',
        'FILTER': {}
    },

    'netX50': {
        'ENV': atEnv.NETX50,
        'LD': 'src/netx50/netx50_intram.ld',
        'SRC': sources_common + sources_netx50,
        'DEFINES': [],
        'BIN': 'spi_macro_test_netx50.bin',
        'FILTER': {}
    }
}

# Collect the build results in the following dicts.
atBin = {}
atBootImages = {}
atLua = {}

# Build all variants.
for strBuildName, atBuildAttributes in atBuildConfigurations.items():
    # Get a clean environment.
    tEnv = atBuildAttributes['ENV'].Clone()
    # Set the include paths.
    tEnv.Append(CPPPATH = astrIncludePaths)
    # Set the linker description file.
    tEnv.Replace(LDFILE = atBuildAttributes['LD'])
    # Set the defines.
    tEnv.Append(CPPDEFINES = atBuildAttributes['DEFINES'])
    # Set the build path.
    tSrc = tEnv.SetBuildPath(os.path.join('targets', strBuildName, 'build'), 'src', atBuildAttributes['SRC'])
    # Compile all sources and link the libraries.
    tElf = tEnv.Elf(os.path.join('targets', strBuildName, strBuildName + '.elf'), tSrc + tEnv['PLATFORM_LIBRARY'])
    # Create a complete dump of the ELF file.
    tTxt = tEnv.ObjDump(os.path.join('targets', strBuildName, strBuildName + '.txt'), tElf, OBJDUMP_FLAGS=['--disassemble', '--source', '--all-headers', '--wide'])
    # Create a binary from the ELF file.
    tBin = tEnv.ObjCopy(os.path.join('targets', strBuildName, atBuildAttributes['BIN']), tElf)
    # Store the build in the dict.
    atBin[strBuildName] = tBin

    # Filter the LUA file.
    tLua = tEnv.GccSymbolTemplate(os.path.join('targets', strBuildName, 'lua', 'spi_macro_test.lua'), tElf, GCCSYMBOLTEMPLATE_TEMPLATE='templates/spi_macro_test.lua')
    # Store the build in the dict.
    atLua[strBuildName] = tLua

    # Filter additional files.
    for strDst, strSrc in atBuildAttributes['FILTER'].items():
        tEnv.GccSymbolTemplate(os.path.join('targets', strBuildName, strDst), tElf, GCCSYMBOLTEMPLATE_TEMPLATE=strSrc)

#----------------------------------------------------------------------------
#
# Build the artifacts.
#
strGroup = PROJECT_GROUP
strModule = PROJECT_MODULE

# Split the group by dots.
aGroup = strGroup.split('.')
# Build the path for all artifacts.
strModulePath = 'targets/jonchki/repository/%s/%s/%s' % ('/'.join(aGroup), strModule, PROJECT_VERSION)

# Set the name of the artifact.
strArtifact0 = 'spi_macro'

tArcList0 = atEnv.DEFAULT.ArchiveList('zip')
tArcList0.AddFiles('netx/',
    atBin['netX50'],
    atBin['netX56'],
    atBin['netX90_MPW'],
    atBin['netX90'],
    atBin['netX90B'],
    atBin['netX500'],
    atBin['netX4000'])
tArcList0.AddFiles('lua/',
    atLua['netX50'],
    'lua/test_class_spi_macro.lua')
tArcList0.AddFiles('templates/',
    'templates/test.lua')
#tArcList0.AddFiles('doc/',
#   tDoc)
tArcList0.AddFiles('',
    'installer/jonchki/install.lua',
    'installer/jonchki/install_testcase.lua')

tArtifact0 = atEnv.DEFAULT.Archive(os.path.join(strModulePath, '%s-%s.zip' % (strArtifact0, PROJECT_VERSION)), None, ARCHIVE_CONTENTS = tArcList0)
tArtifact0Hash = atEnv.DEFAULT.Hash('%s.hash' % tArtifact0[0].get_path(), tArtifact0[0].get_path(), HASH_ALGORITHM='md5,sha1,sha224,sha256,sha384,sha512', HASH_TEMPLATE='${ID_UC}:${HASH}\n')
tConfiguration0 = atEnv.DEFAULT.Version(os.path.join(strModulePath, '%s-%s.xml' % (strArtifact0, PROJECT_VERSION)), 'installer/jonchki/%s.xml' % strModule)
tConfiguration0Hash = atEnv.DEFAULT.Hash('%s.hash' % tConfiguration0[0].get_path(), tConfiguration0[0].get_path(), HASH_ALGORITHM='md5,sha1,sha224,sha256,sha384,sha512', HASH_TEMPLATE='${ID_UC}:${HASH}\n')
tArtifact0Pom = atEnv.DEFAULT.ArtifactVersion(os.path.join(strModulePath, '%s-%s.pom' % (strArtifact0, PROJECT_VERSION)), 'installer/jonchki/pom.xml')


#----------------------------------------------------------------------------
#
# Make a local demo installation.
#
atCopyFiles = {
    # Copy all binaries.
    'targets/testbench/netx/spi_macro_test_netx50.bin':             atBin['netX50'],
    'targets/testbench/netx/spi_macro_test_netx56.bin':             atBin['netX56'],
    'targets/testbench/netx/spi_macro_test_netx90_mpw.bin':         atBin['netX90_MPW'],
    'targets/testbench/netx/spi_macro_test_netx90.bin':             atBin['netX90'],
    'targets/testbench/netx/spi_macro_test_netx90b.bin':            atBin['netX90B'],
    'targets/testbench/netx/spi_macro_test_netx4000.bin':           atBin['netX4000'],

    # Copy the LUA module.
    # NOTE: All files should be the same, just take the netX50 build.
    'targets/testbench/lua/spi_macro_test.lua':                     atLua['netX50']
}
for tDst, tSrc in atCopyFiles.items():
    Command(tDst, tSrc, Copy("$TARGET", "$SOURCE"))
