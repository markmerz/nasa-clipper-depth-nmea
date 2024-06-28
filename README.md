# nasa-clipper-depth-nmea
NMEA0183 converter to NASA Clipper Depth sounder.

An affordable depth sounder, the "NASA Clipper Depth" does not have NMEA0183 output.
It has an I2C output for driving a secondary display. This project taps into
I2C output, decodes display driving protocol, creates NMEA0103 messages from
it and sends them over USB-SERIAL link for distributing with other devices.

This project is heavily inspired by https://wiki.openseamap.org/wiki/De:NASA_Clipper_Range

As of 28.06.2024, the PCB part of the project is in its final form. Arduino code is partial,
lacking actual decoding and recoding parts. I intend to install the device to the boat
and observe the actual protocol on the wire, as I feel this protocol is slightly
different between device versions.

PCB is designed to fit into a sealed plastic box I had lying around. Unfortunately, I don't
remember when and from where I got the box, so I can't provide a reference for where you could get
the same enclosure. The box measures 11.3 x 8.9 x 5.5 centimetres, made from hard grey plastic.

PCB cost about 11€ delivered into the EU, ordered from jlcpcb.com. Manufacturer
website also handles taxes and customs. DIN5 connectors were ~15€, arduino-compatible board was 
~5€, the enclosure was probably ~10€, and the rest of the parts were jellybeans from the box.

My point is that when you don't particularly like tinkering with electronics, it may be 
wiser to buy an echosounder that already has NMEA0183 or even NMEA2000 output. But when you like 
building things, this project is an excellent way to save costs, although it requires You to pay 
a little more.

