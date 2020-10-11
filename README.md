# IR Translator for FX Audio D802

## What Is The FX Audio D802?

The FX Audio D802 [(amazon link)](https://www.amazon.com/D802-192KHz-Digital-Remote-Amplifier/dp/B00WU6JU9Y) is a small class D amplifier manufactured by [FX Audio](http://www.szfxaudio.com/en/index.html). One nice feature is that the amp can be controlled via an included small infrared remote.

## Controlling the D802 With a Non-Learning Remote

Many devices (for example the [Chromecast with Google TV](https://store.google.com/us/product/chromecast_google_tv)) come with a remote that supports a pre-set list of devices for infrared remote control. You configure it by picking the make and model of the TV, amp, or soundbar you want to control.

Being a device from a non-mainstream manufacturer, this leaves devices like the FX Audio D802 out in the cold.

## The IR Translator/Repeater

This Arduino sketch solves this problem by enabling you to remote-control the FX Audio D802 using "foreign" remote control commands. -- In its current configuration (that I use in conjuction with the aforementioned Chromecast) it recognizes the *Volume Up*, *Volume Down* and *Mute* commands intended for a Panasonic sound bar. -- When such commands are received, the corresponding NEC command codes (that I captured with this sketch as well using the original remote that came with the amp) are emitted instead, hence enabling you to remote-control the FX Audio D802 as if it were a Panasonic sound bar.

If any other infrared command is received it is passed through from the receiver to the transmitter (so the original remote that came with the amp will still work).

## Howto

### Bill of Materials

* I used one of these [NodeMCU boards](https://www.amazon.com/gp/product/B07HF44GBT).
* For infrared receivers/transmitters I went with a pair of [these](https://www.amazon.com/gp/product/B0816P2545).
* Since the Arduino and the transmitter are inside a cabinet (where the amp is), I also used a ~5ft long ethernet cable to connect the IR receiver to the NodeMCU board when placing it outside the cabinet.

### Notes

* You want to make sure that there is NO direct line of sight between the amp and the remote you are using (ie: all commands are passed through the IR Translater/Repeater). Otherwise controlling the amp will be unreliable.
* There is a newer version fo the FX Audio D802 available (D802C) that also supports Bluetooth. I do not know whether this version of the amp uses the infrared control commands as the model that I have, so you might have to modify the sketch.


