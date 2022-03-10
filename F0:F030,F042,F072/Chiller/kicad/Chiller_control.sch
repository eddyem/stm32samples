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
P 2875 1825
F 0 "#PWR01" H 2875 1575 50  0001 C CNN
F 1 "GND" H 2875 1675 50  0000 C CNN
F 2 "" H 2875 1825 50  0000 C CNN
F 3 "" H 2875 1825 50  0000 C CNN
	1    2875 1825
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:CP C4
U 1 1 58C454F6
P 3275 1625
F 0 "C4" H 3300 1725 50  0000 L CNN
F 1 "47u" H 3300 1525 50  0000 L CNN
F 2 "Capacitor_Tantalum_SMD.pretty:CP_Tantalum_Case-A_EIA-3216-18_Hand" H 3313 1475 50  0001 C CNN
F 3 "" H 3275 1625 50  0000 C CNN
	1    3275 1625
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR02
U 1 1 58C455CB
P 3475 1325
F 0 "#PWR02" H 3475 1175 50  0001 C CNN
F 1 "+3.3V" H 3475 1465 50  0000 C CNN
F 2 "" H 3475 1325 50  0000 C CNN
F 3 "" H 3475 1325 50  0000 C CNN
	1    3475 1325
	1    0    0    -1  
$EndComp
Text Notes 1125 1000 0    100  ~ 0
5/3.3V MCU power source
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
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 1688 2650 50  0001 C CNN
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
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 1638 3500 50  0001 C CNN
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
P 2425 1475
F 0 "C3" H 2450 1575 50  0000 L CNN
F 1 "0.1" H 2450 1375 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 2463 1325 50  0001 C CNN
F 3 "" H 2425 1475 50  0000 C CNN
	1    2425 1475
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
F 1 "+12V" H 1050 1475 50  0000 C CNN
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
Text Notes 550  1500 0    79   ~ 0
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
P 2875 1325
F 0 "U1" H 2875 1525 50  0000 C CNN
F 1 "LM1117-3.3" H 2625 1450 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-223" H 2875 1325 50  0001 C CNN
F 3 "" H 2875 1325 50  0001 C CNN
	1    2875 1325
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C5
U 1 1 5BEE1D09
P 3525 1575
F 0 "C5" H 3550 1675 50  0000 L CNN
F 1 "0.1" H 3550 1475 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 3563 1425 50  0001 C CNN
F 3 "" H 3525 1575 50  0000 C CNN
	1    3525 1575
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
P 2650 2775
F 0 "R8" V 2730 2775 50  0000 C CNN
F 1 "10k" V 2650 2775 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 2580 2775 50  0001 C CNN
F 3 "" H 2650 2775 50  0001 C CNN
	1    2650 2775
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R6
U 1 1 5BEEA7BE
P 2450 2775
F 0 "R6" V 2530 2775 50  0000 C CNN
F 1 "10k" V 2450 2775 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 2380 2775 50  0001 C CNN
F 3 "" H 2450 2775 50  0001 C CNN
	1    2450 2775
	1    0    0    -1  
$EndComp
Text Notes 10825 2775 0    60   ~ 0
(opendrain)
Text Label 2525 3700 2    60   ~ 0
DigIn1
Text Notes 2500 2325 0    60   ~ 0
EXT in/out
$Comp
L Chiller_control-rescue:Q_NMOS_GSD Q2
U 1 1 5BEDCAD0
P 7825 4625
F 0 "Q2" H 8025 4675 50  0000 L CNN
F 1 "SI2300" H 8025 4575 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 8025 4725 50  0001 C CNN
F 3 "" H 7825 4625 50  0001 C CNN
	1    7825 4625
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R14
U 1 1 5BEDD4AE
P 7775 4850
F 0 "R14" V 7855 4850 50  0000 C CNN
F 1 "10k" V 7775 4850 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 7705 4850 50  0001 C CNN
F 3 "" H 7775 4850 50  0001 C CNN
	1    7775 4850
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R15
U 1 1 5BEDD8A0
P 7925 4275
F 0 "R15" V 8005 4275 50  0000 C CNN
F 1 "10k" V 7925 4275 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 7855 4275 50  0001 C CNN
F 3 "" H 7925 4275 50  0001 C CNN
	1    7925 4275
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR015
U 1 1 5BEDE0EC
P 7625 4575
F 0 "#PWR015" H 7625 4425 50  0001 C CNN
F 1 "+3.3V" H 7625 4715 50  0000 C CNN
F 2 "" H 7625 4575 50  0000 C CNN
F 3 "" H 7625 4575 50  0000 C CNN
	1    7625 4575
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR016
U 1 1 5BEE048F
P 8500 4175
F 0 "#PWR016" H 8500 3925 50  0001 C CNN
F 1 "GND" H 8500 4025 50  0000 C CNN
F 2 "" H 8500 4175 50  0000 C CNN
F 3 "" H 8500 4175 50  0000 C CNN
	1    8500 4175
	1    0    0    -1  
