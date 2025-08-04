
module mem_interface_top #(
    parameter DATA_WIDTH = 32
) (
    input  wire [7:0]  slave_address,
    input  wire        slave_read,
    output wire [DATA_WIDTH-1:0] slave_readdata,
    input  wire        slave_write,
    input  wire [DATA_WIDTH-1:0] slave_writedata,
    input  wire        clk,
    input  wire        reset,
    input  wire [(DATA_WIDTH/8)-1:0] slave_byteenable,
    output wire [DATA_WIDTH-1:0] pwm_vals,
    input  wire [DATA_WIDTH-1:0] encoder_vals,
    output wire [DATA_WIDTH-1:0]  reset_control
);

    wire [DATA_WIDTH-1:0] pwm_vals_reg;
    wire [DATA_WIDTH-1:0] encoder_vals_reg;
    wire [DATA_WIDTH-1:0] reset_control_reg;
    assign encoder_vals_reg = encoder_vals;
    assign pwm_vals  = pwm_vals_reg;
    assign reset_control_reg = reset_control;

    mem_interface #(
        .DATA_WIDTH(DATA_WIDTH)
    ) mem_ip_inst (
        .clk(clk),
        .reset(reset),
        .slave_address(slave_address),
        .slave_read(slave_read),
        .slave_readdata(slave_readdata),
        .slave_write(slave_write),
        .slave_writedata(slave_writedata),
        .slave_byteenable(slave_byteenable),
        .pwm_vals(pwm_vals_reg),
        .reset_control(reset_control_reg),
        .encoder_vals(encoder_vals_reg)
    );

endmodule
