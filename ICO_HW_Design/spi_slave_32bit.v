module SPI_Slave #(
    parameter [15:0] MAX_COUNT = 512  // Must be even
)(
    input  clk,
    input  SPI_CLK,
    input  SPI_PICO,
    input  SPI_CS,
    output SPI_POCI,
    input [15:0] pitch_data,
    input [15:0] yaw_data,
    input rst,
    output reg [15:0] pitch_pwm,
    output reg [15:0] yaw_pwm
);

    // this shouldn't be synthesizable, this is for test bench
    initial begin
        pitch_pwm = MAX_COUNT >> 1;
        yaw_pwm = MAX_COUNT >> 1;
        bitcnt = 5'b000;
        bytes_received = 1'b0;
        bytes_data_received = 32'd0;
        byte_data_sent = 32'd0;
    end

    reg [2:0] SPI_CLKr;
    always @(posedge clk) begin
        if (rst) begin
            SPI_CLKr <= 3'b000;
        end else begin
            SPI_CLKr <= {SPI_CLKr[1:0], SPI_CLK}; 
        end
    end

    wire SPI_CLK_risingedge = (SPI_CLKr[2:1] == 2'b01); 
    wire SPI_CLK_fallingedge = (SPI_CLKr[2:1] == 2'b10); 

    reg [2:0] SPI_CSr;
    always @(posedge clk) begin
        if (rst) begin
            SPI_CSr <= 3'b111;  // Assuming CS is inactive high, start with high state
        end else begin
            SPI_CSr <= {SPI_CSr[1:0], SPI_CS};
        end
    end

    wire SPI_CS_active = ~SPI_CSr[1];
    wire SPI_CS_startmessage = (SPI_CSr[2:1] == 2'b10);
    wire SPI_CS_endmessage = (SPI_CSr[2:1] == 2'b01);

    reg [1:0] SPI_PICOr;
    always @(posedge clk) begin
        if (rst) begin
            SPI_PICOr <= 2'b00;
        end else begin
            SPI_PICOr <= {SPI_PICOr[0], SPI_PICO};
        end
    end

    wire SPI_PICO_data = SPI_PICOr[1];

    reg [4:0] bitcnt; 
    reg bytes_received; 
    reg [31:0] bytes_data_received;

    always @(posedge clk) begin 
        if (rst) begin
            bitcnt <= 5'b000;
            bytes_data_received <= 32'd0;
        end else begin
            if (~SPI_CS_active) begin
                bitcnt <= 5'b000;
            end else if (SPI_CLK_risingedge) begin
                bitcnt <= bitcnt + 5'b001;
                bytes_data_received <= {bytes_data_received[30:0], SPI_PICO_data};
            end
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            bytes_received <= 1'b0;
        end else begin
            bytes_received <= SPI_CS_active && SPI_CLK_risingedge && (bitcnt == 5'd31);
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            pitch_pwm <= MAX_COUNT >> 1;
            yaw_pwm   <= MAX_COUNT >> 1;
        end else if (bytes_received) begin
           {pitch_pwm, yaw_pwm} <= bytes_data_received;
        end
    end

    reg [31:0] byte_data_sent;

    always @(posedge clk) begin
        if (rst) begin
            byte_data_sent <= 32'd0;
        end else if (SPI_CS_active) begin
            if (SPI_CS_startmessage) begin
                byte_data_sent <= {pitch_data, yaw_data};
            end else if (SPI_CLK_fallingedge) begin
                byte_data_sent <= {byte_data_sent[30:0], 1'b0};
            end
        end else begin
            byte_data_sent <= 32'd0;
        end
    end

    assign SPI_POCI = byte_data_sent[31];

endmodule
