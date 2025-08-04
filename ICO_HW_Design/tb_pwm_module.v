`timescale 1ns / 1ps

module tb_PWM_Generator;

    parameter MAX_COUNT = 4096;
    parameter CLK_PERIOD = 10;

    reg clk;
    reg rst;
    reg [15:0] duty_cycle;
    wire pwm_out;
    wire dir_a;
    wire dir_b;

    PWM_Generator #(
        .MAX_COUNT(MAX_COUNT)
    ) Dut (
        .clk(clk),
        .rst(rst),
        .duty_cycle(duty_cycle),
        .pwm_out(pwm_out),
        .dir_a(dir_a),
        .dir_b(dir_b)
    );

    initial clk = 0;
    always #(CLK_PERIOD/2) clk = ~clk;

    initial begin
        $dumpfile("pwm_tb.vcd");
        $dumpvars(0, tb_PWM_Generator);

        rst = 1;
        duty_cycle = MAX_COUNT >> 1; // MIDPOINT is 0 duty
        #(5*CLK_PERIOD);
        rst = 0;
        #(10*CLK_PERIOD);

        // Test case 1: duty_cycle = 50% dir A
        duty_cycle = 2048+1024;
        #(5*2048*CLK_PERIOD);

        // Test case 2: duty_cycle = 50% dir B
        duty_cycle = 2048 - 1024;
        #(5*2048*CLK_PERIOD);

        // Test case 3: duty_cycle = 0%
        duty_cycle = 2048;
        #(5*2048*CLK_PERIOD);

        $display("Test completed.");
        $finish;
    end

endmodule
