//
// Copyright 2015 Ettus Research
//

`timescale 1ns / 1ps


module ppsloop(
    input reset,
    input xoclk, // 40 MHz from VCTCXO
    input ppsgps,
    input ppsext,
    input [1:0] refsel,
    output reg lpps,
    output reg is10meg,
    output reg ispps,
    output reg reflck,
    output plllck,// status of things
    output sclk,
    output mosi,
    output sync_n,
    input [15:0] dac_dflt
   );
  wire ppsref = (refsel==2'b00)?ppsgps:
                (refsel==2'b11)?ppsext:
                                1'b0;
  // reference pps to discilpline the VCTX|CXO to, from GPS or EXT in

  wire clk_200M_o, clk;
  BUFG x_clk_gen ( .I(clk_200M_o), .O(clk));
  wire clk_40M;

  wire n_pps = (refsel==2'b01) | (refsel==2'b10);
  reg _npps, no_pps;
  always @(posedge clk) { no_pps, _npps } <= { _npps, n_pps };

  PLLE2_ADV #(.BANDWIDTH("OPTIMIZED"), .COMPENSATION("INTERNAL"),
     .DIVCLK_DIVIDE(1),
     .CLKFBOUT_MULT(30),
     .CLKOUT0_DIVIDE(6),
     .CLKOUT1_DIVIDE(30),
     .CLKIN1_PERIOD(25.0)
   )
   clkgen (
      .PWRDWN(1'b0), .RST(1'b0),
      .CLKIN1(xoclk),
      .CLKOUT0(clk_200M_o),
      .CLKOUT1(clk_40M),
      .LOCKED(plllck)
   );

    // state machine to manage reference detection and xo adjustment steps
  reg [2:0] sstate, nxt_sstate;
  localparam REFDET=3'b000;
  localparam CFADJ=3'b001;
  localparam SLEDGEA=3'b010;
  localparam SLEDGEB=3'b011;
  localparam FINEADJ=3'b100;

  // state machine to manage lead-lag count
  reg [1:0] llstate, nxt_llstate;
  localparam READY=2'b00;
  localparam COUNT=2'b01;
  localparam DONE=2'b11;
  localparam WAIT=2'b10;

  /* Counter generating a local pps for the xo derived clock domains.
     nxt_lcnt is manipulated by a state machine (sstate) to allow
     quick re-alignment of the local pps rising edge with that of
     the reference.
  */
  reg [27:0] lcnt, nxt_lcnt;
  wire recycle = (28'd199_999_999==lcnt); // sets the period, 1 sec

  always @(posedge clk) begin
    sstate <= nxt_sstate;
    lcnt <= nxt_lcnt;
    lpps <= lcnt > 28'd150_000_000; // ~25% duty cycle
  end

  /* Reference signal detection:
   * Count the time interval between rising edges on the reference
   * signal. The interval counter "rcnt" is restarted at rising edges
   * of ppsref. "ppsref" could be either a pps signal, or a 10 MHz clock.
   * Register "rlst" captures the value of rcnt at each rising edge.
   * From this count value, we know the reference frequency.
   */
  reg [27:0] rcnt, rlst;
  reg signed [28:0] rdiff;
  wire signed [28:0] srlst = { 1'b0, rlst }; // sign extended version of rlst
  wire [27:0] nxt_rcnt;
  reg rcnt_ovfl;
  reg [3:0] ple; // pipeline from reference rising edge det.
  wire valid_ref = is10meg | ispps;


  /* If the reference is at 10 MHz, derive a reference pps using a counter
   * to feed the frequency control logic. To detect a 0.5 ppm deviation
   * on a 10 MHz signal using counters requires the better part of a second
   * anyway, so samples at a 1 Hz rate are appropriate. This allows much of
   * the same logic to be used for pps or 10 Mhz references.
  */
  reg [23:0] tcnt;
  reg tpps;
  wire [23:0] nxt_tcnt = (~is10meg | tcnt==24'd9999999) ? 24'b0 : tcnt+1'b1;
  always @(posedge ppsref) begin
    /* note this is clocked by the reference signal and is not useful when
     * the reference is a pps.
     */
    tcnt <= nxt_tcnt;
    tpps <= (tcnt>24'd7499999);
  end

  /* The reference needs to be synchronized into the local clock domain,
   * and while the local 'pps' is generated synchronously within this
   * domain, it gets passed through identical stages to maintain
   * the time relationship between detected rising edges.
   */
  reg [2:0] refsmp;
  reg [2:0] tsmp;
  reg [2:0] xosmp;
  always @(posedge clk) begin
    // apply same sync delay to all pps flavors
    refsmp <= { refsmp[1:0], ppsref};
    tsmp <= { tsmp[1:0], tpps};
    xosmp <= { xosmp[1:0], lpps };
  end


  wire rising_r = (refsmp[2:1]==2'b01);
  wire rising_t = (tsmp[2:1]==2'b01);
  wire rising_ref = is10meg ? rising_t : rising_r;
  wire rising_xo  = (xosmp[2:1]==2'b01);
  wire lead = rising_xo & ~rising_ref;
  wire lag = ~rising_xo & rising_ref;
  wire trig = rising_xo ^ rising_ref;
  wire dtrig = rising_xo & rising_ref;
  wire untrig = rising_xo | rising_ref;
  wire llrdy = (is10meg ? ~tsmp[2] : ~refsmp[2]) & ~xosmp[2];
  wire rhigh = is10meg ? tsmp[1] : refsmp[1];


  reg [5:0] pcnt;
  reg pcnt_ovfl;
  wire [5:0]  nxt_pcnt = (rising_r | pcnt_ovfl) ? 6'b0 : pcnt+1'b1;
  always @(posedge clk) begin
    pcnt <= nxt_pcnt;
    if (rcnt_ovfl)
      is10meg <= 1'b0;
    else if (pcnt == 6'b111111) begin
      pcnt_ovfl <= 1'b1;
      is10meg <= 1'b0;
    end
    else if (rising_r) begin
      is10meg <=  (pcnt > 6'd16) & (pcnt < 6'd24);
      pcnt_ovfl <= 1'b0;
    end
  end

  reg rr;
  assign nxt_rcnt = rr ? 28'b0 : rcnt+1'b1;
  always @(posedge clk) begin
    rr <= rising_ref;
    ple[3:0] <= {ple[2:0],rising_ref & valid_ref};

    rcnt <= nxt_rcnt;

    // set the overflow flag if no reference edge is detected and
    // hold it asserted until an edge does arrive. This allows clearing of
    // the other flags, even if there is no reference.
    if (rcnt==28'b1111111111111111111111111111)
      rcnt_ovfl <= 1'b1;
    else if (rr)
      rcnt_ovfl <= 1'b0;

    if (rr) begin
      // a rising edge arrived, grab the count and compare to bounds
      rlst <= rcnt;
    end
    if (rr | rcnt_ovfl) begin
      ispps <= ~is10meg & ~rcnt_ovfl & (rcnt > 28'd199997000) & (rcnt < 200003000);
      /* reference frequency detect limits:
       * 10M sampled with 200M should be 20 cycles, 16-24 provides xtra margin
       * to allow for tolerances and possibly sampling at jittery edges
       * allow +- 15 ppm on a pps signal
       */

    end
  end


  reg signed [27:0] coarse;
  reg [15:0] dacv = 16'd32767; // power-on default mid-scale
  wire signed [16:0] sdacv = { 1'b0, dacv};
  /* to exit coarse adjustment, the frequency error shall be small for
   * several cycles
   */
  reg esmall;
  reg [2:0] es;

  reg pr;


  /* The xo can be on-frequency while the rising edges are still
   * out-of-phase, so a phase detector is also required. The
   * counter "llcnt" accumulates how many ticks local pps leads
   * or lags the reference pps . The range of this counter
   * need not be as large as "rcnt". The count increments
   * or decrements based upon which signal has a rising edge first,
   * and the count is halted when the other rising edge occurs.
   * Both signals are required to transition back to the low state
   * to re-arm the detection state machine.
  */
  reg llcntena;
  reg lead_lagn;
  reg signed [11:0] llcnt, nxt_llcnt;
  wire signed [11:0] incr = lead_lagn ? -12'sd1 : 12'sd1; // -1 lead, +1 lag
  reg [3:0] llsmall;
  reg llovfl;

  reg [2:0] refs1, refs0;
  reg refchanged;
  reg refinternal;
  always @(posedge clk) begin
    refs1 <= { refs1[1:0], refsel[1] };
    refs0 <= { refs0[1:0], refsel[0] };
    refchanged <= { refs1[2], refs0[2] } != { refs1[1], refs0[1] };
    refinternal <=  refs1[2] ^ refs0[2]; // not gps or external

    // compute how far off the expected period we are
    if (ple[1]) begin
      rdiff <= srlst-29'd199999999;
    end

    // compute an adjustment for the dac
    if (ple[2]) begin
      // if rdiff is (+), the xo is fast
      // include a bit of gain for quick adjustment
      // an approximate gain was initially determined by 'theory' using
      // the xo tuning sensitivity, and was find-tuned 'by hand'
      // by observing the loop behaviour (with rdiff instrumented and
      // pps signals connected out to an oscilloscope).
      coarse <= sdacv - (rdiff <<< 3);
    end

    // determine when the period error is small
    if (ple[2] | rcnt_ovfl) begin
      es <= { es[1:0], (rdiff<29'sd8 && rdiff>-29'sd8) };
      esmall <= valid_ref & ~rcnt_ovfl & (es[2:0] == 3'b111);
    end
    else if (sstate==REFDET) begin
      es <= 3'b0;
      esmall <= 1'b0;
    end

    // assign the dac value when doing coarse-adjustment
    // in the fine-adjust phaase, the PI control filtering takes over
    if (ple[3] & (sstate==CFADJ)) begin
      dacv <= coarse[15:0];
    end
    else if (sstate==REFDET) begin
      dacv <= 16'd32767; // center the DAC
    end
  end


  always @(*) begin
    nxt_sstate=sstate;
    pr = 1'b0;
    nxt_lcnt = recycle ? 26'd0 : lcnt + 1'b1;
    case (sstate)
    REFDET: begin // determine reference type
      pr = 1'b0;
      if (valid_ref) nxt_sstate = CFADJ;
    end
    CFADJ: begin // coarse freqency adjustment
      pr = 1'b1;
      if (esmall) nxt_sstate = SLEDGEA;
    end
    SLEDGEA: begin // ensure local pps is low and wait for a ref edge
      pr = 1'b1; // preload the integrator
      if (rhigh) nxt_sstate = SLEDGEB;
    end
    SLEDGEB: begin // force local pps rising edge to match reference
      nxt_lcnt = 26'd0;
      pr = 1'b1; // preload the integrator
      if(rhigh) begin
        nxt_lcnt = 28'd149_999_998; // force rising edge in a couple cycles
        nxt_sstate = FINEADJ;
      end
    end
    FINEADJ: begin // wide-ish bandwidth PI control
      if (~valid_ref | llovfl) nxt_sstate = REFDET;
    end
    default: begin
      nxt_sstate = REFDET;
    end
    endcase
    // overriding conditions:
    if (refinternal | refchanged | rcnt_ovfl ) nxt_sstate = REFDET;
  end

  reg llsena;
  always @(posedge clk) begin
    llstate <= nxt_llstate;
    if (llcntena) llcnt <= nxt_llcnt;
    if (llstate==READY) lead_lagn <= lead;
    if (llsena) llsmall <= { (llsmall[2:0] == 3'b111), llsmall[1:0],
                                        (llcnt < 12'sd3)&(llcnt > -12'sd3)};
    if (llcntena)  llovfl <= (llcnt>12'sd1800) | (llcnt< -12'sd1800);
  end

  reg ppsfltena;
  always @(*) begin
    // values to hold by default:
    nxt_llstate = llstate;
    llcntena=1'b0;
    nxt_llcnt=llcnt;
    ppsfltena = 1'b0;
    llsena = 1'b0;

    case (llstate)
    READY: begin
      nxt_llcnt=12'b0;
      if (trig | dtrig) begin
        nxt_llstate = trig ? COUNT : DONE;
        llcntena=1'b1;
        // even if dtrig, set llcnt to 0 to feed the filter pipe
      end
    end
    COUNT: begin
      if (untrig) begin // the second edge arrived
        nxt_llstate = DONE;
      end
      else begin
        llcntena=1'b1;
        nxt_llcnt=llcnt+incr;
      end
    end
    DONE: begin
      nxt_llstate = WAIT;
      ppsfltena = 1'b1;
    end
    WAIT: begin
      if (llrdy) begin
        nxt_llstate = READY;
        llsena = 1'b1;
      end
    end
    endcase
    if (sstate==REFDET) begin
      nxt_llstate = READY;
      llcntena=1'b0;
      ppsfltena = 1'b0;
      llsena = 1'b0;
    end
  end


  reg[15:0] daco;

  reg [1:0] enchain=2'b00;
  always @(posedge clk) enchain <= { enchain[1:0], ppsfltena & (enchain==2'b00) };

  reg signed [23:0] integ;
  reg signed [23:0] prop;
  wire signed [23:0] nxt_integ = integ + (llcnt <<< 6);
  wire signed [23:0] nxt_prop = (llcnt <<< 7);
  wire signed [23:0] eff = integ + prop;
  wire urng = eff[23], orng = eff[23:22]==2'b01;
  reg erng;
  /* The values for proportional and integral gain terms were originally
   * estimated using a model that accounted for the xo tuning sensitivity.
   * When implemented, the loop dynamics observed differed significantly
   * from model results, probably as a result of the Xilinx PLL
   * (which was not modelled) being present in the loop. The gain values
   * were find-tuned 'by hand' by observing the loop behaviour (with llcnt
   * instrumented) and pps signals connected out to an oscilloscope).
   */

  always @(posedge clk) begin
    if (no_pps) begin
     daco <= dac_dflt;
    end
    else if (pr) begin
      integ <= { 2'b00, dacv, 6'b0 }; // precharge the accumulator
      daco <= dacv;
    end
    else begin
      if (enchain[0]) begin
        integ <= nxt_integ;
        prop <= nxt_prop;
      end
      if (enchain[1]) begin
        daco <= eff[21:6];
        erng <= urng | orng;
      end
    end
  end

  wire  fadj=   (sstate==FINEADJ);
  always @(posedge clk) begin
    reflck <= refinternal | fadj;
  end

  ad5662_auto_spi dac
  (
    .clk(clk),
    .dat(daco),
    .sclk(sclk),
    .mosi(mosi),
    .sync_n(sync_n)
  );

endmodule
