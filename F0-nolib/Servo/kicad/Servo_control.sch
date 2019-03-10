EESchema Schematic File Version 4
LIBS:Servo_control-cache
EELAYER 26 0
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
L Servo_control-rescue:GND-Chiller_control-rescue #PWR03
U 1 1 58C453C7
P 1200 1475
F 0 "#PWR03" H 1200 1225 50  0001 C CNN
F 1 "GND" H 1200 1325 50  0000 C CNN
F 2 "" H 1200 1475 50  0000 C CNN
F 3 "" H 1200 1475 50  0000 C CNN
	1    1200 1475
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:CP-Chiller_control-rescue C4
U 1 1 58C454F6
P 1600 1275
F 0 "C4" H 1625 1375 50  0000 L CNN
F 1 "47u" H 1625 1175 50  0000 L CNN
F 2 "Capacitor_Tantalum_SMD:CP_EIA-3216-18_Kemet-A_Pad1.53x1.40mm_HandSolder" H 1638 1125 50  0001 C CNN
F 3 "" H 1600 1275 50  0000 C CNN
	1    1600 1275
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+3.3V-Chiller_control-rescue #PWR07
U 1 1 58C455CB
P 1800 975
F 0 "#PWR07" H 1800 825 50  0001 C CNN
F 1 "+3.3V" H 1800 1115 50  0000 C CNN
F 2 "" H 1800 975 50  0000 C CNN
F 3 "" H 1800 975 50  0000 C CNN
	1    1800 975 
	1    0    0    -1  
