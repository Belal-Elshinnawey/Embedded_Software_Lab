module triangular_pwm #(
    parameter DUTY_WIDTH = 8  // Also sets counter resolution
)(
    input  wire                   clk,
    input  wire                   rst,
    input  wire [DUTY_WIDTH-1:0]  duty,
    output reg                    pwm_out
);
    reg [DUTY_WIDTH-1:0] local_duty;
    always @(posedge clk) begin
        if (rst) begin
            local_duty <=0;
        end else if(counter == 0) begin
            local_duty <= duty;
        end

    end
    // Internal triangular counter
    reg [DUTY_WIDTH-1:0] counter;
    reg direction;  // 0 = up, 1 = down

    wire [DUTY_WIDTH-1:0] max_val = (1 << DUTY_WIDTH) - 1;

    // --- Counter logic (triangle waveform) ---
    always @(posedge clk) begin
        if (rst) begin
            counter   <= 0;
            direction <= 0;
        end else begin
            if (!direction) begin
                if (counter == max_val) begin
                    direction <= 1;
                    counter <= counter - 1;
                end else begin
                    counter <= counter + 1;
                end
            end else begin
                if (counter == 0) begin
                    direction <= 0;
                    counter <= counter + 1;
                end else begin
                    counter <= counter - 1;
                end
            end
        end
    end

    // --- PWM Output ---
    always @(posedge clk) begin
        if (rst) begin
            pwm_out <= 0;
        end else if (local_duty == 0) begin
            pwm_out <= 0;  // Special case: 0% local_duty
        end else if (local_duty == max_val) begin
            pwm_out <= 1;  // Special case: 100% local_duty
        end else begin
            pwm_out <= (counter < local_duty);
        end
    end

endmodule
