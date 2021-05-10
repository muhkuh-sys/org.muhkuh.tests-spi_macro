-- Create the SpiFlashMacroTest class.
local class = require 'pl.class'
local SpiFlashMacroTest = class()


--- Initialize a new SpiFlashMacroTest instance.
-- @param tLogger The logger object used for all kinds of messages.
function SpiFlashMacroTest:_init(tLog)
  self.tLog = tLog

  -- The "penlight" module is always useful.
  self.pl = require'pl.import_into'()
  self.lpeg = require "lpeglabel"

  self.atMacroTokens = {
    SMC_RECEIVE  = ${SMC_RECEIVE},
    SMC_SEND     = ${SMC_SEND},
    SMC_IDLE     = ${SMC_IDLE},
    SMC_DUMMY    = ${SMC_DUMMY},
    SMC_JUMP     = ${SMC_JUMP},
    SMC_CHTR     = ${SMC_CHTR},
    SMC_CMP      = ${SMC_CMP},
    SMC_MASK     = ${SMC_MASK},
    SMC_MODE     = ${SMC_MODE},
    SMC_ADR      = ${SMC_ADR},
    SMC_FAIL     = ${SMC_FAIL},
	SMC_CS     = ${SMC_CS_MODE},

    SMCS_FALSE     = ${SMCS_FALSE},
    SMCS_TRUE     = ${SMCS_TRUE},

    SPI_MODE0    = ${SPI_MODE0},
    SPI_MODE1    = ${SPI_MODE1},
    SPI_MODE2    = ${SPI_MODE2},
    SPI_MODE3    = ${SPI_MODE3},

    SPI_BUS_WIDTH_1BIT = ${SPI_BUS_WIDTH_1BIT},
    SPI_BUS_WIDTH_2BIT = ${SPI_BUS_WIDTH_2BIT},
    SPI_BUS_WIDTH_4BIT = ${SPI_BUS_WIDTH_4BIT},

    SPI_MACRO_CHANGE_TRANSPORT_FIFO       = ${SPI_MACRO_CHANGE_TRANSPORT_FIFO},
    SPI_MACRO_CHANGE_TRANSPORT_ROM        = ${SPI_MACRO_CHANGE_TRANSPORT_ROM},

    SPI_MACRO_CONDITION_Always    = ${SPI_MACRO_CONDITION_Always},
    SPI_MACRO_CONDITION_Equal     = ${SPI_MACRO_CONDITION_Equal},
    SPI_MACRO_CONDITION_NotEqual  = ${SPI_MACRO_CONDITION_NotEqual},
    SPI_MACRO_CONDITION_Zero      = ${SPI_MACRO_CONDITION_Zero},
    SPI_MACRO_CONDITION_NotZero   = ${SPI_MACRO_CONDITION_NotZero},

    MSK_SQI_CFG_IDLE_IO1_OE         = ${MSK_SQI_CFG_IDLE_IO1_OE},
    SRT_SQI_CFG_IDLE_IO1_OE         = ${SRT_SQI_CFG_IDLE_IO1_OE},
    MSK_SQI_CFG_IDLE_IO1_OUT        = ${MSK_SQI_CFG_IDLE_IO1_OUT},
    SRT_SQI_CFG_IDLE_IO1_OUT        = ${SRT_SQI_CFG_IDLE_IO1_OUT},
    MSK_SQI_CFG_IDLE_IO2_OE         = ${MSK_SQI_CFG_IDLE_IO2_OE},
    SRT_SQI_CFG_IDLE_IO2_OE         = ${SRT_SQI_CFG_IDLE_IO2_OE},
    MSK_SQI_CFG_IDLE_IO2_OUT        = ${MSK_SQI_CFG_IDLE_IO2_OUT},
    SRT_SQI_CFG_IDLE_IO2_OUT        = ${SRT_SQI_CFG_IDLE_IO2_OUT},
    MSK_SQI_CFG_IDLE_IO3_OE         = ${MSK_SQI_CFG_IDLE_IO3_OE},
    SRT_SQI_CFG_IDLE_IO3_OE         = ${SRT_SQI_CFG_IDLE_IO3_OE},
    MSK_SQI_CFG_IDLE_IO3_OUT        = ${MSK_SQI_CFG_IDLE_IO3_OUT},
    SRT_SQI_CFG_IDLE_IO3_OUT        = ${SRT_SQI_CFG_IDLE_IO3_OUT}
  }


  self.SPI_MACRO_MAX_SIZE              = ${SPI_MACRO_MAX_SIZE}

  self.UNIT_netX50_SPI0 = 0
  self.UNIT_netX50_SPI1 = 1

  self.UNIT_netX56_SQI = 0
  self.UNIT_netX56_SPI = 1

  self.UNIT_netX90_SQI = 0

  self.UNIT_netX500_SPI = 0

  self.UNIT_netX4000_SQI0 = 0
  self.UNIT_netX4000_SQI1 = 1
  self.UNIT_netX4000_SPI = 2

  -- The vstruct module packs and depacks data.
  self.vstruct = require 'vstruct'
  self.tLog.debug('Using vstruct V%s', self.vstruct._VERSION)

  -- The format for the parameters.
  self.strFormat = string.format([[
    tSpiConfiguration: {
      ulSpeedFifoKhz:u4
      ulSpeedSqiRomKhz:u4
      ausPortControl: {
        usCS0:u2
        usCLK:u2
        usMISO:u2
        usMOSI:u2
        usSIO2:u2
        usSIO3:u2
      }
      aucMmio: {
        ucCS:u1
        ucCLK:u1
        ucMISO:u1
        ucMOSI:u1
        ucSIO2:u1
        ucSIO3:u1
      }
      ucDummyByte:u1
      ucMode:u1
      ucIdleConfiguration:u1
    }
    a4

    uiUnit:u4
    uiChipSelect:u4

    sizSpiMacro:u4
    aucSpiMacro:z%d,1
  ]], self.SPI_MACRO_MAX_SIZE)
