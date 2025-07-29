module edge_pwm #(
    parameter DUTY_WIDTH = 8
)(
    input  wire                   clk,
    input  wire                   rst,
    input  wire [DUTY_WIDTH-1:0]  duty,
    output reg                    pwm_out
);

    reg [DUTY_WIDTH-1:0] counter;

    reg [DUTY_WIDTH-1:0] local_duty;
    always @(posedge clk) begin
        if (rst) begin
            local_duty <=0;
        end else if(counter == 0) begin
            local_duty <= duty;
        end

    end

    wire [DUTY_WIDTH-1:0] max_val = (1 << DUTY_WIDTH) - 1;

    // --- Counter logic ---
    always @(posedge clk) begin
        if (rst)
            counter <= 0;
        else
            counter <= (counter == max_val) ? 0 : counter + 1;
    end

    // --- PWM output logic with edge-aligned style ---
    always @(posedge clk) begin
        if (rst)
            pwm_out <= 0;
        else if (local_duty == 0)
            pwm_out <= 0;
        else if (local_duty == max_val)
            pwm_out <= 1;
        else
            pwm_out <= (counter < local_duty);
    end

endmodule
