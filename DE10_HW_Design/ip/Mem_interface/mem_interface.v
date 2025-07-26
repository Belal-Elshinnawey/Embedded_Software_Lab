`timescale 1 ps / 1 ps

module mem_interface #(
    parameter DATA_WIDTH = 32
) (
    input  wire clk,
    input  wire reset,

    // Avalon-MM slave interface
    input  wire [7:0]  slave_address,
    input  wire        slave_read,
    output reg  [DATA_WIDTH-1:0] slave_readdata,
    input  wire        slave_write,
    input  wire [DATA_WIDTH-1:0] slave_writedata,
    input  wire [(DATA_WIDTH/8)-1:0] slave_byteenable,  

    // External registers
    output reg [DATA_WIDTH-1:0] pwm_vals,
    output reg [DATA_WIDTH-1:0] reset_control,
    input  wire [DATA_WIDTH-1:0] encoder_vals
);

    localparam PWM_ADDR     = 8'h00;
    localparam ENCODER_ADDR = 8'h04;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            pwm_vals <= {16'd2048,16'd2048}; //pwm is 0 in the midpoint. maybe parameterize this later
            slave_readdata <= {DATA_WIDTH{1'b0}};
        end else begin
            if (slave_read) begin
                slave_readdata <= encoder_vals;
            end


            if (slave_write) begin
                pwm_vals <= slave_writedata;  
            end

        end
    end

endmodule
