local class = require 'pl.class'
local TestClass = require 'test_class'
local TestClassSpiMacro = class(TestClass)


function TestClassSpiMacro:_init(strTestName, uiTestCase, tLogWriter, strLogLevel)
  self:super(strTestName, uiTestCase, tLogWriter, strLogLevel)

  local P = self.P
  self:__parameter {
    P:SC('unit', 'This is the unit providing the SPI bus.'):
      required(true):
      constraint('netX50_SPI0,netX50_SPI1,netX56_SQI,netX56_SPI,netX90_SQI,netX4000_SQI0,netX4000_SQI1,netX4000_SPI'),

    P:U8('chip_select', 'The chip select number on the selected SPI bus.'):
      default(0):
      required(true),

    P:U32('speed', 'The speed for the SPI communication in kHz.'):
      default(1000):
      required(true),

    P:U16('port_control_CS', 'The port control value for the CS signal.'):
      default(0xffff):
      required(true),

    P:U16('port_control_CLK', 'The port control value for the CLK signal.'):
      default(0xffff):
      required(true),

    P:U16('port_control_MOSI', 'The port control value for the MOSI signal.'):
      default(0xffff):
      required(true),

    P:U16('port_control_MISO', 'The port control value for the MISO signal.'):
      default(0xffff):
      required(true),

    P:U16('port_control_SIO2', 'The port control value for the SIO2 signal.'):
      default(0xffff):
      required(true),

    P:U16('port_control_SIO3', 'The port control value for the SIO3 signal.'):
      default(0xffff):
      required(true),

    P:U8('mmio_CS', 'The MMIO index for the CS signal.'):
      default(0xff):
      required(true),

    P:U8('mmio_CLK', 'The MMIO index for the CLK signal.'):
      default(0xff):
      required(true),

    P:U8('mmio_MISO', 'The MMIO index for the MISO signal.'):
      default(0xff):
      required(true),

    P:U8('mmio_MOSI', 'The MMIO index for the MOSI signal.'):
      default(0xff):
      required(true),

    P:U8('mmio_SIO2', 'The MMIO index for the SIO2 signal.'):
      default(0xff):
      required(true),

    P:U8('mmio_SIO3', 'The MMIO index for the SIO3 signal.'):
      default(0xff):
      required(true),

    P:U8('dummy_byte', 'The 8 bit value for the dummy cycles.'):
      default(0xff):
      required(true),

    P:SC('mode', 'The SPI mode for the communication.'):
      required(true):
      constraint('0,1,2,3'),

    P:MC('idle_configuration', 'The idle configuration of the interface.'):
      default('0'):
      required(true):
      constraint('0,IO1_OE,IO1_OUT,IO2_OE,IO2_OUT,IO3_OE,IO3_OUT'),

    P:P('macro', 'The file name of the macro to execute.'):
      required(true)
  }
end



function TestClassSpiMacro:run()
  local atParameter = self.atParameter
  local tLog = self.tLog

  ----------------------------------------------------------------------
  --
  -- Parse the parameters and collect all options.
  --
  local cSpiMacroTest = require 'spi_macro_test'
  local f = cSpiMacroTest(tLog)

  -- Parse the idle configuration.
  print(atParameter['idle_configuration']:get())
  local ucIdleConfiguration = 0

  -- Parse the unit.
  local atUnits = {
    ['netX50_SPI0'] = f.UNIT_netX50_SPI0,
    ['netX50_SPI1'] = f.UNIT_netX50_SPI1,

    ['netX56_SQI'] = f.UNIT_netX56_SQI,
    ['netX56_SPI'] = f.UNIT_netX56_SPI,

    ['netX90_SQI'] = f.UNIT_netX90_SQI,

    ['netX4000_SQI0'] = f.UNIT_netX4000_SQI0,
    ['netX4000_SQI1'] = f.UNIT_netX4000_SQI1,
    ['netX4000_SPI'] = f.UNIT_netX4000_SPI
  }
  local uiUnit = atUnits[atParameter['unit']:get()]

  -- Parse the chip select.
  local uiChipSelect = atParameter['chip_select']:get()

  -- Read the macro file.
  local strFileName = self.pl.path.exists(atParameter['macro']:get())
  if strFileName==nil then
    tLog.error('The macro file "%s" does not exist.', atParameter['macro']:get())
    error('Failed to load the macro.')
  end
  local strMacro = self.pl.file.read(strFileName)

  local atSpiConfiguration = {
    ulSpeedFifoKhz = atParameter['speed']:get(),
    ulSpeedSqiRomKhz = 0,
    ausPortControl = {
      usCLK = atParameter['port_control_CLK']:get(),
      usMOSI = atParameter['port_control_MOSI']:get(),
      usMISO = atParameter['port_control_MISO']:get(),
      usSIO2 = atParameter['port_control_SIO2']:get(),
      usSIO3 = atParameter['port_control_SIO3']:get(),
      usCS0 = atParameter['port_control_CS']:get()
    },
    aucMmio = {
      ucCS = atParameter['mmio_CS']:get(),
      ucCLK = atParameter['mmio_CLK']:get(),
      ucMISO = atParameter['mmio_MISO']:get(),
      ucMOSI = atParameter['mmio_MOSI']:get(),
      ucSIO2 = atParameter['mmio_SIO2']:get(),
      ucSIO3 = atParameter['mmio_SIO3']:get(),
    },
    ucDummyByte = atParameter['dummy_byte']:get(),
    ucMode = atParameter['mode']:get(),
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

  local ulResult = tester.mbin_simple_run(nil, tPlugin, 'netx/spi_macro_test_netx${ASIC_TYPE}.bin', tTest)
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

return TestClassSpiMacro
