EESchema Schematic File Version 4
LIBS:Chiller_control-cache
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Chiller_control-rescue:GND #PWR01
U 1 1 58C453C7
P 2150 1825
F 0 "#PWR01" H 2150 1575 50  0001 C CNN
F 1 "GND" H 2150 1675 50  0000 C CNN
F 2 "" H 2150 1825 50  0000 C CNN
F 3 "" H 2150 1825 50  0000 C CNN
	1    2150 1825
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:CP C4
U 1 1 58C454F6
P 2550 1625
F 0 "C4" H 2575 1725 50  0000 L CNN
F 1 "47u" H 2575 1525 50  0000 L CNN
F 2 "Capacitor_Tantalum_SMD.pretty:CP_Tantalum_Case-A_EIA-3216-18_Hand" H 2588 1475 50  0001 C CNN
F 3 "" H 2550 1625 50  0000 C CNN
	1    2550 1625
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR02
U 1 1 58C455CB
P 2750 1325
F 0 "#PWR02" H 2750 1175 50  0001 C CNN
F 1 "+3.3V" H 2750 1465 50  0000 C CNN
F 2 "" H 2750 1325 50  0000 C CNN
F 3 "" H 2750 1325 50  0000 C CNN
	1    2750 1325
	1    0    0    -1  
$EndComp
Text Notes 875  875  0    100  ~ 0
3.3V MCU power source\nwith short-circuit protection
Text Notes 750  2350 0    100  ~ 0
Bootloader init
Text Label 1050 3450 2    60   ~ 0
NRST
Text Label 1050 2500 0    60   ~ 0
BOOT0
$Comp
L Chiller_control-rescue:R R1
U 1 1 590D30C8
P 1050 2800
F 0 "R1" V 1130 2800 50  0000 C CNN
F 1 "10k" V 1050 2800 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 980 2800 50  0001 C CNN
F 3 "" H 1050 2800 50  0000 C CNN
	1    1050 2800
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C2
U 1 1 590D4150
P 1650 2800
F 0 "C2" H 1675 2900 50  0000 L CNN
F 1 "0.1" H 1675 2700 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 1688 2650 50  0001 C CNN
F 3 "" H 1650 2800 50  0000 C CNN
	1    1650 2800
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C1
U 1 1 590D4832
P 1600 3650
F 0 "C1" H 1625 3750 50  0000 L CNN
F 1 "0.1" H 1625 3550 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 1638 3500 50  0001 C CNN
F 3 "" H 1600 3650 50  0000 C CNN
	1    1600 3650
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:SW_Push SW2
U 1 1 5909F6B6
P 1350 2800
F 0 "SW2" H 1400 2900 50  0000 L CNN
F 1 "Boot" H 1350 2740 50  0000 C CNN
F 2 "Buttons_Switches_SMD:SW_SPST_FSMSM" H 1350 3000 50  0001 C CNN
F 3 "" H 1350 3000 50  0000 C CNN
	1    1350 2800
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:SW_Push SW1
U 1 1 590A0134
P 1300 3650
F 0 "SW1" H 1350 3750 50  0000 L CNN
F 1 "Reset" H 1300 3590 50  0000 C CNN
F 2 "Buttons_Switches_SMD:SW_SPST_FSMSM" H 1300 3850 50  0001 C CNN
F 3 "" H 1300 3850 50  0000 C CNN
	1    1300 3650
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR03
U 1 1 590A03AF
P 1050 3000
F 0 "#PWR03" H 1050 2750 50  0001 C CNN
F 1 "GND" H 1050 2850 50  0000 C CNN
F 2 "" H 1050 3000 50  0000 C CNN
F 3 "" H 1050 3000 50  0000 C CNN
	1    1050 3000
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR04
U 1 1 590A509B
P 1450 3900
F 0 "#PWR04" H 1450 3650 50  0001 C CNN
F 1 "GND" H 1450 3750 50  0000 C CNN
F 2 "" H 1450 3900 50  0000 C CNN
F 3 "" H 1450 3900 50  0000 C CNN
	1    1450 3900
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:Conn_01x02 J2
U 1 1 5A170C1F
P 875 1325
F 0 "J2" H 875 1525 50  0000 C CNN
F 1 "12VIN" H 875 1125 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 875 1325 50  0001 C CNN
F 3 "" H 875 1325 50  0001 C CNN
	1    875  1325
	-1   0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C3
U 1 1 5A178C32
P 1700 1475
F 0 "C3" H 1725 1575 50  0000 L CNN
F 1 "0.1" H 1725 1375 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 1738 1325 50  0001 C CNN
F 3 "" H 1700 1475 50  0000 C CNN
	1    1700 1475
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:STM32F030F4Px U3
U 1 1 5A189F52
P 8775 2275
F 0 "U3" H 7175 3200 50  0000 L BNN
F 1 "STM32F030F4Px" H 10375 3200 50  0000 R BNN
F 2 "Housings_SSOP:TSSOP-20_4.4x6.5mm_Pitch0.65mm" H 10375 3150 50  0001 R TNN
F 3 "" H 8775 2275 50  0001 C CNN
	1    8775 2275
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:PWR_FLAG #FLG05
U 1 1 5A17FC22
P 1100 1325
F 0 "#FLG05" H 1100 1420 50  0001 C CNN
F 1 "PWR_FLAG" H 1100 1505 50  0001 C CNN
F 2 "" H 1100 1325 50  0000 C CNN
F 3 "" H 1100 1325 50  0000 C CNN
	1    1100 1325
	-1   0    0    1   
