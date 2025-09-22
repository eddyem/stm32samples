Works with several MLX90640 sensors
=================================

When attached, udev will create symlink /dev/mlx_sensor0. This is the udev rule:

```

ACTION=="add", ENV{USB_IDS}=="0483:5740", ATTRS{interface}=="?*", PROGRAM="/bin/bash -c \"ls /dev | grep $attr{interface} | wc -l \"", SYMLINK+="$attr{interface}%c", MODE="0666", GROUP="tty"

```

Protocol:

```

aa - change I2C address to a (a should be non-shifted value!!!)
c - continue MLX
d - draw image in ASCII
i0..4 - setup I2C with speed 10k, 100k, 400k, 1M or 2M (experimental!)
p - pause MLX
r0..3 - change resolution (0 - 16bit, 3 - 19-bit)
t - show temperature map
C - "cartoon" mode on/off (show each new image)
D - dump MLX parameters
G - get MLX state
Ia addr - set  device address
Ir reg n - read n words from 16-bit register
Iw words - send words (hex/dec/oct/bin) to I2C
Is - scan I2C bus
T - print current Tms


```

To call this help just print '?', 'h' or 'H' in terminal.
