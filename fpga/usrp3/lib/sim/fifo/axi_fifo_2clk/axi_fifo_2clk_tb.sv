//
// Copyright 2016 Ettus Research
//

module axi_fifo_2clk_tb();

    localparam WIDTH    = 32;
    localparam SIZE     = 5;

    reg                 s_axis_clk;
    reg                 s_axis_rst;
    reg [WIDTH-1:0]     s_axis_tdata;
    reg                 s_axis_tvalid;
    reg                 s_axis_tlast;
    wire                s_axis_tready;
    reg                 m_axis_clk;
    reg                 m_axis_rst;
    wire [WIDTH-1:0]    m_axis_tdata;
    wire                m_axis_tvalid;
    wire                m_axis_tlast;
    reg                 m_axis_tready;
    wire [SIZE:0]       s_axis_occupied;
    wire                s_axis_full;
    wire                s_axis_empty;
    wire [SIZE:0]       m_axis_occupied;
    wire                m_axis_full;
    wire                m_axis_empty;

    axi_fifo_2clk #(.SIZE(SIZE),.WIDTH(WIDTH)) axi_fifo_2clk (
        .s_axis_clk(s_axis_clk),
        .s_axis_rst(s_axis_rst),
        .s_axis_tdata(s_axis_tdata),
        .s_axis_tvalid(s_axis_tvalid),
        .s_axis_tlast(s_axis_tlast),
        .s_axis_tready(s_axis_tready),
        .m_axis_clk(m_axis_clk),
        .m_axis_rst(m_axis_rst),
        .m_axis_tdata(m_axis_tdata),
        .m_axis_tvalid(m_axis_tvalid),
        .m_axis_tlast(m_axis_tlast),
        .m_axis_tready(m_axis_tready),
        .s_axis_occupied(s_axis_occupied),
        .s_axis_full(s_axis_full),
        .s_axis_empty(s_axis_empty),
        .m_axis_occupied(m_axis_occupied),
        .m_axis_full(m_axis_full),
        .m_axis_empty(m_axis_empty));

    `define S_AXIS_CLK_PERIOD 7
    initial begin
        s_axis_clk = 1'b0;
        forever begin
            #(`S_AXIS_CLK_PERIOD/2) s_axis_clk = ~s_axis_clk;
        end
    end

    `define S_AXIS_RESET_PERIOD 70
    initial begin
        s_axis_rst = 1'b1;
        #(`S_AXIS_RESET_PERIOD) s_axis_rst = 1'b0;
    end

    `define M_AXIS_CLK_PERIOD 10
    initial begin
        m_axis_clk = 1'b0;
        forever begin
            #(`M_AXIS_CLK_PERIOD/2) m_axis_clk = ~m_axis_clk;
        end
    end

    `define M_AXIS_RESET_PERIOD 100
    initial begin
        m_axis_rst = 1'b1;
        #(`M_AXIS_RESET_PERIOD) m_axis_rst = 1'b0;
    end

    initial begin
        @(posedge m_axis_clk);
        @(posedge s_axis_clk);
        s_axis_tdata  = 'd0;
        s_axis_tlast  = 1'b0;
        s_axis_tvalid = 1'b0;
        m_axis_tready = 1'b0;
        assert(~s_axis_full && ~m_axis_full)                 else $error("FIFO is full during reset!");
        assert(s_axis_empty == 1'b1 && m_axis_empty == 1'b1) else $error("FIFO is not empty during reset!");
        assert(s_axis_occupied == 0 && m_axis_occupied == 0) else $error("FIFO is occupied during reset!");
        while (s_axis_rst) @(negedge s_axis_rst);
        while (m_axis_rst) @(negedge m_axis_rst);
        @(posedge m_axis_clk);
        @(posedge s_axis_clk);
        assert(~s_axis_full && ~m_axis_full)                 else $error("FIFO is full after reset!");
        assert(s_axis_empty == 1'b1 && m_axis_empty == 1'b1) else $error("FIFO is not empty after reset!");
        assert(s_axis_occupied == 0 && m_axis_occupied == 0) else $error("FIFO is occupied after reset!");
        // Fill FIFO
        while (~s_axis_tready) @(posedge s_axis_clk);
        for (int i = 0; i < 1 << DEPTH_LOG2; i++) begin
            s_axis_tdata  = i+1'b1;
            s_axis_tvalid = 1'b1;
            @(posedge s_axis_clk);
        end
        repeat (6) @(posedge s_axis_clk);
        assert(s_axis_full && m_axis_full) else $error("Incorrect FIFO full flag!");
        assert(~s_axis_empty && ~m_axis_empty) else $error("Incorrect FIFO empty flag!");
        assert(s_axis_occupied == (1 << DEPTH_LOG2) && m_axis_occupied == (1 << DEPTH_LOG2)) else $error("Incorrect FIFO occupied count!");
        // Empty FIFO
        s_axis_tdata  = 'd0;
        s_axis_tvalid = 1'b0;
        @(posedge m_axis_clk);
        while (~m_axis_tvalid) @(posedge m_axis_clk);
        for (int i = 0; i < 1 << DEPTH_LOG2; i++) begin
            m_axis_tready = 1'b1;
            @(posedge m_axis_clk);
            assert(m_axis_tdata == i+1'b1) else $error("Incorrect FIFO data! (read)");
        end
        repeat (6) @(posedge m_axis_clk);
        assert(~s_axis_full && ~m_axis_full) else $error("Incorrect FIFO full flag!");
        assert(s_axis_empty && m_axis_empty) else $error("Incorrect FIFO empty flag!");
        assert(s_axis_occupied == 0 && m_axis_occupied == 0) else $error("Incorrect FIFO occupied count!");
    end

endmodule