end



function SpiFlashMacroTest:new_spi_configuration()
  -- Create a new SPI configuration with default values.
  local atSpiConfiguration = {
    -- Start with 1MHz in FIFO mode and no configuration for the ROM mode.
    ulSpeedFifoKhz = 1000,
    ulSpeedSqiRomKhz = 0,
    -- No port control values.
    ausPortControl = {
      usCLK = 0xffff,
      usMOSI = 0xffff,
      usMISO = 0xffff,
      usSIO2 = 0xffff,
      usSIO3 = 0xffff,
      usCS0 = 0xffff
    },
    aucMmio = {
      -- No MMIO for the all signals.
      ucCS = 0xff,
      ucCLK = 0xff,
      ucMISO = 0xff,
      ucMOSI = 0xff
    },
    -- Send 0x00 as the dummy byte.
    ucDummyByte = 0x00,
    -- Use SPI mode 0 by default.
    ucMode = self.SPI_MODE0,
    -- Do not drive any lines in idle mode.
    ucIdleConfiguration = 0
  }

  return atSpiConfiguration
end



function SpiFlashMacroTest:spi_configuration_set_speed_khz(atSpiConfiguration, ulSpeedFifoKhz, ulSpeedSqiRomKhz)
  local tResult = true

  if ulSpeedFifoKhz<=0 then
    self.tLog.error('The speed for the FIFO mode must be larger than 0. Here it is %s.', tostring(ulSpeedFifoKhz))
    tResult = nil
  end
  if ulSpeedSqiRomKhz<0 then
    self.tLog.error('The speed for the SQI ROM mode must be larger than or equal to 0. Here it is %s.', tostring(ulSpeedSqiRomKhz))
    tResult = nil
  end

  if tResult==true then
    atSpiConfiguration.ulSpeedFifoKhz = ulSpeedFifoKhz
    atSpiConfiguration.ulSpeedSqiRomKhz = ulSpeedSqiRomKhz
  end

  return tResult
end



function SpiFlashMacroTest:spi_configuration_set_port_control(atSpiConfiguration, usCLK, usMOSI, usMISO, usSIO2, usSIO3, usCS0)
  local tResult = true
  if usCLK<0 or usCLK>0xffff then
    self.tLog.error('The port control value for CLK must be an integer in the range [0, 65535]. Here it is %d.', tostring(usCLK))
    tResult = nil
  end
  if usMOSI<0 or usMOSI>0xffff then
    self.tLog.error('The port control value for MOSI must be an integer in the range [0, 65535]. Here it is %d.', tostring(usMOSI))
    tResult = nil
  end
  if usMISO<0 or usMISO>0xffff then
    self.tLog.error('The port control value for MISO must be an integer in the range [0, 65535]. Here it is %d.', tostring(usMISO))
    tResult = nil
  end
  if usSIO2<0 or usSIO2>0xffff then
    self.tLog.error('The port control value for SIO2 must be an integer in the range [0, 65535]. Here it is %d.', tostring(usSIO2))
    tResult = nil
  end
  if usSIO3<0 or usSIO3>0xffff then
    self.tLog.error('The port control value for SIO3 must be an integer in the range [0, 65535]. Here it is %d.', tostring(usSIO3))
    tResult = nil
  end
  if usCS0<0 or usCS0>0xffff then
    self.tLog.error('The port control value for CS0 must be an integer in the range [0, 65535]. Here it is %d.', tostring(usCS0))
    tResult = nil
  end

  if tResult==true then
    atSpiConfiguration.ausPortControl.usCLK = usCLK
    atSpiConfiguration.ausPortControl.usMOSI = usMOSI
    atSpiConfiguration.ausPortControl.usMISO = usMISO
    atSpiConfiguration.ausPortControl.usSIO2 = usSIO2
    atSpiConfiguration.ausPortControl.usSIO3 = usSIO3
    atSpiConfiguration.ausPortControl.usCS0 = usCS0
  end

  return tResult
