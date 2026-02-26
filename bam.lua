local atEnv = require 'mbs2'

-- Create all compiler environments.
atEnv:createEnv('NETX50',          {{id='gcc-arm-none-eabi', version='10.3'}}, { asic_typ='NETX50' })
atEnv:createEnv('NETX56',          {{id='gcc-arm-none-eabi', version='10.3'}}, { asic_typ='NETX56' })
atEnv:createEnv('NETX90',          {{id='gcc-arm-none-eabi', version='10.3'}}, { asic_typ='NETX90' })
atEnv:createEnv('NETX500',         {{id='gcc-arm-none-eabi', version='10.3'}}, { asic_typ='NETX500' })
--atEnv:createEnv('NETX4000',    {{id='gcc-arm-none-eabi', version='10.3'}}, { asic_typ='NETX4000' })
atEnv:createEnv('NETX9X2_COM_MPW', {{id='gcc-arm-none-eabi', version='10.3'}}, { asic_typ='NETX9X2_COM_MPW' })

---------------------------------------------------------------------------------------------------------------------
--
-- Build the platform library.
--
local astrPlatformLibSources = {
  'platform/src/lib/rdy_run.c',
  'platform/src/lib/systime.c',
  'platform/src/lib/uart.c',
  'platform/src/lib/uart_standalone.c',
  'platform/src/lib/uprintf.c'
}

local astrPlatformLibIncludes = {
  'platform/src',
  'platform/src/lib'
}


if atEnv:hasEnv('NETX50') then
  local tEnv = atEnv:cloneEnv('NETX50', { label='platform netx50' })
--  tEnv:CompileCommands('targets/platform/netx50/compile_commands.json')
  tEnv:SetBuildPath('targets/platform/netx50', 'platform/src/lib')

  tEnv:AddIncludes(astrPlatformLibIncludes)
  local tObj = tEnv:Compile(astrPlatformLibSources)
  local tLib = tEnv:StaticLibrary('targets/platform/platform_netx50.a', tObj)
  atEnv:mergeEnv(tEnv, { PLATFORM_LIB=tLib })
end


if atEnv:hasEnv('NETX56') then
  local tEnv = atEnv:cloneEnv('NETX56', { label='platform netx56' })
--  tEnv:CompileCommands('targets/platform/netx56/compile_commands.json')
  tEnv:SetBuildPath('targets/platform/netx56', 'platform/src/lib')

  tEnv:AddIncludes(astrPlatformLibIncludes)
  local tObj = tEnv:Compile(astrPlatformLibSources)
  local tLib = tEnv:StaticLibrary('targets/platform/platform_netx56.a', tObj)
  atEnv:mergeEnv(tEnv, { PLATFORM_LIB=tLib })
end


if atEnv:hasEnv('NETX90') then
  local tEnv = atEnv:cloneEnv('NETX90', { label='platform netx90' })
--  tEnv:CompileCommands('targets/platform/netx90/compile_commands.json')
  tEnv:SetBuildPath('targets/platform/netx90', 'platform/src/lib')

  tEnv:AddIncludes(astrPlatformLibIncludes)
  local tObj = tEnv:Compile(astrPlatformLibSources)
  local tLib = tEnv:StaticLibrary('targets/platform/platform_netx90.a', tObj)
  atEnv:mergeEnv(tEnv, { PLATFORM_LIB=tLib })
end



if atEnv:hasEnv('NETX500') then
  local tEnv = atEnv:cloneEnv('NETX500', { label='platform netx500' })
--  tEnv:CompileCommands('targets/platform/netx500/compile_commands.json')
  tEnv:SetBuildPath('targets/platform/netx500', 'platform/src/lib')

  tEnv:AddIncludes(astrPlatformLibIncludes)
  local tObj = tEnv:Compile(astrPlatformLibSources)
  local tLib = tEnv:StaticLibrary('targets/platform/platform_netx500.a', tObj)
  atEnv:mergeEnv(tEnv, { PLATFORM_LIB=tLib })
end


if atEnv:hasEnv('NETX9X2_COM_MPW') then
  local tEnv = atEnv:cloneEnv('NETX9X2_COM_MPW', { label='platform netx9x2 mpw' })
--  tEnv:CompileCommands('targets/platform/netx500/compile_commands.json')
  tEnv:SetBuildPath('targets/platform/netx9x2mpw', 'platform/src/lib')

  tEnv:AddIncludes(astrPlatformLibIncludes)
  local tObj = tEnv:Compile(astrPlatformLibSources)
  local tLib = tEnv:StaticLibrary('targets/platform/platform_netx9x2mpw.a', tObj)
  atEnv:mergeEnv(tEnv, { PLATFORM_LIB=tLib })
end


---------------------------------------------------------------------------------------------------------------------
--
-- Build the test code.
--

-- Process the version template.
local tEnvVersion = atEnv:cloneAnyEnv({ label='all testcodes' })
tEnvVersion:Version('targets/version/version.h', 'templates/version.h')

local astrTestSources = {
  'src/header.c',
  'src/init_muhkuh.S',
  'src/main.c',
  'src/parameter_placeholder.c',
  'src/spi_macro_player.c'
}
local astrSourcesnetx500 = {
  'src/boot_drv_spi_v1.c'
}
local astrSourcesnetx90 = {
  'src/boot_spi.c',
  'src/boot_drv_sqi.c'
}
local astrSourcesnetx56 = {
  'src/boot_spi.c',
  'src/boot_drv_sqi.c',
  'src/boot_drv_spi_v2.c'
}
local astrSourcesnetx50 = {
  'src/boot_spi.c',
  'src/boot_drv_spi_v2.c'
}
local astrIncludes = {
  'platform/src',
  'platform/src/lib',
  'targets/version',
  'src'
}