$EndComp
Text Notes 600  700  0    100  ~ 0
MCU power source
Text Notes 650  1950 0    100  ~ 0
Bootloader init
Text Label 950  3050 2    60   ~ 0
NRST
Text Label 950  2100 0    60   ~ 0
BOOT0
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C3
U 1 1 590D4150
P 1550 2400
F 0 "C3" H 1575 2500 50  0000 L CNN
F 1 "0.1" H 1575 2300 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 1588 2250 50  0001 C CNN
F 3 "" H 1550 2400 50  0000 C CNN
	1    1550 2400
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C2
U 1 1 590D4832
P 1500 3250
F 0 "C2" H 1525 3350 50  0000 L CNN
F 1 "0.1" H 1525 3150 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 1538 3100 50  0001 C CNN
F 3 "" H 1500 3250 50  0000 C CNN
	1    1500 3250
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:SW_Push-Chiller_control-rescue SW2
U 1 1 5909F6B6
P 1250 2400
F 0 "SW2" H 1300 2500 50  0000 L CNN
F 1 "Boot" H 1250 2340 50  0000 C CNN
F 2 "Button_Switch_SMD:SW_SPST_FSMSM" H 1250 2600 50  0001 C CNN
F 3 "" H 1250 2600 50  0000 C CNN
	1    1250 2400
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:SW_Push-Chiller_control-rescue SW1
U 1 1 590A0134
P 1200 3250
F 0 "SW1" H 1250 3350 50  0000 L CNN
F 1 "Reset" H 1200 3190 50  0000 C CNN
F 2 "Button_Switch_SMD:SW_SPST_FSMSM" H 1200 3450 50  0001 C CNN
F 3 "" H 1200 3450 50  0000 C CNN
	1    1200 3250
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR02
U 1 1 590A03AF
P 950 2600
F 0 "#PWR02" H 950 2350 50  0001 C CNN
F 1 "GND" H 950 2450 50  0000 C CNN
F 2 "" H 950 2600 50  0000 C CNN
F 3 "" H 950 2600 50  0000 C CNN
	1    950  2600
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR05
U 1 1 590A509B
P 1350 3500
F 0 "#PWR05" H 1350 3250 50  0001 C CNN
F 1 "GND" H 1350 3350 50  0000 C CNN
F 2 "" H 1350 3500 50  0000 C CNN
F 3 "" H 1350 3500 50  0000 C CNN
	1    1350 3500
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C1
U 1 1 5A178C32
P 750 1125
F 0 "C1" H 775 1225 50  0000 L CNN
F 1 "0.1" H 775 1025 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 788 975 50  0001 C CNN
F 3 "" H 750 1125 50  0000 C CNN
	1    750  1125
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:STM32F030F4Px-Chiller_control-rescue U3
U 1 1 5A189F52
P 8775 2275
F 0 "U3" H 7175 3200 50  0000 L BNN
F 1 "STM32F030F4Px" H 10375 3200 50  0000 R BNN
F 2 "Package_SSOP:TSSOP-20_4.4x6.5mm_P0.65mm" H 10375 3150 50  0001 R TNN
F 3 "" H 8775 2275 50  0001 C CNN
	1    8775 2275
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C13
U 1 1 5A1AB970
P 8075 1025
F 0 "C13" H 8100 1125 50  0000 L CNN
F 1 "1u" H 8100 925 50  0000 L CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.50mm_HandSolder" H 8113 875 50  0001 C CNN
F 3 "" H 8075 1025 50  0000 C CNN
	1    8075 1025
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR025
U 1 1 5A1B3C28
P 8075 1175
F 0 "#PWR025" H 8075 925 50  0001 C CNN
F 1 "GND" H 8075 1025 50  0000 C CNN
F 2 "" H 8075 1175 50  0000 C CNN
F 3 "" H 8075 1175 50  0000 C CNN
	1    8075 1175
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR026
U 1 1 5A1B4A11
P 8775 3225
F 0 "#PWR026" H 8775 2975 50  0001 C CNN
F 1 "GND" H 8775 3075 50  0000 C CNN
F 2 "" H 8775 3225 50  0000 C CNN
F 3 "" H 8775 3225 50  0000 C CNN
	1    8775 3225
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+3.3V-Chiller_control-rescue #PWR024
U 1 1 5A1B5A75
P 8075 825
F 0 "#PWR024" H 8075 675 50  0001 C CNN
F 1 "+3.3V" H 8075 965 50  0000 C CNN
F 2 "" H 8075 825 50  0000 C CNN
F 3 "" H 8075 825 50  0000 C CNN
	1    8075 825 
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+3.3V-Chiller_control-rescue #PWR04
U 1 1 590A1E6C
P 1250 2650
F 0 "#PWR04" H 1250 2500 50  0001 C CNN
F 1 "+3.3V" H 1250 2790 50  0000 C CNN
F 2 "" H 1250 2650 50  0000 C CNN
F 3 "" H 1250 2650 50  0000 C CNN
	1    1250 2650
	-1   0    0    1   
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR06
U 1 1 5A283BCF
P 1550 2600
F 0 "#PWR06" H 1550 2350 50  0001 C CNN
F 1 "GND" H 1550 2450 50  0000 C CNN
F 2 "" H 1550 2600 50  0000 C CNN
F 3 "" H 1550 2600 50  0000 C CNN
	1    1550 2600
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:LM1117-3.3-Chiller_control-rescue U1
U 1 1 5A2588E7
P 1200 975
F 0 "U1" H 1200 1175 50  0000 C CNN
F 1 "LM1117-3.3" H 950 1100 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-223" H 1200 975 50  0001 C CNN
F 3 "" H 1200 975 50  0001 C CNN
	1    1200 975 
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C5
U 1 1 5BEE1D09
P 1850 1225
F 0 "C5" H 1875 1325 50  0000 L CNN
F 1 "0.1" H 1875 1125 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 1888 1075 50  0001 C CNN
F 3 "" H 1850 1225 50  0000 C CNN
	1    1850 1225
	1    0    0    -1  
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
L Servo_control-rescue:L-Chiller_control-rescue L1
U 1 1 5BEE7949
P 8775 1025
F 0 "L1" V 8725 1025 50  0000 C CNN
F 1 "BMBA 0.1mH" V 8850 1025 50  0000 C CNN
F 2 "Inductor_SMD:L_0805_2012Metric_Pad1.15x1.50mm_HandSolder" H 8775 1025 50  0001 C CNN
F 3 "" H 8775 1025 50  0001 C CNN
	1    8775 1025
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C14
U 1 1 5BEE8065
P 9450 1125
F 0 "C14" H 9475 1225 50  0000 L CNN
F 1 "10u" H 9475 1025 50  0000 L CNN
F 2 "Capacitor_SMD:C_1206_3216Metric_Pad1.24x1.80mm_HandSolder" H 9488 975 50  0001 C CNN
F 3 "" H 9450 1125 50  0001 C CNN
	1    9450 1125
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR028
U 1 1 5BEE83C2
P 9675 975
F 0 "#PWR028" H 9675 725 50  0001 C CNN
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
Text Label 10475 2275 0    60   ~ 0
TIM3_CH1
Text Notes 6600 2775 2    60   ~ 0
PWM
Text Notes 11150 2275 2    60   ~ 0
PWM
Text Notes 11150 2375 2    60   ~ 0
PWM
Text Label 7075 2475 2    60   ~ 0
DigOut0
Text Notes 6700 2450 2    60   ~ 0
Alarm
Text Notes 9575 3725 2    60   ~ 0
PWM
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R15
U 1 1 5BEEBD18
P 10075 4475
F 0 "R15" V 10155 4475 50  0000 C CNN
F 1 "10k" V 10075 4475 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 10005 4475 50  0001 C CNN
F 3 "" H 10075 4475 50  0001 C CNN
	1    10075 4475
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R12
U 1 1 5BEEBD1E
P 9775 4275
F 0 "R12" V 9855 4275 50  0000 C CNN
F 1 "510" V 9775 4275 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 9705 4275 50  0001 C CNN
F 3 "" H 9775 4275 50  0001 C CNN
	1    9775 4275
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:Q_NMOS_GSD-Chiller_control-rescue Q3
U 1 1 5BEEBD24
P 10125 4275
F 0 "Q3" H 10325 4325 50  0000 L CNN
F 1 "2N7002" H 10325 4225 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 10325 4375 50  0001 C CNN
F 3 "" H 10125 4275 50  0001 C CNN
	1    10125 4275
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR029
U 1 1 5BEEBD2B
P 10225 4500
F 0 "#PWR029" H 10225 4250 50  0001 C CNN
F 1 "GND" H 10225 4350 50  0000 C CNN
F 2 "" H 10225 4500 50  0000 C CNN
F 3 "" H 10225 4500 50  0000 C CNN
	1    10225 4500
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR030
U 1 1 5BEEC947
P 10275 5375
F 0 "#PWR030" H 10275 5125 50  0001 C CNN
F 1 "GND" H 10275 5225 50  0000 C CNN
F 2 "" H 10275 5375 50  0000 C CNN
F 3 "" H 10275 5375 50  0000 C CNN
	1    10275 5375
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR031
U 1 1 5BEED56A
P 10350 6125
F 0 "#PWR031" H 10350 5875 50  0001 C CNN
F 1 "GND" H 10350 5975 50  0000 C CNN
F 2 "" H 10350 6125 50  0000 C CNN
F 3 "" H 10350 6125 50  0000 C CNN
	1    10350 6125
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+3.3VADC-Chiller_control-rescue #PWR027
U 1 1 5BEF64D3
P 9100 1275
F 0 "#PWR027" H 9250 1225 50  0001 C CNN
F 1 "+3.3VADC" H 9100 1375 50  0000 C CNN
F 2 "" H 9100 1275 50  0001 C CNN
F 3 "" H 9100 1275 50  0001 C CNN
	1    9100 1275
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:PWR_FLAG-Chiller_control-rescue #FLG01
U 1 1 5BF0C65D
P 9325 1275
F 0 "#FLG01" H 9325 1370 50  0001 C CNN
F 1 "PWR_FLAG" H 9325 1455 50  0001 C CNN
F 2 "" H 9325 1275 50  0000 C CNN
F 3 "" H 9325 1275 50  0000 C CNN
	1    9325 1275
	1    0    0    -1  
$EndComp
Wire Wire Line
	950  2600 950  2550
Wire Wire Line
	950  2100 950  2200
Wire Wire Line
	1550 2200 1550 2250
Wire Wire Line
	950  2200 1250 2200