end



function SpiFlashMacroTest:spi_configuration_set_mmio(atSpiConfiguration, ucCS, ucCLK, ucMISO, ucMOSI, ucSIO2, ucSIO3)
  local tResult = true
  if ucCS<0 or ucCS>0xff then
    self.tLog.error('The MMIO index for CS must be an integer in the range [0, 255]. Here it is %d.', tostring(ucCS))
    tResult = nil
  end
  if ucCLK<0 or ucCLK>0xff then
    self.tLog.error('The MMIO index for CLK must be an integer in the range [0, 255]. Here it is %d.', tostring(ucCLK))
    tResult = nil
  end
  if ucMISO<0 or ucMISO>0xff then
    self.tLog.error('The MMIO index for MISO must be an integer in the range [0, 255]. Here it is %d.', tostring(ucMISO))
    tResult = nil
  end
  if ucMOSI<0 or ucMOSI>0xff then
    self.tLog.error('The MMIO index for MOSI must be an integer in the range [0, 255]. Here it is %d.', tostring(ucMOSI))
    tResult = nil
  end
  if ucSIO2<0 or ucSIO2>0xff then
    self.tLog.error('The MMIO index for ucSIO2 must be an integer in the range [0, 255]. Here it is %d.', tostring(ucSIO2))
    tResult = nil
  end
  if ucSIO3<0 or ucSIO3>0xff then
    self.tLog.error('The MMIO index for ucSIO3 must be an integer in the range [0, 255]. Here it is %d.', tostring(ucSIO3))
    tResult = nil
  end

  if tResult==true then
    atSpiConfiguration.aucMmio.ucCS = ucCS
    atSpiConfiguration.aucMmio.ucCLK = ucCLK
    atSpiConfiguration.aucMmio.ucMISO = ucMISO
    atSpiConfiguration.aucMmio.ucMOSI = ucMOSI
    atSpiConfiguration.aucMmio.ucSIO2 = ucSIO2
    atSpiConfiguration.aucMmio.ucSIO3 = ucSIO3
  end

  return tResult
end



function SpiFlashMacroTest:spi_configuration_set_dummy_byte(atSpiConfiguration, ucDummyByte)
  local tResult = true
  if ucDummyByte<0 or ucDummyByte>0xff then
    self.tLog.error('The dummy byte must be an integer in the range [0, 255]. Here it is %d.', tostring(ucDummyByte))
    tResult = nil
  end

  if tResult==true then
    atSpiConfiguration.ucDummyByte = ucDummyByte
  end

  return tResult
end



function SpiFlashMacroTest:spi_configuration_set_mode(atSpiConfiguration, ucMode)
  local tResult = nil
  local atValid = {self.atMacroTokens.SPI_MODE0, self.atMacroTokens.SPI_MODE1, self.atMacroTokens.SPI_MODE2, self.atMacroTokens.SPI_MODE3}
  for _,tMode in pairs(atValid) do
    if tMode==ucMode then
      tResult = true
      break
    end
  end

  if tResult~=true then
    self.tLog.error('Invalid mode specified: %s. Accepted values are %s.', tostring(ucMode), table.concat(atValid, ', '))
  else
    atSpiConfiguration.ucMode = ucMode
  end

  return tResult
end



function SpiFlashMacroTest:spi_configuration_set_idle(atSpiConfiguration, ucIdleConfiguration)
  local tResult = true
  if ucIdleConfiguration<0 or ucIdleConfiguration>0xff then
    self.tLog.error('The idle configuration must be an integer in the range [0, 255]. Here it is %d.', tostring(ucIdleConfiguration))
    tResult = nil
  end

  if tResult==true then
    atSpiConfiguration.ucIdleConfiguration = ucIdleConfiguration
  end

  return tResult
end



