avrdude -c usbasp -p t13 -B50 -U lfuse:w:0x21:m -U hfuse:w:0xf9:m -U flash:w:Release\mt7860-t13a.hex