Connection ~ 1250 2200
Connection ~ 950  2200
Wire Wire Line
	1550 2600 1550 2550
Wire Wire Line
	1500 3050 1500 3100
Connection ~ 1200 3050
Wire Wire Line
	1200 3450 1350 3450
Wire Wire Line
	1500 3450 1500 3400
Wire Wire Line
	1350 3500 1350 3450
Connection ~ 1350 3450
Wire Notes Line
	550  1800 550  3750
Wire Notes Line
	550  3750 1900 3750
Wire Notes Line
	1900 3750 1900 1800
Wire Notes Line
	1900 1800 550  1800
Wire Wire Line
	1500 975  1600 975 
Connection ~ 750  975 
Wire Wire Line
	8675 875  8675 1275
Wire Wire Line
	8775 1175 8775 1275
Wire Wire Line
	8775 3225 8775 3175
Wire Wire Line
	8075 825  8075 875 
Wire Wire Line
	1600 975  1600 1125
Connection ~ 1600 975 
Wire Wire Line
	8075 875  8675 875 
Wire Wire Line
	1200 1275 1200 1425
Wire Wire Line
	750  1425 1200 1425
Connection ~ 1200 1425
Wire Wire Line
	750  1275 750  1425
Connection ~ 8075 875 
Wire Wire Line
	1250 2650 1250 2600
Wire Wire Line
	950  3050 1200 3050
Wire Wire Line
	1850 1425 1850 1375
Connection ~ 1600 1425
Wire Wire Line
	1850 975  1850 1075
Connection ~ 1800 975 
Connection ~ 8675 875 
Wire Wire Line
	9675 975  9450 975 
Wire Wire Line
	8775 1275 9100 1275
Connection ~ 8775 1275
Connection ~ 10225 4475
Wire Wire Line
	10225 4500 10225 4475
Wire Wire Line
	10275 5375 10275 5350
Wire Wire Line
	10350 6125 10350 6100
Connection ~ 9100 1275
Connection ~ 9325 1275
Wire Notes Line
	9075 3575 11050 3575
Wire Notes Line
	11050 3575 11050 6350
Wire Notes Line
	11050 6350 9075 6350
Wire Notes Line
	9075 6350 9075 3575
Wire Wire Line
	1250 2200 1550 2200
Wire Wire Line
	950  2200 950  2250
Wire Wire Line
	1200 3050 1500 3050
Wire Wire Line
	1350 3450 1500 3450
Wire Wire Line
	750  975  900  975 
Wire Wire Line
	1600 975  1800 975 
Wire Wire Line
	1200 1425 1200 1475
Wire Wire Line
	1200 1425 1600 1425
Wire Wire Line
	1600 1425 1850 1425
Wire Wire Line
	1800 975  1850 975 
Wire Wire Line
	8675 875  8775 875 
Wire Wire Line
	9100 1275 9325 1275
Wire Wire Line
	9325 1275 9450 1275
Wire Notes Line
	2125 550  2125 1400
Wire Wire Line
	3125 1175 3125 1150
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR016
U 1 1 5BEE8828
P 3125 1175
F 0 "#PWR016" H 3125 925 50  0001 C CNN
F 1 "GND" H 3125 1025 50  0000 C CNN
F 2 "" H 3125 1175 50  0000 C CNN
F 3 "" H 3125 1175 50  0000 C CNN
	1    3125 1175
	1    0    0    -1  
$EndComp
Text Notes 2950 675  2    60   ~ 0
Alarm Buzzer
$Comp
L Servo_control-rescue:Q_NMOS_GSD-Chiller_control-rescue Q2
U 1 1 5BEEB585
P 4900 1075
F 0 "Q2" H 5100 1125 50  0000 L CNN
F 1 "SI2300" H 5100 1025 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 5100 1175 50  0001 C CNN
F 3 "" H 4900 1075 50  0001 C CNN
	1    4900 1075
	1    0    0    -1  
$EndComp
Text Label 2525 950  2    60   ~ 0
DigOut0
$Comp
L Servo_control-rescue:+5V-power #PWR01
U 1 1 5C56CC29
P 750 975
F 0 "#PWR01" H 750 825 50  0001 C CNN
F 1 "+5V" H 765 1148 50  0000 C CNN
F 2 "" H 750 975 50  0001 C CNN
F 3 "" H 750 975 50  0001 C CNN
	1    750  975 
	1    0    0    -1  
$EndComp
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
	2525 1150 2825 1150
Wire Wire Line
	2525 950  2525 1150
$Comp
L Device:Buzzer BZ1
U 1 1 5C83133C
P 3650 850
F 0 "BZ1" H 3725 1050 50  0000 C CNN
F 1 "Buzzer" H 3654 616 50  0000 C CNN
F 2 "Buzzer_Beeper:Buzzer_12x9.5RM7.6" V 3625 950 50  0001 C CNN
F 3 "~" V 3625 950 50  0001 C CNN
	1    3650 850 
	1    0    0    1   
$EndComp
Wire Wire Line
	3125 750  3550 750 
$Comp
L Servo_control-rescue:+3.3V-Chiller_control-rescue #PWR017
U 1 1 5C84A91E
P 3550 1075
F 0 "#PWR017" H 3550 925 50  0001 C CNN
F 1 "+3.3V" H 3550 1215 50  0000 C CNN
F 2 "" H 3550 1075 50  0000 C CNN
F 3 "" H 3550 1075 50  0000 C CNN
	1    3550 1075
	-1   0    0    1   
$EndComp
Wire Wire Line
	3550 950  3550 1075
Wire Notes Line
	3900 1400 3900 550 
Wire Notes Line
	2125 550  3900 550 
Wire Notes Line
	2125 1400 3900 1400
