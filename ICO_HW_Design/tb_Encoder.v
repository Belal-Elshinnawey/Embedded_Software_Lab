`timescale 1ns / 1ps

module tb_Encoder;

    // Parameters
    localparam CLK_PERIOD = 10;  // 100MHz
    localparam ENCODER_MAX = 64000;

    // Inputs
    reg clk = 0;
    reg rst = 0;
    reg A = 0;
    reg B = 0;

    // Output
    wire [15:0] count;

    // Instantiate the Encoder
    Encoder #(
        .ENCODER_MAX(ENCODER_MAX)
    ) uut (
        .clk(clk),
        .rst(rst),
        .A(A),
        .B(B),
        .count(count)
    );

    // Clock generation
    always #(CLK_PERIOD/2) clk = ~clk;

    // Task to simulate one quadrature step
    task quadrature_step_forward;
        begin
            // 00 -> 01 -> 11 -> 10 -> 00
            A <= 0; B <= 0; #(CLK_PERIOD);
            A <= 0; B <= 1; #(CLK_PERIOD);
            A <= 1; B <= 1; #(CLK_PERIOD);
            A <= 1; B <= 0; #(CLK_PERIOD);
        end
    endtask

    task quadrature_step_backward;
        begin
            // 00 -> 10 -> 11 -> 01 -> 00
            A <= 0; B <= 0; #(CLK_PERIOD);
            A <= 1; B <= 0; #(CLK_PERIOD);
            A <= 1; B <= 1; #(CLK_PERIOD);
            A <= 0; B <= 1; #(CLK_PERIOD);
        end
    endtask

    initial begin
        $dumpfile("encoder.vcd");
        $dumpvars(0, tb_Encoder);

        // Initial reset
        rst <= 1;
        #(2 * CLK_PERIOD);
        rst <= 0;

        // Show initial count
        $display("Initial Count = %d", count);
        #(CLK_PERIOD);

        // Simulate 4 forward steps
        $display("Simulating forward steps");
        repeat (4) quadrature_step_forward;

        #(2 * CLK_PERIOD);
        $display("Count after forward steps = %d", count);

        // Simulate 4 backward steps
        $display("Simulating backward steps");
        repeat (4) quadrature_step_backward;

        #(2 * CLK_PERIOD);
        $display("Count after backward steps = %d", count);

        $finish;
    end

endmodule