$EndComp
Text Notes 7400 3775 0    60   ~ 0
YF-S201C
$Comp
L Chiller_control-rescue:+12V #PWR017
U 1 1 5BEE1671
P 8350 3825
F 0 "#PWR017" H 8350 3675 50  0001 C CNN
F 1 "+12V" H 8350 3965 50  0000 C CNN
F 2 "" H 8350 3825 50  0001 C CNN
F 3 "" H 8350 3825 50  0001 C CNN
	1    8350 3825
	1    0    0    -1  
$EndComp
Text Label 8025 4850 0    60   ~ 0
TIM3_Ch4
$Comp
L Chiller_control-rescue:+3.3V #PWR020
U 1 1 5BEE53EF
P 2550 2625
F 0 "#PWR020" H 2550 2475 50  0001 C CNN
F 1 "+3.3V" H 2550 2765 50  0000 C CNN
F 2 "" H 2550 2625 50  0000 C CNN
F 3 "" H 2550 2625 50  0000 C CNN
	1    2550 2625
	1    0    0    -1  
$EndComp
Text Notes 9575 3725 2    60   ~ 0
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
Text Notes 10500 3725 0    60   ~ 0
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
F 2 "TO_SOT_Packages_THT:TO-220-3_Vertical" H 10375 5250 50  0001 C CNN
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
F 2 "TO_SOT_Packages_THT:TO-220-3_Vertical" H 10450 6000 50  0001 C CNN
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
Text Notes 825  4600 0    60   ~ 0
Thermal
$Comp
L Chiller_control-rescue:R R2
U 1 1 5BEF45BA
P 1025 4925
F 0 "R2" V 1105 4925 50  0000 C CNN
F 1 "1k0.1%" V 1025 4925 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 955 4925 50  0001 C CNN
F 3 "" H 1025 4925 50  0001 C CNN
	1    1025 4925
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
P 2275 5475
F 0 "#PWR027" H 2425 5425 50  0001 C CNN
F 1 "+3.3VADC" H 2275 5575 50  0000 C CNN
F 2 "" H 2275 5475 50  0001 C CNN
F 3 "" H 2275 5475 50  0001 C CNN
	1    2275 5475
	-1   0    0    1   
$EndComp
$Comp
L Chiller_control-rescue:R R3
U 1 1 5BEF7CBC
P 1025 5150
F 0 "R3" V 1105 5150 50  0000 C CNN
F 1 "1k0.1%" V 1025 5150 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 955 5150 50  0001 C CNN
F 3 "" H 1025 5150 50  0001 C CNN
	1    1025 5150
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R4
U 1 1 5BEF8256
P 1025 5375
F 0 "R4" V 1105 5375 50  0000 C CNN
F 1 "1k0.1%" V 1025 5375 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 955 5375 50  0001 C CNN
F 3 "" H 1025 5375 50  0001 C CNN
	1    1025 5375
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR028
U 1 1 5BEF8833
P 675 5200
F 0 "#PWR028" H 675 4950 50  0001 C CNN
F 1 "GND" H 675 5050 50  0000 C CNN
F 2 "" H 675 5200 50  0000 C CNN
F 3 "" H 675 5200 50  0000 C CNN
	1    675  5200
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R5
U 1 1 5BEFAAF1
P 1025 5600
F 0 "R5" V 1105 5600 50  0000 C CNN
F 1 "1k0.1%" V 1025 5600 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 955 5600 50  0001 C CNN
F 3 "" H 1025 5600 50  0001 C CNN
	1    1025 5600
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:MAX3232 U2
U 1 1 5BEFC197
P 3950 6125
F 0 "U2" H 3850 7250 50  0000 R CNN
F 1 "MAX3232" H 3850 7175 50  0000 R CNN
F 2 "Housings_SOIC:SOIC-16_3.9x9.9mm_Pitch1.27mm" H 4000 5075 50  0001 L CNN
F 3 "" H 3950 6225 50  0001 C CNN
	1    3950 6125
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C11
U 1 1 5BEFD47C
P 4950 5725
F 0 "C11" H 4975 5825 50  0000 L CNN
F 1 "0.47" H 4975 5625 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 4988 5575 50  0001 C CNN
F 3 "" H 4950 5725 50  0001 C CNN
	1    4950 5725
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:C C10
U 1 1 5BEFD7DB
P 4800 5375
F 0 "C10" H 4825 5475 50  0000 L CNN
F 1 "0.47" H 4825 5275 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 4838 5225 50  0001 C CNN
F 3 "" H 4800 5375 50  0001 C CNN
	1    4800 5375
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C13
U 1 1 5BEFD89D
P 4950 6025
F 0 "C13" H 4975 6125 50  0000 L CNN
F 1 "0.47" H 4975 5925 50  0000 L CNN
F 2 "Capacitor_SMD.pretty:C_0805_HandSoldering" H 4988 5875 50  0001 C CNN
F 3 "" H 4950 6025 50  0001 C CNN
	1    4950 6025
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:C C9
U 1 1 5BEFF11C
P 4100 4825
F 0 "C9" H 4125 4925 50  0000 L CNN
F 1 "0.1" H 4125 4725 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 4138 4675 50  0001 C CNN
F 3 "" H 4100 4825 50  0000 C CNN
	1    4100 4825
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR029
U 1 1 5BF006EE
P 3950 4725
F 0 "#PWR029" H 3950 4575 50  0001 C CNN
F 1 "+3.3V" H 3950 4865 50  0000 C CNN
F 2 "" H 3950 4725 50  0000 C CNN
F 3 "" H 3950 4725 50  0000 C CNN
	1    3950 4725
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR030
U 1 1 5BF0128D
P 4275 4900
F 0 "#PWR030" H 4275 4650 50  0001 C CNN
F 1 "GND" H 4275 4750 50  0000 C CNN
F 2 "" H 4275 4900 50  0000 C CNN
F 3 "" H 4275 4900 50  0000 C CNN
	1    4275 4900
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR031
U 1 1 5BF022E5
P 5200 5850
F 0 "#PWR031" H 5200 5600 50  0001 C CNN
F 1 "GND" H 5200 5700 50  0000 C CNN
F 2 "" H 5200 5850 50  0000 C CNN
F 3 "" H 5200 5850 50  0000 C CNN
	1    5200 5850
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C8
U 1 1 5BF0257F
P 3075 5375
F 0 "C8" H 3100 5475 50  0000 L CNN
F 1 "0.1" H 3100 5275 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 3113 5225 50  0001 C CNN
F 3 "" H 3075 5375 50  0000 C CNN
	1    3075 5375
	-1   0    0    1   