$Comp
L Servo_control-rescue:Q_NMOS_GSD-Chiller_control-rescue Q4
U 1 1 5C8882AE
P 10175 5150
F 0 "Q4" H 10375 5200 50  0000 L CNN
F 1 "2N7002" H 10375 5100 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 10375 5250 50  0001 C CNN
F 3 "" H 10175 5150 50  0001 C CNN
	1    10175 5150
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:Q_NMOS_GSD-Chiller_control-rescue Q5
U 1 1 5C888320
P 10250 5900
F 0 "Q5" H 10450 5950 50  0000 L CNN
F 1 "2N7002" H 10450 5850 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 10450 6000 50  0001 C CNN
F 3 "" H 10250 5900 50  0001 C CNN
	1    10250 5900
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J7
U 1 1 5C88A162
P 10850 3975
F 0 "J7" H 10825 4200 50  0000 L CNN
F 1 "Servo1" H 10750 3775 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 10850 3975 50  0001 C CNN
F 3 "~" H 10850 3975 50  0001 C CNN
	1    10850 3975
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J8
U 1 1 5C83F212
P 10850 4850
F 0 "J8" H 10825 5075 50  0000 L CNN
F 1 "Servo2" H 10750 4650 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 10850 4850 50  0001 C CNN
F 3 "~" H 10850 4850 50  0001 C CNN
	1    10850 4850
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J9
U 1 1 5C83F4E9
P 10850 5600
F 0 "J9" H 10825 5825 50  0000 L CNN
F 1 "Servo3" H 10750 5400 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 10850 5600 50  0001 C CNN
F 3 "~" H 10850 5600 50  0001 C CNN
	1    10850 5600
	1    0    0    -1  
$EndComp
Wire Wire Line
	10350 5700 10650 5700
Wire Wire Line
	10275 4950 10650 4950
Wire Wire Line
	10225 4075 10650 4075
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR035
U 1 1 5C84362B
P 10650 3825
F 0 "#PWR035" H 10650 3575 50  0001 C CNN
F 1 "GND" H 10650 3675 50  0000 C CNN
F 2 "" H 10650 3825 50  0000 C CNN
F 3 "" H 10650 3825 50  0000 C CNN
	1    10650 3825
	-1   0    0    1   
$EndComp
Wire Wire Line
	10650 3875 10650 3825
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR036
U 1 1 5C8444DD
P 10650 4700
F 0 "#PWR036" H 10650 4450 50  0001 C CNN
F 1 "GND" H 10650 4550 50  0000 C CNN
F 2 "" H 10650 4700 50  0000 C CNN
F 3 "" H 10650 4700 50  0000 C CNN
	1    10650 4700
	-1   0    0    1   
$EndComp
Wire Wire Line
	10650 4700 10650 4750
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR037
U 1 1 5C8453AD
P 10650 5450
F 0 "#PWR037" H 10650 5200 50  0001 C CNN
F 1 "GND" H 10650 5300 50  0000 C CNN
F 2 "" H 10650 5450 50  0000 C CNN
F 3 "" H 10650 5450 50  0000 C CNN
	1    10650 5450
	-1   0    0    1   
$EndComp
Wire Wire Line
	10650 5450 10650 5500
$Comp
L Servo_control-rescue:+5V-power #PWR032
U 1 1 5C846BFE
P 10425 3975
F 0 "#PWR032" H 10425 3825 50  0001 C CNN
F 1 "+5V" H 10440 4148 50  0000 C CNN
F 2 "" H 10425 3975 50  0001 C CNN
F 3 "" H 10425 3975 50  0001 C CNN
	1    10425 3975
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+5V-power #PWR033
U 1 1 5C847311
P 10425 4850
F 0 "#PWR033" H 10425 4700 50  0001 C CNN
F 1 "+5V" H 10440 5023 50  0000 C CNN
F 2 "" H 10425 4850 50  0001 C CNN
F 3 "" H 10425 4850 50  0001 C CNN
	1    10425 4850
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+5V-power #PWR034
U 1 1 5C84751A
P 10450 5600
F 0 "#PWR034" H 10450 5450 50  0001 C CNN
F 1 "+5V" H 10465 5773 50  0000 C CNN
F 2 "" H 10450 5600 50  0001 C CNN
F 3 "" H 10450 5600 50  0001 C CNN
	1    10450 5600
	1    0    0    -1  
$EndComp
Wire Wire Line
	10450 5600 10650 5600
Wire Wire Line
	10425 4850 10650 4850
Wire Wire Line
	10425 3975 10650 3975
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R13
U 1 1 5C84B03F
P 9825 5150
F 0 "R13" V 9905 5150 50  0000 C CNN
F 1 "510" V 9825 5150 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 9755 5150 50  0001 C CNN
F 3 "" H 9825 5150 50  0001 C CNN
	1    9825 5150
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R14
U 1 1 5C84B0D1
P 9900 5900
F 0 "R14" V 9980 5900 50  0000 C CNN
F 1 "510" V 9900 5900 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 9830 5900 50  0001 C CNN
F 3 "" H 9900 5900 50  0001 C CNN
	1    9900 5900
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R16
U 1 1 5C84B278
P 10125 5350
F 0 "R16" V 10205 5350 50  0000 C CNN
F 1 "10k" V 10125 5350 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 10055 5350 50  0001 C CNN
F 3 "" H 10125 5350 50  0001 C CNN
	1    10125 5350
	0    1    1    0   
$EndComp
Connection ~ 10275 5350
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R17
U 1 1 5C84B3BD
P 10200 6100
F 0 "R17" V 10280 6100 50  0000 C CNN
F 1 "10k" V 10200 6100 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 10130 6100 50  0001 C CNN
F 3 "" H 10200 6100 50  0001 C CNN
	1    10200 6100
	0    1    1    0   
