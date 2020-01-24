/*
 * f15_logpwr.v
 *
 * Log Power computation
 * Take a complex 16 bits input and outputs a 16 bits estimate
 * of 2048 * log2(i^2+q^2).
 *
 * Fully-pipelined, 12 levels
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

module f15_logpwr(
	input  wire [15:0] in_real_0,
	input  wire [15:0] in_imag_0,
	output wire [15:0] out_12,
	input  wire [31:0] rng,
	input  wire [ 1:0] random_mode, /* [0] = lsb random ena, [1] = random add */
	input  wire clk,
	input  wire rst
);

	// Signals
		// Randomness control
	reg   [7:0] rng_lsb;
	wire  [6:0] opmode;

		// Power squared
	wire [47:0] dsp_pchain_3;
	wire [47:0] dsp_pout_4;

	wire  [4:0] msb_check;
	wire [31:0] pwr_4;
	reg  [31:0] pwr_5,  pwr_6,  pwr_7,  pwr_8,  pwr_9;
	reg   [4:0] log2_5, log2_6, log2_7, log2_8, log2_9;

		// LUT
	wire [15:0] lut_addr_9;
	wire [31:0] lut_do_11;

	wire        msb_9, msb_11;
	wire  [4:0] lsbs_9, lsbs_11;
	wire  [4:0] log2_11;

		// Final value
	reg  [20:0] final_12;


	// -------------
	// Power squared
	// -------------
		// Output is (in_real * in_real) + (in_imag * in_imag)
		// with possibly some random lsb filled in for in_{real,imag} and some
		// noise added to the result.

	// Randomness control
	always @(posedge clk)
		if (random_mode[0])
			rng_lsb <= rng[31:24];
		else
			rng_lsb <= 8'h00;

	assign opmode = random_mode[1] ? 7'b0110101 : 7'b0000101;

	// Square of in_real + noise
	DSP48E1 #(
		.A_INPUT("DIRECT"),
		.B_INPUT("DIRECT"),
		.USE_DPORT("FALSE"),
		.USE_MULT("MULTIPLY"),
		.AUTORESET_PATDET("NO_RESET"),
		.MASK(48'h3fffffffffff),
		.PATTERN(48'h000000000000),
		.SEL_MASK("MASK"),
		.SEL_PATTERN("PATTERN"),
		.USE_PATTERN_DETECT("NO_PATDET"),
		.ACASCREG(1),
		.ADREG(0),
		.ALUMODEREG(1),
		.AREG(1),
		.BCASCREG(1),
		.BREG(1),
		.CARRYINREG(1),
		.CARRYINSELREG(1),
		.CREG(1),
		.DREG(0),
		.INMODEREG(1),
		.MREG(1),
		.OPMODEREG(1),
		.PREG(1),
		.USE_SIMD("ONE48")
	)
	dsp_real_sq_I (
		.PCOUT(dsp_pchain_3),
		.ACIN(30'h0000),
		.BCIN(18'h000),
		.CARRYCASCIN(1'h0),
		.MULTSIGNIN(1'h0),
		.PCIN(48'h000000000000),
		.ALUMODE(4'b0000),		// Z + X + Y + CIN
		.CARRYINSEL(3'h0),
		.CEINMODE(1'b1),
		.CLK(clk),
		.INMODE(5'b00000),		// B=B2, A=A2
		.OPMODE(opmode),		// X=M1, Y=M2, Z=(random_mode[1] ? C : 0)
		.RSTINMODE(rst),
		.A({{12{in_real_0[15]}}, in_real_0, rng_lsb[7:6]}),
		.B({                     in_real_0, rng_lsb[5:4]}),
		.C({{41{1'b0}},rng[6:0]}),
		.CARRYIN(1'b0),
		.D(25'h0000),
		.CEA1(1'b0),
		.CEA2(1'b1),
		.CEAD(1'b0),
		.CEALUMODE(1'b1),
		.CEB1(1'b0),
		.CEB2(1'b1),
		.CEC(1'b1),
		.CECARRYIN(1'b1),
		.CECTRL(1'b1),
		.CED(1'b0),
		.CEM(1'b1),
		.CEP(1'b1),
		.RSTA(rst),
		.RSTALLCARRYIN(rst),
		.RSTALUMODE(rst),
		.RSTB(rst),
		.RSTC(rst),
		.RSTCTRL(rst),
		.RSTD(rst),
		.RSTM(rst),
		.RSTP(rst)
	);

	// Square of in_imag and final sum
	DSP48E1 #(
		.A_INPUT("DIRECT"),
		.B_INPUT("DIRECT"),
		.USE_DPORT("FALSE"),
		.USE_MULT("MULTIPLY"),
		.AUTORESET_PATDET("NO_RESET"),
		.MASK(48'h3fffffffffff),
		.PATTERN(48'h000000000000),
		.SEL_MASK("MASK"),
		.SEL_PATTERN("PATTERN"),
		.USE_PATTERN_DETECT("NO_PATDET"),
		.ACASCREG(1),
		.ADREG(0),
		.ALUMODEREG(1),
		.AREG(2),
		.BCASCREG(1),
		.BREG(2),
		.CARRYINREG(1),
		.CARRYINSELREG(1),
		.CREG(1),
		.DREG(0),
		.INMODEREG(1),
		.MREG(1),
		.OPMODEREG(1),
		.PREG(1),
		.USE_SIMD("ONE48")
	)
	dsp_imag_sq_I (
		.P(dsp_pout_4),
		.ACIN(30'h0000),
		.BCIN(18'h000),
		.CARRYCASCIN(1'h0),
		.MULTSIGNIN(1'h0),
		.PCIN(dsp_pchain_3),
		.ALUMODE(4'b0000),		// Z + X + Y + CIN
		.CARRYINSEL(3'h0),
		.CEINMODE(1'b1),
		.CLK(clk),
		.INMODE(5'b00000),		// B=B2, A=A2
		.OPMODE(7'b0010101),	// X=M1, Y=M2, Z=PCIN
		.RSTINMODE(rst),
		.A({{12{in_imag_0[15]}}, in_imag_0, rng_lsb[3:2]}),
		.B({                     in_imag_0, rng_lsb[1:0]}),
		.C(48'h0000),
		.CARRYIN(1'b0),
		.D(25'h0000),
		.CEA1(1'b1),
		.CEA2(1'b1),
		.CEAD(1'b0),
		.CEALUMODE(1'b1),
		.CEB1(1'b1),
		.CEB2(1'b1),
		.CEC(1'b1),
		.CECARRYIN(1'b1),
		.CECTRL(1'b1),
		.CED(1'b0),
		.CEM(1'b1),
		.CEP(1'b1),
		.RSTA(rst),
		.RSTALLCARRYIN(rst),
		.RSTALUMODE(rst),
		.RSTB(rst),
		.RSTC(rst),
		.RSTCTRL(rst),
		.RSTD(rst),
		.RSTM(rst),
		.RSTP(rst)
	);

	assign pwr_4 = dsp_pout_4[35:4];


	// ----------------------------------
	// Log2 computation and normalization
	// ----------------------------------
		// When shifting, instead of zero filling, we fill with RNG data
		// Again, this helps reduce the visible quantization effects
		// for very low power values.

	// First stage
	assign msb_check[4] = |(pwr_4[31:16]);

	always @(posedge clk)
	begin
		if (msb_check[4])
			pwr_5 <= pwr_4;
		else
			pwr_5 <= { pwr_4[15:0], rng[31:16] };

		log2_5 <= { msb_check[4], 4'b0000 };
	end

	// Second stage
	assign msb_check[3] = |(pwr_5[31:24]);

	always @(posedge clk)
	begin
		if (msb_check[3])
			pwr_6 <= pwr_5;
		else
			pwr_6 <= { pwr_5[23:0], rng[15:8] };

		log2_6 <= { log2_5[4], msb_check[3], 3'b000 };
	end

	// Third stage
	assign msb_check[2] = |(pwr_6[31:28]);

	always @(posedge clk)
	begin
		if (msb_check[2])
			pwr_7 <= pwr_6;
		else
			pwr_7 <= { pwr_6[27:0], rng[7:4] };

		log2_7 <= { log2_6[4:3], msb_check[2], 2'b00 };
	end

	// Fourth stage
	assign msb_check[1] = |(pwr_7[31:30]);

	always @(posedge clk)
	begin
		if (msb_check[1])
			pwr_8 <= pwr_7;
		else
			pwr_8 <= { pwr_7[29:0], rng[3:2] };

		log2_8 <= { log2_7[4:2], msb_check[1], 1'b0 };
	end

	// Final stage
	assign msb_check[0] =   pwr_8[31];

	always @(posedge clk)
	begin
		if (msb_check[0])
			pwr_9 <= pwr_8;
		else
			pwr_9 <= { pwr_8[30:0], rng[1] };

		log2_9 <= { log2_8[4:1], msb_check[0] };
		log2_9 <= { log2_8[4:1], msb_check[0] };
	end


	// ----------
	// LUT lookup
	// ----------

	// Address mapping
	assign lut_addr_9 = { 1'b0, pwr_9[30:20], 4'h0 };

	// Actual LUT
	RAMB36E1 #(
		.RDADDR_COLLISION_HWCONFIG("PERFORMANCE"),
		.SIM_COLLISION_CHECK("NONE"),
		.DOA_REG(1),
		.DOB_REG(1),
		.EN_ECC_READ("FALSE"),
		.EN_ECC_WRITE("FALSE"),
		.INIT_00(256'h02b202840256022801fa01cd019f01710143011500e700b8008a005c002e0000),
		.INIT_01(256'h058c055f0531050404d604a9047b044e042003f203c503970369033b030e02e0),
		.INIT_02(256'h08610834080707da07ad077f0752072506f806ca069d06700642061505e705ba),
		.INIT_03(256'h0b310b040ad70aaa0a7d0a500a2409f709ca099d09700943091608e908bc088e),
		.INIT_04(256'h0dfb0dce0da20d750d490d1c0cef0cc30c960c6a0c3d0c100be40bb70b8a0b5d),
		.INIT_05(256'h10bf10931067103b100e0fe20fb60f8a0f5d0f310f050ed90eac0e800e530e27),
		.INIT_06(256'h137e1353132712fb12cf12a31277124b121f11f311c7119b116f1143111710eb),
		.INIT_07(256'h1639160d15e215b6158a155f1533150814dc14b014851459142d140213d613aa),
		.INIT_08(256'h18ed18c21897186c1841181517ea17bf17941768173d171216e616bb168f1664),
		.INIT_09(256'h1b9d1b731b481b1d1af21ac71a9c1a711a461a1b19f019c5199a196f19441919),
		.INIT_0A(256'h1e481e1e1df31dc91d9e1d741d491d1e1cf41cc91c9e1c731c491c1e1bf31bc8),
		.INIT_0B(256'h20ee20c4209a20702045201b1ff11fc61f9c1f721f471f1d1ef21ec81e9d1e73),
		.INIT_0C(256'h23902366233c231222e822be2294226a2240221621ec21c12197216d21432119),
		.INIT_0D(256'h262c260325d925af2586255c2532250824df24b5248b24612437240d23e423ba),
		.INIT_0E(256'h28c4289b28712848281e27f527cc27a22779274f272626fc26d326a9267f2656),
		.INIT_0F(256'h2b572b2e2b052adc2ab32a8a2a612a372a0e29e529bc299229692940291728ed),
		.INIT_10(256'h2de62dbd2d942d6b2d432d1a2cf12cc82c9f2c762c4d2c242bfb2bd22ba92b80),
		.INIT_11(256'h30703047301f2ff62fce2fa52f7d2f542f2b2f032eda2eb12e892e602e372e0f),
		.INIT_12(256'h32f632cd32a5327d3255322c320431dc31b3318b3162313a311230e930c13098),
		.INIT_13(256'h3577354f352734ff34d734af3487345f3437340f33e733be3396336e3346331e),
		.INIT_14(256'h37f437cc37a4377d3755372d370536de36b6368e3666363e361635ef35c7359f),
		.INIT_15(256'h3a6c3a453a1e39f639cf39a7398039583931390938e238ba3892386b3843381b),
		.INIT_16(256'h3ce13cba3c933c6b3c443c1d3bf63bcf3ba73b803b593b313b0a3ae33abb3a94),
		.INIT_17(256'h3f513f2a3f033edd3eb63e8f3e683e413e1a3df33dcc3da53d7d3d563d2f3d08),
		.INIT_18(256'h41be41974170414a412340fc40d540af40884061403a40143fed3fc63f9f3f78),
		.INIT_19(256'h442643ff43d943b3438c4366433f431942f242cc42a5427f42584231420b41e4),
		.INIT_1A(256'h468a4664463e461745f145cb45a5457f45584532450c44e544bf44994472444c),
		.INIT_1B(256'h48ea48c4489e48784853482d480747e147bb4795476f4748472246fc46d646b0),
		.INIT_1C(256'h4b474b214afb4ad64ab04a8a4a644a3f4a1949f349cd49a84982495c49364910),
		.INIT_1D(256'h4d9f4d7a4d544d2f4d094ce44cbe4c994c734c4e4c284c034bdd4bb84b924b6c),
		.INIT_1E(256'h4ff44fcf4faa4f844f5f4f3a4f154eef4eca4ea54e7f4e5a4e354e0f4dea4dc5),
		.INIT_1F(256'h5245522051fb51d651b1518c51675142511d50f850d350ae50895063503e5019),
		.INIT_20(256'h5492546e5449542453ff53da53b65391536c5347532252fd52d952b4528f526a),
		.INIT_21(256'h56dc56b75693566e564a5625560155dc55b85593556e554a5525550054dc54b7),
		.INIT_22(256'h592258fe58d958b55891586c5848582457ff57db57b75792576e574957255700),
		.INIT_23(256'h5b645b405b1c5af85ad45ab05a8c5a685a445a2059fb59d759b3598f596a5946),
		.INIT_24(256'h5da35d805d5c5d385d145cf05ccc5ca85c845c605c3d5c195bf55bd15bad5b89),
		.INIT_25(256'h5fdf5fbb5f985f745f505f2d5f095ee55ec25e9e5e7a5e565e335e0f5deb5dc7),
		.INIT_26(256'h621761f461d061ad618961666142611f60fb60d860b46091606d604a60266003),
		.INIT_27(256'h644c6429640563e263bf639c637863556332630f62eb62c862a56281625e623a),
		.INIT_28(256'h667d665a6637661465f165ce65ab658865656542651f64fc64d864b56492646f),
		.INIT_29(256'h68ab688868666843682067fd67da67b767946772674f672c670966e666c366a0),
		.INIT_2A(256'h6ad66ab36a916a6e6a4b6a296a0669e469c1699e697b69596936691368f168ce),
		.INIT_2B(256'h6cfd6cdb6cb96c966c746c516c2f6c0c6bea6bc76ba56b836b606b3d6b1b6af8),
		.INIT_2C(256'h6f226eff6edd6ebb6e996e776e546e326e106dee6dcb6da96d876d646d426d20),
		.INIT_2D(256'h7143712170ff70dd70bb709970777055703370116fee6fcc6faa6f886f666f44),
		.INIT_2E(256'h7361733f731d72fb72da72b87296727472527230720e71ec71cb71a971877165),
		.INIT_2F(256'h757c755a7539751774f574d474b27490746f744d742b740a73e873c673a47383),
		.INIT_30(256'h779477727751772f770e76ec76cb76aa7688766776457624760275e075bf759d),
		.INIT_31(256'h79a87987796679457924790278e178c0789e787d785c783a781977f877d677b5),
		.INIT_32(256'h7bba7b997b787b577b367b157af47ad37ab27a917a707a4e7a2d7a0c79eb79ca),
		.INIT_33(256'h7dc97da87d887d677d467d257d047ce37cc27ca17c807c5f7c3e7c1d7bfc7bdb),
		.INIT_34(256'h7fd57fb57f947f737f537f327f117ef07ed07eaf7e8e7e6d7e4d7e2c7e0b7dea),
		.INIT_35(256'h81de81be819d817d815c813c811b80fb80da80ba809980788058803780177ff6),
		.INIT_36(256'h83e583c483a48384836383438323830282e282c182a1828182608240821f81ff),
		.INIT_37(256'h85e885c885a88588856785478527850784e784c684a684868466844584258405),
		.INIT_38(256'h87e987c987a98789876987498729870986e986c986a986898668864886288608),
		.INIT_39(256'h89e789c789a78987896789488928890888e888c888a888888868884888298809),
		.INIT_3A(256'h8be28bc28ba28b838b638b438b248b048ae48ac58aa58a858a668a468a268a06),
		.INIT_3B(256'h8dda8dbb8d9b8d7c8d5c8d3d8d1d8cfe8cde8cbf8c9f8c808c608c408c218c01),
		.INIT_3C(256'h8fd08fb18f918f728f538f338f148ef58ed58eb68e978e778e588e388e198dfa),
		.INIT_3D(256'h91c391a49185916691469127910890e990ca90ab908b906c904d902e900e8fef),
		.INIT_3E(256'h93b39394937693579338931992fa92db92bc929c927d925e923f9220920191e2),
		.INIT_3F(256'h95a19583956495459526950794e894ca94ab948c946d944e942f941093f193d2),
		.INIT_40(256'h978d976e974f9731971296f396d596b696979679965a963b961c95fe95df95c0),
		.INIT_41(256'h997599579938991a98fb98dd98be98a09881986398449826980797e897ca97ab),
		.INIT_42(256'h9b5c9b3d9b1f9b019ae29ac49aa69a879a699a4a9a2c9a0e99ef99d199b29994),
		.INIT_43(256'h9d3f9d219d039ce59cc79ca99c8a9c6c9c4e9c309c119bf39bd59bb79b989b7a),
		.INIT_44(256'h9f219f039ee59ec79ea99e8b9e6d9e4f9e309e129df49dd69db89d9a9d7c9d5e),
		.INIT_45(256'ha100a0e2a0c4a0a6a088a06aa04ca02ea0119ff39fd59fb79f999f7b9f5d9f3f),
		.INIT_46(256'ha2dca2bea2a1a283a265a247a22aa20ca1eea1d0a1b3a195a177a159a13ba11e),
		.INIT_47(256'ha4b6a499a47ba45ea440a422a405a3e7a3c9a3aca38ea371a353a335a318a2fa),
		.INIT_48(256'ha68ea671a653a636a618a5fba5dda5c0a5a2a585a567a54aa52ca50fa4f1a4d4),
		.INIT_49(256'ha863a846a829a80ba7eea7d1a7b4a796a779a75ca73ea721a703a6e6a6c9a6ab),
		.INIT_4A(256'haa36aa19a9fca9dfa9c2a9a5a987a96aa94da930a913a8f5a8d8a8bba89ea881),
		.INIT_4B(256'hac07abeaabcdabb0ab93ab76ab59ab3cab1fab02aae5aac8aaabaa8eaa71aa53),
		.INIT_4C(256'hadd6adb9ad9cad7fad62ad45ad28ad0cacefacd2acb5ac98ac7bac5eac41ac24),
		.INIT_4D(256'hafa2af85af68af4caf2faf12aef5aed9aebcae9fae82ae66ae49ae2cae0fadf2),
		.INIT_4E(256'hb16cb14fb133b116b0fab0ddb0c0b0a4b087b06ab04eb031b015aff8afdbafbf),
		.INIT_4F(256'hb334b317b2fbb2deb2c2b2a5b289b26cb250b233b217b1fab1deb1c1b1a5b188),
		.INIT_50(256'hb4f9b4ddb4c1b4a4b488b46cb44fb433b417b3fab3deb3c2b3a5b389b36cb350),
		.INIT_51(256'hb6bdb6a1b684b668b64cb630b614b5f7b5dbb5bfb5a3b587b56ab54eb532b515),
		.INIT_52(256'hb87eb862b846b82ab80eb7f2b7d6b7bab79eb781b765b749b72db711b6f5b6d9),
		.INIT_53(256'hba3dba21ba05b9e9b9ceb9b2b996b97ab95eb942b926b90ab8eeb8d2b8b6b89a),
		.INIT_54(256'hbbfabbdebbc3bba7bb8bbb6fbb54bb38bb1cbb00bae4bac8baadba91ba75ba59),
		.INIT_55(256'hbdb5bd9abd7ebd62bd47bd2bbd0fbcf4bcd8bcbcbca1bc85bc69bc4dbc32bc16),
		.INIT_56(256'hbf6ebf53bf37bf1cbf00bee5bec9beadbe92be76be5bbe3fbe24be08bdecbdd1),
		.INIT_57(256'hc125c10ac0eec0d3c0b7c09cc081c065c04ac02ec013bff7bfdcbfc1bfa5bf8a),
		.INIT_58(256'hc2dac2bfc2a3c288c26dc251c236c21bc200c1e4c1c9c1aec192c177c15cc140),
		.INIT_59(256'hc48dc472c456c43bc420c405c3eac3cfc3b3c398c37dc362c347c32bc310c2f5),
		.INIT_5A(256'hc63dc622c607c5ecc5d1c5b6c59bc580c565c54ac52fc514c4f9c4dec4c3c4a8),
		.INIT_5B(256'hc7ecc7d1c7b7c79cc781c766c74bc730c715c6fac6dfc6c4c6a9c68ec673c658),
		.INIT_5C(256'hc999c97ec964c949c92ec913c8f9c8dec8c3c8a8c88dc873c858c83dc822c807),
		.INIT_5D(256'hcb44cb2acb0fcaf4cadacabfcaa4ca8aca6fca54ca3aca1fca04c9e9c9cfc9b4),
		.INIT_5E(256'hccedccd3ccb8cc9ecc83cc69cc4ecc34cc19cbfecbe4cbc9cbafcb94cb79cb5f),
		.INIT_5F(256'hce94ce7ace60ce45ce2bce10cdf6cddccdc1cda7cd8ccd72cd57cd3dcd22cd08),
		.INIT_60(256'hd03ad01fd005cfebcfd1cfb6cf9ccf82cf67cf4dcf33cf18cefecee4cec9ceaf),
		.INIT_61(256'hd1ddd1c3d1a9d18fd174d15ad140d126d10cd0f1d0d7d0bdd0a3d088d06ed054),
		.INIT_62(256'hd37fd365d34bd330d316d2fcd2e2d2c8d2aed294d27ad260d246d22cd211d1f7),
		.INIT_63(256'hd51ed504d4ead4d1d4b7d49dd483d469d44fd435d41bd401d3e7d3cdd3b3d399),
		.INIT_64(256'hd6bcd6a2d689d66fd655d63bd621d607d5eed5d4d5bad5a0d586d56cd552d538),
		.INIT_65(256'hd858d83fd825d80bd7f1d7d8d7bed7a4d78bd771d757d73dd723d70ad6f0d6d6),
		.INIT_66(256'hd9f3d9d9d9bfd9a6d98cd973d959d93fd926d90cd8f2d8d9d8bfd8a5d88cd872),
		.INIT_67(256'hdb8bdb72db58db3fdb25db0cdaf2dad9dabfdaa6da8cda72da59da3fda26da0c),
		.INIT_68(256'hdd22dd09dcefdcd6dcbcdca3dc8adc70dc57dc3ddc24dc0adbf1dbd8dbbedba5),
		.INIT_69(256'hdeb7de9ede84de6bde52de39de1fde06ddedddd3ddbadda1dd87dd6edd55dd3b),
		.INIT_6A(256'he04ae031e018dfffdfe6dfccdfb3df9adf81df68df4edf35df1cdf03dee9ded0),
		.INIT_6B(256'he1dce1c3e1aae191e178e15fe145e12ce113e0fae0e1e0c8e0afe096e07de063),
		.INIT_6C(256'he36ce353e33ae321e308e2efe2d6e2bde2a4e28be272e259e240e227e20ee1f5),
		.INIT_6D(256'he4fae4e1e4c8e4afe497e47ee465e44ce433e41ae401e3e8e3cfe3b7e39ee385),
		.INIT_6E(256'he686e66ee655e63ce624e60be5f2e5d9e5c0e5a8e58fe576e55de544e52ce513),
		.INIT_6F(256'he811e7f9e7e0e7c7e7afe796e77de765e74ce733e71be702e6e9e6d1e6b8e69f),
		.INIT_70(256'he99be982e96ae951e938e920e907e8efe8d6e8bee8a5e88ce874e85be843e82a),
		.INIT_71(256'heb22eb0aeaf1ead9eac0eaa8ea90ea77ea5fea46ea2eea15e9fde9e4e9cce9b3),
		.INIT_72(256'heca8ec90ec78ec5fec47ec2eec16ebfeebe5ebcdebb5eb9ceb84eb6beb53eb3b),
		.INIT_73(256'hee2dee14edfcede4edccedb3ed9bed83ed6bed52ed3aed22ed09ecf1ecd9ecc1),
		.INIT_74(256'hefafef97ef7fef67ef4fef37ef1fef06eeeeeed6eebeeea6ee8dee75ee5dee45),
		.INIT_75(256'hf131f119f101f0e8f0d0f0b8f0a0f088f070f058f040f028f010eff8efe0efc8),
		.INIT_76(256'hf2b0f298f280f268f251f239f221f209f1f1f1d9f1c1f1a9f191f179f161f149),
		.INIT_77(256'hf42ef417f3fff3e7f3cff3b7f39ff387f370f358f340f328f310f2f8f2e0f2c8),
		.INIT_78(256'hf5abf593f57bf564f54cf534f51cf505f4edf4d5f4bdf4a5f48ef476f45ef446),
		.INIT_79(256'hf726f70ef6f7f6dff6c7f6b0f698f680f669f651f639f622f60af5f2f5daf5c3),
		.INIT_7A(256'hf8a0f888f870f859f841f82af812f7fbf7e3f7cbf7b4f79cf785f76df755f73e),
		.INIT_7B(256'hfa18fa00f9e9f9d1f9baf9a2f98bf973f95cf944f92df915f8fef8e6f8cff8b7),
		.INIT_7C(256'hfb8efb77fb5ffb48fb31fb19fb02faeafad3fabcfaa4fa8dfa75fa5efa46fa2f),
		.INIT_7D(256'hfd03fcecfcd5fcbdfca6fc8ffc77fc60fc49fc32fc1afc03fbecfbd4fbbdfba5),
		.INIT_7E(256'hfe77fe60fe48fe31fe1afe03fdecfdd4fdbdfda6fd8ffd77fd60fd49fd32fd1a),
		.INIT_7F(256'hffe9ffd2ffbbffa4ff8dff75ff5eff47ff30ff19ff02feebfed3febcfea5fe8e),
		.INIT_A(36'h000000000),
		.INIT_B(36'h000000000),
		.INIT_FILE("NONE"),
		.RAM_MODE("TDP"),
		.RAM_EXTENSION_A("NONE"),
		.RAM_EXTENSION_B("NONE"),
		.READ_WIDTH_A(18),
		.READ_WIDTH_B(0),
		.WRITE_WIDTH_A(0),
		.WRITE_WIDTH_B(36),				// the RAMB36E1 model fails without this
		.RSTREG_PRIORITY_A("RSTREG"),
		.RSTREG_PRIORITY_B("RSTREG"),
		.SRVAL_A(36'h000000000),
		.SRVAL_B(36'h000000000),
		.SIM_DEVICE("7SERIES"),
		.WRITE_MODE_A("READ_FIRST"),
		.WRITE_MODE_B("READ_FIRST")
	)
	log_lut_I (
		.DOADO(lut_do_11),
		.CASCADEINA(1'b0),
		.CASCADEINB(1'b0),
		.INJECTDBITERR(1'b0),
		.INJECTSBITERR(1'b0),
		.ADDRARDADDR(lut_addr_9),
		.CLKARDCLK(clk),
		.ENARDEN(1'b1),
		.REGCEAREGCE(1'b1),
		.RSTRAMARSTRAM(rst),
		.RSTREGARSTREG(rst),
		.WEA(4'h0),
		.DIADI(32'h00000000),
		.DIPADIP(4'h0),
		.ADDRBWRADDR(16'h0000),
		.CLKBWRCLK(1'b0),
		.ENBWREN(1'b0),
		.REGCEB(1'b0),
		.RSTRAMB(1'b0),
		.RSTREGB(1'b0),
		.WEBWE(8'h0),
		.DIBDI(32'h00000000),
		.DIPBDIP(4'h0)
	);

	// LSBs mapping
	assign msb_9  = pwr_9[31];
	assign lsbs_9 = pwr_9[19:15];

	// Delay lines to compensate for LUT delay
	delay_bit #(2)    dl_msb  (msb_9,  msb_11,  clk);
	delay_bus #(2, 5) dl_lsbs (lsbs_9, lsbs_11, clk);
	delay_bus #(2, 5) dl_log2 (log2_9, log2_11, clk);


	// -----------
	// Final value
	// -----------

	// Final add & saturation
	always @(posedge clk)
	begin
		if (!msb_11)
			final_12 <= 16'h0000;
		else
			final_12 <= { log2_11, lut_do_11[15:0] } + lsbs_11;
	end

	// Mapping
	assign out_12 = final_12[20:5];

endmodule // f15_logpwr
