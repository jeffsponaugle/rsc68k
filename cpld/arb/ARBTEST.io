CHIP "C:\ARBTEST"
BEGIN

    DEVICE = "PLCC84";
    "REQ1"                                    : INPUT_PIN = 12 ;
    "REQ0"                                    : INPUT_PIN = 11 ;
    "REQ_CLOCKED1"                            : NODE_NUM = 610 ;
    "REQ_CLOCKED0"                            : NODE_NUM = 612 ;
    "GRANT0"                                  : OUTPUT_PIN = 5 ;
    "PE_BIAS"                                 : NODE_NUM = 615 ;
    "GRANT1"                                  : OUTPUT_PIN = 4 ;
    "TDI"                                     : INPUT_PIN = 14 ;
    "TMS"                                     : INPUT_PIN = 23 ;
    "TCK"                                     : INPUT_PIN = 62 ;
    "TDO"                                     : INPUT_PIN = 71 ;
    "CLK_IN"                                  : INPUT_PIN = 83 ;
    "RESET_IN"                                : INPUT_PIN = 1 ;
END;