$EndComp
$Comp
L Chiller_control-rescue:DB9_Female J1
U 1 1 5BF02D65
P 5600 6525
F 0 "J1" H 5600 7075 50  0000 C CNN
F 1 "DB9_Female" H 5600 5950 50  0000 C CNN
F 2 "modules:DB9-F" H 5600 6525 50  0001 C CNN
F 3 "" H 5600 6525 50  0001 C CNN
	1    5600 6525
	1    0    0    -1  
$EndComp
Text Notes 4750 7500 0    60   ~ 0
2 - RS232-Rx\n3 - RS232-Tx\n5 - GND
Text Label 3150 6425 2    60   ~ 0
USART_Tx
Text Label 3150 6825 2    60   ~ 0
USART_Rx
$Comp
L Chiller_control-rescue:GND #PWR032
U 1 1 5BF0792A
P 3950 7375
F 0 "#PWR032" H 3950 7125 50  0001 C CNN
F 1 "GND" H 3950 7225 50  0000 C CNN
F 2 "" H 3950 7375 50  0000 C CNN
F 3 "" H 3950 7375 50  0000 C CNN
	1    3950 7375
	1    0    0    -1  
$EndComp
NoConn ~ 3150 6625
NoConn ~ 3150 6225
NoConn ~ 5300 6125
NoConn ~ 5300 6225
NoConn ~ 5300 6425
NoConn ~ 5300 6625
NoConn ~ 5300 6725
NoConn ~ 5300 6825
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
P 5275 6950
F 0 "#PWR034" H 5275 6700 50  0001 C CNN
F 1 "GND" H 5275 6800 50  0000 C CNN
F 2 "" H 5275 6950 50  0000 C CNN
F 3 "" H 5275 6950 50  0000 C CNN
	1    5275 6950
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
Text Notes 7925 3675 2    60   ~ 0
Flow sensor
Text Notes 3050 4700 0    60   ~ 0
RS-232
NoConn ~ 1075 7250
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
DIG in/out
Text Label 3075 2975 0    60   ~ 0
Out1
Text Label 3075 3100 0    60   ~ 0
Out2
Text Label 3050 3700 0    60   ~ 0
IN
Text Label 1075 6950 2    60   ~ 0
IN
Text Label 1575 6950 0    60   ~ 0
Out1
Text Label 1575 7050 0    60   ~ 0
Out2
Text Notes 975  6450 0    60   ~ 0
External devices
Text Label 8575 4075 0    60   ~ 0
Red
Text Label 8550 4175 0    60   ~ 0
Black
Text Label 8525 4475 0    60   ~ 0
Yellow
Text Label 1575 7350 0    60   ~ 0
Yellow
Text Label 1575 7250 0    60   ~ 0
Red
Text Label 1075 7350 2    60   ~ 0
Black
NoConn ~ 4750 6225
NoConn ~ 4750 6625
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
	3175 1325 3275 1325
Connection ~ 2425 1325
Wire Wire Line
	8675 875  8675 1275
Wire Wire Line
	8775 1175 8775 1275
Wire Wire Line
	8775 3225 8775 3175
Wire Wire Line
	8075 825  8075 875 
Wire Wire Line
	3275 1325 3275 1475
Connection ~ 3275 1325
Wire Wire Line
	8075 875  8675 875 
Wire Wire Line
	2875 1625 2875 1775
Wire Wire Line
	2425 1775 2875 1775
Connection ~ 2875 1775
Wire Wire Line
	2425 1625 2425 1775
Connection ~ 8075 875 
Wire Wire Line
	1350 3050 1350 3000
Wire Wire Line
	1050 3450 1300 3450
Wire Wire Line
	3525 1775 3525 1725
Connection ~ 3275 1775
Wire Wire Line
	3525 1325 3525 1425
Connection ~ 3475 1325
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
	7925 4850 7925 4825
Wire Wire Line
	7625 4575 7625 4625
