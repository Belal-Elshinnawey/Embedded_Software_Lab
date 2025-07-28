`timescale 1ns / 1ps

module Main_tb;
    reg clk = 0;
    localparam CLK_PERIOD = 10;
    always #(CLK_PERIOD/2) clk = ~clk;
	// Inputs
	reg [31:0] n1;
	reg [31:0] n2;
	reg [1:0] oper;

	// Outputs
	wire [31:0] result;
	wire Overflow;
	wire Underflow;
	wire Exception;

	// Instantiate the Unit Under Test (UUT)
	Main uut (
		.n1(n1), 
		.n2(n2), 
		.oper(oper), 
		.result(result), 
		.Overflow(Overflow), 
		.Underflow(Underflow), 
		.Exception(Exception)
	);
    
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
            for ( i = 0; i < 23; i = i + 1)
                if (bits[22 - i])
                    m = m + (1.0 / (1 << (i + 1)));
    
            fval = (s ? -1.0 : 1.0) * m * (2.0 ** e);
    
            $display("Float Value: %f", fval);
        end
    endtask

	initial begin
		// Initialize Inputs
		n1 = 32'b01000011000011111000111101011100;    // 143.56
		n2 = 32'b11000010101011101101111110111110;    // -87.437


		oper = 2'd0; #50;

		$display("Addtion result : %b",result);
		$display("Overflow : %b , Underflow : %b , Exception : %b",Overflow,Underflow,Exception);		
		print_float(result);
		oper = 2'd1; #50;

		$display("Subtraction result : %b",result);
		$display("Overflow : %b , Underflow : %b , Exception : %b",Overflow,Underflow,Exception);		
		print_float(result);
		oper = 2'd2; #50;

		$display("Multiplication result : %b",result);
		$display("Overflow : %b , Underflow : %b , Exception : %b",Overflow,Underflow,Exception);		
		print_float(result);
		oper = 2'd3; #50;

		$display("Division result : %b",result);
		$display("Overflow : %b , Underflow : %b , Exception : %b",Overflow,Underflow,Exception);
        print_float(result);


	end
      
endmodule

