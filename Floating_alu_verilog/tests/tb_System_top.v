`timescale 1ns / 1ps

module tb_System_Top;

    // Parameters
    localparam CLK_PERIOD = 10;              // 100 MHz
    localparam SPI_CLK_HALF_PERIOD = 50;     // 10 MHz SPI clock (half-period)

    // DUT interface
    reg sys_clock = 0;
    reg reset = 1;

    reg SPI_CLK = 0;
    reg SPI_PICO = 0;
    reg SPI_CS = 1;
    wire SPI_POCI;

    reg opcode1 = 0;
    reg opcode2 = 0;
    reg opcode3 = 0;

    reg [31:0] n1;
	reg [31:0] n2;
    reg [63:0] tx_frame;
    reg [63:0] rx_frame;
    reg [63:0] observed_alu_results;
    reg [31:0] final_results;
    // Clock generator
    always #(CLK_PERIOD / 2) sys_clock = ~sys_clock;

    // Instantiate System_Top DUT
    System_Top dut (
        .sys_clock(sys_clock),
        .reset(reset),
        .SPI_CLK(SPI_CLK),
        .SPI_PICO(SPI_PICO),
        .SPI_CS(SPI_CS),
        .SPI_POCI(SPI_POCI),
        .opcode1(opcode1),
        .opcode2(opcode2),
        .opcode3(opcode3)
    );

    // SPI byte transfer task (MSB first)
    task spi_transfer_byte(input [7:0] tx_byte, output [7:0] rx_byte);
        integer i;
        begin
            rx_byte = 8'h00;
            for (i = 7; i >= 0; i = i - 1) begin
                SPI_PICO = tx_byte[i];
                #(SPI_CLK_HALF_PERIOD);
                SPI_CLK = 1;
                rx_byte[i] = SPI_POCI;
                #(SPI_CLK_HALF_PERIOD);
                SPI_CLK = 0;
            end
        end
    endtask

    // SPI full 64-bit frame transfer task
    task spi_transfer_frame(input [63:0] tx_frame, output [63:0] rx_frame);
        reg [7:0] rx_b0, rx_b1, rx_b2, rx_b3, rx_b4, rx_b5, rx_b6, rx_b7;
        begin
            SPI_CS = 0;

            spi_transfer_byte(tx_frame[63:56], rx_b0);
            spi_transfer_byte(tx_frame[55:48], rx_b1);
            spi_transfer_byte(tx_frame[47:40], rx_b2);
            spi_transfer_byte(tx_frame[39:32], rx_b3);
            spi_transfer_byte(tx_frame[31:24], rx_b4);
            spi_transfer_byte(tx_frame[23:16], rx_b5);
            spi_transfer_byte(tx_frame[15:8],  rx_b6);
            spi_transfer_byte(tx_frame[7:0],   rx_b7);

            #(SPI_CLK_HALF_PERIOD * 2);
            SPI_CS = 1;

            rx_frame = {rx_b0, rx_b1, rx_b2, rx_b3, rx_b4, rx_b5, rx_b6, rx_b7};
        end
    endtask

    // Optional: helper task to print result as float
    task print_float;
        input [31:0] bits;
        real fval;
        integer s;
        integer e;
        integer i;
        real m;
        begin
            s = bits[31];
            e = bits[30:23] - 127;
            m = 1.0;
            for (i = 0; i < 23; i = i + 1)
                if (bits[22 - i])
                    m = m + (1.0 / (1 << (i + 1)));

            fval = (s ? -1.0 : 1.0) * m * (2.0 ** e);
            $display("Float Value: %f", fval);
        end
    endtask

    initial begin
        // Waveform dump
        $dumpfile("system_top_tb.vcd");
        $dumpvars(0, tb_System_Top);

        // Reset
        reset = 1;
        #(10 * CLK_PERIOD);
        reset = 0;
        #(10 * CLK_PERIOD);

        // Choose operation (00 = add, 01 = sub, etc.)
        opcode1 = 0;
        opcode2 = 0;
        opcode3 = 0; // ignored for now

        // Prepare two float operands:
        // operand1 = 143.56 (0x430F8F5C)
        // operand2 = -87.437 (0xC2AEBDFE)
        n1 = 32'b01000011000011111000111101011100;    // 143.56
		n2 = 32'b11000010101011101101111110111110;    // -87.437

        tx_frame = {n1, n2};

        spi_transfer_frame(tx_frame, rx_frame);
        #50;
        spi_transfer_frame(tx_frame, rx_frame);
        observed_alu_results = rx_frame;
        final_results = observed_alu_results[31:0];
        $display("Sent Operands (TX frame): 0x%h", tx_frame);
        $display("Received ALU Result     : 0x%h", final_results);
        print_float(final_results);

        #100;
        $finish;
    end

endmodule