$EndComp
$Comp
L Chiller_control-rescue:+12V #PWR06
U 1 1 5A17FD59
P 1100 1325
F 0 "#PWR06" H 1100 1175 50  0001 C CNN
F 1 "+12V" H 1100 1465 50  0000 C CNN
F 2 "" H 1100 1325 50  0001 C CNN
F 3 "" H 1100 1325 50  0001 C CNN
	1    1100 1325
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C6
U 1 1 5A1AB970
P 8075 1025
F 0 "C6" H 8100 1125 50  0000 L CNN
F 1 "1u" H 8100 925 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 8113 875 50  0001 C CNN
F 3 "" H 8075 1025 50  0000 C CNN
	1    8075 1025
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR07
U 1 1 5A1B3C28
P 8075 1175
F 0 "#PWR07" H 8075 925 50  0001 C CNN
F 1 "GND" H 8075 1025 50  0000 C CNN
F 2 "" H 8075 1175 50  0000 C CNN
F 3 "" H 8075 1175 50  0000 C CNN
	1    8075 1175
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR08
U 1 1 5A1B4A11
P 8775 3225
F 0 "#PWR08" H 8775 2975 50  0001 C CNN
F 1 "GND" H 8775 3075 50  0000 C CNN
F 2 "" H 8775 3225 50  0000 C CNN
F 3 "" H 8775 3225 50  0000 C CNN
	1    8775 3225
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR09
U 1 1 5A1B5A75
P 8075 825
F 0 "#PWR09" H 8075 675 50  0001 C CNN
F 1 "+3.3V" H 8075 965 50  0000 C CNN
F 2 "" H 8075 825 50  0000 C CNN
F 3 "" H 8075 825 50  0000 C CNN
	1    8075 825 
	1    0    0    -1  
$EndComp
Text Notes 725  1925 0    79   ~ 0
+12\nGND
$Comp
L Chiller_control-rescue:+3.3V #PWR010
U 1 1 590A1E6C
P 1350 3050
F 0 "#PWR010" H 1350 2900 50  0001 C CNN
F 1 "+3.3V" H 1350 3190 50  0000 C CNN
F 2 "" H 1350 3050 50  0000 C CNN
F 3 "" H 1350 3050 50  0000 C CNN
	1    1350 3050
	-1   0    0    1   
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR011
U 1 1 5A283BCF
P 1650 3000
F 0 "#PWR011" H 1650 2750 50  0001 C CNN
F 1 "GND" H 1650 2850 50  0000 C CNN
F 2 "" H 1650 3000 50  0000 C CNN
F 3 "" H 1650 3000 50  0000 C CNN
	1    1650 3000
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:LM1117-3.3 U1
U 1 1 5A2588E7
P 2150 1325
F 0 "U1" H 2000 1450 50  0000 C CNN
F 1 "LM1117-3.3" H 2150 1450 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-223" H 2150 1325 50  0001 C CNN
F 3 "" H 2150 1325 50  0001 C CNN
	1    2150 1325
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C12
U 1 1 5A386BD0
P 4000 2250
F 0 "C12" H 4025 2350 50  0000 L CNN
F 1 "0.1" H 4025 2150 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 4038 2100 50  0001 C CNN
F 3 "" H 4000 2250 50  0000 C CNN
	1    4000 2250
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C5
U 1 1 5BEE1D09
P 2800 1575
F 0 "C5" H 2825 1675 50  0000 L CNN
F 1 "0.1" H 2825 1475 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 2838 1425 50  0001 C CNN
F 3 "" H 2800 1575 50  0000 C CNN
	1    2800 1575
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:D D1
U 1 1 5BEE236F
P 1275 1325
F 0 "D1" H 1275 1425 50  0000 C CNN
F 1 "SS14" H 1275 1225 50  0000 C CNN
F 2 "Diode_SMD.pretty:D_SMA_Handsoldering" H 1275 1325 50  0001 C CNN
F 3 "" H 1275 1325 50  0001 C CNN
	1    1275 1325
	-1   0    0    1   
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR012
U 1 1 5BEE2561
P 1075 1500
F 0 "#PWR012" H 1075 1250 50  0001 C CNN
F 1 "GND" H 1075 1350 50  0000 C CNN
F 2 "" H 1075 1500 50  0000 C CNN
F 3 "" H 1075 1500 50  0000 C CNN
	1    1075 1500
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:PWR_FLAG #FLG013
U 1 1 5BEE25D3
P 975 1500
F 0 "#FLG013" H 975 1595 50  0001 C CNN
F 1 "PWR_FLAG" H 975 1680 50  0001 C CNN
F 2 "" H 975 1500 50  0000 C CNN
F 3 "" H 975 1500 50  0000 C CNN
	1    975  1500
	-1   0    0    1   
$EndComp
Text Label 7075 1875 2    60   ~ 0
BOOT0
Text Label 7075 1675 2    60   ~ 0
NRST
Text Label 10475 2475 0    60   ~ 0
USART_Tx
Text Label 10475 2575 0    60   ~ 0
USART_Rx
$Comp
L Chiller_control-rescue:L L1
U 1 1 5BEE7949
P 8775 1025
F 0 "L1" V 8725 1025 50  0000 C CNN
F 1 "BMBA 0.1mH" V 8850 1025 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" H 8775 1025 50  0001 C CNN
F 3 "" H 8775 1025 50  0001 C CNN
	1    8775 1025
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C7
U 1 1 5BEE8065
P 9450 1125
F 0 "C7" H 9475 1225 50  0000 L CNN
F 1 "10u" H 9475 1025 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_1206_HandSoldering" H 9488 975 50  0001 C CNN
F 3 "" H 9450 1125 50  0001 C CNN
	1    9450 1125
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR014
U 1 1 5BEE83C2
P 9675 975
F 0 "#PWR014" H 9675 725 50  0001 C CNN
F 1 "GND" H 9675 825 50  0000 C CNN
F 2 "" H 9675 975 50  0000 C CNN
F 3 "" H 9675 975 50  0000 C CNN
	1    9675 975 
	1    0    0    -1  
$EndComp
Text Label 10475 1675 0    60   ~ 0
ADC0
Text Label 10475 1775 0    60   ~ 0
ADC1
Text Label 10475 1875 0    60   ~ 0
ADC2
Text Label 10475 1975 0    60   ~ 0
ADC3
Text Label 10475 2075 0    60   ~ 0
Tim14Ch1
Text Label 10475 2275 0    60   ~ 0
TIM16_Ch1
Text Label 10475 2375 0    60   ~ 0
TIM17_Ch1
Text Label 7075 2775 2    60   ~ 0
TIM3_Ch4
Text Notes 11125 2075 2    60   ~ 0
PWM
Text Notes 11150 2275 2    60   ~ 0
PWM
Text Notes 11150 2375 2    60   ~ 0
PWM
Text Notes 6625 2775 2    60   ~ 0
Flow sensor
Text Label 7075 2475 2    60   ~ 0
DigIn0
Text Label 7075 2575 2    60   ~ 0
DigOut0
Text Label 10475 2175 0    60   ~ 0
DigIn1
Text Label 10475 2675 0    60   ~ 0
DigOut1
Text Label 10475 2775 0    60   ~ 0
DigOut2
Text Notes 10775 1825 0    60   ~ 0
Thermal
Text Notes 10800 2175 0    60   ~ 0
TLE5205 err
Text Notes 6750 2475 2    60   ~ 0
Water level
Text Notes 6700 2575 2    60   ~ 0
Ext. Alarm
Text Notes 10825 2700 0    60   ~ 0
TLE5205 In
$Comp
L Chiller_control-rescue:R R8
U 1 1 5BEEA391
P 2825 2775
F 0 "R8" V 2905 2775 50  0000 C CNN
F 1 "10k" V 2825 2775 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 2755 2775 50  0001 C CNN
F 3 "" H 2825 2775 50  0001 C CNN
	1    2825 2775
	1    0    0    -1  