function SpiFlashMacroTest:compile_spi_configuration(atSpiConfiguration)
  local atCfg = self:new_spi_configuration()

  -- Set the initial speed.
  local tResult = self:spi_configuration_set_speed_khz(atCfg, atSpiConfiguration.ulSpeedFifoKhz, atSpiConfiguration.ulSpeedSqiRomKhz)
  if tResult==true then
    -- Set the port control values.
    tResult = self:spi_configuration_set_port_control(atCfg, atSpiConfiguration.ausPortControl.usCLK, atSpiConfiguration.ausPortControl.usMOSI, atSpiConfiguration.ausPortControl.usMISO, atSpiConfiguration.ausPortControl.usSIO2, atSpiConfiguration.ausPortControl.usSIO3, atSpiConfiguration.ausPortControl.usCS0)
    if tResult==true then
      -- Set the MMIO signals.
      tResult = self:spi_configuration_set_mmio(atCfg, atSpiConfiguration.aucMmio.ucCS, atSpiConfiguration.aucMmio.ucCLK, atSpiConfiguration.aucMmio.ucMISO, atSpiConfiguration.aucMmio.ucMOSI, atSpiConfiguration.aucMmio.ucSIO2, atSpiConfiguration.aucMmio.ucSIO3)
      if tResult==true then
        tResult = self:spi_configuration_set_dummy_byte(atCfg, atSpiConfiguration.ucDummyByte)
        if tResult==true then
          tResult = self:spi_configuration_set_mode(atCfg, atSpiConfiguration.ucMode)
          if tResult==true then
            tResult = self:spi_configuration_set_idle(atCfg, atSpiConfiguration.ucIdleConfiguration)
          end
        end
      end
    end
  end

  if tResult~=true then
    error('Failed to set the SPI configuration.')
  end

  return atCfg
end