$EndComp
Connection ~ 10350 6100
Text Label 10475 2375 0    60   ~ 0
TIM3_CH2
Text Label 9625 4275 2    60   ~ 0
TIM3_CH1
Text Label 9675 5150 2    60   ~ 0
TIM3_CH2
Text Label 7075 2775 2    60   ~ 0
TIM3_CH4
Text Label 9750 5900 2    60   ~ 0
TIM3_CH4
Text Label 3650 2325 0    60   ~ 0
ADC0
Text Label 3650 2475 0    60   ~ 0
ADC1
Text Label 3650 2625 0    60   ~ 0
ADC2
Text Label 3650 2775 0    60   ~ 0
ADC3
$Comp
L Power_Protection:SP0504BAHT D2
U 1 1 5C8580E1
P 3050 2075
F 0 "D2" H 2845 2029 50  0000 R CNN
F 1 "SP0504BAHT" H 2845 2120 50  0000 R CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 3350 2025 50  0001 L CNN
F 3 "http://www.littelfuse.com/~/media/files/littelfuse/technical%20resources/documents/data%20sheets/sp05xxba.pdf" H 3175 2200 50  0001 C CNN
	1    3050 2075
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x06 J2
U 1 1 5C85A71E
P 2100 2600
F 0 "J2" H 2020 2075 50  0000 C CNN
F 1 "ADC_in" H 2020 2166 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical" H 2100 2600 50  0001 C CNN
F 3 "~" H 2100 2600 50  0001 C CNN
	1    2100 2600
	-1   0    0    1   
$EndComp
$Comp
L Device:R R3
U 1 1 5C85BD06
P 2550 2475
F 0 "R3" V 2475 2475 50  0000 C CNN
F 1 "220" V 2550 2475 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 2480 2475 50  0001 C CNN
F 3 "" H 2550 2475 50  0001 C CNN
	1    2550 2475
	0    1    1    0   
$EndComp
$Comp
L Device:R R4
U 1 1 5C85C1A1
P 2550 2625
F 0 "R4" V 2475 2625 50  0000 C CNN
F 1 "220" V 2550 2625 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 2480 2625 50  0001 C CNN
F 3 "" H 2550 2625 50  0001 C CNN
	1    2550 2625
	0    1    1    0   
$EndComp
$Comp
L Device:R R5
U 1 1 5C85C24A
P 2550 2775
F 0 "R5" V 2475 2775 50  0000 C CNN
F 1 "220" V 2550 2775 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 2480 2775 50  0001 C CNN
F 3 "" H 2550 2775 50  0001 C CNN
	1    2550 2775
	0    1    1    0   
$EndComp
$Comp
L Device:R R2
U 1 1 5C85C2A8
P 2550 2325
F 0 "R2" V 2475 2325 50  0000 C CNN
F 1 "220" V 2550 2325 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 2480 2325 50  0001 C CNN
F 3 "" H 2550 2325 50  0001 C CNN
	1    2550 2325
	0    1    1    0   
$EndComp
Wire Wire Line
	2300 2700 2300 2775
Wire Wire Line
	2300 2775 2400 2775
Wire Wire Line
	2300 2600 2400 2600
Wire Wire Line
	2400 2600 2400 2625
Wire Wire Line
	2300 2500 2400 2500
Wire Wire Line
	2400 2500 2400 2475
Wire Wire Line
	2300 2400 2400 2400
Wire Wire Line
	2400 2400 2400 2325
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C8
U 1 1 5C86267B
P 2775 2975
F 0 "C8" H 2800 3075 50  0000 L CNN
F 1 "0.1" H 2800 2875 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 2813 2825 50  0001 C CNN
F 3 "" H 2775 2975 50  0000 C CNN
	1    2775 2975
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C10
U 1 1 5C862E72
P 2975 2975
F 0 "C10" H 3000 3075 50  0000 L CNN
F 1 "0.1" H 3000 2875 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 3013 2825 50  0001 C CNN
F 3 "" H 2975 2975 50  0000 C CNN
	1    2975 2975
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C11
U 1 1 5C862EDE
P 3175 2975
F 0 "C11" H 3200 3075 50  0000 L CNN
F 1 "0.1" H 3200 2875 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 3213 2825 50  0001 C CNN
F 3 "" H 3175 2975 50  0000 C CNN
	1    3175 2975
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C12
U 1 1 5C864D09
P 3375 2975
F 0 "C12" H 3400 3075 50  0000 L CNN
F 1 "0.1" H 3400 2875 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 3413 2825 50  0001 C CNN
F 3 "" H 3375 2975 50  0000 C CNN
	1    3375 2975
	1    0    0    -1  
$EndComp
Wire Wire Line
	3375 3125 3175 3125
Wire Wire Line
	3175 3125 2975 3125
Connection ~ 3175 3125
Wire Wire Line
	2975 3125 2775 3125
Connection ~ 2975 3125
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR014
U 1 1 5C869325
P 2725 3125
F 0 "#PWR014" H 2725 2875 50  0001 C CNN
F 1 "GND" H 2725 2975 50  0000 C CNN
F 2 "" H 2725 3125 50  0000 C CNN
F 3 "" H 2725 3125 50  0000 C CNN
	1    2725 3125
	1    0    0    -1  
$EndComp
Wire Wire Line
	2725 3125 2775 3125
Connection ~ 2775 3125
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR013
U 1 1 5C86AADA
P 2675 1875
F 0 "#PWR013" H 2675 1625 50  0001 C CNN
F 1 "GND" H 2675 1725 50  0000 C CNN
F 2 "" H 2675 1875 50  0000 C CNN
F 3 "" H 2675 1875 50  0000 C CNN
	1    2675 1875
	1    0    0    -1  
$EndComp
Wire Wire Line
	2675 1875 3050 1875
Wire Wire Line
	2700 2325 3150 2325
Wire Wire Line
	2700 2475 3050 2475
Wire Wire Line
	2700 2625 2950 2625
Wire Wire Line
	2700 2775 2775 2775
Wire Wire Line
	2775 2825 2775 2775
Connection ~ 2775 2775
Wire Wire Line
	2775 2775 2850 2775
Wire Wire Line
	2975 2825 2975 2625
Connection ~ 2975 2625
Wire Wire Line
	2975 2625 3650 2625
Wire Wire Line
	3175 2825 3175 2475
Connection ~ 3175 2475
Wire Wire Line
	3175 2475 3650 2475
Wire Wire Line
	3375 2825 3375 2325
Connection ~ 3375 2325
Wire Wire Line
	3375 2325 3650 2325
Wire Wire Line
	3150 2275 3150 2325
Connection ~ 3150 2325
Wire Wire Line
	3150 2325 3375 2325