$EndComp
Text Label 2600 2975 2    60   ~ 0
DigOut1
Text Label 2600 3100 2    60   ~ 0
DigOut2
$Comp
L Chiller_control-rescue:R R6
U 1 1 5BEEA7BE
P 2625 2775
F 0 "R6" V 2705 2775 50  0000 C CNN
F 1 "10k" V 2625 2775 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 2555 2775 50  0001 C CNN
F 3 "" H 2625 2775 50  0001 C CNN
	1    2625 2775
	1    0    0    -1  
$EndComp
Text Notes 10825 2775 0    60   ~ 0
(opendrain)
Text Label 4000 975  2    60   ~ 0
DigOut0
$Comp
L Chiller_control-rescue:R R12
U 1 1 5BEEB2E1
P 4450 1175
F 0 "R12" V 4530 1175 50  0000 C CNN
F 1 "10k" V 4450 1175 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 4380 1175 50  0001 C CNN
F 3 "" H 4450 1175 50  0001 C CNN
	1    4450 1175
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R10
U 1 1 5BEEB37B
P 4150 975
F 0 "R10" V 4230 975 50  0000 C CNN
F 1 "510" V 4150 975 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 4080 975 50  0001 C CNN
F 3 "" H 4150 975 50  0001 C CNN
	1    4150 975 
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:Q_NMOS_GSD Q1
U 1 1 5BEEB585
P 4500 975
F 0 "Q1" H 4700 1025 50  0000 L CNN
F 1 "SI2300" H 4700 925 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 4700 1075 50  0001 C CNN
F 3 "" H 4500 975 50  0001 C CNN
	1    4500 975 
	1    0    0    -1  
$EndComp
Text Label 2575 3650 2    60   ~ 0
DigIn1
$Comp
L Chiller_control-rescue:R R7
U 1 1 5BEDA6BA
P 2725 3650
F 0 "R7" V 2805 3650 50  0000 C CNN
F 1 "47" V 2725 3650 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 2655 3650 50  0001 C CNN
F 3 "" H 2725 3650 50  0001 C CNN
	1    2725 3650
	0    1    1    0   
$EndComp
Text Label 3900 2100 2    60   ~ 0
DigIn0
$Comp
L Chiller_control-rescue:R R11
U 1 1 5BEDB615
P 4325 2100
F 0 "R11" V 4405 2100 50  0000 C CNN
F 1 "47" V 4325 2100 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 4255 2100 50  0001 C CNN
F 3 "" H 4325 2100 50  0001 C CNN
	1    4325 2100
	0    1    1    0   
$EndComp
Text Notes 4250 1725 2    60   ~ 0
Water level
Text Notes 2525 2400 0    60   ~ 0
TLE5205\nFor Peltier
Text Notes 4425 700  2    60   ~ 0
Ext. Alarm
$Comp
L Chiller_control-rescue:Q_NMOS_GSD Q2
U 1 1 5BEDCAD0
P 5625 1375
F 0 "Q2" H 5825 1425 50  0000 L CNN
F 1 "SI2300" H 5825 1325 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 5825 1475 50  0001 C CNN
F 3 "" H 5625 1375 50  0001 C CNN
	1    5625 1375
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R14
U 1 1 5BEDD4AE
P 5575 1600
F 0 "R14" V 5655 1600 50  0000 C CNN
F 1 "10k" V 5575 1600 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 5505 1600 50  0001 C CNN
F 3 "" H 5575 1600 50  0001 C CNN
	1    5575 1600
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R15
U 1 1 5BEDD8A0
P 5725 1025
F 0 "R15" V 5805 1025 50  0000 C CNN
F 1 "10k" V 5725 1025 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 5655 1025 50  0001 C CNN
F 3 "" H 5725 1025 50  0001 C CNN
	1    5725 1025
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR015
U 1 1 5BEDE0EC
P 5425 1325
F 0 "#PWR015" H 5425 1175 50  0001 C CNN
F 1 "+3.3V" H 5425 1465 50  0000 C CNN
F 2 "" H 5425 1325 50  0000 C CNN
F 3 "" H 5425 1325 50  0000 C CNN
	1    5425 1325
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR016
U 1 1 5BEE048F
P 5950 975
F 0 "#PWR016" H 5950 725 50  0001 C CNN
F 1 "GND" H 5950 825 50  0000 C CNN
F 2 "" H 5950 975 50  0000 C CNN
F 3 "" H 5950 975 50  0000 C CNN
	1    5950 975 
	1    0    0    -1  
$EndComp
Text Notes 5300 800  0    60   ~ 0
YF-S201C
$Comp
L Chiller_control-rescue:+12V #PWR017
U 1 1 5BEE1671
P 5925 875
F 0 "#PWR017" H 5925 725 50  0001 C CNN
F 1 "+12V" H 5925 1015 50  0000 C CNN
F 2 "" H 5925 875 50  0001 C CNN
F 3 "" H 5925 875 50  0001 C CNN
	1    5925 875 
	1    0    0    -1  
