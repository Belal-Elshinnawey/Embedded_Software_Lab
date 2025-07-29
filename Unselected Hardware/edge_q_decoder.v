module edge_q_decoder (
    input  wire clk,
    input  wire rst,
    input  wire A_raw,
    input  wire B_raw,
    output reg signed [31:0] position
);

    // Synchronizers for A and B (2-stage)
    reg A_sync_0, A_sync_1;
    reg B_sync_0, B_sync_1;
    always @(posedge clk) begin
        A_sync_0 <= A_raw;
        A_sync_1 <= A_sync_0;
        B_sync_0 <= B_raw;
        B_sync_1 <= B_sync_0;
    end

    // Shift registers for edge detection
    reg [1:0] A_shift, B_shift;
    always @(posedge clk) begin
        A_shift <= {A_shift[0], A_sync_1};
        B_shift <= {B_shift[0], B_sync_1};
    end

    wire A_rise = (A_shift == 2'b01);
    wire A_fall = (A_shift == 2'b10);
    wire B_rise = (B_shift == 2'b01);
    wire B_fall = (B_shift == 2'b10);

    // Position update logic (4x decoding)
    always @(posedge clk) begin
        if (rst) begin
            position <= 0;
        end else begin
            if (A_rise)
                position <= position + (B_sync_1 ? -1 : 1);
            else if (A_fall)
                position <= position + (B_sync_1 ? 1 : -1);
            else if (B_rise)
                position <= position + (A_sync_1 ? 1 : -1);
            else if (B_fall)
                position <= position + (A_sync_1 ? -1 : 1);
        end
    end

endmodule
