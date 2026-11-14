"""Prepare the 189.6-second Verdance theme for full-length native looping."""

import argparse
from array import array
import hashlib
import math
from pathlib import Path
import sys
import wave


MASTER_SHA256 = "a4c0b2eabeb913d3e7e41b585c591c14f1619252f9ec08c4c3c5bab612be8fe1"


def prepare(source, destination):
    if hashlib.sha256(source.read_bytes()).hexdigest() != MASTER_SHA256:
        raise ValueError("Expected the original decoded Verdance master")
    if destination.exists():
        raise FileExistsError(destination)
    with wave.open(str(source), "rb") as reader:
        params = reader.getparams()
        if (params.nchannels, params.sampwidth, params.framerate) != (2, 2, 48000):
            raise ValueError("Expected 48 kHz, stereo 16-bit PCM")
        samples = array("h", reader.readframes(params.nframes))
    if sys.byteorder != "little":
        samples.byteswap()
    # Four milliseconds per edge, with flat gain slopes at the joint and at unity.
    # Processing both channels together preserves their relative levels.
    edge = 192
    for frame in range(edge):
        gain = 0.5 * (1 - math.cos(math.pi * frame / (edge - 1)))
        for channel in range(params.nchannels):
            for index in (frame * 2 + channel, (params.nframes - 1 - frame) * 2 + channel):
                samples[index] = round(samples[index] * gain)
    if sys.byteorder != "little":
        samples.byteswap()
    with wave.open(str(destination), "wb") as writer:
        writer.setparams(params)
        writer.writeframes(samples.tobytes())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