$EndComp
Text Label 5825 1600 0    60   ~ 0
TIM3_Ch4
$Comp
L Chiller_control-rescue:GND #PWR018
U 1 1 5BEE2D93
P 4000 2450
F 0 "#PWR018" H 4000 2200 50  0001 C CNN
F 1 "GND" H 4000 2300 50  0000 C CNN
F 2 "" H 4000 2450 50  0000 C CNN
F 3 "" H 4000 2450 50  0000 C CNN
	1    4000 2450
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R13
U 1 1 5BEE33E1
P 4475 1900
F 0 "R13" V 4555 1900 50  0000 C CNN
F 1 "10k" V 4475 1900 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 4405 1900 50  0001 C CNN
F 3 "" H 4475 1900 50  0001 C CNN
	1    4475 1900
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR019
U 1 1 5BEE4159
P 4475 1725
F 0 "#PWR019" H 4475 1575 50  0001 C CNN
F 1 "+3.3V" H 4475 1865 50  0000 C CNN
F 2 "" H 4475 1725 50  0000 C CNN
F 3 "" H 4475 1725 50  0000 C CNN
	1    4475 1725
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R9
U 1 1 5BEE5083
P 2925 3500
F 0 "R9" V 3005 3500 50  0000 C CNN
F 1 "10k" V 2925 3500 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 2855 3500 50  0001 C CNN
F 3 "" H 2925 3500 50  0001 C CNN
	1    2925 3500
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR020
U 1 1 5BEE53EF
P 2725 2625
F 0 "#PWR020" H 2725 2475 50  0001 C CNN
F 1 "+3.3V" H 2725 2765 50  0000 C CNN
F 2 "" H 2725 2625 50  0000 C CNN
F 3 "" H 2725 2625 50  0000 C CNN
	1    2725 2625
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR021
U 1 1 5BEE7095
P 2925 3350
F 0 "#PWR021" H 2925 3200 50  0001 C CNN
F 1 "+3.3V" H 2925 3490 50  0000 C CNN
F 2 "" H 2925 3350 50  0000 C CNN
F 3 "" H 2925 3350 50  0000 C CNN
	1    2925 3350
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR022
U 1 1 5BEE8828
P 4600 1200
F 0 "#PWR022" H 4600 950 50  0001 C CNN
F 1 "GND" H 4600 1050 50  0000 C CNN
F 2 "" H 4600 1200 50  0000 C CNN
F 3 "" H 4600 1200 50  0000 C CNN
	1    4600 1200
	1    0    0    -1  
$EndComp
Text Notes 10025 3750 2    60   ~ 0
PWM
$Comp
L Chiller_control-rescue:R R19
U 1 1 5BEEBD18
P 10075 4475
F 0 "R19" V 10155 4475 50  0000 C CNN
F 1 "10k" V 10075 4475 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 10005 4475 50  0001 C CNN
F 3 "" H 10075 4475 50  0001 C CNN
	1    10075 4475
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R16
U 1 1 5BEEBD1E
P 9775 4275
F 0 "R16" V 9855 4275 50  0000 C CNN
F 1 "510" V 9775 4275 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 9705 4275 50  0001 C CNN
F 3 "" H 9775 4275 50  0001 C CNN
	1    9775 4275
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:Q_NMOS_GSD Q3
U 1 1 5BEEBD24
P 10125 4275
F 0 "Q3" H 10325 4325 50  0000 L CNN
F 1 "SI2300" H 10325 4225 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 10325 4375 50  0001 C CNN
F 3 "" H 10125 4275 50  0001 C CNN
	1    10125 4275
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR023
U 1 1 5BEEBD2B
P 10225 4500
F 0 "#PWR023" H 10225 4250 50  0001 C CNN
F 1 "GND" H 10225 4350 50  0000 C CNN
F 2 "" H 10225 4500 50  0000 C CNN
F 3 "" H 10225 4500 50  0000 C CNN
	1    10225 4500
	1    0    0    -1  
$EndComp
Text Label 9625 4275 2    60   ~ 0
Tim14Ch1
Text Notes 10250 4025 0    60   ~ 0
Cooler
$Comp
L Chiller_control-rescue:R R20
U 1 1 5BEEC934
P 10125 5350
F 0 "R20" V 10205 5350 50  0000 C CNN
F 1 "10k" V 10125 5350 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 10055 5350 50  0001 C CNN
F 3 "" H 10125 5350 50  0001 C CNN
	1    10125 5350
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R17
U 1 1 5BEEC93A
P 9825 5150
F 0 "R17" V 9905 5150 50  0000 C CNN
F 1 "510" V 9825 5150 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 9755 5150 50  0001 C CNN
F 3 "" H 9825 5150 50  0001 C CNN
	1    9825 5150
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:Q_NMOS_GDS Q4
U 1 1 5BEEC940
P 10175 5150
F 0 "Q4" H 10375 5200 50  0000 L CNN
F 1 "IRL3303" H 10375 5100 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-220_Horizontal" H 10375 5250 50  0001 C CNN
F 3 "" H 10175 5150 50  0001 C CNN
	1    10175 5150
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR024
U 1 1 5BEEC947
P 10275 5375
F 0 "#PWR024" H 10275 5125 50  0001 C CNN
F 1 "GND" H 10275 5225 50  0000 C CNN
F 2 "" H 10275 5375 50  0000 C CNN
F 3 "" H 10275 5375 50  0000 C CNN
	1    10275 5375
	1    0    0    -1  
$EndComp
Text Notes 10300 4900 0    60   ~ 0
Heater
$Comp
L Chiller_control-rescue:R R21
U 1 1 5BEED557
P 10200 6100
F 0 "R21" V 10280 6100 50  0000 C CNN
F 1 "10k" V 10200 6100 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 10130 6100 50  0001 C CNN
F 3 "" H 10200 6100 50  0001 C CNN
	1    10200 6100
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R18
U 1 1 5BEED55D
P 9900 5900
F 0 "R18" V 9980 5900 50  0000 C CNN
F 1 "510" V 9900 5900 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 9830 5900 50  0001 C CNN
F 3 "" H 9900 5900 50  0001 C CNN
	1    9900 5900
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:Q_NMOS_GDS Q5
U 1 1 5BEED563
P 10250 5900
F 0 "Q5" H 10450 5950 50  0000 L CNN
F 1 "IRL3303" H 10450 5850 50  0000 L CNN
F 2 "TO_SOT_Packages_THT:TO-220_Horizontal" H 10450 6000 50  0001 C CNN
F 3 "" H 10250 5900 50  0001 C CNN
	1    10250 5900
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR025
U 1 1 5BEED56A
P 10350 6125
F 0 "#PWR025" H 10350 5875 50  0001 C CNN
F 1 "GND" H 10350 5975 50  0000 C CNN
F 2 "" H 10350 6125 50  0000 C CNN
F 3 "" H 10350 6125 50  0000 C CNN
	1    10350 6125
	1    0    0    -1  