Wire Wire Line
	3050 2275 3050 2475
Connection ~ 3050 2475
Wire Wire Line
	3050 2475 3175 2475
Wire Wire Line
	2950 2275 2950 2625
Connection ~ 2950 2625
Wire Wire Line
	2950 2625 2975 2625
Wire Wire Line
	2850 2275 2850 2775
Connection ~ 2850 2775
Wire Wire Line
	2850 2775 3650 2775
Wire Notes Line
	2000 1800 3925 1800
Wire Notes Line
	3925 1800 3925 3325
Wire Notes Line
	3925 3325 2000 3325
Wire Notes Line
	2000 3325 2000 1800
$Comp
L ch34x:CH340G U2
U 1 1 5C891E7A
P 3400 4550
F 0 "U2" H 3400 5147 60  0000 C CNN
F 1 "CH340G" H 3400 5041 60  0000 C CNN
F 2 "Package_SOIC:SOIC-16_3.9x9.9mm_P1.27mm" H 3400 5041 60  0001 C CNN
F 3 "" H 3500 4350 60  0000 C CNN
	1    3400 4550
	1    0    0    -1  
$EndComp
NoConn ~ 3850 4900
NoConn ~ 3850 4800
NoConn ~ 3850 4700
NoConn ~ 3850 4600
NoConn ~ 3850 4500
NoConn ~ 3850 4400
NoConn ~ 3850 4300
$Comp
L Servo_control-rescue:+3.3V-Chiller_control-rescue #PWR018
U 1 1 5C8ACD5C
P 3850 4150
F 0 "#PWR018" H 3850 4000 50  0001 C CNN
F 1 "+3.3V" H 3850 4290 50  0000 C CNN
F 2 "" H 3850 4150 50  0000 C CNN
F 3 "" H 3850 4150 50  0000 C CNN
	1    3850 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	3850 4150 3850 4200
$Comp
L Servo_control-rescue:C-Chiller_control-rescue C6
U 1 1 5C8AFE6B
P 2400 4350
F 0 "C6" H 2425 4450 50  0000 L CNN
F 1 "0.1" H 2425 4250 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 2438 4200 50  0001 C CNN
F 3 "" H 2400 4350 50  0000 C CNN
	1    2400 4350
	-1   0    0    1   
$EndComp
Text Label 2950 4400 2    60   ~ 0
USART_Tx
Text Label 2950 4300 2    60   ~ 0
USART_Rx
Wire Wire Line
	2400 4200 2875 4200
Wire Wire Line
	2950 4500 2400 4500
Wire Wire Line
	2950 4700 2400 4700
Wire Wire Line
	2400 4700 2400 4800
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR09
U 1 1 5C8CA4F4
P 2250 5000
F 0 "#PWR09" H 2250 4750 50  0001 C CNN
F 1 "GND" H 2250 4850 50  0000 C CNN
F 2 "" H 2250 5000 50  0000 C CNN
F 3 "" H 2250 5000 50  0000 C CNN
	1    2250 5000
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR015
U 1 1 5C8CA557
P 2875 4150
F 0 "#PWR015" H 2875 3900 50  0001 C CNN
F 1 "GND" H 2875 4000 50  0000 C CNN
F 2 "" H 2875 4150 50  0000 C CNN
F 3 "" H 2875 4150 50  0000 C CNN
	1    2875 4150
	-1   0    0    1   
$EndComp
Wire Wire Line
	2875 4150 2875 4200
Connection ~ 2875 4200
Wire Wire Line
	2875 4200 2950 4200
$Comp
L Device:Crystal Y1
U 1 1 5C8CDA20
P 2750 5075
F 0 "Y1" H 2950 5150 50  0000 C CNN
F 1 "12MHz" H 2750 5252 50  0000 C CNN
F 2 "Crystal:Crystal_HC49-U_Vertical" H 2750 5075 50  0001 C CNN
F 3 "" H 2750 5075 50  0001 C CNN
	1    2750 5075
	1    0    0    -1  
$EndComp
$Comp
L Device:C C7
U 1 1 5C8CDF63
P 2600 5225
F 0 "C7" H 2800 5275 50  0000 R CNN
F 1 "22p" H 2825 5175 50  0000 R CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 2638 5075 50  0001 C CNN
F 3 "" H 2600 5225 50  0001 C CNN
	1    2600 5225
	-1   0    0    -1  
$EndComp
$Comp
L Device:C C9
U 1 1 5C8CE285
P 2900 5225
F 0 "C9" H 3015 5271 50  0000 L CNN
F 1 "22p" H 3015 5180 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad0.84x1.00mm_HandSolder" H 2938 5075 50  0001 C CNN
F 3 "" H 2900 5225 50  0001 C CNN
	1    2900 5225
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 5375 2900 5375
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR012
U 1 1 5C8D4DFD
P 2575 5375
F 0 "#PWR012" H 2575 5125 50  0001 C CNN
F 1 "GND" H 2575 5225 50  0000 C CNN
F 2 "" H 2575 5375 50  0000 C CNN
F 3 "" H 2575 5375 50  0000 C CNN
	1    2575 5375
	1    0    0    -1  
$EndComp
Wire Wire Line
	2575 5375 2600 5375
Connection ~ 2600 5375
Wire Wire Line
	2900 5075 2900 4900
Wire Wire Line
	2900 4900 2950 4900
Connection ~ 2900 5075
Wire Wire Line
	2950 4800 2600 4800
Wire Wire Line
	2600 4800 2600 5075
Connection ~ 2600 5075
Wire Wire Line
	1150 4450 1150 4400
$Comp
L Servo_control-rescue:+5V-power #PWR08
U 1 1 5C8E7AF9
P 2175 4275
F 0 "#PWR08" H 2175 4125 50  0001 C CNN
F 1 "+5V" H 2190 4448 50  0000 C CNN
F 2 "" H 2175 4275 50  0001 C CNN
F 3 "" H 2175 4275 50  0001 C CNN
	1    2175 4275
	1    0    0    -1  
$EndComp
Connection ~ 850  5050
Wire Notes Line
	550  550  2050 550 