Connection ~ 7625 4625
Wire Wire Line
	8025 4850 7925 4850
Connection ~ 7925 4850
Wire Wire Line
	2450 2625 2550 2625
Connection ~ 2550 2625
Wire Wire Line
	2425 2975 2450 2975
Wire Wire Line
	2450 2975 2450 2925
Wire Wire Line
	2425 3100 2650 3100
Wire Wire Line
	2650 3100 2650 2925
Connection ~ 10225 4475
Wire Wire Line
	10225 4500 10225 4475
Wire Wire Line
	10225 4075 10650 4075
Connection ~ 10275 5350
Wire Wire Line
	10275 5375 10275 5350
Wire Wire Line
	10275 4950 10675 4950
Connection ~ 10350 6100
Wire Wire Line
	10350 6125 10350 6100
Wire Wire Line
	10350 5700 10725 5700
Connection ~ 9100 1275
Wire Wire Line
	800  5375 875  5375
Wire Wire Line
	800  4925 800  5150
Wire Wire Line
	800  4925 875  4925
Wire Wire Line
	675  5150 800  5150
Connection ~ 800  5150
Wire Wire Line
	675  5150 675  5200
Wire Wire Line
	1175 4925 1400 4925
Wire Wire Line
	800  5600 875  5600
Connection ~ 800  5375
Wire Wire Line
	3950 4725 3950 4825
Connection ~ 3950 4825
Wire Wire Line
	4275 4900 4275 4825
Wire Wire Line
	4275 4825 4250 4825
Wire Wire Line
	4750 5225 4800 5225
Wire Wire Line
	4800 5525 4750 5525
Wire Wire Line
	4800 6025 4750 6025
Wire Wire Line
	4750 5725 4800 5725
Wire Wire Line
	5100 5725 5100 5850
Wire Wire Line
	5200 5850 5100 5850
Connection ~ 5100 5850
Wire Wire Line
	3075 5525 3150 5525
Wire Wire Line
	3075 5225 3150 5225
Wire Wire Line
	5300 6325 4750 6325
Wire Wire Line
	3950 7375 3950 7325
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
Wire Notes Line
	2700 7550 5900 7550
Wire Notes Line
	5900 7550 5900 4500
Wire Notes Line
	5900 4500 2700 4500
Wire Notes Line
	2700 4500 2700 7550
Wire Notes Line
	650  6350 650  7550
Wire Notes Line
	650  7550 2475 7550
Wire Notes Line
	2475 7550 2475 6350
Wire Notes Line
	2475 6350 650  6350
Wire Wire Line
	4750 6325 4750 6425
Wire Wire Line
	4750 6825 5000 6825
Wire Wire Line
	5000 6825 5000 6525
Wire Wire Line
	5000 6525 5300 6525
Wire Wire Line
	1075 1325 1100 1325
Wire Wire Line
	5275 6950 5275 6925
Wire Wire Line
	5275 6925 5300 6925
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
	2425 1325 2575 1325
Wire Wire Line
	3275 1325 3475 1325
Wire Wire Line
	2875 1775 2875 1825
Wire Wire Line
	2875 1775 3275 1775
Wire Wire Line
	3275 1775 3525 1775
Wire Wire Line
	3475 1325 3525 1325
Wire Wire Line
	8675 875  8775 875 
Wire Wire Line
	7625 4625 7625 4850
Wire Wire Line
	2550 2625 2650 2625
Wire Wire Line
	9100 1275 9325 1275
Wire Wire Line
	800  5150 800  5375
Wire Wire Line
	800  5150 875  5150
Wire Wire Line
	800  5375 800  5600
Wire Wire Line
	3950 4825 3950 4925
Wire Wire Line
	5100 5850 5100 6025
Wire Wire Line
	9325 1275 9450 1275
Wire Wire Line
	4525 2375 4700 2375
Wire Notes Line
	4125 850  4125 1700
Wire Notes Line
	4125 1800 5450 1800
Wire Notes Line
	4125 2925 4125 1800
Wire Notes Line
	5450 2925 4125 2925
Wire Notes Line
	5450 1800 5450 2925
Wire Wire Line
	5125 1475 5125 1450
Connection ~ 5125 1450
Wire Wire Line
	5000 2000 5000 2025
Connection ~ 4525 2375
Connection ~ 5000 2375
Wire Wire Line
	5000 2375 5125 2375
Wire Wire Line
	5000 2325 5000 2375
Wire Wire Line
	4525 2675 4525 2725
Wire Wire Line
	4425 2375 4525 2375