$EndComp
Text Notes 10375 5650 0    60   ~ 0
Pump
Text Label 9675 5150 2    60   ~ 0
TIM16_Ch1
Text Label 9750 5900 2    60   ~ 0
TIM17_Ch1
Text Notes 1200 4475 0    60   ~ 0
Thermal
$Comp
L Chiller_control-rescue:R R2
U 1 1 5BEF45BA
P 1075 4700
F 0 "R2" V 1155 4700 50  0000 C CNN
F 1 "1k0.1%" V 1075 4700 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 1005 4700 50  0001 C CNN
F 3 "" H 1075 4700 50  0001 C CNN
	1    1075 4700
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:+3.3VADC #PWR026
U 1 1 5BEF64D3
P 9100 1275
F 0 "#PWR026" H 9250 1225 50  0001 C CNN
F 1 "+3.3VADC" H 9100 1375 50  0000 C CNN
F 2 "" H 9100 1275 50  0001 C CNN
F 3 "" H 9100 1275 50  0001 C CNN
	1    9100 1275
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3VADC #PWR027
U 1 1 5BEF6744
P 2125 4750
F 0 "#PWR027" H 2275 4700 50  0001 C CNN
F 1 "+3.3VADC" H 2125 4850 50  0000 C CNN
F 2 "" H 2125 4750 50  0001 C CNN
F 3 "" H 2125 4750 50  0001 C CNN
	1    2125 4750
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R3
U 1 1 5BEF7CBC
P 1075 4925
F 0 "R3" V 1155 4925 50  0000 C CNN
F 1 "1k0.1%" V 1075 4925 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 1005 4925 50  0001 C CNN
F 3 "" H 1075 4925 50  0001 C CNN
	1    1075 4925
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R4
U 1 1 5BEF8256
P 1075 5150
F 0 "R4" V 1155 5150 50  0000 C CNN
F 1 "1k0.1%" V 1075 5150 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 1005 5150 50  0001 C CNN
F 3 "" H 1075 5150 50  0001 C CNN
	1    1075 5150
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR028
U 1 1 5BEF8833
P 725 4975
F 0 "#PWR028" H 725 4725 50  0001 C CNN
F 1 "GND" H 725 4825 50  0000 C CNN
F 2 "" H 725 4975 50  0000 C CNN
F 3 "" H 725 4975 50  0000 C CNN
	1    725  4975
	1    0    0    -1  
$EndComp
Text Label 1250 4700 0    60   ~ 0
ADC0
Text Label 1250 4925 0    60   ~ 0
ADC1
Text Label 1250 5150 0    60   ~ 0
ADC2
Text Label 1250 5375 0    60   ~ 0
ADC3
$Comp
L Chiller_control-rescue:R R5
U 1 1 5BEFAAF1
P 1075 5375
F 0 "R5" V 1155 5375 50  0000 C CNN
F 1 "1k0.1%" V 1075 5375 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 1005 5375 50  0001 C CNN
F 3 "" H 1075 5375 50  0001 C CNN
	1    1075 5375
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:MAX3232 U2
U 1 1 5BEFC197
P 4800 4675
F 0 "U2" H 4700 5800 50  0000 R CNN
F 1 "MAX3232" H 4700 5725 50  0000 R CNN
F 2 "Housings_SOIC:SOIC-16_3.9x9.9mm_Pitch1.27mm" H 4850 3625 50  0001 L CNN
F 3 "" H 4800 4775 50  0001 C CNN
	1    4800 4675
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C11
U 1 1 5BEFD47C
P 5800 4275
F 0 "C11" H 5825 4375 50  0000 L CNN
F 1 "0.47" H 5825 4175 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 5838 4125 50  0001 C CNN
F 3 "" H 5800 4275 50  0001 C CNN
	1    5800 4275
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:C C10
U 1 1 5BEFD7DB
P 5650 3925
F 0 "C10" H 5675 4025 50  0000 L CNN
F 1 "0.47" H 5675 3825 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 5688 3775 50  0001 C CNN
F 3 "" H 5650 3925 50  0001 C CNN
	1    5650 3925
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C13
U 1 1 5BEFD89D
P 5800 4575
F 0 "C13" H 5825 4675 50  0000 L CNN
F 1 "0.47" H 5825 4475 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 5838 4425 50  0001 C CNN
F 3 "" H 5800 4575 50  0001 C CNN
	1    5800 4575
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:C C9
U 1 1 5BEFF11C
P 4950 3375
F 0 "C9" H 4975 3475 50  0000 L CNN
F 1 "0.1" H 4975 3275 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 4988 3225 50  0001 C CNN
F 3 "" H 4950 3375 50  0000 C CNN
	1    4950 3375
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR029
U 1 1 5BF006EE
P 4800 3275
F 0 "#PWR029" H 4800 3125 50  0001 C CNN
F 1 "+3.3V" H 4800 3415 50  0000 C CNN
F 2 "" H 4800 3275 50  0000 C CNN
F 3 "" H 4800 3275 50  0000 C CNN
	1    4800 3275
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR030
U 1 1 5BF0128D
P 5125 3450
F 0 "#PWR030" H 5125 3200 50  0001 C CNN
F 1 "GND" H 5125 3300 50  0000 C CNN
F 2 "" H 5125 3450 50  0000 C CNN
F 3 "" H 5125 3450 50  0000 C CNN
	1    5125 3450
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR031
U 1 1 5BF022E5
P 6050 4400
F 0 "#PWR031" H 6050 4150 50  0001 C CNN
F 1 "GND" H 6050 4250 50  0000 C CNN
F 2 "" H 6050 4400 50  0000 C CNN
F 3 "" H 6050 4400 50  0000 C CNN
	1    6050 4400
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C8
U 1 1 5BF0257F
P 3925 3925
F 0 "C8" H 3950 4025 50  0000 L CNN
F 1 "0.1" H 3950 3825 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0603_HandSoldering" H 3963 3775 50  0001 C CNN
F 3 "" H 3925 3925 50  0000 C CNN
	1    3925 3925
	-1   0    0    1   
$EndComp
$Comp
L Chiller_control-rescue:DB9_Female J1
U 1 1 5BF02D65
P 6450 5075
F 0 "J1" H 6450 5625 50  0000 C CNN
F 1 "DB9_Female" H 6450 4500 50  0000 C CNN
F 2 "modules:DB9-F" H 6450 5075 50  0001 C CNN
F 3 "" H 6450 5075 50  0001 C CNN
	1    6450 5075
	1    0    0    -1  
