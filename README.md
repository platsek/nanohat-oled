# NanoHat OLED with YouTube stats
Resources friendly NanoHat OLED and GPIO Button Control for Armbian.

## Thank You
I used a lot of others people code/work.
I hope the following list covers them all:

- https://github.com/crouchingtigerhiddenadam/nano-hat-oled-armbian
- https://github.com/Digilent/linux-userspace-examples
- https://github.com/OnionIoT/i2c-exp-driver
- https://github.com/Narukara/SSD1306
- https://android.googlesource.com/kernel/common/+/refs/heads/upstream-master/tools/gpio
- https://www.asciiart.eu

## Getting Started

### Prerequisites
Enable i2c0:
```
sudo nano /boot/armbianEnv.txt
```
Add `i2c0` to the `overlays=` line, for example if the line appears as follows:
```
overlays=usbhost1 usbhost2
```
Then add `i2c0` with a space seperating it from the other values:
```
overlays=i2c0 usbhost1 usbhost2
```
Save these changes by pressing `ctrl+x`, `ctrl+y` and `enter` as prompted at the bottom of the screen.  
  
Reboot the system for the changes to take effect:
```
sudo reboot now
```

### Dependencies
Install the dependencies from APT:
```
sudo apt -y install \
  gcc \
  git \
  libssl-dev
```

### Get the Code
Clone from Github:
```
cd /tmp
git clone https://github.com/platsek/nanohat-oled.git
cd nanohat-oled/src
```

### Compile
Edit `yt.c`
```
nano yt.c
```
Then go to the line 79 and change/add your YT channel id and Google API key:
```
"GET https://youtube.googleapis.com/youtube/v3/channels?part=statistics&id=<channel id>&key=<API key> HTTP/1.1\r\n\r\nAccept: application/json\r\n";
```
If you don't know what Google API key and YT channel id are, read this:
```
https://hackernoon.com/how-to-fetch-statistics-from-youtube-api-using-python
```
Save the changes by pressing `ctrl+x`, `ctrl+y` and `enter` as prompted at the bottom of the screen.  
  
Compile the code:
```
gcc -o oled-app main.c i2c.c gpio.c oled.c font.c yt.c -lssl
```

### Install
Make the program directory:
```
sudo mkdir /etc/share/nanohatoled
```
Copy the program file:
```
sudo mv /tmp/nanohat-oled/oled-app /usr/share/nanohatoled/
```
Edit `rc.local`:
```
sudo nano /etc/rc.local
```
Then find the line:
```
exit 0
```
And add `/usr/bin/nice -n 10 /usr/share/nanohatoled/oled-app &` before `exit 0` so the lines look like this:
```
/usr/bin/nice -n 10 /usr/share/nanohatoled/oled-app &
exit 0
```
Save the changes by pressing `ctrl+x`, `ctrl+y` and `enter` as prompted at the bottom of the screen.  
  
Reboot the system for the changes to take effect:
```
sudo reboot now
```

## Troubleshooting

### Compatibility with BakeBit and NanoHatOLED
This does not require the FriendlyElec BakeBit or NanoHatOLED software to be installed. If this has already been installed you will need to disable it.
```
sudo nano /etc/rc.local
```
Then find the lines:
```
/usr/local/bin/oled-start
exit 0
```
And comment out the `oled-start` line by adding `#` at the start of the line, so the lines look like this:
```
# /usr/local/bin/oled-start
exit 0
```
Save these changes by pressing `ctrl+x`, `ctrl+y` and `enter` as prompted at the bottom of the screen.  
  
Reboot the system for the changes to take effect.
```
sudo reboot now
```
