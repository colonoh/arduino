8-bit, 16kHz, raw (headerless) PCM

https://www.pi4j.com/1.2/pins/model-b-rev2.html
https://mtm.cba.mit.edu/2021/2021-10_microcontroller-primer/fab-arduino-tiny/ATtiny_x14.gif 


## Steps to write the audio data
1. Copy all the mp3 files to a new folder called `source` in the directory as create_flash_image.py
2. Make sure `ffmpeg` is present
3. `python create_flash_image.py output.bin source`
3. Record the audio track index
4. scp .\output.bin steve@192.168.50.161:/home/steve
5. truncate -s 16M output.bin # make it fill up the entire chip 
6. flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=2048 -w output.bin
7. flashrom -p linux_spi:dev=/dev/spidev0.0,spispeed=2048 -v output.bin


Example output from the Python script:
```
Track table:
  Idx  Offset      Length      Duration   Filename
  ---  ----------  ----------  ---------  --------
    0  0x000000d6  0x00011280     4.4s    abcdelicious.raw
    1  0x00011356  0x00050b80    20.7s    abcsingwithme.raw
    2  0x00061ed6  0x00007b00     2.0s    bubbles.raw
    3  0x000699d6  0x00007080     1.8s    chimesounds.raw
    4  0x00070a56  0x00019080     6.4s    creamandsugar.raw
    5  0x00089ad6  0x00008580     2.1s    goodjob.raw
    6  0x00092056  0x0001ce00     7.4s    happyandyouknowit.raw
    7  0x000aee56  0x00006000     1.5s    hyper.raw
    8  0x000b4e56  0x0000d680     3.4s    iknewyoucouldbrewit.raw
    9  0x000c24d6  0x0000db00     3.5s    onetwothreemoresugarplease.raw
   10  0x000cffd6  0x00029d00    10.7s    onetwothreesingwithme.raw
   11  0x000f9cd6  0x00005e80     1.5s    pipessound.raw
   12  0x000ffb56  0x00035e80    13.8s    redorangeyellowgreenandblue.raw
   13  0x001359d6  0x0000be80     3.0s    sipbleepahhh.raw
   14  0x00141856  0x00007800     1.9s    someotherpipesthing.raw
   15  0x00149056  0x0000f780     4.0s    thanksalatte.raw
   16  0x001587d6  0x0000687e     1.7s    Zareyoustillthere.raw
   17  0x0015f054  0x00009cbc     2.5s    Zcoffeeinmycoffeehole - Copy (2).raw
   18  0x00168d10  0x00009cbc     2.5s    Zcoffeeinmycoffeehole - Copy (3).raw
   19  0x001729cc  0x00009cbc     2.5s    Zcoffeeinmycoffeehole - Copy (4).raw
   20  0x0017c688  0x00009cbc     2.5s    Zcoffeeinmycoffeehole - Copy.raw
   21  0x00186344  0x00009cbc     2.5s    Zcoffeeinmycoffeehole.raw
   22  0x00190000  0x00004cbc     1.2s    Zgetmad.raw
   23  0x00194cbc  0x000118d1     4.5s    Zi_like_your_style.raw
   24  0x001a658d  0x00056398    22.1s    Zradio-cropped-2.raw
   25  0x001fc925  0x0002e1f6    11.8s    Zwhenlivegivesyoulemons.raw
```

## Steps to program the MCU
1. Connect to the 3V3, GND, and UPDI pins with a programmer
2. Make sure all the Arduino settings are correct (10MHz internal)
3. Upload Using Programmer