$EndComp
Text Notes 5600 6050 0    60   ~ 0
2 - RS232-Rx\n3 - RS232-Tx\n5 - GND
Text Label 4000 4975 2    60   ~ 0
USART_Tx
Text Label 4000 5375 2    60   ~ 0
USART_Rx
$Comp
L Chiller_control-rescue:GND #PWR032
U 1 1 5BF0792A
P 4800 5925
F 0 "#PWR032" H 4800 5675 50  0001 C CNN
F 1 "GND" H 4800 5775 50  0000 C CNN
F 2 "" H 4800 5925 50  0000 C CNN
F 3 "" H 4800 5925 50  0000 C CNN
	1    4800 5925
	1    0    0    -1  
$EndComp
NoConn ~ 4000 5175
NoConn ~ 4000 4775
NoConn ~ 6150 4675
NoConn ~ 6150 4775
NoConn ~ 6150 4975
NoConn ~ 6150 5175
NoConn ~ 6150 5275
NoConn ~ 6150 5375
$Comp
L Chiller_control-rescue:PWR_FLAG #FLG033
U 1 1 5BF0C65D
P 9325 1275
F 0 "#FLG033" H 9325 1370 50  0001 C CNN
F 1 "PWR_FLAG" H 9325 1455 50  0001 C CNN
F 2 "" H 9325 1275 50  0000 C CNN
F 3 "" H 9325 1275 50  0000 C CNN
	1    9325 1275
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR034
U 1 1 5BF1992D
P 6125 5500
F 0 "#PWR034" H 6125 5250 50  0001 C CNN
F 1 "GND" H 6125 5350 50  0000 C CNN
F 2 "" H 6125 5500 50  0000 C CNN
F 3 "" H 6125 5500 50  0000 C CNN
	1    6125 5500
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:Conn_01x02 J5
U 1 1 5BF1B9CF
P 10850 3975
F 0 "J5" H 10850 4175 50  0000 C CNN
F 1 "Cooler" H 10850 3775 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 10850 3975 50  0001 C CNN
F 3 "" H 10850 3975 50  0001 C CNN
	1    10850 3975
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:Conn_01x02 J6
U 1 1 5BF1EE9B
P 10875 4850
F 0 "J6" H 10875 5050 50  0000 C CNN
F 1 "Heater" H 10875 4650 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 10875 4850 50  0001 C CNN
F 3 "" H 10875 4850 50  0001 C CNN
	1    10875 4850
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:Conn_01x02 J7
U 1 1 5BF1F6CF
P 10925 5600
F 0 "J7" H 10925 5800 50  0000 C CNN
F 1 "Pump" H 10925 5400 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 10925 5600 50  0001 C CNN
F 3 "" H 10925 5600 50  0001 C CNN
	1    10925 5600
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+12V #PWR035
U 1 1 5BF20A89
P 10650 3925
F 0 "#PWR035" H 10650 3775 50  0001 C CNN
F 1 "+12V" H 10650 4065 50  0000 C CNN
F 2 "" H 10650 3925 50  0001 C CNN
F 3 "" H 10650 3925 50  0001 C CNN
	1    10650 3925
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+12V #PWR036
U 1 1 5BF21CB9
P 10675 4800
F 0 "#PWR036" H 10675 4650 50  0001 C CNN
F 1 "+12V" H 10675 4940 50  0000 C CNN
F 2 "" H 10675 4800 50  0001 C CNN
F 3 "" H 10675 4800 50  0001 C CNN
	1    10675 4800
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+12V #PWR037
U 1 1 5BF22482
P 10725 5525
F 0 "#PWR037" H 10725 5375 50  0001 C CNN
F 1 "+12V" H 10725 5665 50  0000 C CNN
F 2 "" H 10725 5525 50  0001 C CNN
F 3 "" H 10725 5525 50  0001 C CNN
	1    10725 5525
	1    0    0    -1  
$EndComp
Text Notes 5775 675  2    60   ~ 0
Flow sensor
$Comp
L Chiller_control-rescue:Conn_02x04_Odd_Even J4
U 1 1 5BF2FEAE
P 1825 4950
F 0 "J4" H 1875 5150 50  0000 C CNN
F 1 "Tsens" H 1875 4650 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x04_P2.54mm_Vertical" H 1825 4950 50  0001 C CNN
F 3 "" H 1825 4950 50  0001 C CNN
	1    1825 4950
	1    0    0    -1  
$EndComp
Text Notes 3900 3250 0    60   ~ 0
RS-232
$Comp
L Chiller_control-rescue:Conn_02x07_Odd_Even J3
U 1 1 5BF41ABB
P 1275 7050
F 0 "J3" H 1325 7450 50  0000 C CNN
F 1 "Ext. devices" H 1325 6650 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x07_P2.54mm_Vertical" H 1275 7050 50  0001 C CNN
F 3 "" H 1275 7050 50  0001 C CNN
	1    1275 7050
	1    0    0    -1  
$EndComp
NoConn ~ 1075 6750
NoConn ~ 1075 7050
NoConn ~ 1075 7250
Text Label 4600 2100 0    60   ~ 0
W.levl
Text Label 1575 7150 0    60   ~ 0
W.levl
$Comp
L Chiller_control-rescue:GND #PWR038
U 1 1 5BF4C7AF
P 725 7150
F 0 "#PWR038" H 725 6900 50  0001 C CNN
F 1 "GND" H 725 7000 50  0000 C CNN
F 2 "" H 725 7150 50  0000 C CNN
F 3 "" H 725 7150 50  0000 C CNN
	1    725  7150
	1    0    0    -1  
$EndComp
Text Notes 1875 7175 0    60   ~ 0
Water level
Text Notes 2425 7375 2    60   ~ 0
Flow sensor
Text Notes 1900 7000 0    60   ~ 0
TLE5205
Text Notes 2350 6800 2    60   ~ 0
Ext. Alarm
$Comp
L Chiller_control-rescue:+3.3V #PWR039
U 1 1 5BF508A3
P 1750 6850
F 0 "#PWR039" H 1750 6700 50  0001 C CNN
F 1 "+3.3V" H 1750 6990 50  0000 C CNN
F 2 "" H 1750 6850 50  0000 C CNN
F 3 "" H 1750 6850 50  0000 C CNN
	1    1750 6850
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+12V #PWR040
U 1 1 5BF51AF0
P 1575 6675
F 0 "#PWR040" H 1575 6525 50  0001 C CNN
F 1 "+12V" H 1575 6815 50  0000 C CNN
F 2 "" H 1575 6675 50  0001 C CNN
F 3 "" H 1575 6675 50  0001 C CNN
	1    1575 6675
	1    0    0    -1  
