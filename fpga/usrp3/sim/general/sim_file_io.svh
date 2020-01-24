//
// Copyright 2015 Ettus Research LLC
//

`ifndef WORKING_DIR
  `define WORKING_DIR "."
`endif

`define ABSPATH(name) {`WORKING_DIR, "/", name}

typedef enum {
  READ, WRITE, APPEND
} fopen_mode_t;

typedef enum {
  HEX, DEC, OCT, BIN, FLOAT
} fformat_t;

// Create a handle to a data_file with
// - FILENAME: Name of the file
// - FORMAT: Data format (HEX, DEC, OCT, BIN, FLOAT)
// - DWIDTH: Width of each element stored in the file (one line per word)
//

//TODO: We would ideally use a class but that is not
//      supported by most simulators.
interface data_file_t #(
  parameter FILENAME = "test.hex", 
  parameter FORMAT = HEX, 
  parameter DWIDTH = 64
) (input clk);
  bit     is_open;
  integer handle;

  // Open the data file for reading or writing.
  //
  // Usage: open(mode)
  // where
  //  - mode: RW mode (Choose from: READ, WRITE, APPEND)
  //
  function open(fopen_mode_t mode = READ);
    if (mode == APPEND)
      handle  = $fopen(`ABSPATH(FILENAME), "a");
    else if (mode == WRITE)
      handle  = $fopen(`ABSPATH(FILENAME), "w");
    else
      handle  = $fopen(`ABSPATH(FILENAME), "r");

    if (handle == 0) begin
      $error("Could not open file: %s", `ABSPATH(FILENAME));
      $finish();
    end
    is_open = 1;
  endfunction


  // Close an open data file. No-op if file isn't already open
  //
  // Usage: close()
  //
  function close();
    $fclose(handle);
    handle  = 0;
    is_open = 0;
  endfunction

  // Is end-of-file reached.
  //
  // Usage: is_eof() Returns eof
  // where
  // - eof: A boolean
  //
  function logic is_eof();
    return ($feof(handle));
  endfunction

  // Read a line from the datafile
  //
  // Usage: readline() Returns data
  // where
  // - data: A logic array of width DWIDTH containing the read word
  //
  function logic [DWIDTH-1:0] readline();
    automatic logic [DWIDTH-1:0] word = 64'h0;
    automatic integer status;

    if (FORMAT == HEX)
      status = $fscanf(handle, "%x\n", word);
    else if (FORMAT == DEC)
      status = $fscanf(handle, "%d\n", word);
    else if (FORMAT == OCT)
      status = $fscanf(handle, "%o\n", word);
    else if (FORMAT == BIN)
      status = $fscanf(handle, "%b\n", word);
    else if (FORMAT == DEC)
      status = $fscanf(handle, "%g\n", word);
    else
      $error("Invalid format");

    return word;
  endfunction

  // Write a line to the datafile
  //
  // Usage: writeline(data) 
  // where
  // - data: A logic array of width DWIDTH to write to the file
  //
  function void writeline(logic [DWIDTH-1:0] word);
    if (FORMAT == HEX)
      $fdisplay(handle, "%x", word);
    else if (FORMAT == DEC)
      $fdisplay(handle, "%d", word);
    else if (FORMAT == OCT)
      $fdisplay(handle, "%o", word);
    else if (FORMAT == BIN)
      $fdisplay(handle, "%b", word);
    else if (FORMAT == DEC)
      $fdisplay(handle, "%g", word);
    else
      $error("Invalid format");
  endfunction

endinterface