Wire Notes Line
	2050 550  2050 1700
Wire Notes Line
	2050 1700 550  1700
Wire Notes Line
	550  1700 550  550 
NoConn ~ 1150 4850
Wire Wire Line
	750  5050 850  5050
Text Notes 2150 1975 0    100  ~ 0
ADC
Text Notes 700  4075 0    100  ~ 0
USB
Text Label 7075 2575 2    60   ~ 0
DigOut1
Text Notes 6350 2575 0    50   ~ 0
Ext. load
$Comp
L Servo_control-rescue:Q_NMOS_GSD-Chiller_control-rescue Q1
U 1 1 5C9233AA
P 3025 950
F 0 "Q1" H 3225 1000 50  0000 L CNN
F 1 "2N7002" H 3225 900 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 3225 1050 50  0001 C CNN
F 3 "" H 3025 950 50  0001 C CNN
	1    3025 950 
	1    0    0    -1  
$EndComp
Text Label 4400 1075 2    60   ~ 0
DigOut1
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R8
U 1 1 5C924EC0
P 4550 1075
F 0 "R8" V 4630 1075 50  0000 C CNN
F 1 "510" V 4550 1075 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 4480 1075 50  0001 C CNN
F 3 "" H 4550 1075 50  0001 C CNN
	1    4550 1075
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R9
U 1 1 5C924F9E
P 4550 1275
F 0 "R9" V 4630 1275 50  0000 C CNN
F 1 "10k" V 4550 1275 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 4480 1275 50  0001 C CNN
F 3 "" H 4550 1275 50  0001 C CNN
	1    4550 1275
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R6
U 1 1 5C929CAD
P 2675 950
F 0 "R6" V 2755 950 50  0000 C CNN
F 1 "510" V 2675 950 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 2605 950 50  0001 C CNN
F 3 "" H 2675 950 50  0001 C CNN
	1    2675 950 
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R7
U 1 1 5C929D7F
P 2975 1150
F 0 "R7" V 3055 1150 50  0000 C CNN
F 1 "10k" V 2975 1150 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 2905 1150 50  0001 C CNN
F 3 "" H 2975 1150 50  0001 C CNN
	1    2975 1150
	0    1    1    0   
$EndComp
Connection ~ 3125 1150
Wire Wire Line
	4400 1275 4400 1075
Wire Wire Line
	4700 1275 5000 1275
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR023
U 1 1 5C936297
P 5000 1275
F 0 "#PWR023" H 5000 1025 50  0001 C CNN
F 1 "GND" H 5000 1125 50  0000 C CNN
F 2 "" H 5000 1275 50  0000 C CNN
F 3 "" H 5000 1275 50  0000 C CNN
	1    5000 1275
	1    0    0    -1  
$EndComp
Connection ~ 5000 1275
$Comp
L Connector_Generic:Conn_01x02 J6
U 1 1 5C93CB05
P 5200 775
F 0 "J6" H 5279 767 50  0000 L CNN
F 1 "Ext_LED" H 5279 676 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 5200 775 50  0001 C CNN
F 3 "~" H 5200 775 50  0001 C CNN
	1    5200 775 
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+5V-power #PWR022
U 1 1 5C93D5BD
P 5000 775
F 0 "#PWR022" H 5000 625 50  0001 C CNN
F 1 "+5V" H 5015 948 50  0000 C CNN
F 2 "" H 5000 775 50  0001 C CNN
F 3 "" H 5000 775 50  0001 C CNN
	1    5000 775 
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR010
U 1 1 5C93ECB6
P 2300 2800
F 0 "#PWR010" H 2300 2550 50  0001 C CNN
F 1 "GND" H 2300 2650 50  0000 C CNN
F 2 "" H 2300 2800 50  0000 C CNN
F 3 "" H 2300 2800 50  0000 C CNN
	1    2300 2800
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:+3.3V-Chiller_control-rescue #PWR011
U 1 1 5C93F2A7
P 2350 2225
F 0 "#PWR011" H 2350 2075 50  0001 C CNN
F 1 "+3.3V" H 2350 2365 50  0000 C CNN
F 2 "" H 2350 2225 50  0000 C CNN
F 3 "" H 2350 2225 50  0000 C CNN
	1    2350 2225
	1    0    0    -1  
$EndComp
Wire Wire Line
	2300 2300 2350 2300
Wire Wire Line
	2350 2300 2350 2225
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R1
U 1 1 5C943CF3
P 950 2400
F 0 "R1" V 1030 2400 50  0000 C CNN
F 1 "10k" V 950 2400 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 880 2400 50  0001 C CNN
F 3 "" H 950 2400 50  0001 C CNN
	1    950  2400
	-1   0    0    1   
$EndComp
Wire Notes Line
	4000 550  5625 550 
Wire Notes Line
	5625 550  5625 1500
Wire Notes Line
	5625 1500 4000 1500
Wire Notes Line
	4000 550  4000 1500
$Comp
L Device:D D3
U 1 1 5C95243A
P 4900 2375
F 0 "D3" H 4900 2475 50  0000 C CNN
F 1 "SMAJ5.0" H 4900 2275 50  0000 C CNN
F 2 "Diode_SMD:D_SMA_Handsoldering" H 4900 2375 50  0001 C CNN
F 3 "" H 4900 2375 50  0001 C CNN
	1    4900 2375
	0    1    1    0   
$EndComp
$Comp
L Device:D D4
U 1 1 5C952A1B
P 5175 2375
F 0 "D4" H 5175 2475 50  0000 C CNN
F 1 "SMAJ5.0" H 5175 2275 50  0000 C CNN
F 2 "Diode_SMD:D_SMA_Handsoldering" H 5175 2375 50  0001 C CNN
F 3 "" H 5175 2375 50  0001 C CNN
	1    5175 2375
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R10
U 1 1 5C957A49
P 4650 2000
F 0 "R10" V 4730 2000 50  0000 C CNN
F 1 "1k" V 4650 2000 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 4580 2000 50  0001 C CNN
F 3 "" H 4650 2000 50  0001 C CNN
	1    4650 2000
	0    1    1    0   
