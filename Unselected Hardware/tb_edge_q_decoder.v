`timescale 1ns / 1ps

module tb_edge_q_decoder;

    reg clk = 0;
    reg rst = 1;
    reg A_raw = 0, B_raw = 0;
    wire signed [31:0] position;

    localparam CLK_PERIOD = 10;

    edge_q_decoder uut (
        .clk(clk),
        .rst(rst),
        .A_raw(A_raw),
        .B_raw(B_raw),
        .position(position)
    );

    // Clock generation
    always #(CLK_PERIOD/2) clk = ~clk;

    // Generate quadrature waveform
    task quadrature_step(input integer steps, input integer direction);
        integer i;
        begin
            for (i = 0; i < steps; i = i + 1) begin
                if (direction == 1) begin
                    // Forward step sequence: 00 → 01 → 11 → 10 → 00
                    {A_raw, B_raw} = 2'b00; #(CLK_PERIOD);
                    {A_raw, B_raw} = 2'b01; #(CLK_PERIOD);
                    {A_raw, B_raw} = 2'b11; #(CLK_PERIOD);
                    {A_raw, B_raw} = 2'b10; #(CLK_PERIOD);
                end else begin
                    // Reverse step sequence: 00 → 10 → 11 → 01 → 00
                    {A_raw, B_raw} = 2'b00; #(CLK_PERIOD);
                    {A_raw, B_raw} = 2'b10; #(CLK_PERIOD);
                    {A_raw, B_raw} = 2'b11; #(CLK_PERIOD);
                    {A_raw, B_raw} = 2'b01; #(CLK_PERIOD);
                end
            end
            {A_raw, B_raw} = 2'b00; #(CLK_PERIOD);
        end
    endtask

    initial begin
        $dumpfile("edge_q_decoder.vcd");
        $dumpvars(0, tb_edge_q_decoder);

        #(10 * CLK_PERIOD);
        rst = 0;

        // Forward motion: 10,000 steps
        quadrature_step(100, 1);

        // Wait briefly
        #(100 * CLK_PERIOD);

        // Reverse motion: 10,000 steps
        quadrature_step(100, 0);

        #(100 * CLK_PERIOD);
        $display("Final position: %0d", position);  // Should be 0
        $finish;
    end

endmodule
