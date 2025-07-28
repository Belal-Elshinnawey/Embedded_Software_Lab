module System_Top(
    input sys_clock,
    input reset,
    input SPI_CLK,
    input SPI_PICO,
    input SPI_CS,
    output SPI_POCI,
    input opcode1,
    input opcode2,
    input opcode3 
);

    // Internal wires
    wire clk;
    wire rst;

    wire [31:0] operand1;
    wire [31:0] operand2;
    wire [31:0] result;

    // Clock/reset assignment
    assign clk = sys_clock;
    assign rst = reset;

    // Instantiate Main ALU module
    Main alu_unit (
        .n1(operand1),
        .n2(operand2),
        .oper({opcode2, opcode1}),
        .result(result)
    );

    // Instantiate SPI_Slave module
    SPI_Slave spi_slave (
        .clk(clk),
        .rst(rst),
        .SPI_CLK(SPI_CLK),
        .SPI_PICO(SPI_PICO),
        .SPI_CS(SPI_CS),
        .SPI_POCI(SPI_POCI),
        .alu_results({32'b0, result}), // send 64-bit value: upper 32 bits = 0
        .operand1(operand1),
        .operand2(operand2)
    );

endmodule