$EndComp
$Comp
L Servo_control-rescue:R-Chiller_control-rescue R11
U 1 1 5C957B70
P 4650 2175
F 0 "R11" V 4730 2175 50  0000 C CNN
F 1 "1k" V 4650 2175 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.50mm_HandSolder" V 4580 2175 50  0001 C CNN
F 3 "" H 4650 2175 50  0001 C CNN
	1    4650 2175
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x03 J5
U 1 1 5C9581BA
P 4200 2175
F 0 "J5" H 4120 1850 50  0000 C CNN
F 1 "Conn_01x03" H 4120 1941 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 4200 2175 50  0001 C CNN
F 3 "~" H 4200 2175 50  0001 C CNN
	1    4200 2175
	-1   0    0    1   
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR021
U 1 1 5C95874A
P 4400 2275
F 0 "#PWR021" H 4400 2025 50  0001 C CNN
F 1 "GND" H 4400 2125 50  0000 C CNN
F 2 "" H 4400 2275 50  0000 C CNN
F 3 "" H 4400 2275 50  0000 C CNN
	1    4400 2275
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 2175 4500 2175
Wire Wire Line
	4400 2075 4500 2075
Wire Wire Line
	4500 2075 4500 2000
Wire Wire Line
	4800 2175 4900 2175
Wire Wire Line
	4900 2175 4900 2225
Wire Wire Line
	4800 2000 5175 2000
Wire Wire Line
	5175 2000 5175 2225
Wire Wire Line
	4900 2175 5250 2175
Connection ~ 4900 2175
Wire Wire Line
	5175 2000 5250 2000
Connection ~ 5175 2000
Text Label 10475 2075 0    60   ~ 0
DigIn0
Text Label 10475 2175 0    60   ~ 0
DigIn1
Text Label 5250 2175 0    60   ~ 0
DigIn0
Text Label 5250 2000 0    60   ~ 0
DigIn1
Wire Notes Line
	4000 1800 5575 1800
Wire Notes Line
	5575 1800 5575 2575
Wire Notes Line
	5575 2575 4000 2575
Wire Notes Line
	4000 2575 4000 1800
Text Label 10475 2675 0    60   ~ 0
Jumper0
Text Label 10475 2775 0    60   ~ 0
Jumper1
Text Label 4325 2950 0    60   ~ 0
Jumper0
Text Label 4325 3475 0    60   ~ 0
Jumper1
$Comp
L Connector_Generic:Conn_01x02 J3
U 1 1 5C98F117
P 4125 3050
F 0 "J3" H 4045 2725 50  0000 C CNN
F 1 "Jumper0" H 4045 2816 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4125 3050 50  0001 C CNN
F 3 "~" H 4125 3050 50  0001 C CNN
	1    4125 3050
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 J4
U 1 1 5C98F3D6
P 4125 3575
F 0 "J4" H 4045 3250 50  0000 C CNN
F 1 "Jumper1" H 4045 3341 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical" H 4125 3575 50  0001 C CNN
F 3 "~" H 4125 3575 50  0001 C CNN
	1    4125 3575
	-1   0    0    1   
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR019
U 1 1 5C9901F3
P 4325 3050
F 0 "#PWR019" H 4325 2800 50  0001 C CNN
F 1 "GND" H 4325 2900 50  0000 C CNN
F 2 "" H 4325 3050 50  0000 C CNN
F 3 "" H 4325 3050 50  0000 C CNN
	1    4325 3050
	1    0    0    -1  
$EndComp
$Comp
L Servo_control-rescue:GND-Chiller_control-rescue #PWR020
U 1 1 5C9902E2
P 4325 3575
F 0 "#PWR020" H 4325 3325 50  0001 C CNN
F 1 "GND" H 4325 3425 50  0000 C CNN
F 2 "" H 4325 3575 50  0000 C CNN
F 3 "" H 4325 3575 50  0000 C CNN
	1    4325 3575
	1    0    0    -1  
$EndComp
Wire Notes Line
	4000 2650 4750 2650
Wire Notes Line
	4750 2650 4750 3800
Wire Notes Line
	4750 3800 4000 3800
Wire Notes Line
	4000 3800 4000 2650
Wire Wire Line
	4900 2525 5175 2525
Wire Wire Line
	4900 2525 4500 2525
Wire Wire Line
	4500 2525 4500 2275
Wire Wire Line
	4500 2275 4400 2275
Connection ~ 4900 2525
Connection ~ 4400 2275
$Comp
L Connector_Specialized:USB_B_Micro J1
U 1 1 5C8BC41C
P 850 4650
F 0 "J1" H 905 5117 50  0000 C CNN
F 1 "USB_B_Micro" H 905 5026 50  0000 C CNN
F 2 "Connector_USB:USB_Micro-B_Wuerth-629105150521" H 1000 4600 50  0001 C CNN
F 3 "~" H 1000 4600 50  0001 C CNN
	1    850  4650
	1    0    0    -1  
$EndComp
$Comp
L elements:USB6B1 D1
U 1 1 5C9BF24E
P 1750 4700
F 0 "D1" H 1750 5215 50  0000 C CNN
F 1 "USB6B1" H 1750 5124 50  0000 C CNN
F 2 "Package_SOIC:SOIC-8_3.9x4.9mm_P1.27mm" H 1750 5124 50  0001 C CNN
F 3 "" V 1950 4600 50  0000 C CNN
	1    1750 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1250 4400 1150 4400
Wire Wire Line
	850  5050 1250 5050
Wire Wire Line
	1250 5000 1250 5050
Wire Wire Line
	2250 4800 2400 4800
Wire Wire Line
	2250 4600 2950 4600
Wire Wire Line
	2250 4400 2250 4275
Wire Wire Line
	2250 4275 2175 4275
Wire Wire Line
	1150 4750 1250 4750
Wire Wire Line
	1250 4750 1250 4600
Wire Wire Line
	1150 4650 1200 4650
Wire Wire Line
	1200 4650 1200 4800
Wire Wire Line
	1200 4800 1250 4800
$EndSCHEMATC
