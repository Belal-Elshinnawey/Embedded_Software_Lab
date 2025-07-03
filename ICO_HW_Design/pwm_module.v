module PWM_Generator #(
    parameter [15:0] MAX_COUNT = 512  // Must be even
)(
    input  wire        clk,
    input  wire        rst,
    input  wire [15:0] duty_cycle,    // 0 to MAX_COUNT
    output reg         pwm_out,
    output reg         dir_a,
    output reg         dir_b
);

    // Rationale: Make the pwm module start from the middle point
    // if the controller sends higher than the middle point, then its in the forward direction
    // if less then its the backward direction.
    // that way, the direction command is EMBEDDED in the pwm value
    // Midpoint constant
    localparam [15:0] MIDPOINT = MAX_COUNT >> 1;

    // Internal registers
    reg [15:0] counter;
    reg [15:0] local_duty;
    reg [15:0] counter_set_point;
    initial begin
        local_duty = MIDPOINT;
        dir_a = 0;
        dir_b = 0;
    end
    // PWM counter: counts from 0 to MAX_COUNT/2
    always @(posedge clk) begin
        if (rst)
            counter <= 0;
        else if (counter >= MIDPOINT - 1)
            counter <= 0;
        else
            counter <= counter + 1;
    end

    // Just in case the user feels hacky
    always @(posedge clk) begin
        if (rst)
            local_duty <= MIDPOINT;
        else if (duty_cycle > MAX_COUNT)
            local_duty <= MAX_COUNT;
        else
            local_duty <= duty_cycle;
    end

    // Compute counter set point
    always @(posedge clk) begin
        if (rst)
            counter_set_point <= MIDPOINT;
        else if (local_duty < MIDPOINT)
            counter_set_point <= MIDPOINT - local_duty;
        else
            counter_set_point <= local_duty - MIDPOINT;
    end

    // PWM output logic
    always @(posedge clk) begin
        if (rst)
            pwm_out <= 0;
        else
            pwm_out <= (counter < counter_set_point);
    end

    // Direction logic
    always @(posedge clk) begin
        if (rst) begin
            dir_a <= 0;
            dir_b <= 0;
        end else if (local_duty > MIDPOINT) begin
            dir_a <= 1;
            dir_b <= 0;
        end else if (local_duty < MIDPOINT) begin
            dir_a <= 0;
            dir_b <= 1;
        end else begin
            dir_a <= 0;
            dir_b <= 0; // this should be enough to make the h bridge output nothing, but it wasnt always the case, idk why
        end
    end

endmodule
