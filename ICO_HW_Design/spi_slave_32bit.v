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

    reg [2:0] SPI_CLKr; //3 registesr, 1 sync, then 2 to hold rising, and falling states
    always @(posedge clk) begin
        if (rst) begin
            SPI_CLKr <= 3'b000;
        end else begin
            SPI_CLKr <= {SPI_CLKr[1:0], SPI_CLK};  //Using registers to sync the SPI signals with the FPGA clock
        end
    end

    wire SPI_CLK_risingedge = (SPI_CLKr[2:1] == 2'b01);  // 0 then 1 means rising
    wire SPI_CLK_fallingedge = (SPI_CLKr[2:1] == 2'b10); // 1 then 0 means falling

    reg [2:0] SPI_CSr; //3 registers, same story as spi clk
    always @(posedge clk) begin
        if (rst) begin
            SPI_CSr <= 3'b111;
        end else begin
            SPI_CSr <= {SPI_CSr[1:0], SPI_CS}; // sync the CS signal with the register
        end
    end

    wire SPI_CS_active = ~SPI_CSr[1]; //Reverse the SPI CS line (CS is active low)
    wire SPI_CS_startmessage = (SPI_CSr[2:1] == 2'b10); // 1 then 0: Falling (starting transfere)
    wire SPI_CS_endmessage = (SPI_CSr[2:1] == 2'b01);  // 0 then 1: rising

    reg [1:0] SPI_PICOr; //Data line from PI to FPGA
    always @(posedge clk) begin
        if (rst) begin
            SPI_PICOr <= 2'b00;
        end else begin
            SPI_PICOr <= {SPI_PICOr[0], SPI_PICO}; //Shift the data in
        end
    end

    wire SPI_PICO_data = SPI_PICOr[1];

    reg [4:0] bitcnt; //counter to read 32 bits then reset to 0
    reg bytes_received; //High when a full transfere is done
    reg [31:0] bytes_data_received; //Holds the data received

    always @(posedge clk) begin 
        if (rst) begin
            bitcnt <= 5'b00000;
            bytes_data_received <= 32'd0;
        end else begin
            if (~SPI_CS_active) begin // If not active, then count is 0
                bitcnt <= 5'b00000;
            end else if (SPI_CLK_risingedge) begin //read data on clk rising edge
                bitcnt <= bitcnt + 5'b00001; //Increment the count by 1
                bytes_data_received <= {bytes_data_received[30:0], SPI_PICO_data}; // shift the data in
            end
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            bytes_received <= 1'b0;
        end else begin
            // when CS is active, and the clk is rising, and bitcnt reached 32 bits, then transfere is done
            bytes_received <= SPI_CS_active && SPI_CLK_risingedge && (bitcnt == 5'd31);
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            pitch_pwm <= MAX_COUNT >> 1;
            yaw_pwm   <= MAX_COUNT >> 1;
        end else if (bytes_received) begin
            // split the data to 2 PWM values
           {pitch_pwm, yaw_pwm} <= bytes_data_received;
        end
    end

    // Holds the data to be sent
    reg [31:0] byte_data_sent;

    always @(posedge clk) begin
        if (rst) begin
            byte_data_sent <= 32'd0;
        end else if (SPI_CS_active) begin // if CS is active
            if (SPI_CS_startmessage) begin
                byte_data_sent <= {pitch_data, yaw_data}; // combine encoder values
            end else if (SPI_CLK_fallingedge) begin //Shift data out on clk falling edge
                byte_data_sent <= {byte_data_sent[30:0], 1'b0}; //Data shifted out from the right
            end
        end else begin // if CS is not active, transmission register is cleared
            byte_data_sent <= 32'd0;
        end
    end

    assign SPI_POCI = byte_data_sent[31]; // The output SPI line is always the most left value

endmodule
