//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module b205_ref_pll(
    input reset,
    input clk,      // 200 MHz sample clock
    input refclk,   // 40 MHz reference clock
    input ref,      // PPS or 10 MHz external reference
    output reg locked,

    // SPI lines to AD5662
    output sclk,
    output mosi,
    output sync_n
    );

    // Base parameters
    localparam SAMPLE_CLOCK_FREQ=200_000_000;
    localparam REF_FREQ_PPS=1;
    localparam REF_FREQ_10MHZ=10_000_000;
    localparam REF_CLK_FREQ=40_000_000;
    localparam PFD_FREQ_PPS=1;
    localparam PFD_FREQ_10MHZ=10;

    // Lock detection parameters
    localparam LOCK_TOLERANCE_PPM=1;
    localparam LOCK_MARGIN_PPS=(SAMPLE_CLOCK_FREQ/PFD_FREQ_PPS)*LOCK_TOLERANCE_PPM/1_000_000;
    localparam LOCK_MARGIN_10MHZ=(SAMPLE_CLOCK_FREQ/PFD_FREQ_10MHZ)*LOCK_TOLERANCE_PPM/1_000_000;

    // Reference frequency detection parameters
    // References are only valid if they are +/-5ppm because that is the range of the VCTXCO
    localparam REF_PERIOD_PPS=SAMPLE_CLOCK_FREQ/REF_FREQ_PPS;
    localparam REF_PERIOD_10MHZ=SAMPLE_CLOCK_FREQ/REF_FREQ_10MHZ;
    localparam REF_PERIOD_PPS_MIN=REF_PERIOD_PPS-(REF_PERIOD_PPS*5/1_000_000)-1;
    localparam REF_PERIOD_PPS_MAX=REF_PERIOD_PPS+(REF_PERIOD_PPS*5/1_000_000)+1;
    localparam REF_PERIOD_10MHZ_MIN=REF_PERIOD_10MHZ-(REF_PERIOD_10MHZ*5/1_000_000)-1;
    localparam REF_PERIOD_10MHZ_MAX=REF_PERIOD_10MHZ+(REF_PERIOD_10MHZ*5/1_000_000)+1;

    // R divider parameters
    localparam RDIV_PPS=REF_FREQ_PPS/PFD_FREQ_PPS;
    localparam RDIV_10MHZ=REF_FREQ_10MHZ/PFD_FREQ_10MHZ;

    // N divider parameters (refclk is divided by 2)
    localparam NDIV_PPS=REF_CLK_FREQ/2/PFD_FREQ_PPS;
    localparam NDIV_10MHZ=REF_CLK_FREQ/2/PFD_FREQ_10MHZ;

    // PFD parameters
    localparam PFD_PERIOD_PPS=SAMPLE_CLOCK_FREQ/PFD_FREQ_PPS;
    localparam PFD_PERIOD_10MHZ=SAMPLE_CLOCK_FREQ/PFD_FREQ_10MHZ;


    // Initial divide by 2 for 40 MHz clock
    // (since refclk cannot be sampled directly)
    reg refclk_div;
    always @(posedge refclk) begin
        refclk_div <= ~refclk_div;
    end

    // flop signals into sample clock domain together
    reg [3:0] refsmp;
    reg [3:0] refclksmp;
    always @(posedge clk) begin
        refsmp <= {refsmp[2:0],ref};
        refclksmp <= {refclksmp[2:0],refclk_div};
    end

    // rising edge detection
    wire ref_rising = (refsmp[3:2] == 2'b01);
    wire refclk_rising = (refclksmp[3:2] == 2'b01);

    // reference frequency detection
    reg [27:0] refcnt;
    reg ref_detected;
    reg ref_is_10M;
    reg ref_is_pps;
    wire valid_ref = ref_is_10M | ref_is_pps;
    always @(posedge clk) begin
        if (reset) begin
            refcnt <= 28'd0;
            ref_detected <= 1'b0;
            ref_is_10M <= 1'b0;
            ref_is_pps <= 1'b0;
        end
        else if (ref_rising) begin
            refcnt <= 28'd1;
            ref_detected <= 1'b1;
            ref_is_10M <= ((refcnt >= REF_PERIOD_10MHZ_MIN) && (refcnt <= REF_PERIOD_10MHZ_MAX));
            ref_is_pps <= ((refcnt >= REF_PERIOD_PPS_MIN) && (refcnt <= REF_PERIOD_PPS_MAX));
        end
        else if ((ref_is_10M && (refcnt > REF_PERIOD_10MHZ_MAX)) || (refcnt > REF_PERIOD_PPS_MAX)) begin
            // consider the reference lost
            refcnt <= 28'd0;
            ref_detected <= 1'b0;
            ref_is_10M <= 1'b0;
            ref_is_pps <= 1'b0;
        end
        else if (ref_detected)
            refcnt <= refcnt + 28'd1;
    end

    // R divider
    wire [23:0] rdiv = ref_is_10M ? RDIV_10MHZ : RDIV_PPS;
    reg [23:0] rcnt;
    wire [23:0] next_rcnt = ~valid_ref ? 24'd0 : (rcnt == rdiv) ? 24'd1 : rcnt + 1'b1;
    reg r_rising;
    always @(posedge clk) begin
        if (ref_rising)
            rcnt <= next_rcnt;
        r_rising <= (ref_rising && ((ref_is_10M && (rcnt == rdiv)) || ref_is_pps));
    end

    // N divider
    // Enable on rising edge of R after valid_ref
    // is asserted so R and N signals start aligned.
    // Disable if reference lost.
    wire [25:0] ndiv = ref_is_10M ? NDIV_10MHZ : NDIV_PPS;
    reg [25:0] ncnt;
    wire [25:0] next_ncnt = ~valid_ref ? 26'd0 : ncnt == ndiv ? 26'd1 : ncnt + 1'b1;
    reg n_rising;
    always @(posedge clk) begin
		if (refclk_rising)
			ncnt <= next_ncnt;
		n_rising <= (refclk_rising && (ncnt == ndiv));
    end

    // Frequency Counter
    wire signed [28:0] period = ref_is_10M ? PFD_PERIOD_10MHZ : PFD_PERIOD_PPS;
    reg signed [28:0] r_period_cnt;
    reg signed [28:0] freq_err;
    always @(posedge clk) begin
        if (reset | ~valid_ref) begin
            r_period_cnt <= 28'd0;
            freq_err <= 29'sd0;
        end
        else if (r_rising) begin
            r_period_cnt <= 28'd1;
            freq_err <= period - r_period_cnt;
        end
        else
            r_period_cnt <= r_period_cnt + 28'd1;
    end

    // Phase Counter
    reg signed [28:0] lead_cnt;
    reg lead_cnt_ena;
    reg signed [28:0] lead;
    always @(posedge clk) begin
        // Count how much N leads R
        // The count is negative because it measures
        // how much the VCTCXO must be slowed down.
        if (~valid_ref | n_rising) begin
            lead_cnt <= 29'sd0;
            lead_cnt_ena <= 1'b1;
            if (r_rising)
                lead <= 29'sd0;
        end
        else if (r_rising) begin
            if (lead_cnt_ena)
                lead <= lead_cnt - 29'sd1;
            else begin
                // R rising with no preceding N rising.
                // N has changed from leading to lagging R,
                // but we don't yet know by how much so
                // assume 1.
                lead <= 29'sd1;
            end
            lead_cnt_ena <= 1'b0;
        end
        else if (lead_cnt_ena)
            lead_cnt <= lead_cnt - 29'sd1;
    end

    // PFD State Machine
    localparam MEASURE=4'd0;
    localparam CAPTURE=4'd1;
    localparam CAPTURE_LAG=4'd2;
    localparam CAPTURE_LEAD=4'd3;
    localparam CALCULATE_ERROR=4'd4;
    localparam CALCULATE_10M_GAIN=4'd5;
    localparam CALCULATE_ADJUSTMENT=4'd6;
    localparam CALCULATE_OUTPUT_VALUE=4'd7;
    localparam APPLY_OUTPUT_VALUE=4'd8;
    reg [3:0] state;
    reg [15:0] daco = 16'd32767;
    wire signed [28:0] lock_margin = ref_is_10M ? LOCK_MARGIN_10MHZ : LOCK_MARGIN_PPS;
    wire signed [28:0] lag = lead + period;
    reg signed [28:0] phase_err;
    reg signed [28:0] err;
    reg signed [28:0] shift;
    reg signed [28:0] adj;
    wire signed [28:0] dacv = {13'd0, daco};
    reg signed [28:0] sum;
    reg [2:0] ld;
    always @(posedge clk) begin
        if (reset || ~valid_ref) begin
            state <= MEASURE;
            daco <= 16'd32767;
            err <= 29'sd0;
            shift <= 29'sd0;
            adj <= 29'sd0;
            ld <= 3'd0;
        end
        else begin
            case(state)
                MEASURE: begin
                    if (r_rising)
                        state <= CAPTURE;
                end
                CAPTURE: begin
                    if (lag < -lead)
                        state <= CAPTURE_LAG;
                    else
                        state <= CAPTURE_LEAD;
                end
                CAPTURE_LAG: begin
                    phase_err <= lag;
                    ld <= {ld[1:0], (lag <= lock_margin)};
                    state <= CALCULATE_ERROR;
                end
                CAPTURE_LEAD: begin
                    phase_err <= lead;
                    ld <= {ld[1:0], (-lead <= lock_margin)};
                    state <= CALCULATE_ERROR;
                end
                CALCULATE_ERROR: begin
                    err <= phase_err + freq_err;
                    state <= ref_is_10M ? CALCULATE_10M_GAIN : CALCULATE_ADJUSTMENT;
                end
                CALCULATE_10M_GAIN: begin
                    shift <= (err < -7 || err > 7) ? 7 : (err < 0 ? -err : err);
                    state <= CALCULATE_ADJUSTMENT;
                end
                CALCULATE_ADJUSTMENT: begin
                    // The VCTCXO is +/-5 ppm from 0.3V to 1.5V and the DAC is 16 bits,
                    // which works out to 0.000228885 ppm per DAC unit.
                    // The 200 MHz sampling clock means each unit of error is 0.005 ppm,
                    // which works out to 21.845 DAC units to correct each unit of error.
                    // Theory is nice, but the proportional and integral gains used here
                    // were determined through manual tuning.
                    if (ref_is_10M)
                        adj <= (err <<< shift);
                    else
                        adj <= (err <<< 4) - err;
                    state <= CALCULATE_OUTPUT_VALUE;
                end
                CALCULATE_OUTPUT_VALUE: begin
                    sum <= dacv + adj;
                    state <= APPLY_OUTPUT_VALUE;
                end
                APPLY_OUTPUT_VALUE: begin
                    // Clip and apply
                    if (sum < 29'sd0)
                        daco <= 16'd0;
                    else if (sum > 29'sd65535)
                        daco <= 16'd65535;
                    else
                        daco <= sum[15:0];
                    state <= MEASURE;
                end
            endcase
        end
    end

    always @(posedge clk)
        locked <= (ld == 3'b111);

    ad5662_auto_spi dac
    (
        .clk(clk),
        .dat(daco),
        .sclk(sclk),
        .mosi(mosi),
        .sync_n(sync_n)
    );
endmodule
