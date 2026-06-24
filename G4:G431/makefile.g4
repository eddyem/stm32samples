# common include for all F0 series
# MCU FAMILY
FAMILY		?= G4
FP_FLAGS	?= -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -mlittle-endian
ASM_FLAGS	?= -mthumb -mcpu=cortex-m4 -march=armv7e-m -mtune=cortex-m4
