local TestClassSpiMacro = require 'test_class_spi_macro'
return function(ulTestID, tLogWriter, strLogLevel) return TestClassSpiMacro('@NAME@', ulTestID, tLogWriter, strLogLevel) end