Text Label 5125 2375 0    60   ~ 0
W.levl
$Comp
L Chiller_control-rescue:GND #PWR022
U 1 1 5BEE8828
P 5125 1475
F 0 "#PWR022" H 5125 1225 50  0001 C CNN
F 1 "GND" H 5125 1325 50  0000 C CNN
F 2 "" H 5125 1475 50  0000 C CNN
F 3 "" H 5125 1475 50  0000 C CNN
	1    5125 1475
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:+3.3V #PWR019
U 1 1 5BEE4159
P 5000 2000
F 0 "#PWR019" H 5000 1850 50  0001 C CNN
F 1 "+3.3V" H 5000 2140 50  0000 C CNN
F 2 "" H 5000 2000 50  0000 C CNN
F 3 "" H 5000 2000 50  0000 C CNN
	1    5000 2000
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R13
U 1 1 5BEE33E1
P 5000 2175
F 0 "R13" V 5080 2175 50  0000 C CNN
F 1 "10k" V 5000 2175 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 4930 2175 50  0001 C CNN
F 3 "" H 5000 2175 50  0001 C CNN
	1    5000 2175
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR018
U 1 1 5BEE2D93
P 4525 2725
F 0 "#PWR018" H 4525 2475 50  0001 C CNN
F 1 "GND" H 4525 2575 50  0000 C CNN
F 2 "" H 4525 2725 50  0000 C CNN
F 3 "" H 4525 2725 50  0000 C CNN
	1    4525 2725
	1    0    0    -1  
$EndComp
Text Notes 4950 975  2    60   ~ 0
Alarm Buzzer
Text Notes 4775 2000 2    60   ~ 0
Water level
$Comp
L Chiller_control-rescue:R R11
U 1 1 5BEDB615
P 4850 2375
F 0 "R11" V 4930 2375 50  0000 C CNN
F 1 "220" V 4850 2375 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0805_HandSoldering" V 4780 2375 50  0001 C CNN
F 3 "" H 4850 2375 50  0001 C CNN
	1    4850 2375
	0    1    1    0   
$EndComp
Text Label 4425 2375 2    60   ~ 0
DigIn0
$Comp
L Chiller_control-rescue:Q_NMOS_GSD Q1
U 1 1 5BEEB585
P 5025 1250
F 0 "Q1" H 5225 1300 50  0000 L CNN
F 1 "SI2300" H 5225 1200 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SOT-23_Handsoldering" H 5225 1350 50  0001 C CNN
F 3 "" H 5025 1250 50  0001 C CNN
	1    5025 1250
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R10
U 1 1 5BEEB37B
P 4675 1250
F 0 "R10" V 4755 1250 50  0000 C CNN
F 1 "510" V 4675 1250 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 4605 1250 50  0001 C CNN
F 3 "" H 4675 1250 50  0001 C CNN
	1    4675 1250
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R12
U 1 1 5BEEB2E1
P 4975 1450
F 0 "R12" V 5055 1450 50  0000 C CNN
F 1 "10k" V 4975 1450 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 4905 1450 50  0001 C CNN
F 3 "" H 4975 1450 50  0001 C CNN
	1    4975 1450
	0    1    1    0   
$EndComp
Text Label 4525 1250 2    60   ~ 0
DigOut0
$Comp
L Chiller_control-rescue:C C12
U 1 1 5A386BD0
P 4525 2525
F 0 "C12" H 4550 2625 50  0000 L CNN
F 1 "0.1" H 4550 2425 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 4563 2375 50  0001 C CNN
F 3 "" H 4525 2525 50  0000 C CNN
	1    4525 2525
	1    0    0    -1  
$EndComp
Connection ~ 2925 3700
Wire Wire Line
	2925 3700 3050 3700
Wire Wire Line
	2875 3700 2925 3700
$Comp
L Chiller_control-rescue:+3.3V #PWR021
U 1 1 5BEE7095
P 2925 3400
F 0 "#PWR021" H 2925 3250 50  0001 C CNN
F 1 "+3.3V" H 2925 3540 50  0000 C CNN
F 2 "" H 2925 3400 50  0000 C CNN
F 3 "" H 2925 3400 50  0000 C CNN
	1    2925 3400
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R9
U 1 1 5BEE5083
P 2925 3550
F 0 "R9" V 3005 3550 50  0000 C CNN
F 1 "10k" V 2925 3550 50  0000 C CNN
F 2 "Resistors_SMD.pretty:R_0603_HandSoldering" V 2855 3550 50  0001 C CNN
F 3 "" H 2925 3550 50  0001 C CNN
	1    2925 3550
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R7
U 1 1 5BEDA6BA
P 2725 3700
F 0 "R7" V 2805 3700 50  0000 C CNN
F 1 "220" V 2725 3700 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.29x1.40mm_HandSolder" V 2655 3700 50  0001 C CNN
F 3 "" H 2725 3700 50  0001 C CNN
	1    2725 3700
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:R R22
U 1 1 5C509370
P 2875 2925
F 0 "R22" V 2955 2925 50  0000 C CNN
F 1 "220" V 2875 2925 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.29x1.40mm_HandSolder" V 2805 2925 50  0001 C CNN
F 3 "" H 2875 2925 50  0001 C CNN
	1    2875 2925
	0    1    1    0   
$EndComp
Text Label 2425 2975 2    60   ~ 0
DigOut1
Text Label 2425 3100 2    60   ~ 0
DigOut2
$Comp
L Chiller_control-rescue:R R23
U 1 1 5C50F547
P 2875 3100
F 0 "R23" V 2955 3100 50  0000 C CNN
F 1 "220" V 2875 3100 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.29x1.40mm_HandSolder" V 2805 3100 50  0001 C CNN
F 3 "" H 2875 3100 50  0001 C CNN
	1    2875 3100
	0    1    1    0   
$EndComp
Wire Wire Line
	2650 3100 2725 3100
