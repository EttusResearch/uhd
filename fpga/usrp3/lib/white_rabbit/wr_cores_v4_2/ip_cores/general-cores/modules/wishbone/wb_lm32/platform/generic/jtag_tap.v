module jtag_tap(
  output tck,
  output tdi,
  input tdo,
  output capture,
  output shift,
  output e1dr,
  output update,
  output reset
);

	assign tck = 1;
	assign tdi = 1;
	assign capture = 0;
	assign shift = 0;
	assign e1dr = 0;
	assign update = 0;
	assign reset = 0;

endmodule
