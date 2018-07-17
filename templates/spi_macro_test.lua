-- Create the SpiFlashMacroTest class.
local class = require 'pl.class'
local SpiFlashMacroTest = class()

-----------------------------------------------------------------------------
--                           Definitions
-----------------------------------------------------------------------------


SpiFlashMacroTest.atMacroTokens = {
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

  SMCS_NNN     = ${SMCS_NNN},
  SMCS_SNN     = ${SMCS_SNN},
  SMCS_SDN     = ${SMCS_SDN},
  SMCS_SDD     = ${SMCS_SDD},

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

SpiFlashMacroTest.SPI_MACRO_MAX_SIZE              = ${SPI_MACRO_MAX_SIZE}

SpiFlashMacroTest.UNIT_netX50_SPI0 = 0
SpiFlashMacroTest.UNIT_netX50_SPI1 = 1

SpiFlashMacroTest.UNIT_netX56_SQI = 0
SpiFlashMacroTest.UNIT_netX56_SPI = 1

SpiFlashMacroTest.UNIT_netX90_SQI = 0

SpiFlashMacroTest.UNIT_netX4000_SQI0 = 0
SpiFlashMacroTest.UNIT_netX4000_SQI1 = 1
SpiFlashMacroTest.UNIT_netX4000_SPI = 2



--- Initialize a new SpiFlashMacroTest instance.
-- @param tLogger The logger object used for all kinds of messages.
function SpiFlashMacroTest:_init(tLog)
  self.tLog = tLog

  -- The "penlight" module is always useful.
  self.pl = require'pl.import_into'()

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



function SpiFlashMacroTest:compile_macro(strMacro)
  local tResult = true
  local aucOpcodes = {}
  local atLabels = {}
  local atLabelReferences = {}

  -- Loop over each line in the string.
  local uiLineNumber = 0
  local ulCurrentAddress = 0
  for strLine in self.pl.stringx.lines(strMacro) do
    -- Count the line numbers for error messages.
    uiLineNumber = uiLineNumber + 1

    -- Strip whitespace from the line.
    local strStrippedLine = self.pl.stringx.strip(strLine)
    -- Ignore lines starting with '#'.
    if string.sub(strStrippedLine, 1, 1)~='#' then
      -- Split the line by commata into tokens.
      local astrTokens = {self.pl.stringx.splitv(strStrippedLine, ',')}
      for _, strToken in pairs(astrTokens) do
        local strStrippedToken = self.pl.stringx.strip(strToken)
        -- Is this a plain number?
        local ucNumber = tonumber(strStrippedToken)
        if ucNumber~=nil then
          table.insert(aucOpcodes, ucNumber)
          self.tLog.debug('[SPI Macro] %02x: 0x%02x', ulCurrentAddress, ucNumber)
          ulCurrentAddress = ulCurrentAddress + 1
        else
          -- If the token ends with a colon, it is a label definition.
          if string.sub(strStrippedToken, -1)==':' then
            -- Get the label name without the colon.
            local strLabelName = string.sub(strStrippedToken, 1, -2)
            -- Does the label already exist?
            if atLabels[strLabelName]~=nil then
              self.tLog.error('[SPI Macro] Label "%s" redefined in line %d.', strLabelName, uiLineNumber)
              tResult = nil
              break
            else
              -- Create the new label.
              atLabels[strLabelName] = ulCurrentAddress
              self.tLog.debug('[SPI Macro] Label "%s" created at address 0x%02x', strLabelName, ulCurrentAddress)
            end
  
          else
            -- Is this a known token?
            local tToken = self.atMacroTokens[strStrippedToken]
            if tToken~=nil then
              table.insert(aucOpcodes, tToken)
              self.tLog.debug('[SPI Macro] %02x: 0x%02x (%s)', ulCurrentAddress, tToken, strStrippedToken)
              ulCurrentAddress = ulCurrentAddress + 1
            else
              -- This must be a label reference.
              local strLabelName = strStrippedToken
              -- Insert a dummy value.
              table.insert(aucOpcodes, 0x00)
              local tRef = atLabelReferences[strLabelName]
              if tRef==nil then
                tRef = {}
                atLabelReferences[strLabelName] = tRef
              end
              table.insert(tRef, ulCurrentAddress)
              self.tLog.debug('[SPI Macro] Added reference to label "%s" at address 0x%02x.', strLabelName, ulCurrentAddress)
              ulCurrentAddress = ulCurrentAddress + 1
            end
          end
        end
      end
    end
  end

  if tResult==true then
    -- Resolve all labels.
    for strLabelName, atRefs in pairs(atLabelReferences) do
      local ulAddress = atLabels[strLabelName]
      if ulAddress==nil then
        self.tLog.error('[SPI Macro] Referenced unknown label "%s" at addresses %s.', strLabelName, table.concat(atRefs, ', '))
        tResult = nil
      else
        for _, uiPosition in ipairs(atRefs) do
          self.tLog.debug('[SPI Macro] Resolve label "%s" at position 0x%02x to address 0x%02x.', strLabelName, uiPosition, ulAddress)
          aucOpcodes[uiPosition + 1] = ulAddress
        end
      end
    end
  end

  if tResult==true then
    -- Combine all data to a string.
    local aucMacro = {}
    for _, ucData in ipairs(aucOpcodes) do
      table.insert(aucMacro, string.char(ucData))
    end
    tResult = table.concat(aucMacro)
  end

  return tResult
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