Connection ~ 2650 3100
Wire Wire Line
	3025 3100 3075 3100
Wire Wire Line
	2450 2975 2725 2975
Wire Wire Line
	2725 2975 2725 2925
Connection ~ 2450 2975
Wire Wire Line
	3025 2925 3025 2975
Wire Wire Line
	3025 2975 3075 2975
Wire Notes Line
	2050 2200 3300 2200
$Comp
L Chiller_control-rescue:CP C15
U 1 1 5C53FDA9
P 10225 3925
F 0 "C15" H 10343 3971 50  0000 L CNN
F 1 "100u" H 10343 3880 50  0000 L CNN
F 2 "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm" H 10263 3775 50  0001 C CNN
F 3 "" H 10225 3925 50  0001 C CNN
	1    10225 3925
	1    0    0    -1  
$EndComp
Connection ~ 10225 4075
$Comp
L Chiller_control-rescue:+12V #PWR033
U 1 1 5C54020D
P 10225 3775
F 0 "#PWR033" H 10225 3625 50  0001 C CNN
F 1 "+12V" H 10225 3915 50  0000 C CNN
F 2 "" H 10225 3775 50  0001 C CNN
F 3 "" H 10225 3775 50  0001 C CNN
	1    10225 3775
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:R R26
U 1 1 5C5422DE
P 8375 4475
F 0 "R26" V 8455 4475 50  0000 C CNN
F 1 "220" V 8375 4475 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.29x1.40mm_HandSolder" V 8305 4475 50  0001 C CNN
F 3 "" H 8375 4475 50  0001 C CNN
	1    8375 4475
	0    1    1    0   
$EndComp
Connection ~ 7925 4425
Wire Wire Line
	7925 4425 8075 4425
Wire Wire Line
	8075 4425 8075 4475
$Comp
L Chiller_control-rescue:C C16
U 1 1 5C548AFE
P 8175 4325
F 0 "C16" H 8290 4371 50  0000 L CNN
F 1 "1u" H 8290 4280 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.29x1.40mm_HandSolder" H 8213 4175 50  0001 C CNN
F 3 "" H 8175 4325 50  0001 C CNN
	1    8175 4325
	1    0    0    -1  
$EndComp
Wire Wire Line
	8075 4475 8175 4475
Wire Wire Line
	8175 4175 8500 4175
Wire Wire Line
	8500 4175 8550 4175
Connection ~ 8500 4175
Wire Wire Line
	8225 4475 8175 4475
Connection ~ 8175 4475
$Comp
L Chiller_control-rescue:R R25
U 1 1 5C565B8F
P 8350 3975
F 0 "R25" V 8430 3975 50  0000 C CNN
F 1 "0" V 8350 3975 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.29x1.40mm_HandSolder" V 8280 3975 50  0001 C CNN
F 3 "" H 8350 3975 50  0001 C CNN
	1    8350 3975
	-1   0    0    1   
$EndComp
Wire Wire Line
	8500 4125 8500 4075
Wire Wire Line
	8500 4075 8575 4075
Wire Wire Line
	7925 4125 8125 4125
Connection ~ 8350 4125
Wire Wire Line
	8350 4125 8500 4125
$Comp
L Chiller_control-rescue:R R24
U 1 1 5C56BFDC
P 8125 3975
F 0 "R24" V 8205 3975 50  0000 C CNN
F 1 "0" V 8125 3975 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.29x1.40mm_HandSolder" V 8055 3975 50  0001 C CNN
F 3 "" H 8125 3975 50  0001 C CNN
	1    8125 3975
	-1   0    0    1   
$EndComp
Connection ~ 8125 4125
Wire Wire Line
	8125 4125 8350 4125
$Comp
L power:+5V #PWR041
U 1 1 5C56C9DA
P 8125 3825
F 0 "#PWR041" H 8125 3675 50  0001 C CNN
F 1 "+5V" H 8140 3998 50  0000 C CNN
F 2 "" H 8125 3825 50  0001 C CNN
F 3 "" H 8125 3825 50  0001 C CNN
	1    8125 3825
	1    0    0    -1  
$EndComp
$Comp
L Regulator_Linear:L7805 U4
U 1 1 5C573DC9
P 1850 1325
F 0 "U4" H 1850 1567 50  0000 C CNN
F 1 "L7805" H 1850 1476 50  0000 C CNN
F 2 "TO_SOT_Packages_THT:TO-220-3_Vertical" H 1875 1175 50  0001 L CIN
F 3 "http://www.st.com/content/ccc/resource/technical/document/datasheet/41/4f/b3/b0/12/d4/47/88/CD00000444.pdf/files/CD00000444.pdf/jcr:content/translations/en.CD00000444.pdf" H 1850 1275 50  0001 C CNN
	1    1850 1325
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C14
U 1 1 5C574153
P 1425 1475
F 0 "C14" H 1450 1550 50  0000 L CNN
F 1 "1u" H 1500 1400 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.29x1.40mm_HandSolder" H 1463 1325 50  0001 C CNN
F 3 "" H 1425 1475 50  0001 C CNN
	1    1425 1475
	1    0    0    -1  
$EndComp
Wire Wire Line
	1425 1325 1550 1325