if atEnv:hasEnv('NETX50') then
  local tEnv = atEnv:cloneEnv('NETX50', { label='testcode netx50' })
--  tEnv:CompileCommands('targets/bootpins/netx50/compile_commands.json')
  tEnv:SetBuildPath('targets/spimacro/netx50', 'src')

  tEnv:AddIncludes(astrIncludes)
  local tObj = tEnv:Compile(
    astrTestSources,
    astrSourcesnetx50
  )
  local tElf = tEnv:Link(
    'targets/spimacro/spi_macro_test_netx50.elf',
    'src/netx50/netx50_intram.ld',
    tObj,
    tEnv.mbs.PLATFORM_LIB
  )
  local tBin = tEnv:Elf2Bin(
    'targets/spimacro/spi_macro_test_netx50.bin',
    tElf
  )
  atEnv:mergeEnv(tEnv, { TESTCODE_ELF=tElf, TESTCODE_BIN=tBin })
end


if atEnv:hasEnv('NETX56') then
  local tEnv = atEnv:cloneEnv('NETX56', { label='testcode netx56' })
--  tEnv:CompileCommands('targets/bootpins/netx56/compile_commands.json')
  tEnv:SetBuildPath('targets/spimacro/netx56', 'src')

  tEnv:AddIncludes(astrIncludes)
  local tObj = tEnv:Compile(
    astrTestSources,
    astrSourcesnetx56
  )
  local tElf = tEnv:Link(
    'targets/spimacro/spi_macro_test_netx56.elf',
    'src/netx56/netx56_intram.ld',
    tObj,
    tEnv.mbs.PLATFORM_LIB
  )
  local tBin = tEnv:Elf2Bin(
    'targets/spimacro/spi_macro_test_netx56.bin',
    tElf
  )
  atEnv:mergeEnv(tEnv, { TESTCODE_ELF=tElf, TESTCODE_BIN=tBin })
end


if atEnv:hasEnv('NETX90') then
  local tEnv = atEnv:cloneEnv('NETX90', { label='testcode netx90' })
--  tEnv:CompileCommands('targets/bootpins/netx90/compile_commands.json')
  tEnv:SetBuildPath('targets/spimacro/netx90', 'src')

  tEnv:AddIncludes(astrIncludes)
  local tObj = tEnv:Compile(
    astrTestSources,
    astrSourcesnetx90
  )
  local tElf = tEnv:Link(
    'targets/spimacro/spi_macro_test_netx90.elf',
    'src/netx90/netx90_intram.ld',
    tObj,
    tEnv.mbs.PLATFORM_LIB
  )
  local tBin = tEnv:Elf2Bin(
    'targets/spimacro/spi_macro_test_netx90.bin',
    tElf
  )
  atEnv:mergeEnv(tEnv, { TESTCODE_ELF=tElf, TESTCODE_BIN=tBin })
end


--[[
if atEnv:hasEnv('NETX9X2_COM_MPW') then
  local tEnv = atEnv:cloneEnv('NETX9X2_COM_MPW', { label='testcode netx9x2 mpw' })
--  tEnv:CompileCommands('targets/bootpins/netx9x2mpw/compile_commands.json')
  tEnv:SetBuildPath('targets/bootpins/netx9x2mpw', 'bootpins/src')

  tEnv:AddIncludes(astrIncludes)
  local tObj = tEnv:Compile(
    astrTestSources,
    'bootpins/src/netx9x2/detect.c'
  )
  local tElf = tEnv:Link(
    'targets/bootpins/bootpins_netx9x2mpw.elf',
    'bootpins/src/netx9x2/netx9x2.ld',
    tObj,
    tEnv.mbs.PLATFORM_LIB
  )
  local tBin = tEnv:Elf2Bin(
    'targets/bootpins/bootpins_netx9x2_mpw.bin',
    tElf
  )
  atEnv:mergeEnv(tEnv, { TESTCODE_ELF=tElf, TESTCODE_BIN=tBin })
end
--]]

if atEnv:hasEnv('NETX500') then
  local tEnv = atEnv:cloneEnv('NETX500', { label='testcode netx500' })
--  tEnv:CompileCommands('targets/bootpins/netx500/compile_commands.json')
  tEnv:SetBuildPath('targets/spimacro/netx500', 'src')

  tEnv:AddIncludes(astrIncludes)
  local tObj = tEnv:Compile(
    astrTestSources,
    astrSourcesnetx500
  )
  local tElf = tEnv:Link(
    'targets/spimacro/spi_macro_test_netx500.elf',
    'src/netx500/netx500.ld',
    tObj,
    tEnv.mbs.PLATFORM_LIB
  )
  local tBin = tEnv:Elf2Bin(
    'targets/spimacro/spi_macro_test_netx500.bin',
    tElf
  )
  atEnv:mergeEnv(tEnv, { TESTCODE_ELF=tElf, TESTCODE_BIN=tBin })
end


local tEnvLua = atEnv:cloneAnyEnv({ label='lua teststep' })
local tLua = tEnvLua:GCCSymbolTemplate(
  'targets/lua/spi_macro_test.lua',
  tEnvLua.mbs.TESTCODE_ELF,
  'templates/spi_macro_test.lua',
  {
    -- Use a Lua friendly pattern.
    PATTERN = 'XX_([%a_][%w_]*)_XX'
  }
)