$EndComp
Text Label 4850 775  0    60   ~ 0
Alrm
Text Label 1075 6850 2    60   ~ 0
Alrm
Text Label 3075 2975 0    60   ~ 0
In1
Text Label 3075 3100 0    60   ~ 0
In2
Text Label 3050 3650 0    60   ~ 0
EF
Text Label 1075 6950 2    60   ~ 0
EF
Text Label 1575 6950 0    60   ~ 0
In1
Text Label 1575 7050 0    60   ~ 0
In2
Text Notes 975  6450 0    60   ~ 0
External devices
Text Label 6050 875  0    60   ~ 0
Red
Text Label 6050 975  0    60   ~ 0
Black
Text Label 6050 1175 0    60   ~ 0
Yellow
Text Label 1575 7350 0    60   ~ 0
Yellow
Text Label 1575 7250 0    60   ~ 0
Red
Text Label 1075 7350 2    60   ~ 0
Black
NoConn ~ 5600 4775
NoConn ~ 5600 5175
$Comp
L Chiller_control-rescue:PWR_FLAG #FLG041
U 1 1 5BF7AD8F
P 1525 1325
F 0 "#FLG041" H 1525 1420 50  0001 C CNN
F 1 "PWR_FLAG" H 1525 1505 50  0001 C CNN
F 2 "" H 1525 1325 50  0000 C CNN
F 3 "" H 1525 1325 50  0000 C CNN
	1    1525 1325
	-1   0    0    1   
$EndComp
Wire Wire Line
	1050 3000 1050 2950
Wire Wire Line
	1050 2500 1050 2600
Wire Wire Line
	1650 2600 1650 2650
Wire Wire Line
	1050 2600 1350 2600
Connection ~ 1350 2600
Connection ~ 1050 2600
Wire Wire Line
	1650 3000 1650 2950
Wire Wire Line
	1600 3450 1600 3500
Connection ~ 1300 3450
Wire Wire Line
	1300 3850 1450 3850
Wire Wire Line
	1600 3850 1600 3800
Wire Wire Line
	1450 3900 1450 3850
Connection ~ 1450 3850
Wire Notes Line
	650  2200 650  4150
Wire Notes Line
	650  4150 2000 4150
Wire Notes Line
	2000 4150 2000 2200
Wire Notes Line
	2000 2200 650  2200
Connection ~ 1100 1325
Wire Wire Line
	2450 1325 2550 1325
Connection ~ 1700 1325
Wire Notes Line
	625  575  625  2025
Wire Notes Line
	625  2025 3475 2025
Wire Notes Line
	3475 2025 3475 575 
Wire Notes Line
	3475 575  625  575 
Wire Wire Line
	8675 875  8675 1275
Wire Wire Line
	8775 1175 8775 1275
Wire Wire Line
	8775 3225 8775 3175
Wire Wire Line
	8075 825  8075 875 
Wire Wire Line
	2550 1325 2550 1475
Connection ~ 2550 1325
Wire Wire Line
	8075 875  8675 875 
Wire Wire Line
	2150 1625 2150 1775
Wire Wire Line
	1700 1775 2150 1775
Connection ~ 2150 1775
Wire Wire Line
	1700 1625 1700 1775
Connection ~ 8075 875 
Wire Wire Line
	1350 3050 1350 3000
Wire Wire Line
	1050 3450 1300 3450
Wire Wire Line
	2800 1775 2800 1725
Connection ~ 2550 1775
Wire Wire Line
	2800 1325 2800 1425
Connection ~ 2750 1325
Wire Wire Line
	1075 1500 1075 1425
Wire Wire Line
	975  1500 1075 1500
Connection ~ 1075 1500
Connection ~ 8675 875 
Wire Wire Line
	9675 975  9450 975 
Wire Wire Line
	8775 1275 9100 1275
Connection ~ 8775 1275
Wire Notes Line
	10750 1600 10750 1950
Wire Wire Line
	5725 1600 5725 1575
Wire Wire Line
	5425 1325 5425 1375
Connection ~ 5425 1375
Connection ~ 5725 1175
Wire Wire Line
	5725 875  5925 875 
Wire Wire Line
	5725 1175 6050 1175
Wire Wire Line
	5950 975  6050 975 
Connection ~ 5925 875 
Wire Wire Line
	5825 1600 5725 1600
Connection ~ 5725 1600
Wire Wire Line
	3900 2100 4000 2100
Wire Wire Line
	4000 2400 4000 2450
Wire Wire Line
	4475 2050 4475 2100
Wire Wire Line
	4475 2100 4600 2100
Connection ~ 4475 2100
Connection ~ 4000 2100
Wire Wire Line
	4475 1725 4475 1750
Wire Wire Line
	2625 2625 2725 2625
Connection ~ 2725 2625
Wire Wire Line
	2600 2975 2625 2975
Wire Wire Line
	2625 2975 2625 2925
Wire Wire Line
	2600 3100 2825 3100
Wire Wire Line
	2825 3100 2825 2925
Connection ~ 2625 2975
Connection ~ 2825 3100
Wire Wire Line
	2875 3650 2925 3650
Connection ~ 2925 3650
Connection ~ 4600 1175
Wire Wire Line
	4600 1200 4600 1175
Wire Wire Line
	4600 775  4850 775 
Connection ~ 10225 4475
Wire Wire Line
	10225 4500 10225 4475
Connection ~ 9925 4275
Wire Wire Line
	10225 4075 10650 4075
Connection ~ 10275 5350
Wire Wire Line
	10275 5375 10275 5350
Connection ~ 9975 5150
Wire Wire Line
	10275 4950 10675 4950
Connection ~ 10350 6100
Wire Wire Line
	10350 6125 10350 6100
Wire Wire Line
	10350 5700 10725 5700
Connection ~ 9100 1275
Wire Wire Line
	850  5150 925  5150
Wire Wire Line
	850  4700 850  4925
Wire Wire Line
	850  4700 925  4700
