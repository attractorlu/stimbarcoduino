# stimbarcoduino
Barcode generator for synchronizing multiple recording devices

## Overview

Stimbarcoduino is an Arduino sketch that implements a "barcode generator" for synchronizing multiple recording devices, similar to the barcode generator provided by OpenEphys, but with a few improvements. When uploaded to any Arduino (or Teensy, etc.), the system will output "bar codes" at 5-s intervals that can be used to synchronize multiple recording streams by teeing the output to one digital input on each device. For instance, by connecting the output to the TTL input of a Neuropixel IMEC card and also to one of the digital inputs of a National Instruments card, the timing of the other inputs to that NI card can be referenced with great precision to the timing of the electroophysiology recorded by the Neuropixel system.

This barcode generator improves upon the version provided by OpenEphys in two ways:

* **More easily decodable codes**

  Our barcodes comprise a "start" bar that is a HIGH pulse of 10 ms duration, followed by 16 phases (LOW - HIGH - LOW - HIGH ... LOW - HIGH) that encode a 16-bit number by their length: a 5-ms phase means "LOW," a 10-ms phase means "HIGH." Because the initial HIGH pulse is always 10-ms long, the sampling frequency can be inferred from the barcode itself, so the barcode can be decoded without advance knowledge of the sampling frequency. Barcodes are emitted every 5 s, which is much longer than the pulse duration, so gaps between bar codes can be detected, e.g., as pauses lasting more than 10x the median pulse duration.

* **Pass-through of stimulus markers**

  In addition to producing barcodes, the hardware monitors a TTL input and copies any HIGH pulses on that input to the barcode output. This may be used to copy stimulus markers into the IMEC card of a Neuropixel system, which is helpful for online visualization of stimulus responses using the "triggered refresh" option in OpenEphys. Stimbarcoduino automatically suspends barcode generation for 2.5 seconds. If a stimulus marker happens to arrive during the emission of a barcode, that barcode is aborted. As a consequence, the combined stream is not suitable for perfect reconstruction of an experiment, and it is important to separately record the stimulus markers on a (NIDAQ) digital channel.

## Building the hardware

Grab your favorite basic Arduino board and enclosure.

The sketch outputs barcodes on digital pin number 4. Wire that to a BNC jack and plug in a BNC "Tee" to connect to all of your recording systems. (Of course, you can also wire it up to multiple BNC jacks to avoid the need of external Tees.)

The sketch monitors digital pin number 3 for stimulus markers. Connect it to a BNC jack to receive stimulus markers. 

***VERY IMPORTANT:*** *Connect a 10–22 kΩ resistor between pin 3 and ground, to make sure that no spurious stimulus markers are inferred when nothing else is connected to the pin.* 

## Compiling

Load the sketch in the Arduino software, specify your board type, and hit "Upload."

## Using the device

The device does not need to be connected to a computer through USB during use: All it needs is 5V power. This can come from a computer or from a wall outlet.

## Decoding the barcodes

The following piece of python code can be used to decode the bar codes. The input to the function must be the sample time stamps of the transitions (up or down) in the channel that receives the barcodes. (If you need to extract those time stamps from a time series, see the "schmitt" function in our python-daw repository.) The output will be lists of the sample times of the start of each detected code and those codes themselves. 

    import numpy as np

    def barcodes(sss):
        '''BARCODES - Decode bar codes from arduino code "stimbarcoduino"
        times, codes = BARCODES(sss) decodes bar codes from stimbarcoduino. 
        
        SSS must be the sample time stamps of up and down transitions. You do
        not have to specify which are up and which are down, but they must 
        alternate.
        
        TIMES will be the sample time stamps of the start of each bar code,
        in the same time unit as SSS.
        
        CODES will be a vector of the (16-bit) bar codes received at those 
        times.'''
        
        ds = np.diff(sss)
        ds0 = np.median(ds)
        split = np.nonzero(ds > ds0*5)[0]
        split = np.concatenate(([0], split+1, [len(sss)]), 0)
        sss = [ sss[split[k]:split[k+1]] for k in range(len(split)-1) ]
        
        times = []
        codes = []
        
        for ss in sss:
            if len(ss)==18:
                # Potential barcode
                s0 = ss[0]
                dss = np.diff(ss)
                code = 0
                onems = dss[0]/10
                thr = dss[0]*3//4
                if np.any(dss < 3*onems) or np.any(dss>12*onems):
                    print(f'Likely faulty bar code of length {len(ss)} at {ss[0]}')
                else:
                    for ds in dss[1:]:
                        code *= 2
                        if ds > thr:
                            code += 1
                    codes.append(code)
                    times.append(s0)
            elif len(ss)>5:
                print(f'Likely faulty bar code of length {len(ss)} at {ss[0]}')
        
        return times, codes
    
