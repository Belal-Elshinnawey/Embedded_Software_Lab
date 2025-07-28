
module SPI_Slave(
    input  clk,
    input  SPI_CLK,
    input  SPI_PICO,
    input  SPI_CS,
    output SPI_POCI,
    input [63:0] alu_results,
    input rst,
    output reg [31:0] operand1,
    output reg [31:0] operand2
);

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

    reg [5:0] bitcnt; 
    reg bytes_received; 
    reg [63:0] bytes_data_received;

    always @(posedge clk) begin 
        if (rst) begin
            bitcnt <= 6'b000000;
            bytes_data_received <= 64'd0;
        end else begin
            if (~SPI_CS_active) begin
                bitcnt <= 6'b000000;
            end else if (SPI_CLK_risingedge) begin
                bitcnt <= bitcnt + 6'b000001;
                bytes_data_received <= {bytes_data_received[62:0], SPI_PICO_data};
            end
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            bytes_received <= 1'b0;
        end else begin
            bytes_received <= SPI_CS_active && SPI_CLK_risingedge && (bitcnt == 6'd63);
        end
    end

    always @(posedge clk) begin
        if (rst) begin
            operand1 <= 0;
            operand2 <= 0;
        end else if (bytes_received) begin
           {operand2, operand1} <= bytes_data_received;
        end
    end

    reg [63:0] byte_data_sent;

    always @(posedge clk) begin
        if (rst) begin
            byte_data_sent <= 64'd0;
        end else if (SPI_CS_active) begin
            if (SPI_CS_startmessage) begin
                byte_data_sent <= alu_results;
            end else if (SPI_CLK_fallingedge) begin
                byte_data_sent <= {byte_data_sent[62:0], 1'b0};
            end
        end else begin
            byte_data_sent <= 64'd0;
        end
    end
    assign SPI_POCI = byte_data_sent[63];
endmodule
