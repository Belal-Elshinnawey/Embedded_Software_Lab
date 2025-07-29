module triangular_counter #(
    parameter WIDTH = 8
)(
    input  wire              clk,
    input  wire              rst,
    output reg [WIDTH-1:0]   value
);

    reg direction;  // 0 = up, 1 = down

    wire [WIDTH-1:0] max_value = (1 << WIDTH) - 1;

    always @(posedge clk) begin
        if (rst) begin
            value     <= 0;
            direction <= 0;
        end else begin
            if (!direction) begin
                if (value == max_value) begin
                    direction <= 1;
                    value <= value - 1;  // Immediately start counting down
                end else begin
                    value <= value + 1;
                end
            end else begin
                if (value == 0) begin
                    direction <= 0;
                    value <= value + 1;  // Immediately start counting up
                end else begin
                    value <= value - 1;
                end
            end
        end
    end

endmodule
