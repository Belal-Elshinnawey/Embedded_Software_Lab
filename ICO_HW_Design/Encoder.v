// no reason to find the encoder count. 
// this is a servo implementation and it doesnt even make a full rotation, so the PS can read one end position, 
// then another end position during the homing sequence, and use the difference to calculate the current position. 
// start the encoder count at some value in the middle ex: 32k
// the mid value is too high to reset to 0 in either direction
// and use the value as is.
module Encoder #(
    parameter ENCODER_MAX = 64000)(
    input clk,
    input rst,
    input A,
    input B,
    output reg [15:0] count
);
    reg A_sync_0, A_sync_1;
    reg B_sync_0, B_sync_1;
    reg prev_A, prev_B;
    wire [3:0] transition = {prev_A, prev_B, A_sync_1, B_sync_1};

    always @(posedge clk) begin
        if (rst) begin
            A_sync_0 <= 0; A_sync_1 <= 0;
            B_sync_0 <= 0; B_sync_1 <= 0;
        end else begin
            A_sync_0 <= A;
            A_sync_1 <= A_sync_0;
            B_sync_0 <= B;
            B_sync_1 <= B_sync_0;
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            count   <= ENCODER_MAX >> 1;
            prev_A  <= 0;
            prev_B  <= 0;
        end else begin
            case (transition)
                4'b0001, 4'b0111, 4'b1110, 4'b1000: begin
                    count <= (count == ENCODER_MAX - 1) ? 0 : count + 1;
                end
                4'b0010, 4'b1011, 4'b1101, 4'b0100: begin
                    count <= (count == 0) ? ENCODER_MAX - 1 : count - 1;
                end
                default:;
            endcase
            prev_A <= A_sync_1;
            prev_B <= B_sync_1;
        end
    end
endmodule
