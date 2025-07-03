module TopEntity (
    input clk_100mhz, // i was told this is 100mhz
    input reset, // used PI io 24, not sure if schematics are accurate.

    input  SPI_CLK,
    input  SPI_PICO,
    input  SPI_CS,
    output SPI_POCI,

    input PITCH_ENC_A,
    input PITCH_ENC_B,
    input YAW_ENC_A,
    input YAW_ENC_B,

    output YAW_DIRA,
    output YAW_DIRB,
    output YAW_PWM_VAL,

    output PITCH_DIRA,
    output PITCH_DIRB,
    output PITCH_PWM_VAL,

    output led2 // mostly unuesed
);
    //took this from the ico board github. PLL 
    wire clk_50mhz;
	wire pll_locked;
	SB_PLL40_PAD #(
		.FEEDBACK_PATH("SIMPLE"),
		.DELAY_ADJUSTMENT_MODE_FEEDBACK("FIXED"),
		.DELAY_ADJUSTMENT_MODE_RELATIVE("FIXED"),
		.PLLOUT_SELECT("GENCLK"),
		.FDA_FEEDBACK(4'b1111),
		.FDA_RELATIVE(4'b1111),
		.DIVR(4'b0000),
		.DIVF(7'b0000001),
		.DIVQ(3'b010),
		.FILTER_RANGE(3'b101)
	) pll (
		.PACKAGEPIN   (clk_100mhz),
		.PLLOUTGLOBAL (clk_50mhz),
		.LOCK         (pll_locked),
		.BYPASS       (1'b0      ),
		.RESETB       (1'b1      )
	);
    
	wire clk = clk_50mhz;
    localparam CLOCK_FREQ = 50_000_000; // these are not used, just for me though
    localparam PWM_FREQ   = 12_000;
    localparam MAX_COUNT = 4096; //counting up from 4096 to 8192 means 4096 of clock pulses == 50mhz/4096 = 12.2070312kHz, if you cant hear this and you're below 40, see a doctor
    wire sys_reset = reset; // wait untill pll clock locks with hard clock

    wire [15:0] pitch_position;
    wire [15:0] yaw_position;
    wire [15:0] yaw_pwm;
    wire [15:0] pitch_pwm;
    wire yaw_dir_a, yaw_dir_b;
    wire pitch_dir_a, pitch_dir_b;

    Encoder #(.ENCODER_MAX(64000)) pitch_encoder (
        .clk(clk),
        .rst(sys_reset),
        .A(PITCH_ENC_A),
        .B(PITCH_ENC_B),
        .count(pitch_position)
    );

    Encoder #(.ENCODER_MAX(64000)) yaw_encoder (
        .clk(clk),
        .rst(sys_reset),
        .A(YAW_ENC_A),
        .B(YAW_ENC_B),
        .count(yaw_position)
    );

    PWM_Generator #(
        .MAX_COUNT(MAX_COUNT) 
    ) yaw_pwm_generator (
        .clk(clk),
        .rst(sys_reset),
        .duty_cycle(yaw_pwm),
        .pwm_out(YAW_PWM_VAL),
        .dir_a(yaw_dir_a),
        .dir_b(yaw_dir_b)
    );

    PWM_Generator #(
        .MAX_COUNT(MAX_COUNT)
    ) pitch_pwm_generator (
        .clk(clk),
        .rst(sys_reset),
        .duty_cycle(pitch_pwm),
        .pwm_out(PITCH_PWM_VAL),
        .dir_a(pitch_dir_a),
        .dir_b(pitch_dir_b)
    );

    assign YAW_DIRA   = yaw_dir_a;
    assign YAW_DIRB   = yaw_dir_b;
    assign PITCH_DIRA = pitch_dir_a;
    assign PITCH_DIRB = pitch_dir_b;
    
    SPI_Slave #(
        .MAX_COUNT(MAX_COUNT) 
    )spi_interface (
        .clk(clk),
        .rst(sys_reset),
        .SPI_CLK(SPI_CLK),
        .SPI_CS(SPI_CS),
        .SPI_POCI(SPI_POCI),
        .SPI_PICO(SPI_PICO),
        .pitch_data(pitch_position),
        .yaw_data(yaw_position),
        .pitch_pwm(pitch_pwm),
        .yaw_pwm(yaw_pwm)
    );

endmodule
