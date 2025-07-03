
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
    // this shouldn't be synthisizable, this is for test bench
    initial begin
        current_write_state = WRITE_PITCH1;
        next_write_state = WRITE_PITCH1;
        pitch_pwm = MAX_COUNT >> 1;
        yaw_pwm = MAX_COUNT >> 1;
        bitcnt = 3'b000;
        byte_received = 1'b0;
        byte_data_received = 8'd0;
        bitcnt            = 3'b000;
    end

    // a single transfere is both a read and a write. a state machine for writing to PWM, and a state machine for reading encoders work together.
    // this is better than command receive as it reduces the overhead. Not the most reliable, and requires synchronization
    reg [2:0] ones_counter; // vector used for synchronization
    reg reset_read_state;   // signal from the write state to reset the read state

    // write pwm states
    localparam [1:0] WRITE_PITCH1 = 2'b00;
    localparam [1:0] WRITE_PITCH2 = 2'b01;
    localparam [1:0] WRITE_YAW1   = 2'b10;
    localparam [1:0] WRITE_YAW2   = 2'b11;
    reg [1:0] current_write_state, next_write_state;

    // read encoder states
    localparam [1:0] READ_PITCH1 = 2'b00;
    localparam [1:0] READ_PITCH2 = 2'b01;
    localparam [1:0] READ_YAW1   = 2'b10;
    localparam [1:0] READ_YAW2   = 2'b11;
    reg [1:0] current_read_state, next_read_state;

    // sync the local clock by using the 3 register method (r,l,f)
    reg [2:0] SPI_CLKr;
    always @(posedge clk) SPI_CLKr <= {SPI_CLKr[1:0], SPI_CLK}; // shift in the external clock, and update the local clock register (most right is most recent)
    wire SPI_CLK_risingedge = (SPI_CLKr[2:1] == 2'b01); // if the most righ is _/ {0 then 1} then its a rising edge
    wire SPI_CLK_fallingedge = (SPI_CLKr[2:1] == 2'b10); 

    reg [2:0] SPI_CSr;// same sync for CS line
    always @(posedge clk) SPI_CSr <= {SPI_CSr[1:0], SPI_CS};
    wire SPI_CS_active = ~SPI_CSr[1];
    wire SPI_CS_startmessage = (SPI_CSr[2:1] == 2'b10);
    wire SPI_CS_endmessage = (SPI_CSr[2:1] == 2'b01);

    reg [1:0] SPI_PICOr;// same sync for input line MOSI
    always @(posedge clk) SPI_PICOr <= {SPI_PICOr[0], SPI_PICO};
    wire SPI_PICO_data = SPI_PICOr[1];
    // counter to go up to 8
    reg [2:0] bitcnt; 
    // could make this count to 32 and send a full read and write in a single send, but this works so no reason to change it unless too slow
    reg byte_received; //signal for full byte transfere
    reg [7:0] byte_data_received; // data shift reg

    always @(posedge clk) begin // counter and shift data in logic
        if (~SPI_CS_active) begin
            bitcnt <= 3'b000;
        end else if (SPI_CLK_risingedge) begin
            bitcnt <= bitcnt + 3'b001;
            byte_data_received <= {byte_data_received[6:0], SPI_PICO_data};
        end
    end

    // if its been 8 bits, and the clk rising (going to inactive mode) then its a full byte received.
    always @(posedge clk) byte_received <= SPI_CS_active && SPI_CLK_risingedge && (bitcnt == 3'b111);

    // update the state
    always @(posedge clk) begin
        if (rst)
            current_write_state <= WRITE_PITCH1;
        else if (byte_received) 
            current_write_state <= next_write_state;
    end

    // next state update
    always @(*) begin
        if (byte_received) begin // not needed, the next state tracks the current state regardless if received or not
            case (current_write_state)
                WRITE_PITCH1: next_write_state = WRITE_PITCH2;
                WRITE_PITCH2: next_write_state = WRITE_YAW1;
                WRITE_YAW1:   next_write_state = WRITE_YAW2;
                WRITE_YAW2:   next_write_state = WRITE_PITCH1;
                default:      next_write_state = WRITE_PITCH1;
            endcase
        end else begin
            next_write_state = current_write_state;
        end
    end

    // writing data based on state
    always @(posedge clk) begin
        if (rst) begin
            pitch_pwm <= MAX_COUNT >> 1;
            yaw_pwm   <= MAX_COUNT >> 1;
            ones_counter <= 0;
            reset_read_state <=0;
        end else if (byte_received) begin
            case (current_write_state)
                WRITE_PITCH1: pitch_pwm[15:8] <= byte_data_received;
                WRITE_PITCH2: pitch_pwm[7:0]  <= byte_data_received;
                WRITE_YAW1:   yaw_pwm[15:8]   <= byte_data_received;
                WRITE_YAW2:   yaw_pwm[7:0]    <= byte_data_received;
            endcase
        end
    end

    reg [7:0] byte_data_sent;
    always @(posedge clk) begin
        if (rst) // reset if hard reset or if the sync sequence was received.
            current_read_state <= READ_PITCH1;
        else if (SPI_CS_endmessage) 
            current_read_state <= next_read_state;
    end

    always @(*) begin
        if (SPI_CS_endmessage) begin
            case (current_read_state)
                READ_PITCH1: next_read_state = READ_PITCH2;
                READ_PITCH2: next_read_state = READ_YAW1;
                READ_YAW1:   next_read_state = READ_YAW2;
                READ_YAW2:   next_read_state = READ_PITCH1;
                default:     next_read_state = READ_PITCH1;
            endcase
        end else begin
            next_read_state = current_read_state;
        end
    end

    always @(posedge clk) begin
        if (SPI_CS_active) begin
            if (SPI_CS_startmessage) begin
                case (current_read_state)
                    READ_PITCH1: byte_data_sent <= pitch_data[15:8];
                    READ_PITCH2: byte_data_sent <= pitch_data[7:0];
                    READ_YAW1:   byte_data_sent <= yaw_data[15:8];
                    READ_YAW2:   byte_data_sent <= yaw_data[7:0];
                endcase
            end
            else if (SPI_CLK_fallingedge) begin
                byte_data_sent <= {byte_data_sent[6:0], 1'b0};
            end
        end
    end

    assign SPI_POCI = byte_data_sent[7];
endmodule
