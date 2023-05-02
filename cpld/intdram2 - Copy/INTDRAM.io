CHIP "C:\USERS\JBS\DOCUMENTS\SRC\RSC68K\CPLD\INTDRAM2\INTDRAM"
BEGIN

    DEVICE = "PLCC84";
    "RESET_REQ"                               : NODE_NUM = 601 ;
    "EXPA_IRQ_MASK"                           : NODE_NUM = 602 ;
    "CPU_RESET_INV"                           : OUTPUT_PIN = 12 ;
    "DEBUG_BUTTON_MASK"                       : NODE_NUM = 604 ;
    "CLK_1M"                                  : OUTPUT_PIN = 11 ;
    "CLK_500K"                                : OUTPUT_PIN = 10 ;
    "REFRESH_TIMER4"                          : NODE_NUM = 607 ;
    "EXPB_IRQ"                                : INPUT_PIN = 9 ;
    "REFRESH_TIMER7"                          : NODE_NUM = 608 ;
    "DRAM_ACCESS_TIMER1"                      : NODE_NUM = 609 ;
    "REFRESH_COUNTER_RESET"                   : NODE_NUM = 610 ;
    "EXPA_IRQ"                                : INPUT_PIN = 8 ;
    "DRAM_MEM_CAS_PRE"                        : NODE_NUM = 611 ;
    "DRAM_MEM_RAS"                            : NODE_NUM = 612 ;
    "DRAM_ACCESS_TIMER0"                      : NODE_NUM = 613 ;
    "RTC_IRQ"                                 : INPUT_PIN = 5 ;
    "REFRESH_TIMER5"                          : NODE_NUM = 614 ;
    "DRAM_ACCESS_TIMER2"                      : NODE_NUM = 615 ;
    "UARTB_IRQ"                               : INPUT_PIN = 4 ;
    "REFRESH_TIMER6"                          : NODE_NUM = 616 ;
    "CPU_FC1"                                 : INPUT_PIN = 22 ;
    "DRAM_MEM_CAS"                            : NODE_NUM = 617 ;
    "UARTB_IRQ_MASK"                          : NODE_NUM = 618 ;
    "CPU_FC0"                                 : INPUT_PIN = 21 ;
    "UARTA_IRQ_MASK"                          : NODE_NUM = 619 ;
    "POWER_IRQ_MASK"                          : NODE_NUM = 620 ;
    "CPU_HALT"                                : OUTPUT_PIN = 20 ;
    "REFRESH_TIMER2"                          : NODE_NUM = 622 ;
    "IRQL6A_MASK"                             : NODE_NUM = 623 ;
    "CLK_32M"                                 : INPUT_PIN = 18 ;
    "IRQL6B_MASK"                             : NODE_NUM = 624 ;
    "CPU_RESET_IN"                            : INPUT_PIN = 17 ;
    "IRQL4A_MASK"                             : NODE_NUM = 625 ;
    "IDE_IRQ_MASK"                            : NODE_NUM = 626 ;
    "CPU_VPA"                                 : OUTPUT_PIN = 16 ;
    "EXPB_IRQ_MASK"                           : NODE_NUM = 628 ;
    "DRAM_REFRESH_RAS"                        : NODE_NUM = 629 ;
    "REFRESH_TIMER1"                          : NODE_NUM = 630 ;
    "DRAM_REFRESH_CAS_PRE"                    : NODE_NUM = 631 ;
    "TDI"                                     : INPUT_PIN = 14 ;
    "POWERCTL_OE"                             : NODE_NUM = 632 ;
    "CPU_RW"                                  : INPUT_PIN = 31 ;
    "CLK_16M"                                 : OUTPUT_PIN = 30 ;
    "CPU_RESET"                               : OUTPUT_PIN = 29 ;
    "IPL2"                                    : OUTPUT_PIN = 28 ;
    "IPL1"                                    : OUTPUT_PIN = 27 ;
    "IPL0"                                    : OUTPUT_PIN = 25 ;
    "CPU_FC2"                                 : INPUT_PIN = 24 ;
    "AS_GATED"                                : NODE_NUM = 646 ;
    "REFRESH_STATE_TIMER0"                    : NODE_NUM = 647 ;
    "TMS"                                     : INPUT_PIN = 23 ;
    "REFRESH_STATE_TIMER1"                    : NODE_NUM = 648 ;
    "CPU_D4"                                  : OUTPUT_PIN = 41 ;
    "CLK_8M"                                  : NODE_NUM = 650 ;
    "CPU_D3"                                  : OUTPUT_PIN = 40 ;
    "REFRESH_TIMER_RESET"                     : NODE_NUM = 652 ;
    "CPU_D2"                                  : OUTPUT_PIN = 39 ;
    "REFRESH_COMPLETED"                       : NODE_NUM = 654 ;
    "REFRESH_REQUESTED_PRE"                   : NODE_NUM = 655 ;
    "CPU_D1"                                  : OUTPUT_PIN = 37 ;
    "CPU_D0"                                  : OUTPUT_PIN = 36 ;
    "REFRESH_REQUESTED_SYNC"                  : NODE_NUM = 658 ;
    "CPU_AS"                                  : INPUT_PIN = 35 ;
    "REFRESH_TIMER3"                          : NODE_NUM = 659 ;
    "REFRESH_TIMER0"                          : NODE_NUM = 660 ;
    "CPU_LDS"                                 : INPUT_PIN = 34 ;
    "DRAM_REFRESH_CAS"                        : NODE_NUM = 661 ;
    "DTACK_TIMER_1"                           : NODE_NUM = 662 ;
    "DTACK_TIMER_0"                           : NODE_NUM = 663 ;
    "CPU_UDS"                                 : INPUT_PIN = 33 ;
    "REFRESH_STATE_TIMER2"                    : NODE_NUM = 664 ;
    "CPU_D5"                                  : OUTPUT_PIN = 44 ;
    "XXL_388"                                 : NODE_NUM = 666 ;
    "CPU_D6"                                  : OUTPUT_PIN = 45 ;
    "IRQL6B_AP"                               : NODE_NUM = 668 ;
    "CPU_D7"                                  : OUTPUT_PIN = 46 ;
    "XXL_386"                                 : NODE_NUM = 670 ;
    "XXL_407"                                 : NODE_NUM = 671 ;
    "IRQL6A_AP"                               : NODE_NUM = 672 ;
    "DEBUG_INT_AP"                            : NODE_NUM = 673 ;
    "XXL_405"                                 : NODE_NUM = 674 ;
    "POWERBUTTON_IRQ_AP"                      : NODE_NUM = 675 ;
    "XXL_389"                                 : NODE_NUM = 676 ;
    "IRQL4A_AP"                               : NODE_NUM = 677 ;
    "XXL_402"                                 : NODE_NUM = 678 ;
    "XXL_387"                                 : NODE_NUM = 679 ;
    "DTACK_FROM_INT"                          : OUTPUT_PIN = 52 ;
    "XXL_401"                                 : NODE_NUM = 681 ;
    "XXL_397"                                 : NODE_NUM = 682 ;
    "DRAM_CS"                                 : INPUT_PIN = 54 ;
    "XXL_394"                                 : NODE_NUM = 683 ;
    "XXL_410"                                 : NODE_NUM = 684 ;
    "INTC_CS"                                 : INPUT_PIN = 55 ;
    "XXL_392"                                 : NODE_NUM = 685 ;
    "CPU_INTACK2"                             : INPUT_PIN = 56 ;
    "XXL_406"                                 : NODE_NUM = 686 ;
    "XXL_393"                                 : NODE_NUM = 687 ;
    "CPU_INTACK1"                             : INPUT_PIN = 57 ;
    "XXL_404"                                 : NODE_NUM = 688 ;
    "XXL_403"                                 : NODE_NUM = 689 ;
    "XXL_408"                                 : NODE_NUM = 690 ;
    "CPU_INTACK0"                             : INPUT_PIN = 58 ;
    "XXL_398"                                 : NODE_NUM = 691 ;
    "XXL_390"                                 : NODE_NUM = 692 ;
    "KEYBOARDIRQ"                             : INPUT_PIN = 60 ;
    "XXL_395"                                 : NODE_NUM = 693 ;
    "XXL_409"                                 : NODE_NUM = 694 ;
    "XXL_391"                                 : NODE_NUM = 695 ;
    "TCK"                                     : INPUT_PIN = 62 ;
    "XXL_396"                                 : NODE_NUM = 696 ;
    "DRAM_RW"                                 : OUTPUT_PIN = 63 ;
    "DRAM_MEM_AB"                             : OUTPUT_PIN = 64 ;
    "DRAM_DATA_DIR"                           : OUTPUT_PIN = 65 ;
    "DEBUG_INT"                               : NODE_NUM = 703 ;
    "CAS0"                                    : OUTPUT_PIN = 67 ;
    "CAS1"                                    : OUTPUT_PIN = 68 ;
    "CLK_2M"                                  : NODE_NUM = 706 ;
    "CAS2"                                    : OUTPUT_PIN = 69 ;
    "RESET_REQ_OUT"                           : NODE_NUM = 708 ;
    "CAS3"                                    : OUTPUT_PIN = 70 ;
    "CLK_4M"                                  : NODE_NUM = 710 ;
    "XXL_412"                                 : NODE_NUM = 711 ;
    "TDO"                                     : INPUT_PIN = 71 ;
    "XXL_411"                                 : NODE_NUM = 712 ;
    "RAS2"                                    : OUTPUT_PIN = 73 ;
    "RAS0"                                    : OUTPUT_PIN = 74 ;
    "POWERCTL_STATE"                          : OUTPUT_PIN = 75 ;
    "POWERBUTTON_IRQ"                         : NODE_NUM = 719 ;
    "IRQL6B"                                  : NODE_NUM = 721 ;
    "IRQL4A"                                  : NODE_NUM = 722 ;
    "IRQL6A"                                  : NODE_NUM = 724 ;
    "XXL_399"                                 : NODE_NUM = 727 ;
    "UARTA_IRQ"                               : INPUT_PIN = 2 ;
    "DEBUG_BUTTON"                            : INPUT_PIN = 83 ;
    "PTC2_IRQ"                                : INPUT_PIN = 1 ;
    "PTC1_IRQ"                                : INPUT_PIN = 84 ;
END;
