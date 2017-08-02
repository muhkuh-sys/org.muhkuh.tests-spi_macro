module("test@ID@", package.seeall)

require("parameters")
local f = require 'spi_flash_macro_test'

CFG_strTestName = "@NAME@"

CFG_aParameterDefinitions = {
  {
    name="unit",
    default=nil,
    help="This is the unit providing the SPI bus.",
    mandatory=true,
    validate=parameters.test_choice_single,
    constrains="netX50_SPI0,netX50_SPI1,netX56_SQI,netX56_SPI,netX4000_SQI0,netX4000_SQI1,netX4000_SPI"
  },
  {
    name="chip_select",
    default=nil,
    help="The chip select number on the selected SPI bus.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="speed",
    default=nil,
    help="The speed for the SPI communication in kHz.",
    mandatory=false,
    validate=parameters.test_uint32,
    constrains=nil
  },
  {
    name="port_control_CS",
    default=0xffff,
    help="The port control value for the CS signal.",
    mandatory=false,
    validate=parameters.test_uint16,
    constrains=nil
  },
  {
    name="port_control_CLK",
    default=0xffff,
    help="The port control value for the CLK signal.",
    mandatory=false,
    validate=parameters.test_uint16,
    constrains=nil
  },
  {
    name="port_control_MOSI",
    default=0xffff,
    help="The port control value for the MOSI signal.",
    mandatory=false,
    validate=parameters.test_uint16,
    constrains=nil
  },
  {
    name="port_control_MISO",
    default=0xffff,
    help="The port control value for the MISO signal.",
    mandatory=false,
    validate=parameters.test_uint16,
    constrains=nil
  },
  {
    name="port_control_SIO2",
    default=0xffff,
    help="The port control value for the SIO2 signal.",
    mandatory=false,
    validate=parameters.test_uint16,
    constrains=nil
  },
  {
    name="port_control_SIO3",
    default=0xffff,
    help="The port control value for the SIO3 signal.",
    mandatory=false,
    validate=parameters.test_uint16,
    constrains=nil
  },
  {
    name="mmio_CS",
    default=0xff,
    help="The MMIO index for the CS signal.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="mmio_CLK",
    default=0xff,
    help="The MMIO index for the CLK signal.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="mmio_MISO",
    default=0xff,
    help="The MMIO index for the MISO signal.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="mmio_MOSI",
    default=0xff,
    help="The MMIO index for the MOSI signal.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="mmio_SIO2",
    default=0xff,
    help="The MMIO index for the SIO2 signal.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="mmio_SIO3",
    default=0xff,
    help="The MMIO index for the SIO3 signal.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="dummy_byte",
    default=0xff,
    help="The 8 bit value for the dummy cycles.",
    mandatory=false,
    validate=parameters.test_uint8,
    constrains=nil
  },
  {
    name="mode",
    default=nil,
    help="The SPI mode for the communication.",
    mandatory=true,
    validate=parameters.test_choice_single,
    constrains="0,1,2,3"
  },
  {
    name="idle_configuration",
    default=0,
    help="The idle configuration of the interface.",
    mandatory=false,
    validate=parameters.test_choice_multiple,
    constrains="0,IO1_OE,IO1_OUT,IO2_OE,IO2_OUT,IO3_OE,IO3_OUT"
  },
  {
    name="macro",
    default=nil,
    help="The macro to execute.",
    mandatory=true,
    validate=nil,
    constrains=nil
  }
}

local astrEscapes = {
  ['n']='\n',
  ['"']='"'
}
local function unescape(strEscaped)
  return string.gsub(strEscaped, '\\([n"])', astrEscapes)
end

function run(aParameters)
  ----------------------------------------------------------------------
  --
  -- Parse the parameters and collect all options.
  --
  local Logging = require 'logging'
  local cLogger = require 'logging.console'()
  cLogger:setLevel(Logging.DEBUG)

  local cSpiFlashMacroTest = require 'spi_flash_macro_test'
  local f = cSpiFlashMacroTest(cLogger)

  -- Parse the idle configuration.
  print(aParameters['idle_configuration'])
  local ucIdleConfiguration = 0

  -- Parse the unit.
  local atUnits = {
    ['netX50_SPI0'] = f.UNIT_netX50_SPI0,
    ['netX50_SPI1'] = f.UNIT_netX50_SPI1,

    ['netX56_SQI'] = f.UNIT_netX56_SQI,
    ['netX56_SPI'] = f.UNIT_netX56_SPI,

    ['netX4000_SQI0'] = f.UNIT_netX4000_SQI0,
    ['netX4000_SQI1'] = f.UNIT_netX4000_SQI1,
    ['netX4000_SPI'] = f.UNIT_netX4000_SPI
  }
  local uiUnit = atUnits[aParameters['unit']]

  -- Parse the chip select.
  local uiChipSelect = tonumber(aParameters['chip_select'])

  -- Unescape the macro.
  local strMacro = unescape(string.sub(aParameters['macro'], 2, -2))

  local atSpiConfiguration = {
    ulSpeedFifoKhz = tonumber(aParameters['speed']),
    ulSpeedSqiRomKhz = 0,
    ausPortControl = {
      usCLK = tonumber(aParameters['port_control_CLK']),
      usMOSI = tonumber(aParameters['port_control_MOSI']),
      usMISO = tonumber(aParameters['port_control_MISO']),
      usSIO2 = tonumber(aParameters['port_control_SIO2']),
      usSIO3 = tonumber(aParameters['port_control_SIO3']),
      usCS0 = tonumber(aParameters['port_control_CS'])
    },
    aucMmio = {
      ucCS = tonumber(aParameters['mmio_CS']),
      ucCLK = tonumber(aParameters['mmio_CLK']),
      ucMISO = tonumber(aParameters['mmio_MISO']),
      ucMOSI = tonumber(aParameters['mmio_MOSI']),
      ucSIO2 = tonumber(aParameters['mmio_SIO2']),
      ucSIO3 = tonumber(aParameters['mmio_SIO3']),
    },
    ucDummyByte = tonumber(aParameters['dummy_byte']),
    ucMode = tonumber(aParameters['mode']),
    ucIdleConfiguration = ucIdleConfiguration
  }
  local tSpiCfg = f:compile_spi_configuration(atSpiConfiguration)

  local aucMacro = f:compile_macro(strMacro)
  if aucMacro==nil then
    error('Failed to compile the macro!')
  end

  local tTest = f:compile(tSpiCfg, uiUnit, uiChipSelect, aucMacro)

  tPlugin = tester.getCommonPlugin()
  if not tPlugin then
    error('No plugin selected, nothing to do!')
  end

  local ulResult = tester.mbin_simple_run(nil, tPlugin, 'netx/spi_flash_macro_test_netx${ASIC_TYPE}.bin', tTest)
  if ulResult~=0 then
    error('The SPI macro failed.')
  end

  print("")
  print(" #######  ##    ## ")
  print("##     ## ##   ##  ")
  print("##     ## ##  ##   ")
  print("##     ## #####    ")
  print("##     ## ##  ##   ")
  print("##     ## ##   ##  ")
  print(" #######  ##    ## ")
  print("")
end
