local class = require 'pl.class'
local TestClass = require 'test_class'
local TestClassSpiMacro = class(TestClass)


function TestClassSpiMacro:_init(strTestName, uiTestCase, tLogWriter, strLogLevel)
  self:super(strTestName, uiTestCase, tLogWriter, strLogLevel)

  self.lpeg = require "lpeglabel"
  self.json = require 'dkjson'

  local P = self.P
  self:__parameter {
    P:P('plugin', 'A pattern for the plugin to use.'):
      required(false),

    P:P('plugin_options', 'Plugin options as a JSON object.'):
      required(false),

    P:SC('unit', 'This is the unit providing the SPI bus.'):
      required(true):
      constraint(table.concat({
        'netX50_SPI0',
        'netX50_SPI1',
        'netX56_SQI',
        'netX56_SPI',
        'netX90_SQI',
        'netX500_SPI',
        'netX4000_SQI0',
        'netX4000_SQI1',
        'netX4000_SPI'
      }, ',')),

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
      required(false),

    P:P('macro_dp', 'The data provider item for the macro.'):
      required(false),

    P:U32('timeout', 'A timeout in milliseconds for the complete script. A value of 0 disables the timeout.'):
      default(0):
      required(false)
  }
end



function TestClassSpiMacro:run()
  local atParameter = self.atParameter
  local lpeg = self.lpeg
  local pl = self.pl
  local json = self.json
  local tLog = self.tLog
  local tester = _G.tester

  ----------------------------------------------------------------------
  --
  -- Parse the parameters and collect all options.
  --
  local strPluginPattern = atParameter['plugin']:get()
  local strPluginOptions = atParameter['plugin_options']:get()

  local uiTimeoutMs = atParameter['timeout']:get()

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

    ['netX500_SPI'] = f.UNIT_netX500_SPI,

    ['netX4000_SQI0'] = f.UNIT_netX4000_SQI0,
    ['netX4000_SQI1'] = f.UNIT_netX4000_SQI1,
    ['netX4000_SPI'] = f.UNIT_netX4000_SPI
  }
  local uiUnit = atUnits[atParameter['unit']:get()]

  -- Parse the chip select.
  local uiChipSelect = atParameter['chip_select']:get()

  -- Read the macro parameter.
  local strMacroFile
  if atParameter['macro']:has_value() then
    strMacroFile = atParameter['macro']:get()
  elseif atParameter['macro_dp']:has_value() then
    local strDataProviderItem = atParameter['macro_dp']:get()
    local tItem = _G.tester:getDataItem(strDataProviderItem)
    if tItem==nil then
      local strMsg = string.format('No data provider item found with the name "%s".', strDataProviderItem)
      tLog.error(strMsg)
      error(strMsg)
    end
    strMacroFile = tItem.path
    if strMacroFile==nil then
      local strMsg = string.format(
        'The data provider item "%s" has no "path" attribute. Is this really a suitable provider for an SPI macro?',
        strDataProviderItem
      )
      tLog.error(strMsg)
      error(strMsg)
    end
  else
    local strMsg = 'No "macro" and no "macro_dp" parameter set.'
    tLog.error(strMsg)
    error(strMsg)
  end

  local path = require 'pl.path'
  if path.exists(strMacroFile)~=strMacroFile then
    local strMsg = string.format('Failed to load the macro: the file "%s" does not exist.', strMacroFile)
    tLog.error(strMsg)
    error(strMsg)
  end
  if path.isfile(strMacroFile)~=true then
    local strMsg = string.format('Failed to load the macro: the path "%s" does not point to a file.', strMacroFile)
    tLog.error(strMsg)
    error(strMsg)
  end
  local file = require 'pl.file'
  local strMacro = file.read(strMacroFile)

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
    ucMode = tonumber(atParameter['mode']:get()),
    ucIdleConfiguration = ucIdleConfiguration
  }
  local tSpiCfg = f:compile_spi_configuration(atSpiConfiguration)

  local tResult, aucMacro, strMsg = f:compile_macro(strMacro, uiTimeoutMs)

  if tResult ~= true then
    error(strMsg)
  end

  local tTest = f:compile(tSpiCfg, uiUnit, uiChipSelect, aucMacro)

  local atPluginOptions = {}
  if strPluginOptions~=nil then
    local tJson, uiPos, strJsonErr = json.decode(strPluginOptions)
    if tJson==nil then
      tLog.warning('Ignoring invalid plugin options. Error parsing the JSON: %d %s', uiPos, strJsonErr)
    else
      atPluginOptions = tJson
    end
  end
  local tPlugin = _G.tester:getCommonPlugin(strPluginPattern, atPluginOptions)
  if tPlugin==nil then
    local pretty = require 'pl.pretty'
    local strError = string.format(
      'Failed to establish a connection to the netX with pattern "%s" and options "%s".',
      strPluginPattern,
      pretty.write(atPluginOptions)
    )
    error(strError)
  end

  -- Local callback function to collect all messages of the NetX
  local tLog_NetX = pl.List()
  local fnCallback = function(a, _)
    tLog.debug("[NetX] %s", a)
    tLog_NetX:append(a)
    return true
  end

  local aAttr = tester:mbin_open("netx/spi_macro_test_netx${ASIC_TYPE}.bin", tPlugin)
  tester:mbin_debug(aAttr)
  tester:mbin_write(tPlugin, aAttr)
  tester:mbin_set_parameter(tPlugin, aAttr, tTest)
  local ulResult = tester:mbin_execute(tPlugin, aAttr, tTest, fnCallback)

  -- In the case an error occurs when processing by the NetX
  if ulResult ~= 0 then
    local error_msg

    if #tLog_NetX ~= 0 then
      -- Search pattern of error message
      local Error = lpeg.P("[ERROR]")
      local Grammar_err = {
        [1] = Error + 1 * lpeg.V(1) -- rule #1
      }
      -- Filted messages of NetX - only the [ERROR] messages should be displayed
      tLog_NetX =
        tLog_NetX:filter(
        function(x)
          return (lpeg.match(Grammar_err, x) ~= nil)
        end,
        Grammar_err,
        lpeg.match
      )

      -- Concat all error message, each for one line
      error_msg = table.concat(tLog_NetX)
      error_msg = string.gsub(error_msg, "%c", "")
      error_msg = pl.stringx.replace(error_msg, "[ERROR]", "\n[ERROR]")
    else
      -- Default error message
      error_msg = "The SPI macro failed."
    end

    error(error_msg)
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
