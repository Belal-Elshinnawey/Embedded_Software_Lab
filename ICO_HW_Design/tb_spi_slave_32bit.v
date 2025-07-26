`timescale 1ns / 1ps

module tb_SPI_Slave;

    // Parameters
    localparam CLK_PERIOD = 10;             // 100 MHz system clock
    localparam SPI_CLK_HALF_PERIOD = 50;    // 10 MHz SPI clock (half period for toggling)

    // DUT interface signals
    reg clk = 0;
    reg rst = 1;
    reg SPI_CLK = 0;
    reg SPI_PICO = 0;
    reg SPI_CS = 1;  // Active low
    wire SPI_POCI;
    reg [15:0] pitch_data = 16'hDEAD;
    reg [15:0] yaw_data   = 16'hBEEF;
    wire [15:0] pitch_pwm;
    wire [15:0] yaw_pwm;

    // Instantiate the Device Under Test (DUT)
    SPI_Slave uut (
        .clk(clk),
        .rst(rst),
        .SPI_CLK(SPI_CLK),
        .SPI_PICO(SPI_PICO),
        .SPI_CS(SPI_CS),
        .SPI_POCI(SPI_POCI),
        .pitch_data(pitch_data),
        .yaw_data(yaw_data),
        .pitch_pwm(pitch_pwm),
        .yaw_pwm(yaw_pwm)
    );

    // System clock generator
    always #(CLK_PERIOD / 2) clk = ~clk;

    // SPI full-duplex byte transfer
    task spi_transfer_byte(input [7:0] tx_byte, output [7:0] rx_byte);
        integer i;
        begin
            rx_byte = 8'h00;
            for (i = 7; i >= 0; i = i - 1) begin
                SPI_PICO = tx_byte[i];
                #(SPI_CLK_HALF_PERIOD);
                SPI_CLK = 1;
                rx_byte[i] = SPI_POCI;  // Sample on rising edge
                #(SPI_CLK_HALF_PERIOD);
                SPI_CLK = 0;
            end
        end
    endtask

    // SPI full-duplex 32-bit frame transfer (pitch + yaw)
    task spi_transfer_frame(input [31:0] tx_frame, output [31:0] rx_frame);
        reg [7:0] rx_b0, rx_b1, rx_b2, rx_b3;
        begin
            SPI_CS = 0;

            spi_transfer_byte(tx_frame[31:24], rx_b0);
            spi_transfer_byte(tx_frame[23:16], rx_b1);
            spi_transfer_byte(tx_frame[15:8],  rx_b2);
            spi_transfer_byte(tx_frame[7:0],   rx_b3);

            #(SPI_CLK_HALF_PERIOD * 2);
            SPI_CS = 1;

            rx_frame = {rx_b0, rx_b1, rx_b2, rx_b3};
        end
    endtask

    reg [31:0] tx_frame;
    reg [31:0] rx_frame;

    initial begin
        // VCD dump for waveform viewing
        $dumpfile("spi_slave_tb.vcd");
        $dumpvars(0, tb_SPI_Slave);

        // Reset sequence
        #(10 * CLK_PERIOD);
        rst = 0;

        // SPI frame to send: pitch = 0xBEEF, yaw = 0xDEAD
        #100;
        tx_frame = 32'hBEEFDEAD;

        spi_transfer_frame(tx_frame, rx_frame);

        // Wait and observe output
        #500;

        // Output results
        $display("Sent Frame     : 0x%h", tx_frame);
        $display("Received Frame : 0x%h", rx_frame);
        $display("pitch_pwm      : 0x%h", pitch_pwm);
        $display("yaw_pwm        : 0x%h", yaw_pwm);

        // Test verification
        if (pitch_pwm == 16'hBEEF && yaw_pwm == 16'hDEAD &&
            rx_frame[31:16] == pitch_data && rx_frame[15:0] == yaw_data) begin
            $display("Test PASSED.");
        end else begin
            $display("Test FAILED.");
        end

        #100;
        $finish;
    end

endmodule
