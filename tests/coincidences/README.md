# Coincidence timing test

## Synopsis

This tests the coincidence timing between two separate SBCs when
driven with the same pulse from a third SBC.

## Configuration

You'll need three boards, a pile of test leads, and a breadboard (or
similar, of course).  Tie the grounds from all boards together through
the breadboard.  Run the pulse output from the "driver" board to the
breadboard, and fan out from there to the input on both "receiver"
boards.

Start the "receiver" program on both receiver boards, and only then
start the "driver" program on the driver board.  This should ensure
that both receiver boards see an identical data stream.  Optionally,
you can observe the driver and reset pulses on an oscilloscope.