Wire Wire Line
	725  4925 850  4925
Connection ~ 850  4925
Wire Wire Line
	725  4925 725  4975
Wire Wire Line
	1225 4700 1625 4700
Wire Wire Line
	1225 4925 1350 4925
Wire Wire Line
	1225 5150 1350 5150
Wire Wire Line
	850  5375 925  5375
Connection ~ 850  5150
Wire Wire Line
	1225 5375 1625 5375
Wire Wire Line
	4800 3275 4800 3375
Connection ~ 4800 3375
Wire Wire Line
	5125 3450 5125 3375
Wire Wire Line
	5125 3375 5100 3375
Wire Wire Line
	5600 3775 5650 3775
Wire Wire Line
	5650 4075 5600 4075
Wire Wire Line
	5650 4575 5600 4575
Wire Wire Line
	5600 4275 5650 4275
Wire Wire Line
	5950 4275 5950 4400
Wire Wire Line
	6050 4400 5950 4400
Connection ~ 5950 4400
Wire Wire Line
	3925 4075 4000 4075
Wire Wire Line
	3925 3775 4000 3775
Wire Wire Line
	6150 4875 5600 4875
Wire Wire Line
	4800 5925 4800 5875
Connection ~ 9325 1275
Wire Wire Line
	10650 3925 10650 3975
Wire Wire Line
	10675 4800 10675 4850
Wire Wire Line
	10725 5600 10725 5525
Wire Notes Line
	9075 3575 11050 3575
Wire Notes Line
	11050 3575 11050 6350
Wire Notes Line
	11050 6350 9075 6350
Wire Notes Line
	9075 6350 9075 3575
Wire Wire Line
	1625 5375 1625 5150
Wire Wire Line
	1350 5050 1625 5050
Wire Wire Line
	1350 5150 1350 5050
Wire Wire Line
	1350 4950 1625 4950
Wire Wire Line
	1350 4925 1350 4950
Wire Wire Line
	1625 4700 1625 4850
Wire Wire Line
	2125 4750 2125 4850
Connection ~ 2125 4850
Connection ~ 2125 5050
Connection ~ 2125 4950
Wire Notes Line
	650  4325 2325 4325
Wire Notes Line
	2325 4325 2325 5550
Wire Notes Line
	2325 5550 650  5550
Wire Notes Line
	650  5550 650  4325
Wire Notes Line
	3550 6100 6750 6100
Wire Notes Line
	6750 6100 6750 3050
Wire Notes Line
	6750 3050 3550 3050
Wire Notes Line
	3550 3050 3550 6100
Wire Notes Line
	975  6900 1650 6900
Wire Notes Line
	975  7100 1650 7100
Wire Notes Line
	975  7200 1650 7200
Wire Wire Line
	725  7150 1075 7150
Wire Wire Line
	1750 6850 1575 6850
Wire Wire Line
	1575 6675 1575 6750
Wire Notes Line
	5175 575  5175 1750
Wire Notes Line
	5175 1750 6350 1750
Wire Notes Line
	6350 1750 6350 575 
Wire Notes Line
	6350 575  5175 575 
Wire Notes Line
	4925 1525 4925 2650
Wire Notes Line
	4925 2650 3600 2650
Wire Notes Line
	3600 2650 3600 1525
Wire Notes Line
	3600 1525 4925 1525
Wire Notes Line
	2200 2200 2200 3800
Wire Notes Line
	2200 3800 3250 3800
Wire Notes Line
	3250 3800 3250 2200
Wire Notes Line
	3250 2200 2200 2200
Wire Notes Line
	3600 575  3600 1425
Wire Notes Line
	3600 1425 5075 1425
Wire Notes Line
	5075 1425 5075 575 
Wire Notes Line
	5075 575  3600 575 
Wire Notes Line
	650  6350 650  7550
Wire Notes Line
	650  7550 2475 7550
Wire Notes Line
	2475 7550 2475 6350
Wire Notes Line
	2475 6350 650  6350
Wire Wire Line
	5600 4875 5600 4975
Wire Wire Line
	5600 5375 5850 5375
Wire Wire Line
	5850 5375 5850 5075
Wire Wire Line
	5850 5075 6150 5075
Wire Wire Line
	1425 1325 1525 1325
Wire Wire Line
	1075 1325 1100 1325
Connection ~ 1525 1325
Wire Wire Line
	6125 5500 6125 5475
Wire Wire Line
	6125 5475 6150 5475
Connection ~ 10050 5900
Wire Wire Line
	4300 975  4300 1175
Connection ~ 4300 975 
Wire Wire Line
	10050 6100 10050 5900
Wire Wire Line
	9975 5350 9975 5150
Wire Wire Line
	9925 4475 9925 4275
Wire Wire Line
	1350 2600 1650 2600
Wire Wire Line
	1050 2600 1050 2650
Wire Wire Line
	1300 3450 1600 3450
Wire Wire Line
	1450 3850 1600 3850
Wire Wire Line
	1100 1325 1125 1325
Wire Wire Line
	1700 1325 1850 1325
Wire Wire Line
	2550 1325 2750 1325
Wire Wire Line
	2150 1775 2150 1825
Wire Wire Line
	2150 1775 2550 1775
Wire Wire Line
	2550 1775 2800 1775
Wire Wire Line
	2750 1325 2800 1325
Wire Wire Line
	8675 875  8775 875 
Wire Wire Line
	5425 1375 5425 1600
Wire Wire Line
	5925 875  6050 875 
Wire Wire Line
	4000 2100 4175 2100
Wire Wire Line
	2725 2625 2825 2625
Wire Wire Line
	2625 2975 3075 2975
Wire Wire Line
	2825 3100 3075 3100
Wire Wire Line
	2925 3650 3050 3650
Wire Wire Line
	9100 1275 9325 1275
Wire Wire Line
	850  4925 850  5150
Wire Wire Line
	850  4925 925  4925
Wire Wire Line
	850  5150 850  5375
Wire Wire Line
	4800 3375 4800 3475
Wire Wire Line
	5950 4400 5950 4575
Wire Wire Line
	9325 1275 9450 1275
Wire Wire Line
	2125 4850 2125 4950
Wire Wire Line
	2125 5050 2125 5150
Wire Wire Line
	2125 4950 2125 5050
Wire Wire Line
	1525 1325 1700 1325
$EndSCHEMATC
