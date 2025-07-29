`timescale 1ns / 1ps

module tb_compare_pwm;

    localparam DUTY_WIDTH = 8;
    localparam CLK_PERIOD = 10;

    reg clk = 0;
    reg rst;
    reg [DUTY_WIDTH-1:0] duty;

    wire pwm_edge;
    wire pwm_tri;

    // Instantiate edge-aligned PWM
    edge_pwm #(
        .DUTY_WIDTH(DUTY_WIDTH)
    ) edge_inst (
        .clk(clk),
        .rst(rst),
        .duty(duty),
        .pwm_out(pwm_edge)
    );

    // Instantiate triangular (center-aligned) PWM
    triangular_pwm #(
        .DUTY_WIDTH(7)
    ) tri_inst (
        .clk(clk),
        .rst(rst),
        .duty(duty>>1),
        .pwm_out(pwm_tri)
    );

    // Clock generation
    always #(CLK_PERIOD / 2) clk = ~clk;

    initial begin
        $dumpfile("compare_pwm.vcd");
        $dumpvars(0, tb_compare_pwm);

        rst = 1;
        duty = 0;
        #(5 * CLK_PERIOD);
        rst = 0;

        // Sweep through a few duty settings
        duty = 0;  #(3000 * CLK_PERIOD);
        duty = 8;  #(3000 * CLK_PERIOD);
        duty = 16;  #(3000 * CLK_PERIOD);
        duty = 32;  #(3000 * CLK_PERIOD);
        duty = 64;  #(3000 * CLK_PERIOD);
        duty = 128; #(3000 * CLK_PERIOD);
        duty = 192; #(3000 * CLK_PERIOD);
        duty = 255; #(3000 * CLK_PERIOD);

        $finish;
    end

endmodule
