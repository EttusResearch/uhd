/*
 * f15_histo_mem.v
 *
 * Histogram State storage. Basically a memory with 2 R/W ports where
 * each port can do read & write at different address at the same time
 * if those address are inteleaved (like read at odd address when writing
 * to even address).
 *
 * This allows two independent process to do READ/MODIFY/WRITE.
 *
 * Copyright (C) 2014  Ettus Corporation LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * vim: ts=4 sw=4
 */

`ifdef SIM
`default_nettype none
`endif

module f15_histo_mem #(
	parameter integer ADDR_WIDTH = 16
)(
	// Port A Read
	input  wire [ADDR_WIDTH-1:0] addr_AR,
	output reg  [8:0] data_AR,
	input  wire ena_AR,

	// Port A Write
	input  wire [ADDR_WIDTH-1:0] addr_AW,
	input  wire [8:0] data_AW,
	input  wire ena_AW,

	// Port B Read
	input  wire [ADDR_WIDTH-1:0] addr_BR,
	output reg  [8:0] data_BR,
	input  wire ena_BR,

	// Port B Write
	input  wire [ADDR_WIDTH-1:0] addr_BW,
	input  wire [8:0] data_BW,
	input  wire ena_BW,

	// Error detection
	output reg  conflict_A,
	output reg  conflict_B,

	// Common
	input  wire clk,
	input  wire rst
);

	// Signals
		// Memory banks IF
	wire [ADDR_WIDTH-2:0] even_addra, odd_addra;
	wire [ADDR_WIDTH-2:0] even_addrb, odd_addrb;
	wire [8:0] even_dia, odd_dia;
	wire [8:0] even_dib, odd_dib;
	wire [8:0] even_doa, odd_doa;
	wire [8:0] even_dob, odd_dob;
	wire even_wea, odd_wea;
	wire even_web, odd_web;
	wire even_rea, odd_rea;
	wire even_reb, odd_reb;

		// Control
	wire sel_A, sel_B;


	// Mux selection
	assign sel_A = ena_AR ? addr_AR[0] : ~addr_AW[0];
	assign sel_B = ena_BR ? addr_BR[0] : ~addr_BW[0];

	// Conflict detection
	always @(posedge clk)
	begin
		conflict_A <= !(addr_AR[0] ^ addr_AW[0]) & ena_AR & ena_AW;
		conflict_B <= !(addr_BR[0] ^ addr_BW[0]) & ena_BR & ena_BW;
	end

	// Control signals
	assign even_wea =  sel_A & ena_AW;
	assign odd_wea  = !sel_A & ena_AW;
	assign even_web =  sel_B & ena_BW;
	assign odd_web  = !sel_B & ena_BW;
	assign even_rea = !sel_A & ena_AR;
	assign odd_rea  =  sel_A & ena_AR;
	assign even_reb = !sel_B & ena_BR;
	assign odd_reb  =  sel_B & ena_BR;

	// Address path mapping
	assign even_addra = sel_A ? addr_AW[ADDR_WIDTH-1:1] : addr_AR[ADDR_WIDTH-1:1];
	assign even_addrb = sel_B ? addr_BW[ADDR_WIDTH-1:1] : addr_BR[ADDR_WIDTH-1:1];
	assign odd_addra  = sel_A ? addr_AR[ADDR_WIDTH-1:1] : addr_AW[ADDR_WIDTH-1:1];
	assign odd_addrb  = sel_B ? addr_BR[ADDR_WIDTH-1:1] : addr_BW[ADDR_WIDTH-1:1];

	// Data path mapping
	assign even_dia = data_AW;
	assign odd_dia  = data_AW;
	assign even_dib = data_BW;
	assign odd_dib  = data_BW;

	always @(posedge clk)
	begin
		data_AR <= even_doa | odd_doa;
		data_BR <= even_dob | odd_dob;
	end

	// Instanciate memory banks
	f15_histo_mem_bank #(
		.ADDR_WIDTH(ADDR_WIDTH-1)
	) mem_even (
		.addra(even_addra),
		.addrb(even_addrb),
		.dia(even_dia),
		.dib(even_dib),
		.doa(even_doa),
		.dob(even_dob),
		.wea(even_wea),
		.web(even_web),
		.rea(even_rea),
		.reb(even_reb),
		.clk(clk),
		.rst(rst)
	);

	f15_histo_mem_bank #(
		.ADDR_WIDTH(ADDR_WIDTH-1)
	) mem_odd (
		.addra(odd_addra),
		.addrb(odd_addrb),
		.dia(odd_dia),
		.dib(odd_dib),
		.doa(odd_doa),
		.dob(odd_dob),
		.wea(odd_wea),
		.web(odd_web),
		.rea(odd_rea),
		.reb(odd_reb),
		.clk(clk),
		.rst(rst)
	);

endmodule // f15_histo_mem


module f15_histo_mem_bank #(
	parameter integer ADDR_WIDTH = 15
)(
	input  wire [ADDR_WIDTH-1:0] addra,
	input  wire [ADDR_WIDTH-1:0] addrb,
	input  wire [8:0] dia,
	input  wire [8:0] dib,
	output reg  [8:0] doa,
	output reg  [8:0] dob,
	input  wire wea,
	input  wire web,
	input  wire rea,
	input  wire reb,
	input  wire clk,
	input  wire rst
);
	localparam integer N_BRAMS = 1 << (ADDR_WIDTH - 12);
	genvar i;
	integer j;

	// Signals
		// Direct RAM connections
	wire [15:0] ramb_addra;
	wire [15:0] ramb_addrb;
	wire [31:0] ramb_dia;
	wire [31:0] ramb_dib;
	wire [ 3:0] ramb_dipa;
	wire [ 3:0] ramb_dipb;
	wire [31:0] ramb_doa[0:N_BRAMS-1];
	wire [31:0] ramb_dob[0:N_BRAMS-1];
	wire [ 3:0] ramb_dopa[0:N_BRAMS-1];
	wire [ 3:0] ramb_dopb[0:N_BRAMS-1];
	wire        ramb_wea[0:N_BRAMS-1];
	wire        ramb_web[0:N_BRAMS-1];
	reg         ramb_rstdoa[0:N_BRAMS-1];
	reg         ramb_rstdob[0:N_BRAMS-1];

		// Control
	reg onehota[0:N_BRAMS-1];
	reg onehotb[0:N_BRAMS-1];

	// Map address LSB and data inputs
	assign ramb_addra = { 1'b0, addra[11:0], 3'b000 };
	assign ramb_addrb = { 1'b0, addrb[11:0], 3'b000 };

	assign ramb_dia   = { 16'h0000, dia[8:1] };
	assign ramb_dib   = { 16'h0000, dib[8:1] };
	assign ramb_dipa  = {  3'b000,  dia[0] };
	assign ramb_dipb  = {  3'b000,  dib[0] };

	// OR all the RAMB outputs
	always @*
	begin
		doa = 9'h0;
		dob = 9'h0;
		for (j=0; j<N_BRAMS; j=j+1) begin
			doa = doa | { ramb_doa[j][7:0], ramb_dopa[j][0] };
			dob = dob | { ramb_dob[j][7:0], ramb_dopb[j][0] };
		end
	end

	// Generate array
	generate
		for (i=0; i<N_BRAMS; i=i+1) begin

			// Decode address MSB to one-hot signal
			always @(addra,addrb)
			begin
				onehota[i] <= (addra[ADDR_WIDTH-1:12] == i) ? 1'b1 : 1'b0;
				onehotb[i] <= (addrb[ADDR_WIDTH-1:12] == i) ? 1'b1 : 1'b0;
			end

			// If no read, then reset the output reg to zero
			always @(posedge clk)
			begin
				ramb_rstdoa[i] <= !(onehota[i] & rea);
				ramb_rstdob[i] <= !(onehotb[i] & reb);
			end

			// Mask the write enable with decoded address
			assign ramb_wea[i] = onehota[i] & wea;
			assign ramb_web[i] = onehotb[i] & web;

			// Instantiate RAM Block
			RAMB36E1 #(
				.RDADDR_COLLISION_HWCONFIG("PERFORMANCE"),
				.SIM_COLLISION_CHECK("NONE"),
				.DOA_REG(1),
				.DOB_REG(1),
				.EN_ECC_READ("FALSE"),
				.EN_ECC_WRITE("FALSE"),
				.RAM_EXTENSION_A("NONE"),
				.RAM_EXTENSION_B("NONE"),
				.RAM_MODE("TDP"),
				.READ_WIDTH_A(9),
				.READ_WIDTH_B(9),
				.WRITE_WIDTH_A(9),
				.WRITE_WIDTH_B(9),
				.RSTREG_PRIORITY_A("RSTREG"),
				.RSTREG_PRIORITY_B("RSTREG"),
				.SIM_DEVICE("7SERIES"),
				.SRVAL_A(36'h000000000),
				.SRVAL_B(36'h000000000),
				.WRITE_MODE_A("READ_FIRST"),
				.WRITE_MODE_B("READ_FIRST")
			)
			mem_elem_I (
				.DOADO(ramb_doa[i]),
				.DOPADOP(ramb_dopa[i]),
				.DOBDO(ramb_dob[i]),
				.DOPBDOP(ramb_dopb[i]),
				.CASCADEINA(1'b0),
				.CASCADEINB(1'b0),
				.INJECTDBITERR(1'b0),
				.INJECTSBITERR(1'b0),
				.ADDRARDADDR(ramb_addra),
				.CLKARDCLK(clk),
				.ENARDEN(1'b1),
				.REGCEAREGCE(1'b1),
				.RSTRAMARSTRAM(rst),
				.RSTREGARSTREG(ramb_rstdoa[i]),
				.WEA({3'b0, ramb_wea[i]}),
				.DIADI(ramb_dia),
				.DIPADIP(ramb_dipa),
				.ADDRBWRADDR(ramb_addrb),
				.CLKBWRCLK(clk),
				.ENBWREN(1'b1),
				.REGCEB(1'b1),
				.RSTRAMB(rst),
				.RSTREGB(ramb_rstdob[i]),
				.WEBWE({7'b0, ramb_web[i]}),
				.DIBDI(ramb_dib),
				.DIPBDIP(ramb_dipb)
			);

		end
	endgenerate

endmodule // f15_histo_mem_bank
