IR allsky-camera project using 5 sensors MLX90640
=================================================

When attached, udev will create symlink /dev/ir-allsky0. This is the udev rule:

```

ACTION=="add", ENV{USB_IDS}=="0483:5740", ATTRS{interface}=="?*", PROGRAM="/bin/bash -c \"ls /dev | grep $attr{interface} | wc -l \"", SYMLINK+="$attr{interface}%c", MODE="0666", GROUP="tty"

```

Protocol:

```

dn - draw nth image in ASCII
gn - get nth image 'as is' - float array of 768x4 bytes
l - list active sensors IDs
mn - show temperature map of nth image
tn - show nth image aquisition time
B - reinit BME280
E - get environment parameters (temperature etc)
G - get MLX state
R - reset device
T - print current Tms
    Debugging options:
aa - change I2C address to a (a should be non-shifted value!!!)
c - continue MLX
i0..4 - setup I2C with speed 10k, 100k, 400k, 1M or 2M (experimental!)
p - pause MLX
s - stop MLX (and start from zero @ 'c')
C - "cartoon" mode on/off (show each new image) - USB only!!!
Dn - dump MLX parameters for sensor number n
Ia addr [n] - set  device address for interactive work or (with n) change address of n'th sensor
Ir reg n - read n words from 16-bit register
Iw words - send words (hex/dec/oct/bin) to I2C
Is - scan I2C bus
Us - send string 's' to other interface


```

To call this help just print '?', 'h' or 'H' in terminal.
