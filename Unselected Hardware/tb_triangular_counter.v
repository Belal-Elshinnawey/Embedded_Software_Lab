// tb_triangular_counter.v
`timescale 1ns / 1ps

module tb_triangular_counter;

    localparam WIDTH = 8;
    localparam CLK_PERIOD = 10;

    reg clk = 0;
    reg rst;
    wire [WIDTH-1:0] value;

    // Instantiate the counter
    triangular_counter #(
        .WIDTH(WIDTH)
    ) uut (
        .clk(clk),
        .rst(rst),
        .value(value)
    );

    // Clock generator
    always #(CLK_PERIOD / 2) clk = ~clk;

    initial begin
        $dumpfile("triangular_counter.vcd");
        $dumpvars(0, tb_triangular_counter);

        rst = 1;
        #(5 * CLK_PERIOD);
        rst = 0;

        #(600 * CLK_PERIOD);  // Let it run for a few up-down cycles

        $finish;
    end

endmodule