Connection ~ 1425 1325
$Comp
L Chiller_control-rescue:PWR_FLAG #FLG01
U 1 1 5C57AE5C
P 1425 1325
F 0 "#FLG01" H 1425 1420 50  0001 C CNN
F 1 "PWR_FLAG" H 1425 1505 50  0001 C CNN
F 2 "" H 1425 1325 50  0000 C CNN
F 3 "" H 1425 1325 50  0000 C CNN
	1    1425 1325
	1    0    0    -1  
$EndComp
Wire Wire Line
	1425 1625 1425 1775
Wire Wire Line
	1425 1775 1850 1775
Connection ~ 2425 1775
Wire Wire Line
	1850 1625 1850 1775
Connection ~ 1850 1775
Wire Wire Line
	1850 1775 2425 1775
$Comp
L power:+5V #PWR013
U 1 1 5C56CC29
P 2425 1325
F 0 "#PWR013" H 2425 1175 50  0001 C CNN
F 1 "+5V" H 2440 1498 50  0000 C CNN
F 2 "" H 2425 1325 50  0001 C CNN
F 3 "" H 2425 1325 50  0001 C CNN
	1    2425 1325
	1    0    0    -1  
$EndComp
Wire Notes Line
	500  825  3700 825 
Wire Notes Line
	3700 2125 500  2125
Wire Notes Line
	500  2125 500  825 
Wire Notes Line
	3700 825  3700 2125
Wire Notes Line
	7325 3575 8875 3575
Wire Notes Line
	8875 5025 7325 5025
Wire Notes Line
	7325 5025 7325 3575
Wire Notes Line
	8875 3575 8875 5025
Text Notes 7475 4075 0    60   ~ 0
Solder only\nR24 or R25
$Comp
L Chiller_control-rescue:C C18
U 1 1 5C5C6D73
P 1425 5850
F 0 "C18" H 1450 5950 50  0000 L CNN
F 1 "0.1" H 1450 5750 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 1463 5700 50  0001 C CNN
F 3 "" H 1425 5850 50  0000 C CNN
	1    1425 5850
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C20
U 1 1 5C5D4FAB
P 1675 5850
F 0 "C20" H 1700 5950 50  0000 L CNN
F 1 "0.1" H 1700 5750 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 1713 5700 50  0001 C CNN
F 3 "" H 1675 5850 50  0000 C CNN
	1    1675 5850
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C17
U 1 1 5C5D51AF
P 1400 4675
F 0 "C17" H 1425 4775 50  0000 L CNN
F 1 "0.1" H 1425 4575 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 1438 4525 50  0001 C CNN
F 3 "" H 1400 4675 50  0000 C CNN
	1    1400 4675
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:C C19
U 1 1 5C5D5336
P 1675 4675
F 0 "C19" H 1700 4775 50  0000 L CNN
F 1 "0.1" H 1700 4575 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 1713 4525 50  0001 C CNN
F 3 "" H 1675 4675 50  0000 C CNN
	1    1675 4675
	1    0    0    -1  
$EndComp
Wire Wire Line
	1400 4825 1400 4925
Wire Wire Line
	1175 5600 1425 5600
Connection ~ 1400 4925
Wire Wire Line
	1175 5375 1675 5375
Wire Wire Line
	1675 5375 1675 5700
Wire Wire Line
	1425 5700 1425 5600
Connection ~ 1425 5600
Wire Wire Line
	1775 5300 1675 5300
Wire Wire Line
	1675 5300 1675 5375
Connection ~ 1675 5375
Wire Wire Line
	1400 4925 1725 4925
Wire Wire Line
	1425 5600 1725 5600
$Comp
L Chiller_control-rescue:GND #PWR043
U 1 1 5C61C952
P 1900 4525
F 0 "#PWR043" H 1900 4275 50  0001 C CNN
F 1 "GND" H 1900 4375 50  0000 C CNN
F 2 "" H 1900 4525 50  0000 C CNN
F 3 "" H 1900 4525 50  0000 C CNN
	1    1900 4525
	1    0    0    -1  
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR042
U 1 1 5C61CAAB
P 1550 6000
F 0 "#PWR042" H 1550 5750 50  0001 C CNN
F 1 "GND" H 1550 5850 50  0000 C CNN
F 2 "" H 1550 6000 50  0000 C CNN
F 3 "" H 1550 6000 50  0000 C CNN
	1    1550 6000
	1    0    0    -1  
$EndComp
Wire Wire Line
	1425 6000 1550 6000
Wire Wire Line
	1550 6000 1675 6000
Connection ~ 1550 6000
Wire Notes Line
	550  4475 2475 4475
Wire Notes Line
	2475 4475 2475 6200
Wire Notes Line
	2475 6200 550  6200
Wire Notes Line
	550  6200 550  4475
Text Notes 3050 3500 1    60   ~ 0
*
Text Notes 2775 2725 1    60   ~ 0
*
Text Notes 2575 2725 1    60   ~ 0
*
Text Notes 2125 3525 0    60   ~ 0
* Solder R6,\nR8 and R9\nif necessary
$Comp
L Chiller_control-rescue:C C21
U 1 1 5C6511ED
P 2775 3925
F 0 "C21" H 2800 4025 50  0000 L CNN
F 1 "0.1" H 2800 3825 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.99x1.00mm_HandSolder" H 2813 3775 50  0001 C CNN
F 3 "" H 2775 3925 50  0000 C CNN
	1    2775 3925
	0    1    1    0   
