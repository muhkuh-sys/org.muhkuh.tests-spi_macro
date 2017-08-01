module("test@ID@", package.seeall)

require("parameters")
require("ramtest")

CFG_strTestName = "@NAME@"

CFG_aParameterDefinitions = {
	{
		name="interface",
		default=nil,
		help="This is the interface where the RAM is connected.",
		mandatory=true,
		validate=parameters.test_choice_single,
		constrains="RAM,SDRAM_HIF,SDRAM_MEM,SRAM_HIF,SRAM_MEM"
	},
	{
		name="ram_start",
		default=nil,
		help="Only if interface is RAM: the start of the RAM area.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="ram_size",
		default=nil,
		help="Only if interface is RAM: The size of the RAM area in bytes.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sdram_netx",
		default=nil,
		help="This specifies the chip type for the parameter set.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sdram_general_ctrl",
		default=nil,
		help="Only if interface is SDRAM: The complete value for the netX general_ctrl register.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sdram_timing_ctrl",
		default=nil,
		help="Only if interface is SDRAM: The complete value for the netX timing_ctrl register.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sdram_mr",
		default=nil,
		help="Only if interface is SDRAM: The complete value for the netX mr register.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sdram_size_exponent",
		default=23,
		help="Only if interface is SDRAM: The size exponent.",
		mandatory=true,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sram_chip_select",
		default=nil,
		help="Only if interface is SRAM: The chip select of the SRAM device.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sram_ctrl",
		default=nil,
		help="Only if interface is SRAM: The complete value for the netX extsramX_ctrl register.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="sram_size",
		default=nil,
		help="Only if interface is SRAM: The size of the SRAM area in bytes.",
		mandatory=false,
		validate=parameters.test_uint32,
		constrains=nil
	},
	{
		name="checks",
		default="DATABUS,MARCHC,CHECKERBOARD,32BIT,BURST",
		help="This determines which checks to run. Select one or more values from this list and separate them with commata: 08BIT, 16BIT, 32BIT and BURST.",
		mandatory=true,
		validate=parameters.test_choice_multiple,
		constrains="DATABUS,08BIT,16BIT,32BIT,MARCHC,CHECKERBOARD,BURST"
	},
	{
		name="loops",
		default="1",
		help="The number of loops to run.",
		mandatory=true,
		validate=parameters.test_uint32,
		constrains=nil
	}
}

function run(aParameters)
	----------------------------------------------------------------------
	--
	-- Parse the parameters and collect all options.
	--
	
	-- Parse the interface option.
	local strValue = aParameters["interface"]
	local atInterfaces = {
		["RAM"]       = ramtest.INTERFACE_RAM,
		["SDRAM_HIF"] = ramtest.INTERFACE_SDRAM_HIF,
		["SDRAM_MEM"] = ramtest.INTERFACE_SDRAM_MEM,
		["SRAM_HIF"]  = ramtest.INTERFACE_SRAM_HIF,
		["SRAM_MEM"]  = ramtest.INTERFACE_SRAM_MEM
	}
	local ulInterface = atInterfaces[strValue]
	if ulInterface==nil then
		error(string.format("Unknown interface ID: %s", strValue))
	end
	
	-- Parse the test list.
	local astrElements = parameters.split_string(aParameters["checks"])
	local ulChecks = 0
	local atTests = {
		["DATABUS"]      = ramtest.CHECK_DATABUS,
		["08BIT"]        = ramtest.CHECK_08BIT,
		["16BIT"]        = ramtest.CHECK_16BIT,
		["32BIT"]        = ramtest.CHECK_32BIT,
		["MARCHC"]       = ramtest.CHECK_MARCHC,
		["CHECKERBOARD"] = ramtest.CHECK_CHECKERBOARD,
		["BURST"]        = ramtest.CHECK_BURST
	}
	for iCnt,strElement in ipairs(astrElements) do
		local ulValue = atTests[strElement]
		if ulValue==nil then
			error(string.format("Unknown check ID: %s", strElement))
		end
		ulChecks = ulChecks + ulValue
	end
	
	local ulLoops = tonumber(aParameters["loops"])
	
	
	local atRamAttributes = {
		["interface"] = ulInterface
	}
	-- Check if the required parameters are present. This depends on the interface.
	-- The RAM interface needs ram_start and ram_size.
	if ulInterface==ramtest.INTERFACE_RAM then
		if aParameters["ram_start"]==nil or aParameters["ram_size"] then
			error("The RAM interface needs the ram_start and ram_size parameter set.")
		end
		
		atRamAttributes["ram_start"]  = tonumber(aParameters["ram_start"])
		atRamAttributes["ram_size"]   = tonumber(aParameters["ram_size"])
	-- The SDRAM interfaces need sdram_general_ctrl, sdram_timing_ctrl, sdram_mr.
	-- NOTE: The sdram_size_exponent is optional. It can be derived from the sdram_general_ctrl.
	elseif ulInterface==ramtest.INTERFACE_SDRAM_HIF or ulInterface==ramtest.INTERFACE_SDRAM_MEM then
		if aParameters["sdram_netx"]==nil or aParameters["sdram_general_ctrl"]==nil or aParameters["sdram_timing_ctrl"]==nil or aParameters["sdram_mr"]==nil then
			error("The SDRAM interface needs the sdram_netx, sdram_general_ctrl, sdram_timing_ctrl and sdram_mr parameter set.")
		end
		
		atRamAttributes["netX"]          = tonumber(aParameters["sdram_netx"])
		atRamAttributes["general_ctrl"]  = tonumber(aParameters["sdram_general_ctrl"])
		atRamAttributes["timing_ctrl"]   = tonumber(aParameters["sdram_timing_ctrl"])
		atRamAttributes["mr"]            = tonumber(aParameters["sdram_mr"])
		atRamAttributes["size_exponent"] = tonumber(aParameters["sdram_size_exponent"])
	-- The SRAM interface needs sram_chip_select, sram_ctrl and sram_size.
	elseif ulInterface==ramtest.INTERFACE_SRAM_HIF or ulInterface==ramtest.INTERFACE_SRAM_MEM then
		if aParameters["sram_chip_select"]==nil or aParameters["sram_ctrl"]==nil or aParameters["sram_size"]==nil then
			error("The SRAM interface needs the sram_chip_select, sram_ctrl and sram_size parameter set.")
		end
		
		atRamAttributes["sram_chip_select"]  = tonumber(aParameters["sram_chip_select"])
		atRamAttributes["sram_ctrl"]         = tonumber(aParameters["sram_ctrl"])
		atRamAttributes["sram_size"]         = tonumber(aParameters["sram_size"])
	else
		error("Unknown interface ID:"..ulInterface)
	end
	
	
	----------------------------------------------------------------------
	--
	-- Open the connection to the netX.
	-- (or re-use an existing connection.)
	--
	local tPlugin = tester.getCommonPlugin()
	if tPlugin==nil then
		error("No plug-in selected, nothing to do!")
	end
	
	

	local ulRAMStart = ramtest.get_ram_start(tPlugin, atRamAttributes)
	local ulRAMSize  = ramtest.get_ram_size(tPlugin, atRamAttributes)
	
	ramtest.setup_ram(tPlugin, atRamAttributes)
	ramtest.test_ram(tPlugin, ulRAMStart, ulRAMSize, ulChecks, ulLoops)
	ramtest.disable_ram(tPlugin, atRamAttributes)
	
	
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