--- Check the macro for syntax errors and converts each character into its equivalent macro token.
-- @param strMacro macro
-- @param uiMacroTimeoutMs time out [ms]
-- @return tResult: true/false
-- @return aucMacro: string with all occurring macro tokens
-- @return tMsg: string with all occurring error messages
function SpiFlashMacroTest:compile_macro(strMacro, uiMacroTimeoutMs)
	uiMacroTimeoutMs = uiMacroTimeoutMs or 0

	local lpeg = self.lpeg
	local pl = self.pl
	local tLog = self.tLog

	-- Be optimistic
	local tResult = true

	-- Combination of all data as string.
	local aucMacro = pl.List()

	-- Error messages
	local tMsg = pl.List("\n")

	local aucOpcodes = pl.List()
	local atLabels = {}
	local atLabelReferences = {}

	-- Save typing function names with "lpeg" in front of them:
	local P, V, Cg, Ct, Cc, S, R, C, Cf, Cb, Cs =
		lpeg.P,
		lpeg.V,
		lpeg.Cg,
		lpeg.Ct,
		lpeg.Cc,
		lpeg.S,
		lpeg.R,
		lpeg.C,
		lpeg.Cf,
		lpeg.Cb,
		lpeg.Cs

	-- Additional shortcut of lpeglabel
	local T = lpeg.T

	-- Match optional whitespace.
	local OptionalSpace = S(" \t") ^ 0

	local OpeningBracket = P("(")
	local ClosingBracket = P(")")

	local Comma = P(",")

	-- Auxiliary function: add optinal spaces around pattern
	local function OptSpace(pattern)
		return OptionalSpace * pattern * OptionalSpace
	end

	-- Auxiliary function: add brackets around pattern
	local function Brackets(pattern)
		return OpeningBracket * pattern * ClosingBracket
	end

	-- Auxiliary function: add number of bytes to table at first position
	local function AddNumbBytes(tdata)
		return pl.List(tdata):insert(1, #tdata)
	end

	-- Auxiliary function: match everything up to the pattern (return a caption)
	local function UpTo(pattern)
		return C((P(1) - pattern) ^ 0) * pattern
	end

	-- Auxiliary function: return a grammar which tries to match the given pattern in a string
	local function Anywhere(pattern)
		return P {pattern + 1 * lpeg.V(1)}
	end

	-- Auxiliary function: detect pattern in given table - func is optional
	local function Detect(pattern, tdata, func)
		func = func or function(dummy)
				return dummy
			end

		local grammar_search = Anywhere(pattern)

		local str
		local tmatch
		if type(tdata) == "table" then
			for _, value in ipairs(tdata) do
				str = tostring(value)
				tmatch = lpeg.match(grammar_search, func(str))
				if tmatch ~= nil then
					return str
				end
			end
		end
		return nil
	end

	-- Auxiliary function: list-pattern with separator
	local function List(pattern, sep)
		return pattern * (sep * pattern) ^ 0
	end

	-- Match an integer. This can be decimal, hexadecimal or binary.
	local DecimalInteger = R("09") ^ 1
	local HexInteger = P("0x") * (R("09", "af", "AF")) ^ 1
	local BinInteger = P("0b") * R("01") ^ 1
	local Integer = HexInteger + DecimalInteger + BinInteger

	-- Pattern of macro condition, bus width and cs mode
	local Condition = P("Always") + P("Equal") + P("NotEqual") + P("Zero") + P("NotZero")

	local Bus_Width = P("1BIT") + P("2BIT") + P("4BIT")

	local CS_Mode = P("true") + P("false")

	-- String pattern, either with single or double quotes
	local String =
		P {
		"start",
		start = V "single_quoted" + V "double_quoted",
		single_quoted = P("'") * UpTo(P("'")),
		double_quoted = P('"') * UpTo(P('"'))
	}

	-- Caption which match each char of a string
	local Chars = C(P(1)) ^ 0

	-- Add comment and "empty line", match to neglect it
	local EmptyLine = S(" \t\n\r") ^ 0 * -1
	local Comment = OptSpace(P("#")) * P(1) ^ 0

	-- Caption of label (label without ":" !)
	local Label = OptSpace(Cg((P(1) - S(":")) ^ 1, "cmd_label") * P(":")) -- (P(1) - S(":")) ^ 1

	-- Pattern of parameters: either with one or two parameters
	local ParamNumb_1 = (P(1) - V "closingBracket") ^ 0
	local ParamNumb_2 = (P(1) - V "comma") ^ 0 * V "comma" * (P(1) - V "closingBracket") ^ 0

	-- Auxiliary function: creates group captions of a command which consists of the pattern of the command and the pattern of parameters
	function Cmd(pattern_Cmd, pattern_param)
		return OptSpace(Cg(P(pattern_Cmd), "cmd")) * Cg(V "openingBracket" * pattern_param * V "closingBracket", "param")
	end

	-- Pattern of commands
	local Cmds =
		Cmd(P("RECEIVE"), ParamNumb_1) + Cmd(P("RECEIVE"), ParamNumb_1) + Cmd(P("SEND"), ParamNumb_1) +
		Cmd(P("IDLE"), ParamNumb_1) +
		Cmd(P("DUMMY"), ParamNumb_1) +
		Cmd(P("CHTR"), ParamNumb_1) +
		Cmd(P("CMP"), ParamNumb_1) +
		Cmd(P("MASK"), ParamNumb_1) +
		Cmd(P("MODE"), ParamNumb_1) +
		Cmd(P("ADR"), ParamNumb_1) +
		Cmd(P("CS"), ParamNumb_1) +
		Cmd(P("JUMP"), ParamNumb_2) +
		Cmd(P("FAIL"), ParamNumb_2)

	-- A line of the macro includes: a command consisting of name and paramter brackets (...) or label definition or comment or empty line
	local tGrammar_Cmds =
		Ct(
		P {
			"start", --> this tells LPEG which rule to process first
			start = Comment + EmptyLine + Label + Cmds + T "errCmd",
			openingBracket = OpeningBracket + T "errOpeningBrackets",
			closingBracket = ClosingBracket + T "errClosingBrackets",
			comma = Comma + T "errComma"
		}
	)

	-- Table of errror messages
	local tError = {
		errCmd = "Fail command - expected: <Command Name>(<Parameter(s)>) | <Label Name>: | #Comment",
		errOpeningBrackets = "Fail brackets - miss opening bracket (...)",
		errClosingBrackets = "Fail brackets - miss closing bracket (...)",
		errComma = "Fail separator - miss comma between parameters",
		errCS_Mode = "Fail parameter - expected CS mode: true or false",
		errCondition = "Fail parameter - expected condition: Always, Equal, NotEqual, Zero or NotZero",
		errBus_Width = "Fail parameter - expected bus width: 1BIT, 2BIT or 4BIT",
		errIntegerReceive = "Fail parameter - expected number: <number of received bytes>",
		errIntegerIdle = "Fail parameter - expected number: <number of idle cycles>",
		errIntegerDummy = "Fail parameter - expected number: <number of dummy bytes>",
		errListSend = "Fail parameter - expected list of numbers: <data to be transferred>,...",
		errListCMP = "Fail parameter - expected list of numbers: <data to be compared>,...",
		errListMask = "Fail parameter - expected list of numbers: <data to be masked>,...",
		errStringLabel = "Fail parameter - expected string: <name of label>",
		errStringErrMsg = "Fail parameter - expected string: <error message>",
		errLabel = "Fail label - label is redefined",
		errMacroToken = "Fail macro token - macro token does not exist in table",
		fail = "Fail - undefined error"
	}

	-- Auxiliary function to record error messages with the corresponding error label
	local function recordError(strLineMacro, uiLineNumber, errLab)
		tMsg:append(
			string.format(
				'[SPI Macro] [ERROR] line: %d - Fail: "%s" - %s\n',
				uiLineNumber,
				pl.stringx.strip(strLineMacro),
				tError[errLab]
			)
		)
	end

	-- Table with specified pattern of the paramters (...) of the commands
	local tGrammar_CmdParamters =
		pl.Map {
		RECEIVE = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = OptSpace(Integer / tonumber) * #ClosingBracket + T "errIntegerReceive"
		},
		SEND = P {
			"paramters",
			paramters = Brackets(V "param_1"),
			param_1 = Ct(List(OptSpace(Integer / tonumber), P(","))) / AddNumbBytes * #ClosingBracket + T "errListSend"
		},
		IDLE = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = OptSpace(Integer / tonumber) * #ClosingBracket + T "errIntegerIdle"
		},
		DUMMY = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = OptSpace(Integer / tonumber) * #ClosingBracket + T "errIntegerDummy"
		},
		JUMP = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1" * Comma * V "param_2")),
			param_1 = OptSpace(C(Condition)) * #Comma + T "errCondition",
			param_2 = OptSpace(Ct(Cg(String, "label"))) * #ClosingBracket + T "errStringLabel"
		},
		CHTR = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = C(P(1) ^ 0)
		},
		CMP = P {
			"paramters",
			paramters = Brackets(V "param_1"),
			param_1 = Ct(List(OptSpace(Integer / tonumber), P(","))) / AddNumbBytes * #ClosingBracket + T "errListCMP"
		},
		MASK = P {
			"paramters",
			paramters = Brackets(V "param_1"),
			param_1 = Ct(List(OptSpace(Integer / tonumber), P(","))) / AddNumbBytes * #ClosingBracket + T "errListMask"
		},
		MODE = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = OptSpace(C(Bus_Width)) * #ClosingBracket + T "errBus_Width"
		},
		ADR = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = C(P(1) ^ 0)
		},
		FAIL = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1" * Comma * V "param_2")),
			param_1 = OptSpace(C(Condition)) * #Comma + T "errCondition",
			param_2 = OptSpace(Ct(Cg(String, "string"))) * #ClosingBracket + T "errStringErrMsg"
		},
		CS = P {
			"paramters",
			paramters = Brackets(Ct(V "param_1")),
			param_1 = OptSpace(C(CS_Mode)) * #ClosingBracket + T "errCS_Mode"
		}
	}

	local uiLineNumber = 0
	local ulCurrentAddress = 0

	-- Key table of 'atMacroTokens'
	local MacroTokens_keys = pl.tablex.keys(self.atMacroTokens)

	-- Every single line of the macro is checked for errors and the corresponding command is extracted
	for strLine in pl.stringx.lines(strMacro) do
		-- Count the line numbers for error messages.
		uiLineNumber = uiLineNumber + 1
		tLog.debug("[SPI Macro] [line: %d] %s", uiLineNumber, strLine)

		-- Match the line with the Cmds grammar pattern
		local tmatchCmd, errLab = lpeg.match(tGrammar_Cmds, strLine)
		-- Append an error if the command is unkown
		if not tmatchCmd then
			-- No command match, add an error message
			tResult = false
			recordError(strLine, uiLineNumber, errLab)
		elseif tmatchCmd.cmd_label ~= nil then
			-- If the token ends with a colon, it is a label definition.
			-- Get the label name.
			local strLabelName = tmatchCmd.cmd_label
			tLog.debug("[SPI Macro] [LABEL] %s - address: %d", strLabelName, ulCurrentAddress)
			-- Does the label already exist?
			if atLabels[strLabelName] ~= nil then
				-- Label already exist, add an error message
				recordError(strLine, uiLineNumber, "errLabel")
				tResult = false
			else
				-- Create the new label.
				atLabels[strLabelName] = ulCurrentAddress
				tLog.debug('[SPI Macro] [LABEL] label "%s" created at address %d', strLabelName, ulCurrentAddress)
			end
		else
			-- The 'Cmds' pattern contains named group captures of the command and the parameters
			if tmatchCmd.cmd ~= nil and tmatchCmd.param ~= nil then
				local strCmd = tmatchCmd.cmd
				local strParam = tmatchCmd.param
				tLog.debug("[SPI Macro] [COMMAND] %s - address: %d", strCmd, ulCurrentAddress)

				-- Detect command key in table 'atMacroTokens'
				local strMacroToken = Detect(P("_") * P(string.upper(strCmd)) * -1, MacroTokens_keys, string.upper)
				if strMacroToken ~= nil then
					local tToken = self.atMacroTokens[strMacroToken]
					tLog.debug("[SPI Macro] [COMMAND] detected: %s - %d", strMacroToken, tToken)
					aucOpcodes:append(tToken)
					ulCurrentAddress = ulCurrentAddress + 1
				else
					-- No macro token match, add an error message
					tResult = false
					recordError(strLine, uiLineNumber, "errMacroToken")
				end

				-- Get the corresponding parameter pattern of the matched command
				local tGrammar_Param = tGrammar_CmdParamters:get(strCmd)
				-- Match the parameter(s) of the corresponding command (tmatchParam must be a TABLE!)
				local tmatchParam, errLab = lpeg.match(tGrammar_Param, strParam)
				if not tmatchParam then
					-- No paramter match, add an error message
					tResult = false
					recordError(strLine, uiLineNumber, errLab)
				else
					tLog.debug("[SPI Macro] [PARAMETER] [TABLE] paramters: %s - size: %d", strParam, #tmatchParam)
					-- Consider every parameter matched within the bracket
					for _, param in ipairs(tmatchParam) do
						-- Is this a known token?
						if type(param) == "string" then
							-- Parameters of type string within the table 'tmatchParam' are considered as macro tokens of table 'atMacroTokens'
							tLog.debug("[SPI Macro] [PARAMETER] [STRING] matched parameter: %s - address: %d", param, ulCurrentAddress)

							-- Detect parameter key in table 'atMacroTokens'
							local strMacroToken = Detect(P("_") * P(string.upper(param)) * -1, MacroTokens_keys, string.upper)
							if strMacroToken ~= nil then
								local tToken = self.atMacroTokens[strMacroToken]
								tLog.debug("[SPI Macro] [PARAMETER] [STRING] detected: %s - %d", strMacroToken, tToken)
								aucOpcodes:append(tToken)
								ulCurrentAddress = ulCurrentAddress + 1
							else
								-- No macro token match, add an error message
								tResult = false
								recordError(strLine, uiLineNumber, "errMacroToken")
							end
						elseif type(param) == "number" then
							-- Parameters of type number within the table 'tmatchParam'
							local ucNumber = param
							tLog.debug("[SPI Macro] [PARAMETER] [NUMBER] matched parameter: %d - address: %d", ucNumber, ulCurrentAddress)
							aucOpcodes:append(ucNumber)
							ulCurrentAddress = ulCurrentAddress + 1
						elseif type(param) == "table" then
							--  Parameters of type table within the table 'tmatchParam' are considered as label (with group capture "label") or as string (with group capture "string")
							if param.label ~= nil then
								-- This must be a label reference.
								local strLabelName = param.label

								-- Insert a dummy value.
								aucOpcodes:append(0x00)
								local tRef = atLabelReferences[strLabelName]
								if tRef == nil then
									tRef = {}
									atLabelReferences[strLabelName] = tRef
								end
								tLog.debug(
									"[SPI Macro] [PARAMETER] [TABLE] [LABEL] added reference to label: %s - address: %d",
									strLabelName,
									ulCurrentAddress
								)
								table.insert(tRef, ulCurrentAddress)
								ulCurrentAddress = ulCurrentAddress + 1
							elseif param.string ~= nil then
								-- This must be a matched string in a table
								-- Add null character to the end of the string
								local strMsg = param.string .. "\0"
								local uiMsg = #strMsg
								tLog.debug(
									"[SPI Macro] [PARAMETER] [TABLE] [STRING] string: %s - length: %d - address: %d",
									strMsg,
									uiMsg,
									ulCurrentAddress
								)

								-- Add the the length of the string
								aucOpcodes:append(uiMsg)
								ulCurrentAddress = ulCurrentAddress + 1

								-- Add each char of the string separately to the table 'aucOpcodes' due to the pointer reference
								local tChars = lpeg.match(Ct(Chars), strMsg)
								for _, char in ipairs(tChars) do
									aucOpcodes:append(char)
									ulCurrentAddress = ulCurrentAddress + 1
								end
							end
						end
					end
				end
			end
		end
	end

	if tResult == true then
		-- Resolve all labels.
		for strLabelName, atRefs in pairs(atLabelReferences) do
			local ulAddress = atLabels[strLabelName]
			if ulAddress == nil then
				tResult = false
				tMsg:append(
					string.format(
						'[SPI Macro] [ERROR] referenced unknown label "%s" at addresses %s.',
						strLabelName,
						table.concat(atRefs, ", ")
					)
				)
			else
				for _, uiPosition in ipairs(atRefs) do
					tLog.debug('[SPI Macro] Resolve label "%s" at position %d to address %d.', strLabelName, uiPosition, ulAddress)
					aucOpcodes[uiPosition + 1] = ulAddress
				end
			end
		end
	end

	if tResult == true then
		-- Build the header.
		local strHeaderFormat = [[
  uiTimeoutMs:u4
  ]]
		local tParameter = {
			uiTimeoutMs = uiMacroTimeoutMs
		}
		local tResult_Vstruct = self.vstruct.write(strHeaderFormat, tParameter)

		if tResult_Vstruct == nil then
			tResult = false
			tMsg:append(string.format("[SPI Macro] [ERROR] Failed to build the header."))
		else
			-- Combine all data to a string.
			aucMacro:append(tResult_Vstruct)

			for key, ucData in ipairs(aucOpcodes) do
				tLog.debug("[SPI Macro] [aucOpcodes] - address: %d - data: %s - type: %s", key, ucData, type(ucData))
				if type(ucData) == "number" then
					aucMacro:append(string.char(ucData))
				elseif type(ucData) == "string" then
					aucMacro:append(ucData)
				end
			end

			local sizMacro = #aucMacro:join()
			if sizMacro > self.SPI_MACRO_MAX_SIZE then
				tResult = false
				tMsg:append(
					string.format(
						"The macro has a size of %d bytes. This exceeds the available size of %d bytes.",
						sizMacro,
						self.SPI_MACRO_MAX_SIZE
					)
				)
			end
		end
	end

	return tResult, aucMacro:join(), tMsg:join() -- table.concat(strMsg)
end



function SpiFlashMacroTest:compile(atSpiConfiguration, uiUnit, uiChipSelect, strSpiMacro)
  local tResult = true

  local strType = type(uiUnit)
  if strType~='number' then
    self.tLog.error('The unit must be a number. Here it has the type %s.', strType)
    tResult = nil
  elseif uiUnit<0 then
    self.tLog.error('The unit must be larger than or equal to 0. Here it is %d.', uiUnit)
    tResult = nil
  end

  strType = type(uiChipSelect)
  if strType~='number' then
    self.tLog.error('The chip select must be a number. Here it has the type %s.', strType)
    tResult = nil
  elseif uiChipSelect<0 then
    self.tLog.error('The chip select must be larger than or equal to 0.')
    tResult = nil
  end

  if string.len(strSpiMacro)>self.SPI_MACRO_MAX_SIZE then
    self.tLog.error('The SPI macro is too long.')
    fAllOK = false
  end

  if tResult==true then
    local atGeneralParameters = {
      tSpiConfiguration = atSpiConfiguration,
      uiUnit = uiUnit,
      uiChipSelect = uiChipSelect,
      ulTestAreaStart = ulTestAreaStart,
      ulTestAreaEnd = ulTestAreaEnd,
      ulReadTestTimeInMs = ulReadTimeInMiliseconds,
      strSpiMacro = strSpiMacro
    }

    -- Collect all parameters in a table.
    local tParameter = {
      tSpiConfiguration = {
        ulSpeedFifoKhz = atGeneralParameters.tSpiConfiguration.ulSpeedFifoKhz,
        ulSpeedSqiRomKhz = atGeneralParameters.tSpiConfiguration.ulSpeedSqiRomKhz,
        ausPortControl = {
          usCLK = atGeneralParameters.tSpiConfiguration.ausPortControl.usCLK,
          usMOSI = atGeneralParameters.tSpiConfiguration.ausPortControl.usMOSI,
          usMISO = atGeneralParameters.tSpiConfiguration.ausPortControl.usMISO,
          usSIO2 = atGeneralParameters.tSpiConfiguration.ausPortControl.usSIO2,
          usSIO3 = atGeneralParameters.tSpiConfiguration.ausPortControl.usSIO3,
          usCS0 = atGeneralParameters.tSpiConfiguration.ausPortControl.usCS0
        },
        aucMmio = {
          ucCS = atGeneralParameters.tSpiConfiguration.aucMmio.ucCS,
          ucCLK = atGeneralParameters.tSpiConfiguration.aucMmio.ucCLK,
          ucMISO = atGeneralParameters.tSpiConfiguration.aucMmio.ucMISO,
          ucMOSI = atGeneralParameters.tSpiConfiguration.aucMmio.ucMOSI,
          ucSIO2 = atGeneralParameters.tSpiConfiguration.aucMmio.ucSIO2,
          ucSIO3 = atGeneralParameters.tSpiConfiguration.aucMmio.ucSIO3
        },
        ucDummyByte = atGeneralParameters.tSpiConfiguration.ucDummyByte,
        ucMode = atGeneralParameters.tSpiConfiguration.ucMode,
        ucIdleConfiguration = atGeneralParameters.tSpiConfiguration.ucIdleConfiguration
      },
      uiUnit = atGeneralParameters.uiUnit,
      uiChipSelect = atGeneralParameters.uiChipSelect,
      sizSpiMacro = string.len(atGeneralParameters.strSpiMacro),
      aucSpiMacro = atGeneralParameters.strSpiMacro
    }
    -- Convert the parameters to a byte string.
    tResult = self.vstruct.write(self.strFormat, tParameter)
  else
    error("Failed to set the unit.")
  end

  return tResult
end


return SpiFlashMacroTest