$EndComp
$Comp
L Chiller_control-rescue:GND #PWR044
U 1 1 5C6593E4
P 3000 3925
F 0 "#PWR044" H 3000 3675 50  0001 C CNN
F 1 "GND" H 3000 3775 50  0000 C CNN
F 2 "" H 3000 3925 50  0000 C CNN
F 3 "" H 3000 3925 50  0000 C CNN
	1    3000 3925
	1    0    0    -1  
$EndComp
Wire Notes Line
	2050 4150 3300 4150
Wire Notes Line
	3300 2200 3300 4150
Wire Notes Line
	2050 2200 2050 4150
Wire Wire Line
	3000 3925 2925 3925
Wire Wire Line
	2575 3700 2575 3925
Wire Wire Line
	2575 3925 2625 3925
Wire Wire Line
	2525 3700 2575 3700
Connection ~ 2575 3700
Connection ~ 1675 4525
Wire Wire Line
	1675 4525 1900 4525
Wire Wire Line
	1400 4525 1675 4525
Wire Wire Line
	1675 4825 1675 5150
Wire Wire Line
	1675 5200 1775 5200
Wire Wire Line
	1175 5150 1675 5150
Connection ~ 1675 5150
Wire Wire Line
	1675 5150 1675 5200
Wire Wire Line
	2150 1325 2425 1325
Wire Wire Line
	1775 5400 1725 5400
Wire Wire Line
	1725 5400 1725 5600
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J4
U 1 1 5C71D1DB
P 1975 5200
F 0 "J4" H 2025 4775 50  0000 C CNN
F 1 "DC3-10P" H 2025 4866 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x05_P2.54mm_Vertical" H 1975 5200 50  0001 C CNN
F 3 "~" H 1975 5200 50  0001 C CNN
	1    1975 5200
	1    0    0    1   
$EndComp
NoConn ~ 1775 5000
NoConn ~ 2275 5000
Wire Wire Line
	9625 4275 9625 4475
Wire Wire Line
	9625 4475 9925 4475
Wire Wire Line
	9675 5150 9675 5350
Wire Wire Line
	9675 5350 9975 5350
Wire Wire Line
	9750 5900 9750 6100
Wire Wire Line
	9750 6100 10050 6100
Wire Wire Line
	4525 1450 4825 1450
Wire Wire Line
	4525 1250 4525 1450
Text Label 1425 5600 0    60   ~ 0
ADC3
Text Label 1400 5375 0    60   ~ 0
ADC2
Text Label 1400 5150 0    60   ~ 0
ADC1
Text Label 1400 4925 0    60   ~ 0
ADC0
Wire Wire Line
	1775 5100 1725 5100
Wire Wire Line
	1725 5100 1725 4925
Wire Wire Line
	2275 5475 2275 5400
Wire Wire Line
	2275 5400 2275 5300
Connection ~ 2275 5400
Wire Wire Line
	2275 5300 2275 5200
Connection ~ 2275 5300
Wire Wire Line
	2275 5200 2275 5100
Connection ~ 2275 5200
$Comp
L Connector_Generic:Conn_02x05_Odd_Even J3
U 1 1 5C81FA0E
P 1375 7150
F 0 "J3" H 1425 7567 50  0000 C CNN
F 1 "DC3-10P" H 1425 7476 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x05_P2.54mm_Vertical" H 1375 7150 50  0001 C CNN
F 3 "~" H 1375 7150 50  0001 C CNN
	1    1375 7150
	-1   0    0    -1  
$EndComp
$Comp
L Device:Buzzer BZ1
U 1 1 5C83133C
P 5650 1150
F 0 "BZ1" H 5725 1350 50  0000 C CNN
F 1 "Buzzer" H 5654 916 50  0000 C CNN
F 2 "Buzzer_Beeper:Buzzer_12x9.5RM7.6" V 5625 1250 50  0001 C CNN
F 3 "~" V 5625 1250 50  0001 C CNN
	1    5650 1150
	1    0    0    1   
$EndComp
Wire Wire Line
	5125 1050 5550 1050
$Comp
L Chiller_control-rescue:+3.3V #PWR05
U 1 1 5C84A91E
P 5550 1375
F 0 "#PWR05" H 5550 1225 50  0001 C CNN
F 1 "+3.3V" H 5550 1515 50  0000 C CNN
F 2 "" H 5550 1375 50  0000 C CNN
F 3 "" H 5550 1375 50  0000 C CNN
	1    5550 1375
	-1   0    0    1   
$EndComp
Wire Wire Line
	5550 1250 5550 1375
Wire Notes Line
	5900 1700 5900 850 
Wire Notes Line
	4125 850  5900 850 
Wire Notes Line
	4125 1700 5900 1700
Wire Wire Line
	1075 7050 1075 7150
Connection ~ 1075 7150
Wire Wire Line
	725  7150 1075 7150
$EndSCHEMATC
