module b200_tb ();

   wire cat_ce;
   wire cat_miso;
   wire cat_mosi;
   wire cat_sclk;

   wire fx3_ce;
   wire fx3_miso;
   wire fx3_mosi;
   wire fx3_sclk;

   wire pll_ce;
   wire pll_mosi;
   wire pll_sclk;

   // UART
   wire FPGA_RXD0;
   wire FPGA_TXD0;

   // Catalina Controls
   wire codec_enable;
   wire codec_en_agc;
   wire codec_reset;
   wire codec_sync;
   wire codec_txrx;
   wire [3:0] codec_ctrl_in;      // These should be outputs
   wire [7:0] codec_ctrl_out;      // MUST BE INPUT

   // Catalina Data
   wire       codec_data_clk_p;          // Clock from CAT (RX)
   wire       codec_fb_clk_p;           // Clock to CAT (TX)
   wire [11:0] rx_codec_d;
   wire [11:0] tx_codec_d;
   wire        rx_frame_p;
   wire        tx_frame_p;

   wire        cat_clkout_fpga;

   //always on 40MHz clock
   wire        codec_main_clk_p;
   wire        codec_main_clk_n;

   // Debug Bus
   wire [31:0] debug;
   wire [1:0]  debug_clk;

   // GPIF; FX3 Slave FIFO
   wire        IFCLK;        // pclk
   wire        FX3_EXTINT;
   wire        GPIF_CTL0;    // n_slcs
   wire        GPIF_CTL1;    // n_slwr
   wire        GPIF_CTL2;    // n_sloe
   wire        GPIF_CTL3;    // n_slrd
   wire        GPIF_CTL7;    // n_pktend
   wire        GPIF_CTL4;     // slfifo_flags[0]
   wire        GPIF_CTL5;     // slfifo_flags[1]
   wire        GPIF_CTL6;     // slfifo_flags[2]
   wire        GPIF_CTL8;     // slfifo_flags[3]
   wire        GPIF_CTL11;   // slfifo_addr[1]
   wire        GPIF_CTL12;   // slfifo_addr[0]
   wire [31:0] GPIF_D;
   wire        GPIF_CTL9;     // global_reset

   // GPS
   wire        gps_lock;
   wire        gps_rxd;
   wire        gps_txd;
   wire        gps_txd_nmea;

   // LEDS
   wire        LED_RX1;
   wire        LED_RX2;
   wire        LED_TXRX1_RX;
   wire        LED_TXRX1_TX;
   wire        LED_TXRX2_RX;
   wire        LED_TXRX2_TX;

   // Misc Hardware Control
   wire        ref_sel;
   wire        pll_lock;
   wire        FPGA_CFG_CS;           // Driven by FX3 gpio.
   wire        AUX_PWR_ON;            // Driven by FX3 gpio.

   // PPS
   wire        PPS_IN_EXT;
   wire        PPS_IN_INT;

   // RF Hardware Control
   wire        SFDX1_RX;
   wire        SFDX1_TX;
   wire        SFDX2_RX;
   wire        SFDX2_TX;
   wire        SRX1_RX;
   wire        SRX1_TX;
   wire        SRX2_RX;
   wire        SRX2_TX;
   wire        tx_bandsel_a;
   wire        tx_bandsel_b;
   wire        tx_enable1;
   wire        tx_enable2;
   wire        rx_bandsel_a;
   wire        rx_bandsel_b;
   wire        rx_bandsel_c;


   b200 b200_i1(
		// SPI Interfaces
		.cat_ce(),
		.cat_miso(),
		.cat_mosi(),
		.cat_sclk(),

		.fx3_ce(),
		.fx3_miso(),
		.fx3_mosi(),
		.fx3_sclk(),

		.pll_ce(),
		.pll_mosi(),
		.pll_sclk(),

		// UART
		.FPGA_RXD0(),
		.FPGA_TXD0(),

		// Catalina Controls
		.codec_enable(),
		.codec_en_agc(),
		.codec_reset(),
		.codec_sync(),
		.codec_txrx(),
		.codec_ctrl_in(),      // These should be outputs
		.codec_ctrl_out(),      // MUST BE INPUT

		// Catalina Data
		.codec_data_clk_p(),          // Clock from CAT (RX)
		.codec_fb_clk_p(),           // Clock to CAT (TX)
		.rx_codec_d(),
		.tx_codec_d(),
		.rx_frame_p(),
		.tx_frame_p(),

		.cat_clkout_fpga(),

		//always on 40MHz clock
		.codec_main_clk_p(),
		.codec_main_clk_n(),

		// Debug Bus
		.debug(),
		.debug_clk(),

		// GPIF, FX3 Slave FIFO
		.IFCLK(),        // pclk
		.FX3_EXTINT(),
		.GPIF_CTL0(),    // n_slcs
		.GPIF_CTL1(),    // n_slwr
		.GPIF_CTL2(),    // n_sloe
		.GPIF_CTL3(),    // n_slrd
		.GPIF_CTL7(),    // n_pktend
		.GPIF_CTL4(),     // slfifo_flags[0]
		.GPIF_CTL5(),     // slfifo_flags[1]
		.GPIF_CTL6(),     // slfifo_flags[2]
		.GPIF_CTL8(),     // slfifo_flags[3]
		.GPIF_CTL11(),   // slfifo_addr[1]
		.GPIF_CTL12(),   // slfifo_addr[0]
		.GPIF_D(),
		.GPIF_CTL9(),     // global_reset

		// GPS
		.gps_lock(),
		.gps_rxd(),
		.gps_txd(),
		.gps_txd_nmea(),

		// LEDS
		.LED_RX1(),
		.LED_RX2(),
		.LED_TXRX1_RX(),
		.LED_TXRX1_TX(),
		.LED_TXRX2_RX(),
		.LED_TXRX2_TX(),

		// Misc Hardware Control
		.ref_sel(),
		.pll_lock(),
		.FPGA_CFG_CS(),           // Driven by FX3 gpio.
		.AUX_PWR_ON(),            // Driven by FX3 gpio.

		// PPS
		.PPS_IN_EXT(),
		.PPS_IN_INT(),

		// RF Hardware Control
		.SFDX1_RX(),
		.SFDX1_TX(),
		.SFDX2_RX(),
		.SFDX2_TX(),
		.SRX1_RX(),
		.SRX1_TX(),
		.SRX2_RX(),
		.SRX2_TX(),
		.tx_bandsel_a(),
		.tx_bandsel_b(),
		.tx_enable1(),
		.tx_enable2(),
		.rx_bandsel_a(),
		.rx_bandsel_b(),
		.rx_bandsel_c()
		);

endmodule // b200_tb
