// -- (c) Copyright 2009 - 2011 Xilinx, Inc. All rights reserved.
// --
// -- This file contains confidential and proprietary information
// -- of Xilinx, Inc. and is protected under U.S. and 
// -- international copyright and other intellectual property
// -- laws.
// --
// -- DISCLAIMER
// -- This disclaimer is not a license and does not grant any
// -- rights to the materials distributed herewith. Except as
// -- otherwise provided in a valid license issued to you by
// -- Xilinx, and to the maximum extent permitted by applicable
// -- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// -- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// -- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// -- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// -- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// -- (2) Xilinx shall not be liable (whether in contract or tort,
// -- including negligence, or under any other theory of
// -- liability) for any loss or damage of any kind or nature
// -- related to, arising under or in connection with these
// -- materials, including for any direct, or any indirect,
// -- special, incidental, or consequential loss or damage
// -- (including loss of data, profits, goodwill, or any type of
// -- loss or damage suffered as a result of any action brought
// -- by a third party) even if such damage or loss was
// -- reasonably foreseeable or Xilinx had been advised of the
// -- possibility of the same.
// --
// -- CRITICAL APPLICATIONS
// -- Xilinx products are not designed or intended to be fail-
// -- safe, or for use in any application requiring fail-safe
// -- performance, such as life-support or safety devices or
// -- systems, Class III medical devices, nuclear facilities,
// -- applications related to the deployment of airbags, or any
// -- other applications that could lead to death, personal
// -- injury, or severe property or environmental damage
// -- (individually and collectively, "Critical
// -- Applications"). Customer assumes the sole risk and
// -- liability of any use of Xilinx products in Critical
// -- Applications, subject only to applicable laws and
// -- regulations governing limitations on product liability.
// --
// -- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// -- PART OF THIS FILE AT ALL TIMES.
//-----------------------------------------------------------------------------
//
// File name: axi_interconnect_v1_06_a.v
//
// Description: 
//   This is the top-level module of a N-master to 1-slave AXI Interconnect core.
//   The interface of this module consists of a set of enumerated slave and master 
//     AXI interfaces, plus a set of enumerated parameters associated with each
//     AXI interface and with the core as a whole.
//   This module concatenates the top-level AXI slave interfaces into
//     a vectored slave master interface passed to the axi_interconnect
//     module below. It also concatenates the top-level enumerated parameters 
//     associated with the slave interfaces into vectored parameters.
//
//-----------------------------------------------------------------------------
// Structure:
//   axi_interconnect_v1_06_a
//     axi_interconnect
//
`timescale 1ps/1ps
`default_nettype none

module axi_interconnect_v1_06_a #
  (
   parameter         C_FAMILY                         = "rtl", 
                       // FPGA Family.
   parameter integer C_NUM_SLAVE_PORTS                = 2, 
                       // RANGE = (1:16)
                       // Number of Slave Interfaces (SI) for connecting 
                       // to master IP.
   parameter integer C_THREAD_ID_WIDTH                = 0, 
                       // Bits of ID signals sampled at each SI (global).
                       // RANGE = (0:8)
   parameter integer C_THREAD_ID_PORT_WIDTH           = 1, 
                       // RANGE = (1:8)
                       // Width of ID ports on each SI (global).
   parameter integer C_AXI_ADDR_WIDTH                 = 32, 
                       // RANGE = (12:64)
                       // Width of S_AXI_AWADDR, S_AXI_ARADDR, M_AXI_AWADDR and 
                       // M_AXI_ARADDR for every SI/MI.
   parameter integer C_S00_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S01_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S02_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S03_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S04_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S05_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S06_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S07_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S08_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S09_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S10_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S11_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S12_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S13_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S14_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
   parameter integer C_S15_AXI_DATA_WIDTH             = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
                       // Width of WDATA and RDATA per SI.
   parameter integer C_M00_AXI_DATA_WIDTH               = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
                       // Width of WDATA and RDATA on MI.
   parameter integer C_INTERCONNECT_DATA_WIDTH        = 32, 
                       // RANGE = (32, 64, 128, 256, 512, 1024)
                       // Data width of the internal interconnect write and read 
                       // data paths.
   parameter         C_S00_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S00_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S01_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S01_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S02_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S02_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S03_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S03_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S04_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S04_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S05_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S05_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S06_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S06_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S07_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S07_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S08_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S08_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S09_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S09_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S10_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S10_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S11_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S11_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S12_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S12_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S13_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S13_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S14_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S14_AXI_IS_ACLK_ASYNC == 0)
   parameter         C_S15_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_S15_AXI_IS_ACLK_ASYNC == 0)
                       // Clock frequency ratio of each SI w.r.t. internal interconnect.
                       // Ignored if SI clock is async.
   parameter         C_S00_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S01_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S02_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S03_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S04_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S05_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S06_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S07_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S08_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S09_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S10_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S11_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S12_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S13_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S14_AXI_IS_ACLK_ASYNC        = 1'b0,
   parameter         C_S15_AXI_IS_ACLK_ASYNC        = 1'b0,
                       // SI clock is asynchronous w.r.t. Interconnect clock (Boolean)
   parameter         C_M00_AXI_ACLK_RATIO             = "1:1", 
                       // VALUES = ("1:16", "1:15", "1:14", "1:13", "1:12", "1:11", "1:10", "1:9", "1:8", "1:7", "1:6", "1:5", "1:4", "1:3", "1:2", "1:1", "2:1", "3:1", "4:1", "5:1", "6:1", "7:1", "8:1", "9:1", "10:1", "11:1", "12:1", "13:1", "14:1", "15:1", "16:1")
                       // ISVALID = (C_M00_AXI_IS_ACLK_ASYNC == 0)
                       // Clock frequency ratio of each MI w.r.t. internal interconnect.
                       // Ignored if MI clock is async.
   parameter         C_M00_AXI_IS_ACLK_ASYNC          = 1'b0,
                       // MI clock is asynchronous w.r.t. Interconnect clock (Boolean)
   parameter         C_S00_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S01_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S02_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S03_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S04_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S05_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S06_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S07_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S08_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S09_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S10_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S11_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S12_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S13_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S14_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
   parameter         C_S15_AXI_READ_WRITE_SUPPORT     = "READ/WRITE", 
                       // VALUES = ("READ/WRITE", "READ-ONLY", "WRITE-ONLY")
                       // Indicates whether each SI supports read/write transactions.
   parameter integer C_S00_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S01_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S02_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S03_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S04_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S05_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S06_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S07_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S08_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S09_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S10_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S11_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S12_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S13_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S14_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S15_AXI_WRITE_ACCEPTANCE       = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
                       // Maximum number of active write transactions that each SI 
                       // can accept.
   parameter integer C_S00_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S01_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S02_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S03_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S04_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S05_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S06_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S07_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S08_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S09_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S10_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S11_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S12_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S13_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S14_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
   parameter integer C_S15_AXI_READ_ACCEPTANCE        = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
                       // Maximum number of active read transactions that each SI 
                       // can accept.
   parameter integer C_M00_AXI_WRITE_ISSUING          = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
                       // Maximum number of data-active write transactions that 
                       // the MI can generate at any one time.
   parameter integer C_M00_AXI_READ_ISSUING           = 1,
                       // RANGE = (1, 2, 4, 8, 16, 32)
                       // Maximum number of active read transactions that 
                       // the MI can generate at any one time.
   parameter integer C_S00_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S01_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S02_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S03_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S04_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S05_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S06_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S07_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S08_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S09_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S10_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S11_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S12_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S13_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S14_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
   parameter integer C_S15_AXI_ARB_PRIORITY           = 0,
                       // RANGE = (0:15)
                       // Arbitration priority among each SI. 
                       // Higher values indicate higher priority.
   parameter integer C_S00_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S01_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S02_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S03_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S04_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S05_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S06_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S07_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S08_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S09_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S10_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S11_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S12_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S13_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S14_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S15_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
                       // Depth of SI-side write data FIFO per SI.
   parameter integer C_S00_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S01_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S02_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S03_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S04_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S05_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S06_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S07_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S08_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S09_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S10_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S11_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S12_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S13_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S14_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
   parameter integer C_S15_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
                       // Depth of SI-side read data FIFO per SI.
   parameter integer C_M00_AXI_WRITE_FIFO_DEPTH       = 0,
                       // RANGE = (0, 32, 512)
                       // Depth of MI-side write data FIFO.
   parameter integer C_M00_AXI_READ_FIFO_DEPTH        = 0,
                       // RANGE = (0, 32, 512)
                       // Depth of MI-side read data FIFO.
   parameter integer C_S00_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S01_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S02_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S03_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S04_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S05_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S06_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S07_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S08_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S09_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S10_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S11_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S12_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S13_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S14_AXI_WRITE_FIFO_DELAY       = 1'b0,
   parameter integer C_S15_AXI_WRITE_FIFO_DELAY       = 1'b0,
                       // SI-side write channel packet FIFO, per SI.
   parameter integer C_S00_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S01_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S02_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S03_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S04_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S05_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S06_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S07_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S08_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S09_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S10_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S11_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S12_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S13_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S14_AXI_READ_FIFO_DELAY        = 1'b0,
   parameter integer C_S15_AXI_READ_FIFO_DELAY        = 1'b0,
                       // SI-side read channel packet FIFO, per SI.
   parameter integer C_M00_AXI_WRITE_FIFO_DELAY       = 1'b0,
                       // MI-side write channel packet FIFO.
   parameter integer C_M00_AXI_READ_FIFO_DELAY        = 1'b0,
                       // MI-side read channel packet FIFO.
   parameter         C_S00_AXI_REGISTER            = 1'b0,
   parameter         C_S01_AXI_REGISTER            = 1'b0,
   parameter         C_S02_AXI_REGISTER            = 1'b0,
   parameter         C_S03_AXI_REGISTER            = 1'b0,
   parameter         C_S04_AXI_REGISTER            = 1'b0,
   parameter         C_S05_AXI_REGISTER            = 1'b0,
   parameter         C_S06_AXI_REGISTER            = 1'b0,
   parameter         C_S07_AXI_REGISTER            = 1'b0,
   parameter         C_S08_AXI_REGISTER            = 1'b0,
   parameter         C_S09_AXI_REGISTER            = 1'b0,
   parameter         C_S10_AXI_REGISTER            = 1'b0,
   parameter         C_S11_AXI_REGISTER            = 1'b0,
   parameter         C_S12_AXI_REGISTER            = 1'b0,
   parameter         C_S13_AXI_REGISTER            = 1'b0,
   parameter         C_S14_AXI_REGISTER            = 1'b0,
   parameter         C_S15_AXI_REGISTER            = 1'b0,
                       // Insert register slice on all channels at each SI (Boolean).
   parameter         C_M00_AXI_REGISTER           = 1'b0
                       // Insert register slice on all channels at the MI (Boolean).
   )
  (
   // System Signals
   input  wire                                 INTERCONNECT_ACLK,
   input  wire                                 INTERCONNECT_ARESETN,

   // Slave Interface S00
   output wire                                 S00_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 0)
   input  wire                                 S00_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S00_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S00_AXI_AWADDR,
   input  wire [7:0]                           S00_AXI_AWLEN,
   input  wire [2:0]                           S00_AXI_AWSIZE,
   input  wire [1:0]                           S00_AXI_AWBURST,
   input  wire                                 S00_AXI_AWLOCK,
   input  wire [3:0]                           S00_AXI_AWCACHE,
   input  wire [2:0]                           S00_AXI_AWPROT,
   input  wire [3:0]                           S00_AXI_AWQOS,
   input  wire                                 S00_AXI_AWVALID,
   output wire                                 S00_AXI_AWREADY,
   input  wire [C_S00_AXI_DATA_WIDTH-1:0]      S00_AXI_WDATA,
   input  wire [C_S00_AXI_DATA_WIDTH/8-1:0]    S00_AXI_WSTRB,
   input  wire                                 S00_AXI_WLAST,
   input  wire                                 S00_AXI_WVALID,
   output wire                                 S00_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S00_AXI_BID,
   output wire [1:0]                           S00_AXI_BRESP,
   output wire                                 S00_AXI_BVALID,
   input  wire                                 S00_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S00_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S00_AXI_ARADDR,
   input  wire [7:0]                           S00_AXI_ARLEN,
   input  wire [2:0]                           S00_AXI_ARSIZE,
   input  wire [1:0]                           S00_AXI_ARBURST,
   input  wire                                 S00_AXI_ARLOCK,
   input  wire [3:0]                           S00_AXI_ARCACHE,
   input  wire [2:0]                           S00_AXI_ARPROT,
   input  wire [3:0]                           S00_AXI_ARQOS,
   input  wire                                 S00_AXI_ARVALID,
   output wire                                 S00_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S00_AXI_RID,
   output wire [C_S00_AXI_DATA_WIDTH-1:0]      S00_AXI_RDATA,
   output wire [1:0]                           S00_AXI_RRESP,
   output wire                                 S00_AXI_RLAST,
   output wire                                 S00_AXI_RVALID,
   input  wire                                 S00_AXI_RREADY,
   
   // Slave Interface S01
   output wire                                 S01_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 1)
   input  wire                                 S01_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S01_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S01_AXI_AWADDR,
   input  wire [7:0]                           S01_AXI_AWLEN,
   input  wire [2:0]                           S01_AXI_AWSIZE,
   input  wire [1:0]                           S01_AXI_AWBURST,
   input  wire                                 S01_AXI_AWLOCK,
   input  wire [3:0]                           S01_AXI_AWCACHE,
   input  wire [2:0]                           S01_AXI_AWPROT,
   input  wire [3:0]                           S01_AXI_AWQOS,
   input  wire                                 S01_AXI_AWVALID,
   output wire                                 S01_AXI_AWREADY,
   input  wire [C_S01_AXI_DATA_WIDTH-1:0]      S01_AXI_WDATA,
   input  wire [C_S01_AXI_DATA_WIDTH/8-1:0]    S01_AXI_WSTRB,
   input  wire                                 S01_AXI_WLAST,
   input  wire                                 S01_AXI_WVALID,
   output wire                                 S01_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S01_AXI_BID,
   output wire [1:0]                           S01_AXI_BRESP,
   output wire                                 S01_AXI_BVALID,
   input  wire                                 S01_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S01_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S01_AXI_ARADDR,
   input  wire [7:0]                           S01_AXI_ARLEN,
   input  wire [2:0]                           S01_AXI_ARSIZE,
   input  wire [1:0]                           S01_AXI_ARBURST,
   input  wire                                 S01_AXI_ARLOCK,
   input  wire [3:0]                           S01_AXI_ARCACHE,
   input  wire [2:0]                           S01_AXI_ARPROT,
   input  wire [3:0]                           S01_AXI_ARQOS,
   input  wire                                 S01_AXI_ARVALID,
   output wire                                 S01_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S01_AXI_RID,
   output wire [C_S01_AXI_DATA_WIDTH-1:0]      S01_AXI_RDATA,
   output wire [1:0]                           S01_AXI_RRESP,
   output wire                                 S01_AXI_RLAST,
   output wire                                 S01_AXI_RVALID,
   input  wire                                 S01_AXI_RREADY,
   
   // Slave Interface S02
   output wire                                 S02_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 2)
   input  wire                                 S02_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S02_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S02_AXI_AWADDR,
   input  wire [7:0]                           S02_AXI_AWLEN,
   input  wire [2:0]                           S02_AXI_AWSIZE,
   input  wire [1:0]                           S02_AXI_AWBURST,
   input  wire                                 S02_AXI_AWLOCK,
   input  wire [3:0]                           S02_AXI_AWCACHE,
   input  wire [2:0]                           S02_AXI_AWPROT,
   input  wire [3:0]                           S02_AXI_AWQOS,
   input  wire                                 S02_AXI_AWVALID,
   output wire                                 S02_AXI_AWREADY,
   input  wire [C_S02_AXI_DATA_WIDTH-1:0]      S02_AXI_WDATA,
   input  wire [C_S02_AXI_DATA_WIDTH/8-1:0]    S02_AXI_WSTRB,
   input  wire                                 S02_AXI_WLAST,
   input  wire                                 S02_AXI_WVALID,
   output wire                                 S02_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S02_AXI_BID,
   output wire [1:0]                           S02_AXI_BRESP,
   output wire                                 S02_AXI_BVALID,
   input  wire                                 S02_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S02_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S02_AXI_ARADDR,
   input  wire [7:0]                           S02_AXI_ARLEN,
   input  wire [2:0]                           S02_AXI_ARSIZE,
   input  wire [1:0]                           S02_AXI_ARBURST,
   input  wire                                 S02_AXI_ARLOCK,
   input  wire [3:0]                           S02_AXI_ARCACHE,
   input  wire [2:0]                           S02_AXI_ARPROT,
   input  wire [3:0]                           S02_AXI_ARQOS,
   input  wire                                 S02_AXI_ARVALID,
   output wire                                 S02_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S02_AXI_RID,
   output wire [C_S02_AXI_DATA_WIDTH-1:0]      S02_AXI_RDATA,
   output wire [1:0]                           S02_AXI_RRESP,
   output wire                                 S02_AXI_RLAST,
   output wire                                 S02_AXI_RVALID,
   input  wire                                 S02_AXI_RREADY,
   
   // Slave Interface S03
   output wire                                 S03_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 3)
   input  wire                                 S03_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S03_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S03_AXI_AWADDR,
   input  wire [7:0]                           S03_AXI_AWLEN,
   input  wire [2:0]                           S03_AXI_AWSIZE,
   input  wire [1:0]                           S03_AXI_AWBURST,
   input  wire                                 S03_AXI_AWLOCK,
   input  wire [3:0]                           S03_AXI_AWCACHE,
   input  wire [2:0]                           S03_AXI_AWPROT,
   input  wire [3:0]                           S03_AXI_AWQOS,
   input  wire                                 S03_AXI_AWVALID,
   output wire                                 S03_AXI_AWREADY,
   input  wire [C_S03_AXI_DATA_WIDTH-1:0]      S03_AXI_WDATA,
   input  wire [C_S03_AXI_DATA_WIDTH/8-1:0]    S03_AXI_WSTRB,
   input  wire                                 S03_AXI_WLAST,
   input  wire                                 S03_AXI_WVALID,
   output wire                                 S03_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S03_AXI_BID,
   output wire [1:0]                           S03_AXI_BRESP,
   output wire                                 S03_AXI_BVALID,
   input  wire                                 S03_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S03_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S03_AXI_ARADDR,
   input  wire [7:0]                           S03_AXI_ARLEN,
   input  wire [2:0]                           S03_AXI_ARSIZE,
   input  wire [1:0]                           S03_AXI_ARBURST,
   input  wire                                 S03_AXI_ARLOCK,
   input  wire [3:0]                           S03_AXI_ARCACHE,
   input  wire [2:0]                           S03_AXI_ARPROT,
   input  wire [3:0]                           S03_AXI_ARQOS,
   input  wire                                 S03_AXI_ARVALID,
   output wire                                 S03_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S03_AXI_RID,
   output wire [C_S03_AXI_DATA_WIDTH-1:0]      S03_AXI_RDATA,
   output wire [1:0]                           S03_AXI_RRESP,
   output wire                                 S03_AXI_RLAST,
   output wire                                 S03_AXI_RVALID,
   input  wire                                 S03_AXI_RREADY,
   
   // Slave Interface S04
   output wire                                 S04_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 4)
   input  wire                                 S04_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S04_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S04_AXI_AWADDR,
   input  wire [7:0]                           S04_AXI_AWLEN,
   input  wire [2:0]                           S04_AXI_AWSIZE,
   input  wire [1:0]                           S04_AXI_AWBURST,
   input  wire                                 S04_AXI_AWLOCK,
   input  wire [3:0]                           S04_AXI_AWCACHE,
   input  wire [2:0]                           S04_AXI_AWPROT,
   input  wire [3:0]                           S04_AXI_AWQOS,
   input  wire                                 S04_AXI_AWVALID,
   output wire                                 S04_AXI_AWREADY,
   input  wire [C_S04_AXI_DATA_WIDTH-1:0]      S04_AXI_WDATA,
   input  wire [C_S04_AXI_DATA_WIDTH/8-1:0]    S04_AXI_WSTRB,
   input  wire                                 S04_AXI_WLAST,
   input  wire                                 S04_AXI_WVALID,
   output wire                                 S04_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S04_AXI_BID,
   output wire [1:0]                           S04_AXI_BRESP,
   output wire                                 S04_AXI_BVALID,
   input  wire                                 S04_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S04_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S04_AXI_ARADDR,
   input  wire [7:0]                           S04_AXI_ARLEN,
   input  wire [2:0]                           S04_AXI_ARSIZE,
   input  wire [1:0]                           S04_AXI_ARBURST,
   input  wire                                 S04_AXI_ARLOCK,
   input  wire [3:0]                           S04_AXI_ARCACHE,
   input  wire [2:0]                           S04_AXI_ARPROT,
   input  wire [3:0]                           S04_AXI_ARQOS,
   input  wire                                 S04_AXI_ARVALID,
   output wire                                 S04_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S04_AXI_RID,
   output wire [C_S04_AXI_DATA_WIDTH-1:0]      S04_AXI_RDATA,
   output wire [1:0]                           S04_AXI_RRESP,
   output wire                                 S04_AXI_RLAST,
   output wire                                 S04_AXI_RVALID,
   input  wire                                 S04_AXI_RREADY,
   
   // Slave Interface S05
   output wire                                 S05_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 5)
   input  wire                                 S05_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S05_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S05_AXI_AWADDR,
   input  wire [7:0]                           S05_AXI_AWLEN,
   input  wire [2:0]                           S05_AXI_AWSIZE,
   input  wire [1:0]                           S05_AXI_AWBURST,
   input  wire                                 S05_AXI_AWLOCK,
   input  wire [3:0]                           S05_AXI_AWCACHE,
   input  wire [2:0]                           S05_AXI_AWPROT,
   input  wire [3:0]                           S05_AXI_AWQOS,
   input  wire                                 S05_AXI_AWVALID,
   output wire                                 S05_AXI_AWREADY,
   input  wire [C_S05_AXI_DATA_WIDTH-1:0]      S05_AXI_WDATA,
   input  wire [C_S05_AXI_DATA_WIDTH/8-1:0]    S05_AXI_WSTRB,
   input  wire                                 S05_AXI_WLAST,
   input  wire                                 S05_AXI_WVALID,
   output wire                                 S05_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S05_AXI_BID,
   output wire [1:0]                           S05_AXI_BRESP,
   output wire                                 S05_AXI_BVALID,
   input  wire                                 S05_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S05_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S05_AXI_ARADDR,
   input  wire [7:0]                           S05_AXI_ARLEN,
   input  wire [2:0]                           S05_AXI_ARSIZE,
   input  wire [1:0]                           S05_AXI_ARBURST,
   input  wire                                 S05_AXI_ARLOCK,
   input  wire [3:0]                           S05_AXI_ARCACHE,
   input  wire [2:0]                           S05_AXI_ARPROT,
   input  wire [3:0]                           S05_AXI_ARQOS,
   input  wire                                 S05_AXI_ARVALID,
   output wire                                 S05_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S05_AXI_RID,
   output wire [C_S05_AXI_DATA_WIDTH-1:0]      S05_AXI_RDATA,
   output wire [1:0]                           S05_AXI_RRESP,
   output wire                                 S05_AXI_RLAST,
   output wire                                 S05_AXI_RVALID,
   input  wire                                 S05_AXI_RREADY,
   
   // Slave Interface S06
   output wire                                 S06_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 6)
   input  wire                                 S06_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S06_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S06_AXI_AWADDR,
   input  wire [7:0]                           S06_AXI_AWLEN,
   input  wire [2:0]                           S06_AXI_AWSIZE,
   input  wire [1:0]                           S06_AXI_AWBURST,
   input  wire                                 S06_AXI_AWLOCK,
   input  wire [3:0]                           S06_AXI_AWCACHE,
   input  wire [2:0]                           S06_AXI_AWPROT,
   input  wire [3:0]                           S06_AXI_AWQOS,
   input  wire                                 S06_AXI_AWVALID,
   output wire                                 S06_AXI_AWREADY,
   input  wire [C_S06_AXI_DATA_WIDTH-1:0]      S06_AXI_WDATA,
   input  wire [C_S06_AXI_DATA_WIDTH/8-1:0]    S06_AXI_WSTRB,
   input  wire                                 S06_AXI_WLAST,
   input  wire                                 S06_AXI_WVALID,
   output wire                                 S06_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S06_AXI_BID,
   output wire [1:0]                           S06_AXI_BRESP,
   output wire                                 S06_AXI_BVALID,
   input  wire                                 S06_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S06_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S06_AXI_ARADDR,
   input  wire [7:0]                           S06_AXI_ARLEN,
   input  wire [2:0]                           S06_AXI_ARSIZE,
   input  wire [1:0]                           S06_AXI_ARBURST,
   input  wire                                 S06_AXI_ARLOCK,
   input  wire [3:0]                           S06_AXI_ARCACHE,
   input  wire [2:0]                           S06_AXI_ARPROT,
   input  wire [3:0]                           S06_AXI_ARQOS,
   input  wire                                 S06_AXI_ARVALID,
   output wire                                 S06_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S06_AXI_RID,
   output wire [C_S06_AXI_DATA_WIDTH-1:0]      S06_AXI_RDATA,
   output wire [1:0]                           S06_AXI_RRESP,
   output wire                                 S06_AXI_RLAST,
   output wire                                 S06_AXI_RVALID,
   input  wire                                 S06_AXI_RREADY,
   
   // Slave Interface S07
   output wire                                 S07_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 7)
   input  wire                                 S07_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S07_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S07_AXI_AWADDR,
   input  wire [7:0]                           S07_AXI_AWLEN,
   input  wire [2:0]                           S07_AXI_AWSIZE,
   input  wire [1:0]                           S07_AXI_AWBURST,
   input  wire                                 S07_AXI_AWLOCK,
   input  wire [3:0]                           S07_AXI_AWCACHE,
   input  wire [2:0]                           S07_AXI_AWPROT,
   input  wire [3:0]                           S07_AXI_AWQOS,
   input  wire                                 S07_AXI_AWVALID,
   output wire                                 S07_AXI_AWREADY,
   input  wire [C_S07_AXI_DATA_WIDTH-1:0]      S07_AXI_WDATA,
   input  wire [C_S07_AXI_DATA_WIDTH/8-1:0]    S07_AXI_WSTRB,
   input  wire                                 S07_AXI_WLAST,
   input  wire                                 S07_AXI_WVALID,
   output wire                                 S07_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S07_AXI_BID,
   output wire [1:0]                           S07_AXI_BRESP,
   output wire                                 S07_AXI_BVALID,
   input  wire                                 S07_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S07_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S07_AXI_ARADDR,
   input  wire [7:0]                           S07_AXI_ARLEN,
   input  wire [2:0]                           S07_AXI_ARSIZE,
   input  wire [1:0]                           S07_AXI_ARBURST,
   input  wire                                 S07_AXI_ARLOCK,
   input  wire [3:0]                           S07_AXI_ARCACHE,
   input  wire [2:0]                           S07_AXI_ARPROT,
   input  wire [3:0]                           S07_AXI_ARQOS,
   input  wire                                 S07_AXI_ARVALID,
   output wire                                 S07_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S07_AXI_RID,
   output wire [C_S07_AXI_DATA_WIDTH-1:0]      S07_AXI_RDATA,
   output wire [1:0]                           S07_AXI_RRESP,
   output wire                                 S07_AXI_RLAST,
   output wire                                 S07_AXI_RVALID,
   input  wire                                 S07_AXI_RREADY,
   
   // Slave Interface S08
   output wire                                 S08_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 8)
   input  wire                                 S08_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S08_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S08_AXI_AWADDR,
   input  wire [7:0]                           S08_AXI_AWLEN,
   input  wire [2:0]                           S08_AXI_AWSIZE,
   input  wire [1:0]                           S08_AXI_AWBURST,
   input  wire                                 S08_AXI_AWLOCK,
   input  wire [3:0]                           S08_AXI_AWCACHE,
   input  wire [2:0]                           S08_AXI_AWPROT,
   input  wire [3:0]                           S08_AXI_AWQOS,
   input  wire                                 S08_AXI_AWVALID,
   output wire                                 S08_AXI_AWREADY,
   input  wire [C_S08_AXI_DATA_WIDTH-1:0]      S08_AXI_WDATA,
   input  wire [C_S08_AXI_DATA_WIDTH/8-1:0]    S08_AXI_WSTRB,
   input  wire                                 S08_AXI_WLAST,
   input  wire                                 S08_AXI_WVALID,
   output wire                                 S08_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S08_AXI_BID,
   output wire [1:0]                           S08_AXI_BRESP,
   output wire                                 S08_AXI_BVALID,
   input  wire                                 S08_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S08_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S08_AXI_ARADDR,
   input  wire [7:0]                           S08_AXI_ARLEN,
   input  wire [2:0]                           S08_AXI_ARSIZE,
   input  wire [1:0]                           S08_AXI_ARBURST,
   input  wire                                 S08_AXI_ARLOCK,
   input  wire [3:0]                           S08_AXI_ARCACHE,
   input  wire [2:0]                           S08_AXI_ARPROT,
   input  wire [3:0]                           S08_AXI_ARQOS,
   input  wire                                 S08_AXI_ARVALID,
   output wire                                 S08_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S08_AXI_RID,
   output wire [C_S08_AXI_DATA_WIDTH-1:0]      S08_AXI_RDATA,
   output wire [1:0]                           S08_AXI_RRESP,
   output wire                                 S08_AXI_RLAST,
   output wire                                 S08_AXI_RVALID,
   input  wire                                 S08_AXI_RREADY,
   
   // Slave Interface S09
   output wire                                 S09_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 9)
   input  wire                                 S09_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S09_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S09_AXI_AWADDR,
   input  wire [7:0]                           S09_AXI_AWLEN,
   input  wire [2:0]                           S09_AXI_AWSIZE,
   input  wire [1:0]                           S09_AXI_AWBURST,
   input  wire                                 S09_AXI_AWLOCK,
   input  wire [3:0]                           S09_AXI_AWCACHE,
   input  wire [2:0]                           S09_AXI_AWPROT,
   input  wire [3:0]                           S09_AXI_AWQOS,
   input  wire                                 S09_AXI_AWVALID,
   output wire                                 S09_AXI_AWREADY,
   input  wire [C_S09_AXI_DATA_WIDTH-1:0]      S09_AXI_WDATA,
   input  wire [C_S09_AXI_DATA_WIDTH/8-1:0]    S09_AXI_WSTRB,
   input  wire                                 S09_AXI_WLAST,
   input  wire                                 S09_AXI_WVALID,
   output wire                                 S09_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S09_AXI_BID,
   output wire [1:0]                           S09_AXI_BRESP,
   output wire                                 S09_AXI_BVALID,
   input  wire                                 S09_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S09_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S09_AXI_ARADDR,
   input  wire [7:0]                           S09_AXI_ARLEN,
   input  wire [2:0]                           S09_AXI_ARSIZE,
   input  wire [1:0]                           S09_AXI_ARBURST,
   input  wire                                 S09_AXI_ARLOCK,
   input  wire [3:0]                           S09_AXI_ARCACHE,
   input  wire [2:0]                           S09_AXI_ARPROT,
   input  wire [3:0]                           S09_AXI_ARQOS,
   input  wire                                 S09_AXI_ARVALID,
   output wire                                 S09_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S09_AXI_RID,
   output wire [C_S09_AXI_DATA_WIDTH-1:0]      S09_AXI_RDATA,
   output wire [1:0]                           S09_AXI_RRESP,
   output wire                                 S09_AXI_RLAST,
   output wire                                 S09_AXI_RVALID,
   input  wire                                 S09_AXI_RREADY,
   
   // Slave Interface S10
   output wire                                 S10_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 10)
   input  wire                                 S10_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S10_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S10_AXI_AWADDR,
   input  wire [7:0]                           S10_AXI_AWLEN,
   input  wire [2:0]                           S10_AXI_AWSIZE,
   input  wire [1:0]                           S10_AXI_AWBURST,
   input  wire                                 S10_AXI_AWLOCK,
   input  wire [3:0]                           S10_AXI_AWCACHE,
   input  wire [2:0]                           S10_AXI_AWPROT,
   input  wire [3:0]                           S10_AXI_AWQOS,
   input  wire                                 S10_AXI_AWVALID,
   output wire                                 S10_AXI_AWREADY,
   input  wire [C_S10_AXI_DATA_WIDTH-1:0]      S10_AXI_WDATA,
   input  wire [C_S10_AXI_DATA_WIDTH/8-1:0]    S10_AXI_WSTRB,
   input  wire                                 S10_AXI_WLAST,
   input  wire                                 S10_AXI_WVALID,
   output wire                                 S10_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S10_AXI_BID,
   output wire [1:0]                           S10_AXI_BRESP,
   output wire                                 S10_AXI_BVALID,
   input  wire                                 S10_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S10_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S10_AXI_ARADDR,
   input  wire [7:0]                           S10_AXI_ARLEN,
   input  wire [2:0]                           S10_AXI_ARSIZE,
   input  wire [1:0]                           S10_AXI_ARBURST,
   input  wire                                 S10_AXI_ARLOCK,
   input  wire [3:0]                           S10_AXI_ARCACHE,
   input  wire [2:0]                           S10_AXI_ARPROT,
   input  wire [3:0]                           S10_AXI_ARQOS,
   input  wire                                 S10_AXI_ARVALID,
   output wire                                 S10_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S10_AXI_RID,
   output wire [C_S10_AXI_DATA_WIDTH-1:0]      S10_AXI_RDATA,
   output wire [1:0]                           S10_AXI_RRESP,
   output wire                                 S10_AXI_RLAST,
   output wire                                 S10_AXI_RVALID,
   input  wire                                 S10_AXI_RREADY,
   
   // Slave Interface S11
   output wire                                 S11_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 11)
   input  wire                                 S11_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S11_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S11_AXI_AWADDR,
   input  wire [7:0]                           S11_AXI_AWLEN,
   input  wire [2:0]                           S11_AXI_AWSIZE,
   input  wire [1:0]                           S11_AXI_AWBURST,
   input  wire                                 S11_AXI_AWLOCK,
   input  wire [3:0]                           S11_AXI_AWCACHE,
   input  wire [2:0]                           S11_AXI_AWPROT,
   input  wire [3:0]                           S11_AXI_AWQOS,
   input  wire                                 S11_AXI_AWVALID,
   output wire                                 S11_AXI_AWREADY,
   input  wire [C_S11_AXI_DATA_WIDTH-1:0]      S11_AXI_WDATA,
   input  wire [C_S11_AXI_DATA_WIDTH/8-1:0]    S11_AXI_WSTRB,
   input  wire                                 S11_AXI_WLAST,
   input  wire                                 S11_AXI_WVALID,
   output wire                                 S11_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S11_AXI_BID,
   output wire [1:0]                           S11_AXI_BRESP,
   output wire                                 S11_AXI_BVALID,
   input  wire                                 S11_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S11_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S11_AXI_ARADDR,
   input  wire [7:0]                           S11_AXI_ARLEN,
   input  wire [2:0]                           S11_AXI_ARSIZE,
   input  wire [1:0]                           S11_AXI_ARBURST,
   input  wire                                 S11_AXI_ARLOCK,
   input  wire [3:0]                           S11_AXI_ARCACHE,
   input  wire [2:0]                           S11_AXI_ARPROT,
   input  wire [3:0]                           S11_AXI_ARQOS,
   input  wire                                 S11_AXI_ARVALID,
   output wire                                 S11_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S11_AXI_RID,
   output wire [C_S11_AXI_DATA_WIDTH-1:0]      S11_AXI_RDATA,
   output wire [1:0]                           S11_AXI_RRESP,
   output wire                                 S11_AXI_RLAST,
   output wire                                 S11_AXI_RVALID,
   input  wire                                 S11_AXI_RREADY,
   
   // Slave Interface S12
   output wire                                 S12_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 12)
   input  wire                                 S12_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S12_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S12_AXI_AWADDR,
   input  wire [7:0]                           S12_AXI_AWLEN,
   input  wire [2:0]                           S12_AXI_AWSIZE,
   input  wire [1:0]                           S12_AXI_AWBURST,
   input  wire                                 S12_AXI_AWLOCK,
   input  wire [3:0]                           S12_AXI_AWCACHE,
   input  wire [2:0]                           S12_AXI_AWPROT,
   input  wire [3:0]                           S12_AXI_AWQOS,
   input  wire                                 S12_AXI_AWVALID,
   output wire                                 S12_AXI_AWREADY,
   input  wire [C_S12_AXI_DATA_WIDTH-1:0]      S12_AXI_WDATA,
   input  wire [C_S12_AXI_DATA_WIDTH/8-1:0]    S12_AXI_WSTRB,
   input  wire                                 S12_AXI_WLAST,
   input  wire                                 S12_AXI_WVALID,
   output wire                                 S12_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S12_AXI_BID,
   output wire [1:0]                           S12_AXI_BRESP,
   output wire                                 S12_AXI_BVALID,
   input  wire                                 S12_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S12_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S12_AXI_ARADDR,
   input  wire [7:0]                           S12_AXI_ARLEN,
   input  wire [2:0]                           S12_AXI_ARSIZE,
   input  wire [1:0]                           S12_AXI_ARBURST,
   input  wire                                 S12_AXI_ARLOCK,
   input  wire [3:0]                           S12_AXI_ARCACHE,
   input  wire [2:0]                           S12_AXI_ARPROT,
   input  wire [3:0]                           S12_AXI_ARQOS,
   input  wire                                 S12_AXI_ARVALID,
   output wire                                 S12_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S12_AXI_RID,
   output wire [C_S12_AXI_DATA_WIDTH-1:0]      S12_AXI_RDATA,
   output wire [1:0]                           S12_AXI_RRESP,
   output wire                                 S12_AXI_RLAST,
   output wire                                 S12_AXI_RVALID,
   input  wire                                 S12_AXI_RREADY,
   
   // Slave Interface S13
   output wire                                 S13_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 13)
   input  wire                                 S13_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S13_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S13_AXI_AWADDR,
   input  wire [7:0]                           S13_AXI_AWLEN,
   input  wire [2:0]                           S13_AXI_AWSIZE,
   input  wire [1:0]                           S13_AXI_AWBURST,
   input  wire                                 S13_AXI_AWLOCK,
   input  wire [3:0]                           S13_AXI_AWCACHE,
   input  wire [2:0]                           S13_AXI_AWPROT,
   input  wire [3:0]                           S13_AXI_AWQOS,
   input  wire                                 S13_AXI_AWVALID,
   output wire                                 S13_AXI_AWREADY,
   input  wire [C_S13_AXI_DATA_WIDTH-1:0]      S13_AXI_WDATA,
   input  wire [C_S13_AXI_DATA_WIDTH/8-1:0]    S13_AXI_WSTRB,
   input  wire                                 S13_AXI_WLAST,
   input  wire                                 S13_AXI_WVALID,
   output wire                                 S13_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S13_AXI_BID,
   output wire [1:0]                           S13_AXI_BRESP,
   output wire                                 S13_AXI_BVALID,
   input  wire                                 S13_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S13_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S13_AXI_ARADDR,
   input  wire [7:0]                           S13_AXI_ARLEN,
   input  wire [2:0]                           S13_AXI_ARSIZE,
   input  wire [1:0]                           S13_AXI_ARBURST,
   input  wire                                 S13_AXI_ARLOCK,
   input  wire [3:0]                           S13_AXI_ARCACHE,
   input  wire [2:0]                           S13_AXI_ARPROT,
   input  wire [3:0]                           S13_AXI_ARQOS,
   input  wire                                 S13_AXI_ARVALID,
   output wire                                 S13_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S13_AXI_RID,
   output wire [C_S13_AXI_DATA_WIDTH-1:0]      S13_AXI_RDATA,
   output wire [1:0]                           S13_AXI_RRESP,
   output wire                                 S13_AXI_RLAST,
   output wire                                 S13_AXI_RVALID,
   input  wire                                 S13_AXI_RREADY,
   
   // Slave Interface S14
   output wire                                 S14_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 14)
   input  wire                                 S14_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S14_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S14_AXI_AWADDR,
   input  wire [7:0]                           S14_AXI_AWLEN,
   input  wire [2:0]                           S14_AXI_AWSIZE,
   input  wire [1:0]                           S14_AXI_AWBURST,
   input  wire                                 S14_AXI_AWLOCK,
   input  wire [3:0]                           S14_AXI_AWCACHE,
   input  wire [2:0]                           S14_AXI_AWPROT,
   input  wire [3:0]                           S14_AXI_AWQOS,
   input  wire                                 S14_AXI_AWVALID,
   output wire                                 S14_AXI_AWREADY,
   input  wire [C_S14_AXI_DATA_WIDTH-1:0]      S14_AXI_WDATA,
   input  wire [C_S14_AXI_DATA_WIDTH/8-1:0]    S14_AXI_WSTRB,
   input  wire                                 S14_AXI_WLAST,
   input  wire                                 S14_AXI_WVALID,
   output wire                                 S14_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S14_AXI_BID,
   output wire [1:0]                           S14_AXI_BRESP,
   output wire                                 S14_AXI_BVALID,
   input  wire                                 S14_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S14_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S14_AXI_ARADDR,
   input  wire [7:0]                           S14_AXI_ARLEN,
   input  wire [2:0]                           S14_AXI_ARSIZE,
   input  wire [1:0]                           S14_AXI_ARBURST,
   input  wire                                 S14_AXI_ARLOCK,
   input  wire [3:0]                           S14_AXI_ARCACHE,
   input  wire [2:0]                           S14_AXI_ARPROT,
   input  wire [3:0]                           S14_AXI_ARQOS,
   input  wire                                 S14_AXI_ARVALID,
   output wire                                 S14_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S14_AXI_RID,
   output wire [C_S14_AXI_DATA_WIDTH-1:0]      S14_AXI_RDATA,
   output wire [1:0]                           S14_AXI_RRESP,
   output wire                                 S14_AXI_RLAST,
   output wire                                 S14_AXI_RVALID,
   input  wire                                 S14_AXI_RREADY,
   
   // Slave Interface S15
   output wire                                 S15_AXI_ARESET_OUT_N,
   // ISVALID = (C_NUM_SLAVE_PORTS > 15)
   input  wire                                 S15_AXI_ACLK,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S15_AXI_AWID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S15_AXI_AWADDR,
   input  wire [7:0]                           S15_AXI_AWLEN,
   input  wire [2:0]                           S15_AXI_AWSIZE,
   input  wire [1:0]                           S15_AXI_AWBURST,
   input  wire                                 S15_AXI_AWLOCK,
   input  wire [3:0]                           S15_AXI_AWCACHE,
   input  wire [2:0]                           S15_AXI_AWPROT,
   input  wire [3:0]                           S15_AXI_AWQOS,
   input  wire                                 S15_AXI_AWVALID,
   output wire                                 S15_AXI_AWREADY,
   input  wire [C_S15_AXI_DATA_WIDTH-1:0]      S15_AXI_WDATA,
   input  wire [C_S15_AXI_DATA_WIDTH/8-1:0]    S15_AXI_WSTRB,
   input  wire                                 S15_AXI_WLAST,
   input  wire                                 S15_AXI_WVALID,
   output wire                                 S15_AXI_WREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S15_AXI_BID,
   output wire [1:0]                           S15_AXI_BRESP,
   output wire                                 S15_AXI_BVALID,
   input  wire                                 S15_AXI_BREADY,
   input  wire [C_THREAD_ID_PORT_WIDTH-1:0]    S15_AXI_ARID,
   input  wire [C_AXI_ADDR_WIDTH-1:0]          S15_AXI_ARADDR,
   input  wire [7:0]                           S15_AXI_ARLEN,
   input  wire [2:0]                           S15_AXI_ARSIZE,
   input  wire [1:0]                           S15_AXI_ARBURST,
   input  wire                                 S15_AXI_ARLOCK,
   input  wire [3:0]                           S15_AXI_ARCACHE,
   input  wire [2:0]                           S15_AXI_ARPROT,
   input  wire [3:0]                           S15_AXI_ARQOS,
   input  wire                                 S15_AXI_ARVALID,
   output wire                                 S15_AXI_ARREADY,
   output wire [C_THREAD_ID_PORT_WIDTH-1:0]    S15_AXI_RID,
   output wire [C_S15_AXI_DATA_WIDTH-1:0]      S15_AXI_RDATA,
   output wire [1:0]                           S15_AXI_RRESP,
   output wire                                 S15_AXI_RLAST,
   output wire                                 S15_AXI_RVALID,
   input  wire                                 S15_AXI_RREADY,
   
   // Master Interface
   output wire                                 M00_AXI_ARESET_OUT_N,
   input  wire                                 M00_AXI_ACLK,
   output wire [C_THREAD_ID_WIDTH+3:0]         M00_AXI_AWID,
   output wire [C_AXI_ADDR_WIDTH-1:0]          M00_AXI_AWADDR,
   output wire [7:0]                           M00_AXI_AWLEN,
   output wire [2:0]                           M00_AXI_AWSIZE,
   output wire [1:0]                           M00_AXI_AWBURST,
   output wire                                 M00_AXI_AWLOCK,
   output wire [3:0]                           M00_AXI_AWCACHE,
   output wire [2:0]                           M00_AXI_AWPROT,
   output wire [3:0]                           M00_AXI_AWQOS,
   output wire                                 M00_AXI_AWVALID,
   input  wire                                 M00_AXI_AWREADY,
   output wire [C_M00_AXI_DATA_WIDTH-1:0]        M00_AXI_WDATA,
   output wire [C_M00_AXI_DATA_WIDTH/8-1:0]      M00_AXI_WSTRB,
   output wire                                 M00_AXI_WLAST,
   output wire                                 M00_AXI_WVALID,
   input  wire                                 M00_AXI_WREADY,
   input  wire [C_THREAD_ID_WIDTH+3:0]         M00_AXI_BID,
   input  wire [1:0]                           M00_AXI_BRESP,
   input  wire                                 M00_AXI_BVALID,
   output wire                                 M00_AXI_BREADY,
   output wire [C_THREAD_ID_WIDTH+3:0]         M00_AXI_ARID,
   output wire [C_AXI_ADDR_WIDTH-1:0]          M00_AXI_ARADDR,
   output wire [7:0]                           M00_AXI_ARLEN,
   output wire [2:0]                           M00_AXI_ARSIZE,
   output wire [1:0]                           M00_AXI_ARBURST,
   output wire                                 M00_AXI_ARLOCK,
   output wire [3:0]                           M00_AXI_ARCACHE,
   output wire [2:0]                           M00_AXI_ARPROT,
   output wire [3:0]                           M00_AXI_ARQOS,
   output wire                                 M00_AXI_ARVALID,
   input  wire                                 M00_AXI_ARREADY,
   input  wire [C_THREAD_ID_WIDTH+3:0]         M00_AXI_RID,
   input  wire [C_M00_AXI_DATA_WIDTH-1:0]        M00_AXI_RDATA,
   input  wire [1:0]                           M00_AXI_RRESP,
   input  wire                                 M00_AXI_RLAST,
   input  wire                                 M00_AXI_RVALID,
   output wire                                 M00_AXI_RREADY
   );
   
  function integer f_log2
    (
     input integer x
     );
    integer acc;
    begin
      acc=0;
      while (x >> (acc+1))
        acc = acc + 1;
      f_log2 = acc;
    end
  endfunction

  function f_bit1(
  // Cast as bit1.
      input  arg
    );
    begin
      f_bit1 = arg ? 1'b1 : 1'b0;
    end
  endfunction

  function [31:0] f_bit32_qual(
  // Cast as bit32. Replace with null_value if not qualified.
      input [31:0] arg,
      input        qual, // boolean
      input [31:0] null_val
    );
    begin
      f_bit32_qual = qual ? arg : null_val;
    end
  endfunction

  localparam         P_S_AXI_DATA_WIDTH = {
    f_bit32_qual(C_S15_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 15), 32),
    f_bit32_qual(C_S14_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 14), 32),
    f_bit32_qual(C_S13_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 13), 32),
    f_bit32_qual(C_S12_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 12), 32),
    f_bit32_qual(C_S11_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 11), 32),
    f_bit32_qual(C_S10_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 10), 32),
    f_bit32_qual(C_S09_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 09), 32),
    f_bit32_qual(C_S08_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 08), 32),
    f_bit32_qual(C_S07_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 07), 32),
    f_bit32_qual(C_S06_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 06), 32),
    f_bit32_qual(C_S05_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 05), 32),
    f_bit32_qual(C_S04_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 04), 32),
    f_bit32_qual(C_S03_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 03), 32),
    f_bit32_qual(C_S02_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 02), 32),
    f_bit32_qual(C_S01_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 01), 32),
    f_bit32_qual(C_S00_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 00), 32)
    };
  localparam         P_M_AXI_DATA_WIDTH = {{15{32'h32}}, f_bit32_qual(C_M00_AXI_DATA_WIDTH, 1, 0)};
  localparam         P_OR_DATA_WIDTHS = 
    f_bit32_qual(C_S00_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 00), 0) |
    f_bit32_qual(C_S01_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 01), 0) |
    f_bit32_qual(C_S02_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 02), 0) |
    f_bit32_qual(C_S03_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 03), 0) |
    f_bit32_qual(C_S04_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 04), 0) |
    f_bit32_qual(C_S05_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 05), 0) |
    f_bit32_qual(C_S06_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 06), 0) |
    f_bit32_qual(C_S07_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 07), 0) |
    f_bit32_qual(C_S08_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 08), 0) |
    f_bit32_qual(C_S09_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 09), 0) |
    f_bit32_qual(C_S10_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 10), 0) |
    f_bit32_qual(C_S11_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 11), 0) |
    f_bit32_qual(C_S12_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 12), 0) |
    f_bit32_qual(C_S13_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 13), 0) |
    f_bit32_qual(C_S14_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 14), 0) |
    f_bit32_qual(C_S15_AXI_DATA_WIDTH, (C_NUM_SLAVE_PORTS > 15), 0) |
    f_bit32_qual(C_M00_AXI_DATA_WIDTH, 1, 0) |
    f_bit32_qual(C_INTERCONNECT_DATA_WIDTH, 1, 0);
  localparam integer P_AXI_DATA_MAX_WIDTH = 1 << f_log2(P_OR_DATA_WIDTHS);
  localparam         P_M_AXI_BASE_ADDR = {{240{64'hFFFFFFFFFFFFFFFF}}, {15{64'hFFFFFFFFFFFFFFFF}}, 64'h0000000000000000};
  localparam         P_M_AXI_HIGH_ADDR = 64'hFFFFFFFFFFFFFFFF;
  localparam         P_S_AXI_BASE_ID = 
    (C_THREAD_ID_WIDTH == 0) ? {32'hF<<0, 32'hE<<0, 32'hD<<0, 32'hC<<0, 32'hB<<0, 32'hA<<0, 32'h9<<0, 32'h8<<0, 32'h7<<0, 32'h6<<0, 32'h5<<0, 32'h4<<0, 32'h3<<0, 32'h2<<0, 32'h1<<0, 32'h0} :
    (C_THREAD_ID_WIDTH == 1) ? {32'hF<<1, 32'hE<<1, 32'hD<<1, 32'hC<<1, 32'hB<<1, 32'hA<<1, 32'h9<<1, 32'h8<<1, 32'h7<<1, 32'h6<<1, 32'h5<<1, 32'h4<<1, 32'h3<<1, 32'h2<<1, 32'h1<<1, 32'h0} :
    (C_THREAD_ID_WIDTH == 2) ? {32'hF<<2, 32'hE<<2, 32'hD<<2, 32'hC<<2, 32'hB<<2, 32'hA<<2, 32'h9<<2, 32'h8<<2, 32'h7<<2, 32'h6<<2, 32'h5<<2, 32'h4<<2, 32'h3<<2, 32'h2<<2, 32'h1<<2, 32'h0} :
    (C_THREAD_ID_WIDTH == 3) ? {32'hF<<3, 32'hE<<3, 32'hD<<3, 32'hC<<3, 32'hB<<3, 32'hA<<3, 32'h9<<3, 32'h8<<3, 32'h7<<3, 32'h6<<3, 32'h5<<3, 32'h4<<3, 32'h3<<3, 32'h2<<3, 32'h1<<3, 32'h0} :
    (C_THREAD_ID_WIDTH == 4) ? {32'hF<<4, 32'hE<<4, 32'hD<<4, 32'hC<<4, 32'hB<<4, 32'hA<<4, 32'h9<<4, 32'h8<<4, 32'h7<<4, 32'h6<<4, 32'h5<<4, 32'h4<<4, 32'h3<<4, 32'h2<<4, 32'h1<<4, 32'h0} :
    (C_THREAD_ID_WIDTH == 5) ? {32'hF<<5, 32'hE<<5, 32'hD<<5, 32'hC<<5, 32'hB<<5, 32'hA<<5, 32'h9<<5, 32'h8<<5, 32'h7<<5, 32'h6<<5, 32'h5<<5, 32'h4<<5, 32'h3<<5, 32'h2<<5, 32'h1<<5, 32'h0} :
    (C_THREAD_ID_WIDTH == 6) ? {32'hF<<6, 32'hE<<6, 32'hD<<6, 32'hC<<6, 32'hB<<6, 32'hA<<6, 32'h9<<6, 32'h8<<6, 32'h7<<6, 32'h6<<6, 32'h5<<6, 32'h4<<6, 32'h3<<6, 32'h2<<6, 32'h1<<6, 32'h0} :
    (C_THREAD_ID_WIDTH == 7) ? {32'hF<<7, 32'hE<<7, 32'hD<<7, 32'hC<<7, 32'hB<<7, 32'hA<<7, 32'h9<<7, 32'h8<<7, 32'h7<<7, 32'h6<<7, 32'h5<<7, 32'h4<<7, 32'h3<<7, 32'h2<<7, 32'h1<<7, 32'h0} :
                               {32'hF<<8, 32'hE<<8, 32'hD<<8, 32'hC<<8, 32'hB<<8, 32'hA<<8, 32'h9<<8, 32'h8<<8, 32'h7<<8, 32'h6<<8, 32'h5<<8, 32'h4<<8, 32'h3<<8, 32'h2<<8, 32'h1<<8, 32'h0} ;
  localparam integer P_AXI_ID_WIDTH = C_THREAD_ID_WIDTH + 4;
  localparam         P_S_AXI_THREAD_ID_WIDTH = {C_NUM_SLAVE_PORTS{f_bit32_qual(C_THREAD_ID_WIDTH, 1, 0)}};
  localparam integer K = 720720;  // Least_common_multiple(2...16)
  localparam         P_S_AXI_ACLK_RATIO = {
    f_bit32_qual((
      (C_S15_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S15_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S15_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S15_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S15_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S15_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S15_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S15_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S15_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S15_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S15_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S15_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S15_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S15_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S15_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S15_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S15_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S15_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S15_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S15_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S15_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S15_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S15_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S15_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S15_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S15_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S15_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S15_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S15_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S15_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 15), 1),
    f_bit32_qual((
      (C_S14_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S14_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S14_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S14_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S14_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S14_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S14_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S14_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S14_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S14_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S14_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S14_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S14_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S14_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S14_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S14_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S14_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S14_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S14_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S14_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S14_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S14_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S14_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S14_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S14_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S14_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S14_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S14_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S14_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S14_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 14), 1),
    f_bit32_qual((
      (C_S13_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S13_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S13_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S13_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S13_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S13_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S13_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S13_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S13_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S13_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S13_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S13_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S13_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S13_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S13_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S13_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S13_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S13_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S13_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S13_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S13_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S13_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S13_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S13_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S13_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S13_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S13_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S13_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S13_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S13_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 13), 1),
    f_bit32_qual((
      (C_S12_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S12_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S12_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S12_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S12_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S12_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S12_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S12_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S12_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S12_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S12_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S12_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S12_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S12_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S12_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S12_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S12_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S12_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S12_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S12_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S12_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S12_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S12_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S12_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S12_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S12_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S12_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S12_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S12_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S12_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 12), 1),
    f_bit32_qual((
      (C_S11_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S11_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S11_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S11_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S11_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S11_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S11_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S11_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S11_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S11_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S11_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S11_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S11_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S11_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S11_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S11_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S11_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S11_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S11_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S11_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S11_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S11_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S11_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S11_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S11_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S11_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S11_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S11_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S11_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S11_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 11), 1),
    f_bit32_qual((
      (C_S10_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S10_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S10_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S10_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S10_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S10_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S10_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S10_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S10_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S10_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S10_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S10_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S10_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S10_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S10_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S10_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S10_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S10_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S10_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S10_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S10_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S10_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S10_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S10_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S10_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S10_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S10_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S10_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S10_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S10_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 10), 1),
    f_bit32_qual((
      (C_S09_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S09_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S09_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S09_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S09_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S09_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S09_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S09_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S09_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S09_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S09_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S09_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S09_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S09_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S09_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S09_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S09_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S09_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S09_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S09_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S09_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S09_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S09_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S09_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S09_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S09_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S09_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S09_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S09_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S09_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 09), 1),
    f_bit32_qual((
      (C_S08_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S08_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S08_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S08_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S08_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S08_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S08_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S08_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S08_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S08_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S08_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S08_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S08_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S08_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S08_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S08_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S08_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S08_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S08_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S08_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S08_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S08_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S08_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S08_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S08_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S08_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S08_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S08_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S08_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S08_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 08), 1),
    f_bit32_qual((
      (C_S07_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S07_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S07_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S07_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S07_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S07_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S07_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S07_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S07_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S07_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S07_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S07_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S07_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S07_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S07_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S07_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S07_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S07_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S07_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S07_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S07_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S07_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S07_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S07_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S07_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S07_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S07_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S07_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S07_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S07_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 07), 1),
    f_bit32_qual((
      (C_S06_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S06_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S06_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S06_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S06_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S06_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S06_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S06_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S06_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S06_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S06_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S06_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S06_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S06_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S06_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S06_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S06_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S06_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S06_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S06_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S06_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S06_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S06_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S06_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S06_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S06_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S06_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S06_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S06_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S06_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 06), 1),
    f_bit32_qual((
      (C_S05_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S05_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S05_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S05_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S05_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S05_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S05_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S05_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S05_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S05_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S05_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S05_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S05_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S05_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S05_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S05_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S05_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S05_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S05_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S05_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S05_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S05_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S05_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S05_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S05_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S05_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S05_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S05_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S05_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S05_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 05), 1),
    f_bit32_qual((
      (C_S04_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S04_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S04_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S04_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S04_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S04_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S04_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S04_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S04_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S04_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S04_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S04_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S04_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S04_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S04_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S04_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S04_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S04_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S04_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S04_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S04_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S04_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S04_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S04_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S04_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S04_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S04_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S04_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S04_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S04_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 04), 1),
    f_bit32_qual((
      (C_S03_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S03_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S03_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S03_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S03_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S03_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S03_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S03_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S03_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S03_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S03_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S03_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S03_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S03_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S03_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S03_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S03_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S03_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S03_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S03_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S03_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S03_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S03_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S03_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S03_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S03_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S03_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S03_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S03_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S03_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 03), 1),
    f_bit32_qual((
      (C_S02_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S02_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S02_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S02_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S02_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S02_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S02_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S02_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S02_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S02_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S02_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S02_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S02_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S02_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S02_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S02_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S02_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S02_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S02_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S02_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S02_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S02_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S02_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S02_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S02_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S02_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S02_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S02_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S02_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S02_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 02), 1),
    f_bit32_qual((
      (C_S01_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S01_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S01_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S01_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S01_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S01_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S01_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S01_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S01_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S01_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S01_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S01_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S01_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S01_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S01_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S01_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S01_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S01_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S01_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S01_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S01_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S01_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S01_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S01_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S01_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S01_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S01_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S01_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S01_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S01_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 01), 1),
    f_bit32_qual((
      (C_S00_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_S00_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_S00_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_S00_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_S00_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_S00_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_S00_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_S00_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_S00_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_S00_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_S00_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_S00_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_S00_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_S00_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_S00_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_S00_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_S00_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_S00_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_S00_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_S00_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_S00_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_S00_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_S00_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_S00_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_S00_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_S00_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_S00_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_S00_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_S00_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_S00_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), (C_NUM_SLAVE_PORTS > 00), 1)
    };
  localparam         P_S_AXI_IS_ACLK_ASYNC = {
    f_bit1(C_S15_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S14_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S13_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S12_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S11_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S10_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S09_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S08_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S07_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S06_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S05_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S04_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S03_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S02_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S01_AXI_IS_ACLK_ASYNC),
    f_bit1(C_S00_AXI_IS_ACLK_ASYNC)
    };
  localparam         P_M_AXI_ACLK_RATIO = {
    {15{32'h1}},
    f_bit32_qual((
      (C_M00_AXI_ACLK_RATIO == "1:16") ? (K / 16) :
      (C_M00_AXI_ACLK_RATIO == "1:15") ? (K / 15) :
      (C_M00_AXI_ACLK_RATIO == "1:14") ? (K / 14) :
      (C_M00_AXI_ACLK_RATIO == "1:13") ? (K / 13) :
      (C_M00_AXI_ACLK_RATIO == "1:12") ? (K / 12) :
      (C_M00_AXI_ACLK_RATIO == "1:11") ? (K / 11) :
      (C_M00_AXI_ACLK_RATIO == "1:10") ? (K / 10) :
      (C_M00_AXI_ACLK_RATIO == "1:9")  ? (K / 9 ) :
      (C_M00_AXI_ACLK_RATIO == "1:8")  ? (K / 8 ) :
      (C_M00_AXI_ACLK_RATIO == "1:7")  ? (K / 7 ) :
      (C_M00_AXI_ACLK_RATIO == "1:6")  ? (K / 6 ) :
      (C_M00_AXI_ACLK_RATIO == "1:5")  ? (K / 5 ) :
      (C_M00_AXI_ACLK_RATIO == "1:4")  ? (K / 4 ) :
      (C_M00_AXI_ACLK_RATIO == "1:3")  ? (K / 3 ) :
      (C_M00_AXI_ACLK_RATIO == "1:2")  ? (K / 2 ) :
      (C_M00_AXI_ACLK_RATIO ==  "2:1") ? (K * 2 ) :
      (C_M00_AXI_ACLK_RATIO ==  "3:1") ? (K * 3 ) :
      (C_M00_AXI_ACLK_RATIO ==  "4:1") ? (K * 4 ) :
      (C_M00_AXI_ACLK_RATIO ==  "5:1") ? (K * 5 ) :
      (C_M00_AXI_ACLK_RATIO ==  "6:1") ? (K * 6 ) :
      (C_M00_AXI_ACLK_RATIO ==  "7:1") ? (K * 7 ) :
      (C_M00_AXI_ACLK_RATIO ==  "8:1") ? (K * 8 ) :
      (C_M00_AXI_ACLK_RATIO ==  "9:1") ? (K * 9 ) :
      (C_M00_AXI_ACLK_RATIO == "10:1") ? (K * 10) :
      (C_M00_AXI_ACLK_RATIO == "11:1") ? (K * 11) :
      (C_M00_AXI_ACLK_RATIO == "12:1") ? (K * 12) :
      (C_M00_AXI_ACLK_RATIO == "13:1") ? (K * 13) :
      (C_M00_AXI_ACLK_RATIO == "14:1") ? (K * 14) :
      (C_M00_AXI_ACLK_RATIO == "15:1") ? (K * 15) :
      (C_M00_AXI_ACLK_RATIO == "16:1") ? (K * 16) :
      K), 1, 1)
      };
  localparam         P_S_AXI_SUPPORTS_WRITE = {
    f_bit1(C_S15_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S15_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S14_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S14_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S13_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S13_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S12_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S12_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S11_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S11_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S10_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S10_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S09_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S09_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S08_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S08_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S07_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S07_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S06_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S06_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S05_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S05_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S04_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S04_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S03_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S03_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S02_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S02_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S01_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S01_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY"),
    f_bit1(C_S00_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S00_AXI_READ_WRITE_SUPPORT == "WRITE-ONLY")
    };
  localparam         P_S_AXI_SUPPORTS_READ = {
    f_bit1(C_S15_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S15_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S14_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S14_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S13_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S13_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S12_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S12_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S11_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S11_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S10_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S10_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S09_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S09_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S08_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S08_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S07_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S07_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S06_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S06_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S05_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S05_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S04_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S04_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S03_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S03_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S02_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S02_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S01_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S01_AXI_READ_WRITE_SUPPORT == "READ-ONLY"),
    f_bit1(C_S00_AXI_READ_WRITE_SUPPORT == "READ/WRITE" || C_S00_AXI_READ_WRITE_SUPPORT == "READ-ONLY")
    };
  localparam         P_S_AXI_WRITE_ACCEPTANCE = {
    f_bit32_qual(C_S15_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 15), 1),
    f_bit32_qual(C_S14_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 14), 1),
    f_bit32_qual(C_S13_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 13), 1),
    f_bit32_qual(C_S12_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 12), 1),
    f_bit32_qual(C_S11_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 11), 1),
    f_bit32_qual(C_S10_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 10), 1),
    f_bit32_qual(C_S09_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 09), 1),
    f_bit32_qual(C_S08_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 08), 1),
    f_bit32_qual(C_S07_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 07), 1),
    f_bit32_qual(C_S06_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 06), 1),
    f_bit32_qual(C_S05_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 05), 1),
    f_bit32_qual(C_S04_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 04), 1),
    f_bit32_qual(C_S03_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 03), 1),
    f_bit32_qual(C_S02_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 02), 1),
    f_bit32_qual(C_S01_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 01), 1),
    f_bit32_qual(C_S00_AXI_WRITE_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 00), 1)
    };
  localparam         P_S_AXI_READ_ACCEPTANCE = {
    f_bit32_qual(C_S15_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 15), 1),
    f_bit32_qual(C_S14_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 14), 1),
    f_bit32_qual(C_S13_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 13), 1),
    f_bit32_qual(C_S12_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 12), 1),
    f_bit32_qual(C_S11_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 11), 1),
    f_bit32_qual(C_S10_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 10), 1),
    f_bit32_qual(C_S09_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 09), 1),
    f_bit32_qual(C_S08_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 08), 1),
    f_bit32_qual(C_S07_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 07), 1),
    f_bit32_qual(C_S06_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 06), 1),
    f_bit32_qual(C_S05_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 05), 1),
    f_bit32_qual(C_S04_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 04), 1),
    f_bit32_qual(C_S03_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 03), 1),
    f_bit32_qual(C_S02_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 02), 1),
    f_bit32_qual(C_S01_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 01), 1),
    f_bit32_qual(C_S00_AXI_READ_ACCEPTANCE, (C_NUM_SLAVE_PORTS > 00), 1)
    };
  localparam         P_M_AXI_WRITE_ISSUING = {{15{32'h1}}, f_bit32_qual(C_M00_AXI_WRITE_ISSUING, 1, 0)};
  localparam         P_M_AXI_READ_ISSUING = {{15{32'h1}}, f_bit32_qual(C_M00_AXI_READ_ISSUING, 1, 0)};
  localparam         P_S_AXI_ARB_PRIORITY = {
    f_bit32_qual(C_S15_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 15), 0),
    f_bit32_qual(C_S14_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 14), 0),
    f_bit32_qual(C_S13_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 13), 0),
    f_bit32_qual(C_S12_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 12), 0),
    f_bit32_qual(C_S11_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 11), 0),
    f_bit32_qual(C_S10_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 10), 0),
    f_bit32_qual(C_S09_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 09), 0),
    f_bit32_qual(C_S08_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 08), 0),
    f_bit32_qual(C_S07_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 07), 0),
    f_bit32_qual(C_S06_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 06), 0),
    f_bit32_qual(C_S05_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 05), 0),
    f_bit32_qual(C_S04_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 04), 0),
    f_bit32_qual(C_S03_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 03), 0),
    f_bit32_qual(C_S02_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 02), 0),
    f_bit32_qual(C_S01_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 01), 0),
    f_bit32_qual(C_S00_AXI_ARB_PRIORITY, (C_NUM_SLAVE_PORTS > 00), 0)
    };
  localparam         P_S_AXI_WRITE_FIFO_DEPTH = {
    f_bit32_qual(C_S15_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 15), 0),
    f_bit32_qual(C_S14_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 14), 0),
    f_bit32_qual(C_S13_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 13), 0),
    f_bit32_qual(C_S12_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 12), 0),
    f_bit32_qual(C_S11_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 11), 0),
    f_bit32_qual(C_S10_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 10), 0),
    f_bit32_qual(C_S09_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 09), 0),
    f_bit32_qual(C_S08_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 08), 0),
    f_bit32_qual(C_S07_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 07), 0),
    f_bit32_qual(C_S06_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 06), 0),
    f_bit32_qual(C_S05_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 05), 0),
    f_bit32_qual(C_S04_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 04), 0),
    f_bit32_qual(C_S03_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 03), 0),
    f_bit32_qual(C_S02_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 02), 0),
    f_bit32_qual(C_S01_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 01), 0),
    f_bit32_qual(C_S00_AXI_WRITE_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 00), 0)
    };
  localparam         P_S_AXI_READ_FIFO_DEPTH = {
    f_bit32_qual(C_S15_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 15), 0),
    f_bit32_qual(C_S14_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 14), 0),
    f_bit32_qual(C_S13_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 13), 0),
    f_bit32_qual(C_S12_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 12), 0),
    f_bit32_qual(C_S11_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 11), 0),
    f_bit32_qual(C_S10_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 10), 0),
    f_bit32_qual(C_S09_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 09), 0),
    f_bit32_qual(C_S08_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 08), 0),
    f_bit32_qual(C_S07_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 07), 0),
    f_bit32_qual(C_S06_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 06), 0),
    f_bit32_qual(C_S05_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 05), 0),
    f_bit32_qual(C_S04_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 04), 0),
    f_bit32_qual(C_S03_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 03), 0),
    f_bit32_qual(C_S02_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 02), 0),
    f_bit32_qual(C_S01_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 01), 0),
    f_bit32_qual(C_S00_AXI_READ_FIFO_DEPTH, (C_NUM_SLAVE_PORTS > 00), 0)
    };
  localparam         P_S_AXI_WRITE_FIFO_DELAY = {
    f_bit1(C_S15_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S14_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S13_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S12_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S11_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S10_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S09_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S08_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S07_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S06_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S05_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S04_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S03_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S02_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S01_AXI_WRITE_FIFO_DELAY),
    f_bit1(C_S00_AXI_WRITE_FIFO_DELAY)
    };
  localparam         P_S_AXI_READ_FIFO_DELAY = {
    f_bit1(C_S15_AXI_READ_FIFO_DELAY),
    f_bit1(C_S14_AXI_READ_FIFO_DELAY),
    f_bit1(C_S13_AXI_READ_FIFO_DELAY),
    f_bit1(C_S12_AXI_READ_FIFO_DELAY),
    f_bit1(C_S11_AXI_READ_FIFO_DELAY),
    f_bit1(C_S10_AXI_READ_FIFO_DELAY),
    f_bit1(C_S09_AXI_READ_FIFO_DELAY),
    f_bit1(C_S08_AXI_READ_FIFO_DELAY),
    f_bit1(C_S07_AXI_READ_FIFO_DELAY),
    f_bit1(C_S06_AXI_READ_FIFO_DELAY),
    f_bit1(C_S05_AXI_READ_FIFO_DELAY),
    f_bit1(C_S04_AXI_READ_FIFO_DELAY),
    f_bit1(C_S03_AXI_READ_FIFO_DELAY),
    f_bit1(C_S02_AXI_READ_FIFO_DELAY),
    f_bit1(C_S01_AXI_READ_FIFO_DELAY),
    f_bit1(C_S00_AXI_READ_FIFO_DELAY)
    };
  localparam         P_S_AXI_REGISTER = {
    f_bit32_qual((C_S15_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 15), 0),
    f_bit32_qual((C_S14_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 14), 0),
    f_bit32_qual((C_S13_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 13), 0),
    f_bit32_qual((C_S12_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 12), 0),
    f_bit32_qual((C_S11_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 11), 0),
    f_bit32_qual((C_S10_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 10), 0),
    f_bit32_qual((C_S09_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 09), 0),
    f_bit32_qual((C_S08_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 08), 0),
    f_bit32_qual((C_S07_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 07), 0),
    f_bit32_qual((C_S06_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 06), 0),
    f_bit32_qual((C_S05_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 05), 0),
    f_bit32_qual((C_S04_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 04), 0),
    f_bit32_qual((C_S03_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 03), 0),
    f_bit32_qual((C_S02_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 02), 0),
    f_bit32_qual((C_S01_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 01), 0),
    f_bit32_qual((C_S00_AXI_REGISTER ? 8: 0), (C_NUM_SLAVE_PORTS > 00), 0)
    };
  localparam         P_M_AXI_REGISTER = f_bit32_qual((C_M00_AXI_REGISTER ? 8: 0), 1, 0);

  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_areset_out_n;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_aclk;
  wire [C_NUM_SLAVE_PORTS*P_AXI_ID_WIDTH-1:0]             s_axi_awid;
  wire [C_NUM_SLAVE_PORTS*C_AXI_ADDR_WIDTH-1:0]           s_axi_awaddr;
  wire [C_NUM_SLAVE_PORTS*8-1:0]                          s_axi_awlen;
  wire [C_NUM_SLAVE_PORTS*3-1:0]                          s_axi_awsize;
  wire [C_NUM_SLAVE_PORTS*2-1:0]                          s_axi_awburst;
  wire [C_NUM_SLAVE_PORTS*2-1:0]                          s_axi_awlock;
  wire [C_NUM_SLAVE_PORTS*4-1:0]                          s_axi_awcache;
  wire [C_NUM_SLAVE_PORTS*3-1:0]                          s_axi_awprot;
  wire [C_NUM_SLAVE_PORTS*4-1:0]                          s_axi_awqos;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_awvalid;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_awready;
  wire [C_NUM_SLAVE_PORTS*P_AXI_DATA_MAX_WIDTH-1:0]       s_axi_wdata;
  wire [C_NUM_SLAVE_PORTS*P_AXI_DATA_MAX_WIDTH/8-1:0]     s_axi_wstrb;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_wlast;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_wvalid;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_wready;
  wire [C_NUM_SLAVE_PORTS*P_AXI_ID_WIDTH-1:0]             s_axi_bid;
  wire [C_NUM_SLAVE_PORTS*2-1:0]                          s_axi_bresp;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_bvalid;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_bready;
  wire [C_NUM_SLAVE_PORTS*P_AXI_ID_WIDTH-1:0]             s_axi_arid;
  wire [C_NUM_SLAVE_PORTS*C_AXI_ADDR_WIDTH-1:0]           s_axi_araddr;
  wire [C_NUM_SLAVE_PORTS*8-1:0]                          s_axi_arlen;
  wire [C_NUM_SLAVE_PORTS*3-1:0]                          s_axi_arsize;
  wire [C_NUM_SLAVE_PORTS*2-1:0]                          s_axi_arburst;
  wire [C_NUM_SLAVE_PORTS*2-1:0]                          s_axi_arlock;
  wire [C_NUM_SLAVE_PORTS*4-1:0]                          s_axi_arcache;
  wire [C_NUM_SLAVE_PORTS*3-1:0]                          s_axi_arprot;
  wire [C_NUM_SLAVE_PORTS*4-1:0]                          s_axi_arqos;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_arvalid;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_arready;
  wire [C_NUM_SLAVE_PORTS*P_AXI_ID_WIDTH-1:0]             s_axi_rid;
  wire [C_NUM_SLAVE_PORTS*P_AXI_DATA_MAX_WIDTH-1:0]       s_axi_rdata;
  wire [C_NUM_SLAVE_PORTS*2-1:0]                          s_axi_rresp;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_rlast;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_rvalid;
  wire [C_NUM_SLAVE_PORTS-1:0]                            s_axi_rready;
                                                          
  wire [1:0]                                              m_axi_awlock_i;
  wire [1:0]                                              m_axi_arlock_i;
  wire [P_AXI_DATA_MAX_WIDTH-1:0]                         m_axi_wdata;
  wire [P_AXI_DATA_MAX_WIDTH-1:0]                         m_axi_rdata;
  wire [P_AXI_DATA_MAX_WIDTH/8-1:0]                       m_axi_wstrb;
                                                                    
  generate
    if (C_NUM_SLAVE_PORTS > 00) begin : gen_si00
      assign s_axi_aclk      [00 +: 1]                                                   = S00_AXI_ACLK      ;
      assign s_axi_awid      [00 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S00_AXI_AWID      ;
      assign s_axi_awaddr    [00 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S00_AXI_AWADDR    ;
      assign s_axi_awlen     [00 *8 +: 8]                                                = S00_AXI_AWLEN     ;
      assign s_axi_awsize    [00 *3 +: 3]                                                = S00_AXI_AWSIZE    ;
      assign s_axi_awburst   [00 *2 +: 2]                                                = S00_AXI_AWBURST   ;
      assign s_axi_awlock    [00 *2 +: 2]                                                = S00_AXI_AWLOCK    ;
      assign s_axi_awcache   [00 *4 +: 4]                                                = S00_AXI_AWCACHE   ;
      assign s_axi_awprot    [00 *3 +: 3]                                                = S00_AXI_AWPROT    ;
      assign s_axi_awqos     [00 *4 +: 4]                                                = S00_AXI_AWQOS     ;
      assign s_axi_awvalid   [00 +: 1]                                                   = S00_AXI_AWVALID   ;
      assign s_axi_wdata     [00 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S00_AXI_WDATA     ; 
      assign s_axi_wstrb     [00 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S00_AXI_WSTRB     ;    
      assign s_axi_wlast     [00 +: 1]                                                   = S00_AXI_WLAST     ;
      assign s_axi_wvalid    [00 +: 1]                                                   = S00_AXI_WVALID    ;
      assign s_axi_bready    [00 +: 1]                                                   = S00_AXI_BREADY    ;
      assign s_axi_arid      [00 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S00_AXI_ARID      ;
      assign s_axi_araddr    [00 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S00_AXI_ARADDR    ;
      assign s_axi_arlen     [00 *8 +: 8]                                                = S00_AXI_ARLEN     ;
      assign s_axi_arsize    [00 *3 +: 3]                                                = S00_AXI_ARSIZE    ;
      assign s_axi_arburst   [00 *2 +: 2]                                                = S00_AXI_ARBURST   ;
      assign s_axi_arlock    [00 *2 +: 2]                                                = S00_AXI_ARLOCK    ;
      assign s_axi_arcache   [00 *4 +: 4]                                                = S00_AXI_ARCACHE   ;
      assign s_axi_arprot    [00 *3 +: 3]                                                = S00_AXI_ARPROT    ;
      assign s_axi_arqos     [00 *4 +: 4]                                                = S00_AXI_ARQOS     ;
      assign s_axi_arvalid   [00 +: 1]                                                   = S00_AXI_ARVALID   ;
      assign s_axi_rready    [00 +: 1]                                                   = S00_AXI_RREADY    ;
                                                                                         
      assign S00_AXI_ARESET_OUT_N = s_axi_areset_out_n[00 +: 1]                                                   ;
      assign S00_AXI_AWREADY   = s_axi_awready   [00 +: 1]                                                   ;
      assign S00_AXI_WREADY    = s_axi_wready    [00 +: 1]                                                   ;
      assign S00_AXI_BID       = s_axi_bid       [00 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S00_AXI_BRESP     = s_axi_bresp     [00 *2 +: 2]                                                ;
      assign S00_AXI_BVALID    = s_axi_bvalid    [00 +: 1]                                                   ;
      assign S00_AXI_ARREADY   = s_axi_arready   [00 +: 1]                                                   ;
      assign S00_AXI_RID       = s_axi_rid       [00 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S00_AXI_RDATA     = s_axi_rdata     [00 *P_AXI_DATA_MAX_WIDTH +: C_S00_AXI_DATA_WIDTH]       ; 
      assign S00_AXI_RRESP     = s_axi_rresp     [00 *2 +: 2]                                                ;
      assign S00_AXI_RLAST     = s_axi_rlast     [00 +: 1]                                                   ;
      assign S00_AXI_RVALID    = s_axi_rvalid    [00 +: 1]                                                   ;
    end else begin : gen_no_si00
      assign S00_AXI_ARESET_OUT_N = 0 ;
      assign S00_AXI_AWREADY   = 0 ;
      assign S00_AXI_WREADY    = 0 ;
      assign S00_AXI_BID       = 0 ;
      assign S00_AXI_BRESP     = 0 ;
      assign S00_AXI_BVALID    = 0 ;
      assign S00_AXI_ARREADY   = 0 ;
      assign S00_AXI_RID       = 0 ;
      assign S00_AXI_RDATA     = 0 ;
      assign S00_AXI_RRESP     = 0 ;
      assign S00_AXI_RLAST     = 0 ;
      assign S00_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 01) begin : gen_si01
      assign s_axi_aclk      [01 +: 1]                                                   = S01_AXI_ACLK      ;
      assign s_axi_awid      [01 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S01_AXI_AWID      ;
      assign s_axi_awaddr    [01 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S01_AXI_AWADDR    ;
      assign s_axi_awlen     [01 *8 +: 8]                                                = S01_AXI_AWLEN     ;
      assign s_axi_awsize    [01 *3 +: 3]                                                = S01_AXI_AWSIZE    ;
      assign s_axi_awburst   [01 *2 +: 2]                                                = S01_AXI_AWBURST   ;
      assign s_axi_awlock    [01 *2 +: 2]                                                = S01_AXI_AWLOCK    ;
      assign s_axi_awcache   [01 *4 +: 4]                                                = S01_AXI_AWCACHE   ;
      assign s_axi_awprot    [01 *3 +: 3]                                                = S01_AXI_AWPROT    ;
      assign s_axi_awqos     [01 *4 +: 4]                                                = S01_AXI_AWQOS     ;
      assign s_axi_awvalid   [01 +: 1]                                                   = S01_AXI_AWVALID   ;
      assign s_axi_wdata     [01 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S01_AXI_WDATA     ; 
      assign s_axi_wstrb     [01 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S01_AXI_WSTRB     ;    
      assign s_axi_wlast     [01 +: 1]                                                   = S01_AXI_WLAST     ;
      assign s_axi_wvalid    [01 +: 1]                                                   = S01_AXI_WVALID    ;
      assign s_axi_bready    [01 +: 1]                                                   = S01_AXI_BREADY    ;
      assign s_axi_arid      [01 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S01_AXI_ARID      ;
      assign s_axi_araddr    [01 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S01_AXI_ARADDR    ;
      assign s_axi_arlen     [01 *8 +: 8]                                                = S01_AXI_ARLEN     ;
      assign s_axi_arsize    [01 *3 +: 3]                                                = S01_AXI_ARSIZE    ;
      assign s_axi_arburst   [01 *2 +: 2]                                                = S01_AXI_ARBURST   ;
      assign s_axi_arlock    [01 *2 +: 2]                                                = S01_AXI_ARLOCK    ;
      assign s_axi_arcache   [01 *4 +: 4]                                                = S01_AXI_ARCACHE   ;
      assign s_axi_arprot    [01 *3 +: 3]                                                = S01_AXI_ARPROT    ;
      assign s_axi_arqos     [01 *4 +: 4]                                                = S01_AXI_ARQOS     ;
      assign s_axi_arvalid   [01 +: 1]                                                   = S01_AXI_ARVALID   ;
      assign s_axi_rready    [01 +: 1]                                                   = S01_AXI_RREADY    ;
                                                                                         
      assign S01_AXI_ARESET_OUT_N = s_axi_areset_out_n[01 +: 1]                                                   ;
      assign S01_AXI_AWREADY   = s_axi_awready   [01 +: 1]                                                   ;
      assign S01_AXI_WREADY    = s_axi_wready    [01 +: 1]                                                   ;
      assign S01_AXI_BID       = s_axi_bid       [01 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S01_AXI_BRESP     = s_axi_bresp     [01 *2 +: 2]                                                ;
      assign S01_AXI_BVALID    = s_axi_bvalid    [01 +: 1]                                                   ;
      assign S01_AXI_ARREADY   = s_axi_arready   [01 +: 1]                                                   ;
      assign S01_AXI_RID       = s_axi_rid       [01 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S01_AXI_RDATA     = s_axi_rdata     [01 *P_AXI_DATA_MAX_WIDTH +: C_S01_AXI_DATA_WIDTH]       ; 
      assign S01_AXI_RRESP     = s_axi_rresp     [01 *2 +: 2]                                                ;
      assign S01_AXI_RLAST     = s_axi_rlast     [01 +: 1]                                                   ;
      assign S01_AXI_RVALID    = s_axi_rvalid    [01 +: 1]                                                   ;
    end else begin : gen_no_si01
      assign S01_AXI_ARESET_OUT_N = 0 ;
      assign S01_AXI_AWREADY   = 0 ;
      assign S01_AXI_WREADY    = 0 ;
      assign S01_AXI_BID       = 0 ;
      assign S01_AXI_BRESP     = 0 ;
      assign S01_AXI_BVALID    = 0 ;
      assign S01_AXI_ARREADY   = 0 ;
      assign S01_AXI_RID       = 0 ;
      assign S01_AXI_RDATA     = 0 ;
      assign S01_AXI_RRESP     = 0 ;
      assign S01_AXI_RLAST     = 0 ;
      assign S01_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 02) begin : gen_si02
      assign s_axi_aclk      [02 +: 1]                                                   = S02_AXI_ACLK      ;
      assign s_axi_awid      [02 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S02_AXI_AWID      ;
      assign s_axi_awaddr    [02 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S02_AXI_AWADDR    ;
      assign s_axi_awlen     [02 *8 +: 8]                                                = S02_AXI_AWLEN     ;
      assign s_axi_awsize    [02 *3 +: 3]                                                = S02_AXI_AWSIZE    ;
      assign s_axi_awburst   [02 *2 +: 2]                                                = S02_AXI_AWBURST   ;
      assign s_axi_awlock    [02 *2 +: 2]                                                = S02_AXI_AWLOCK    ;
      assign s_axi_awcache   [02 *4 +: 4]                                                = S02_AXI_AWCACHE   ;
      assign s_axi_awprot    [02 *3 +: 3]                                                = S02_AXI_AWPROT    ;
      assign s_axi_awqos     [02 *4 +: 4]                                                = S02_AXI_AWQOS     ;
      assign s_axi_awvalid   [02 +: 1]                                                   = S02_AXI_AWVALID   ;
      assign s_axi_wdata     [02 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S02_AXI_WDATA     ; 
      assign s_axi_wstrb     [02 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S02_AXI_WSTRB     ;    
      assign s_axi_wlast     [02 +: 1]                                                   = S02_AXI_WLAST     ;
      assign s_axi_wvalid    [02 +: 1]                                                   = S02_AXI_WVALID    ;
      assign s_axi_bready    [02 +: 1]                                                   = S02_AXI_BREADY    ;
      assign s_axi_arid      [02 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S02_AXI_ARID      ;
      assign s_axi_araddr    [02 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S02_AXI_ARADDR    ;
      assign s_axi_arlen     [02 *8 +: 8]                                                = S02_AXI_ARLEN     ;
      assign s_axi_arsize    [02 *3 +: 3]                                                = S02_AXI_ARSIZE    ;
      assign s_axi_arburst   [02 *2 +: 2]                                                = S02_AXI_ARBURST   ;
      assign s_axi_arlock    [02 *2 +: 2]                                                = S02_AXI_ARLOCK    ;
      assign s_axi_arcache   [02 *4 +: 4]                                                = S02_AXI_ARCACHE   ;
      assign s_axi_arprot    [02 *3 +: 3]                                                = S02_AXI_ARPROT    ;
      assign s_axi_arqos     [02 *4 +: 4]                                                = S02_AXI_ARQOS     ;
      assign s_axi_arvalid   [02 +: 1]                                                   = S02_AXI_ARVALID   ;
      assign s_axi_rready    [02 +: 1]                                                   = S02_AXI_RREADY    ;
                                                                                         
      assign S02_AXI_ARESET_OUT_N = s_axi_areset_out_n[02 +: 1]                                                   ;
      assign S02_AXI_AWREADY   = s_axi_awready   [02 +: 1]                                                   ;
      assign S02_AXI_WREADY    = s_axi_wready    [02 +: 1]                                                   ;
      assign S02_AXI_BID       = s_axi_bid       [02 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S02_AXI_BRESP     = s_axi_bresp     [02 *2 +: 2]                                                ;
      assign S02_AXI_BVALID    = s_axi_bvalid    [02 +: 1]                                                   ;
      assign S02_AXI_ARREADY   = s_axi_arready   [02 +: 1]                                                   ;
      assign S02_AXI_RID       = s_axi_rid       [02 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S02_AXI_RDATA     = s_axi_rdata     [02 *P_AXI_DATA_MAX_WIDTH +: C_S02_AXI_DATA_WIDTH]       ; 
      assign S02_AXI_RRESP     = s_axi_rresp     [02 *2 +: 2]                                                ;
      assign S02_AXI_RLAST     = s_axi_rlast     [02 +: 1]                                                   ;
      assign S02_AXI_RVALID    = s_axi_rvalid    [02 +: 1]                                                   ;
    end else begin : gen_no_si02
      assign S02_AXI_ARESET_OUT_N = 0 ;
      assign S02_AXI_AWREADY   = 0 ;
      assign S02_AXI_WREADY    = 0 ;
      assign S02_AXI_BID       = 0 ;
      assign S02_AXI_BRESP     = 0 ;
      assign S02_AXI_BVALID    = 0 ;
      assign S02_AXI_ARREADY   = 0 ;
      assign S02_AXI_RID       = 0 ;
      assign S02_AXI_RDATA     = 0 ;
      assign S02_AXI_RRESP     = 0 ;
      assign S02_AXI_RLAST     = 0 ;
      assign S02_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 03) begin : gen_si03
      assign s_axi_aclk      [03 +: 1]                                                   = S03_AXI_ACLK      ;
      assign s_axi_awid      [03 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S03_AXI_AWID      ;
      assign s_axi_awaddr    [03 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S03_AXI_AWADDR    ;
      assign s_axi_awlen     [03 *8 +: 8]                                                = S03_AXI_AWLEN     ;
      assign s_axi_awsize    [03 *3 +: 3]                                                = S03_AXI_AWSIZE    ;
      assign s_axi_awburst   [03 *2 +: 2]                                                = S03_AXI_AWBURST   ;
      assign s_axi_awlock    [03 *2 +: 2]                                                = S03_AXI_AWLOCK    ;
      assign s_axi_awcache   [03 *4 +: 4]                                                = S03_AXI_AWCACHE   ;
      assign s_axi_awprot    [03 *3 +: 3]                                                = S03_AXI_AWPROT    ;
      assign s_axi_awqos     [03 *4 +: 4]                                                = S03_AXI_AWQOS     ;
      assign s_axi_awvalid   [03 +: 1]                                                   = S03_AXI_AWVALID   ;
      assign s_axi_wdata     [03 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S03_AXI_WDATA     ; 
      assign s_axi_wstrb     [03 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S03_AXI_WSTRB     ;    
      assign s_axi_wlast     [03 +: 1]                                                   = S03_AXI_WLAST     ;
      assign s_axi_wvalid    [03 +: 1]                                                   = S03_AXI_WVALID    ;
      assign s_axi_bready    [03 +: 1]                                                   = S03_AXI_BREADY    ;
      assign s_axi_arid      [03 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S03_AXI_ARID      ;
      assign s_axi_araddr    [03 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S03_AXI_ARADDR    ;
      assign s_axi_arlen     [03 *8 +: 8]                                                = S03_AXI_ARLEN     ;
      assign s_axi_arsize    [03 *3 +: 3]                                                = S03_AXI_ARSIZE    ;
      assign s_axi_arburst   [03 *2 +: 2]                                                = S03_AXI_ARBURST   ;
      assign s_axi_arlock    [03 *2 +: 2]                                                = S03_AXI_ARLOCK    ;
      assign s_axi_arcache   [03 *4 +: 4]                                                = S03_AXI_ARCACHE   ;
      assign s_axi_arprot    [03 *3 +: 3]                                                = S03_AXI_ARPROT    ;
      assign s_axi_arqos     [03 *4 +: 4]                                                = S03_AXI_ARQOS     ;
      assign s_axi_arvalid   [03 +: 1]                                                   = S03_AXI_ARVALID   ;
      assign s_axi_rready    [03 +: 1]                                                   = S03_AXI_RREADY    ;
                                                                                         
      assign S03_AXI_ARESET_OUT_N = s_axi_areset_out_n[03 +: 1]                                                   ;
      assign S03_AXI_AWREADY   = s_axi_awready   [03 +: 1]                                                   ;
      assign S03_AXI_WREADY    = s_axi_wready    [03 +: 1]                                                   ;
      assign S03_AXI_BID       = s_axi_bid       [03 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S03_AXI_BRESP     = s_axi_bresp     [03 *2 +: 2]                                                ;
      assign S03_AXI_BVALID    = s_axi_bvalid    [03 +: 1]                                                   ;
      assign S03_AXI_ARREADY   = s_axi_arready   [03 +: 1]                                                   ;
      assign S03_AXI_RID       = s_axi_rid       [03 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S03_AXI_RDATA     = s_axi_rdata     [03 *P_AXI_DATA_MAX_WIDTH +: C_S03_AXI_DATA_WIDTH]       ; 
      assign S03_AXI_RRESP     = s_axi_rresp     [03 *2 +: 2]                                                ;
      assign S03_AXI_RLAST     = s_axi_rlast     [03 +: 1]                                                   ;
      assign S03_AXI_RVALID    = s_axi_rvalid    [03 +: 1]                                                   ;
    end else begin : gen_no_si03
      assign S03_AXI_ARESET_OUT_N = 0 ;
      assign S03_AXI_AWREADY   = 0 ;
      assign S03_AXI_WREADY    = 0 ;
      assign S03_AXI_BID       = 0 ;
      assign S03_AXI_BRESP     = 0 ;
      assign S03_AXI_BVALID    = 0 ;
      assign S03_AXI_ARREADY   = 0 ;
      assign S03_AXI_RID       = 0 ;
      assign S03_AXI_RDATA     = 0 ;
      assign S03_AXI_RRESP     = 0 ;
      assign S03_AXI_RLAST     = 0 ;
      assign S03_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 04) begin : gen_si04
      assign s_axi_aclk      [04 +: 1]                                                   = S04_AXI_ACLK      ;
      assign s_axi_awid      [04 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S04_AXI_AWID      ;
      assign s_axi_awaddr    [04 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S04_AXI_AWADDR    ;
      assign s_axi_awlen     [04 *8 +: 8]                                                = S04_AXI_AWLEN     ;
      assign s_axi_awsize    [04 *3 +: 3]                                                = S04_AXI_AWSIZE    ;
      assign s_axi_awburst   [04 *2 +: 2]                                                = S04_AXI_AWBURST   ;
      assign s_axi_awlock    [04 *2 +: 2]                                                = S04_AXI_AWLOCK    ;
      assign s_axi_awcache   [04 *4 +: 4]                                                = S04_AXI_AWCACHE   ;
      assign s_axi_awprot    [04 *3 +: 3]                                                = S04_AXI_AWPROT    ;
      assign s_axi_awqos     [04 *4 +: 4]                                                = S04_AXI_AWQOS     ;
      assign s_axi_awvalid   [04 +: 1]                                                   = S04_AXI_AWVALID   ;
      assign s_axi_wdata     [04 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S04_AXI_WDATA     ; 
      assign s_axi_wstrb     [04 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S04_AXI_WSTRB     ;    
      assign s_axi_wlast     [04 +: 1]                                                   = S04_AXI_WLAST     ;
      assign s_axi_wvalid    [04 +: 1]                                                   = S04_AXI_WVALID    ;
      assign s_axi_bready    [04 +: 1]                                                   = S04_AXI_BREADY    ;
      assign s_axi_arid      [04 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S04_AXI_ARID      ;
      assign s_axi_araddr    [04 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S04_AXI_ARADDR    ;
      assign s_axi_arlen     [04 *8 +: 8]                                                = S04_AXI_ARLEN     ;
      assign s_axi_arsize    [04 *3 +: 3]                                                = S04_AXI_ARSIZE    ;
      assign s_axi_arburst   [04 *2 +: 2]                                                = S04_AXI_ARBURST   ;
      assign s_axi_arlock    [04 *2 +: 2]                                                = S04_AXI_ARLOCK    ;
      assign s_axi_arcache   [04 *4 +: 4]                                                = S04_AXI_ARCACHE   ;
      assign s_axi_arprot    [04 *3 +: 3]                                                = S04_AXI_ARPROT    ;
      assign s_axi_arqos     [04 *4 +: 4]                                                = S04_AXI_ARQOS     ;
      assign s_axi_arvalid   [04 +: 1]                                                   = S04_AXI_ARVALID   ;
      assign s_axi_rready    [04 +: 1]                                                   = S04_AXI_RREADY    ;
                                                                                         
      assign S04_AXI_ARESET_OUT_N = s_axi_areset_out_n[04 +: 1]                                                   ;
      assign S04_AXI_AWREADY   = s_axi_awready   [04 +: 1]                                                   ;
      assign S04_AXI_WREADY    = s_axi_wready    [04 +: 1]                                                   ;
      assign S04_AXI_BID       = s_axi_bid       [04 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S04_AXI_BRESP     = s_axi_bresp     [04 *2 +: 2]                                                ;
      assign S04_AXI_BVALID    = s_axi_bvalid    [04 +: 1]                                                   ;
      assign S04_AXI_ARREADY   = s_axi_arready   [04 +: 1]                                                   ;
      assign S04_AXI_RID       = s_axi_rid       [04 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S04_AXI_RDATA     = s_axi_rdata     [04 *P_AXI_DATA_MAX_WIDTH +: C_S04_AXI_DATA_WIDTH]       ; 
      assign S04_AXI_RRESP     = s_axi_rresp     [04 *2 +: 2]                                                ;
      assign S04_AXI_RLAST     = s_axi_rlast     [04 +: 1]                                                   ;
      assign S04_AXI_RVALID    = s_axi_rvalid    [04 +: 1]                                                   ;
    end else begin : gen_no_si04
      assign S04_AXI_ARESET_OUT_N = 0 ;
      assign S04_AXI_AWREADY   = 0 ;
      assign S04_AXI_WREADY    = 0 ;
      assign S04_AXI_BID       = 0 ;
      assign S04_AXI_BRESP     = 0 ;
      assign S04_AXI_BVALID    = 0 ;
      assign S04_AXI_ARREADY   = 0 ;
      assign S04_AXI_RID       = 0 ;
      assign S04_AXI_RDATA     = 0 ;
      assign S04_AXI_RRESP     = 0 ;
      assign S04_AXI_RLAST     = 0 ;
      assign S04_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 05) begin : gen_si05
      assign s_axi_aclk      [05 +: 1]                                                   = S05_AXI_ACLK      ;
      assign s_axi_awid      [05 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S05_AXI_AWID      ;
      assign s_axi_awaddr    [05 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S05_AXI_AWADDR    ;
      assign s_axi_awlen     [05 *8 +: 8]                                                = S05_AXI_AWLEN     ;
      assign s_axi_awsize    [05 *3 +: 3]                                                = S05_AXI_AWSIZE    ;
      assign s_axi_awburst   [05 *2 +: 2]                                                = S05_AXI_AWBURST   ;
      assign s_axi_awlock    [05 *2 +: 2]                                                = S05_AXI_AWLOCK    ;
      assign s_axi_awcache   [05 *4 +: 4]                                                = S05_AXI_AWCACHE   ;
      assign s_axi_awprot    [05 *3 +: 3]                                                = S05_AXI_AWPROT    ;
      assign s_axi_awqos     [05 *4 +: 4]                                                = S05_AXI_AWQOS     ;
      assign s_axi_awvalid   [05 +: 1]                                                   = S05_AXI_AWVALID   ;
      assign s_axi_wdata     [05 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S05_AXI_WDATA     ; 
      assign s_axi_wstrb     [05 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S05_AXI_WSTRB     ;    
      assign s_axi_wlast     [05 +: 1]                                                   = S05_AXI_WLAST     ;
      assign s_axi_wvalid    [05 +: 1]                                                   = S05_AXI_WVALID    ;
      assign s_axi_bready    [05 +: 1]                                                   = S05_AXI_BREADY    ;
      assign s_axi_arid      [05 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S05_AXI_ARID      ;
      assign s_axi_araddr    [05 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S05_AXI_ARADDR    ;
      assign s_axi_arlen     [05 *8 +: 8]                                                = S05_AXI_ARLEN     ;
      assign s_axi_arsize    [05 *3 +: 3]                                                = S05_AXI_ARSIZE    ;
      assign s_axi_arburst   [05 *2 +: 2]                                                = S05_AXI_ARBURST   ;
      assign s_axi_arlock    [05 *2 +: 2]                                                = S05_AXI_ARLOCK    ;
      assign s_axi_arcache   [05 *4 +: 4]                                                = S05_AXI_ARCACHE   ;
      assign s_axi_arprot    [05 *3 +: 3]                                                = S05_AXI_ARPROT    ;
      assign s_axi_arqos     [05 *4 +: 4]                                                = S05_AXI_ARQOS     ;
      assign s_axi_arvalid   [05 +: 1]                                                   = S05_AXI_ARVALID   ;
      assign s_axi_rready    [05 +: 1]                                                   = S05_AXI_RREADY    ;
                                                                                         
      assign S05_AXI_ARESET_OUT_N = s_axi_areset_out_n[05 +: 1]                                                   ;
      assign S05_AXI_AWREADY   = s_axi_awready   [05 +: 1]                                                   ;
      assign S05_AXI_WREADY    = s_axi_wready    [05 +: 1]                                                   ;
      assign S05_AXI_BID       = s_axi_bid       [05 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S05_AXI_BRESP     = s_axi_bresp     [05 *2 +: 2]                                                ;
      assign S05_AXI_BVALID    = s_axi_bvalid    [05 +: 1]                                                   ;
      assign S05_AXI_ARREADY   = s_axi_arready   [05 +: 1]                                                   ;
      assign S05_AXI_RID       = s_axi_rid       [05 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S05_AXI_RDATA     = s_axi_rdata     [05 *P_AXI_DATA_MAX_WIDTH +: C_S05_AXI_DATA_WIDTH]       ; 
      assign S05_AXI_RRESP     = s_axi_rresp     [05 *2 +: 2]                                                ;
      assign S05_AXI_RLAST     = s_axi_rlast     [05 +: 1]                                                   ;
      assign S05_AXI_RVALID    = s_axi_rvalid    [05 +: 1]                                                   ;
    end else begin : gen_no_si05
      assign S05_AXI_ARESET_OUT_N = 0 ;
      assign S05_AXI_AWREADY   = 0 ;
      assign S05_AXI_WREADY    = 0 ;
      assign S05_AXI_BID       = 0 ;
      assign S05_AXI_BRESP     = 0 ;
      assign S05_AXI_BVALID    = 0 ;
      assign S05_AXI_ARREADY   = 0 ;
      assign S05_AXI_RID       = 0 ;
      assign S05_AXI_RDATA     = 0 ;
      assign S05_AXI_RRESP     = 0 ;
      assign S05_AXI_RLAST     = 0 ;
      assign S05_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 06) begin : gen_si06
      assign s_axi_aclk      [06 +: 1]                                                   = S06_AXI_ACLK      ;
      assign s_axi_awid      [06 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S06_AXI_AWID      ;
      assign s_axi_awaddr    [06 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S06_AXI_AWADDR    ;
      assign s_axi_awlen     [06 *8 +: 8]                                                = S06_AXI_AWLEN     ;
      assign s_axi_awsize    [06 *3 +: 3]                                                = S06_AXI_AWSIZE    ;
      assign s_axi_awburst   [06 *2 +: 2]                                                = S06_AXI_AWBURST   ;
      assign s_axi_awlock    [06 *2 +: 2]                                                = S06_AXI_AWLOCK    ;
      assign s_axi_awcache   [06 *4 +: 4]                                                = S06_AXI_AWCACHE   ;
      assign s_axi_awprot    [06 *3 +: 3]                                                = S06_AXI_AWPROT    ;
      assign s_axi_awqos     [06 *4 +: 4]                                                = S06_AXI_AWQOS     ;
      assign s_axi_awvalid   [06 +: 1]                                                   = S06_AXI_AWVALID   ;
      assign s_axi_wdata     [06 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S06_AXI_WDATA     ; 
      assign s_axi_wstrb     [06 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S06_AXI_WSTRB     ;    
      assign s_axi_wlast     [06 +: 1]                                                   = S06_AXI_WLAST     ;
      assign s_axi_wvalid    [06 +: 1]                                                   = S06_AXI_WVALID    ;
      assign s_axi_bready    [06 +: 1]                                                   = S06_AXI_BREADY    ;
      assign s_axi_arid      [06 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S06_AXI_ARID      ;
      assign s_axi_araddr    [06 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S06_AXI_ARADDR    ;
      assign s_axi_arlen     [06 *8 +: 8]                                                = S06_AXI_ARLEN     ;
      assign s_axi_arsize    [06 *3 +: 3]                                                = S06_AXI_ARSIZE    ;
      assign s_axi_arburst   [06 *2 +: 2]                                                = S06_AXI_ARBURST   ;
      assign s_axi_arlock    [06 *2 +: 2]                                                = S06_AXI_ARLOCK    ;
      assign s_axi_arcache   [06 *4 +: 4]                                                = S06_AXI_ARCACHE   ;
      assign s_axi_arprot    [06 *3 +: 3]                                                = S06_AXI_ARPROT    ;
      assign s_axi_arqos     [06 *4 +: 4]                                                = S06_AXI_ARQOS     ;
      assign s_axi_arvalid   [06 +: 1]                                                   = S06_AXI_ARVALID   ;
      assign s_axi_rready    [06 +: 1]                                                   = S06_AXI_RREADY    ;
                                                                                         
      assign S06_AXI_ARESET_OUT_N = s_axi_areset_out_n[06 +: 1]                                                   ;
      assign S06_AXI_AWREADY   = s_axi_awready   [06 +: 1]                                                   ;
      assign S06_AXI_WREADY    = s_axi_wready    [06 +: 1]                                                   ;
      assign S06_AXI_BID       = s_axi_bid       [06 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S06_AXI_BRESP     = s_axi_bresp     [06 *2 +: 2]                                                ;
      assign S06_AXI_BVALID    = s_axi_bvalid    [06 +: 1]                                                   ;
      assign S06_AXI_ARREADY   = s_axi_arready   [06 +: 1]                                                   ;
      assign S06_AXI_RID       = s_axi_rid       [06 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S06_AXI_RDATA     = s_axi_rdata     [06 *P_AXI_DATA_MAX_WIDTH +: C_S06_AXI_DATA_WIDTH]       ; 
      assign S06_AXI_RRESP     = s_axi_rresp     [06 *2 +: 2]                                                ;
      assign S06_AXI_RLAST     = s_axi_rlast     [06 +: 1]                                                   ;
      assign S06_AXI_RVALID    = s_axi_rvalid    [06 +: 1]                                                   ;
    end else begin : gen_no_si06
      assign S06_AXI_ARESET_OUT_N = 0 ;
      assign S06_AXI_AWREADY   = 0 ;
      assign S06_AXI_WREADY    = 0 ;
      assign S06_AXI_BID       = 0 ;
      assign S06_AXI_BRESP     = 0 ;
      assign S06_AXI_BVALID    = 0 ;
      assign S06_AXI_ARREADY   = 0 ;
      assign S06_AXI_RID       = 0 ;
      assign S06_AXI_RDATA     = 0 ;
      assign S06_AXI_RRESP     = 0 ;
      assign S06_AXI_RLAST     = 0 ;
      assign S06_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 07) begin : gen_si07
      assign s_axi_aclk      [07 +: 1]                                                   = S07_AXI_ACLK      ;
      assign s_axi_awid      [07 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S07_AXI_AWID      ;
      assign s_axi_awaddr    [07 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S07_AXI_AWADDR    ;
      assign s_axi_awlen     [07 *8 +: 8]                                                = S07_AXI_AWLEN     ;
      assign s_axi_awsize    [07 *3 +: 3]                                                = S07_AXI_AWSIZE    ;
      assign s_axi_awburst   [07 *2 +: 2]                                                = S07_AXI_AWBURST   ;
      assign s_axi_awlock    [07 *2 +: 2]                                                = S07_AXI_AWLOCK    ;
      assign s_axi_awcache   [07 *4 +: 4]                                                = S07_AXI_AWCACHE   ;
      assign s_axi_awprot    [07 *3 +: 3]                                                = S07_AXI_AWPROT    ;
      assign s_axi_awqos     [07 *4 +: 4]                                                = S07_AXI_AWQOS     ;
      assign s_axi_awvalid   [07 +: 1]                                                   = S07_AXI_AWVALID   ;
      assign s_axi_wdata     [07 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S07_AXI_WDATA     ; 
      assign s_axi_wstrb     [07 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S07_AXI_WSTRB     ;    
      assign s_axi_wlast     [07 +: 1]                                                   = S07_AXI_WLAST     ;
      assign s_axi_wvalid    [07 +: 1]                                                   = S07_AXI_WVALID    ;
      assign s_axi_bready    [07 +: 1]                                                   = S07_AXI_BREADY    ;
      assign s_axi_arid      [07 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S07_AXI_ARID      ;
      assign s_axi_araddr    [07 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S07_AXI_ARADDR    ;
      assign s_axi_arlen     [07 *8 +: 8]                                                = S07_AXI_ARLEN     ;
      assign s_axi_arsize    [07 *3 +: 3]                                                = S07_AXI_ARSIZE    ;
      assign s_axi_arburst   [07 *2 +: 2]                                                = S07_AXI_ARBURST   ;
      assign s_axi_arlock    [07 *2 +: 2]                                                = S07_AXI_ARLOCK    ;
      assign s_axi_arcache   [07 *4 +: 4]                                                = S07_AXI_ARCACHE   ;
      assign s_axi_arprot    [07 *3 +: 3]                                                = S07_AXI_ARPROT    ;
      assign s_axi_arqos     [07 *4 +: 4]                                                = S07_AXI_ARQOS     ;
      assign s_axi_arvalid   [07 +: 1]                                                   = S07_AXI_ARVALID   ;
      assign s_axi_rready    [07 +: 1]                                                   = S07_AXI_RREADY    ;
                                                                                         
      assign S07_AXI_ARESET_OUT_N = s_axi_areset_out_n[07 +: 1]                                                   ;
      assign S07_AXI_AWREADY   = s_axi_awready   [07 +: 1]                                                   ;
      assign S07_AXI_WREADY    = s_axi_wready    [07 +: 1]                                                   ;
      assign S07_AXI_BID       = s_axi_bid       [07 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S07_AXI_BRESP     = s_axi_bresp     [07 *2 +: 2]                                                ;
      assign S07_AXI_BVALID    = s_axi_bvalid    [07 +: 1]                                                   ;
      assign S07_AXI_ARREADY   = s_axi_arready   [07 +: 1]                                                   ;
      assign S07_AXI_RID       = s_axi_rid       [07 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S07_AXI_RDATA     = s_axi_rdata     [07 *P_AXI_DATA_MAX_WIDTH +: C_S07_AXI_DATA_WIDTH]       ; 
      assign S07_AXI_RRESP     = s_axi_rresp     [07 *2 +: 2]                                                ;
      assign S07_AXI_RLAST     = s_axi_rlast     [07 +: 1]                                                   ;
      assign S07_AXI_RVALID    = s_axi_rvalid    [07 +: 1]                                                   ;
    end else begin : gen_no_si07
      assign S07_AXI_ARESET_OUT_N = 0 ;
      assign S07_AXI_AWREADY   = 0 ;
      assign S07_AXI_WREADY    = 0 ;
      assign S07_AXI_BID       = 0 ;
      assign S07_AXI_BRESP     = 0 ;
      assign S07_AXI_BVALID    = 0 ;
      assign S07_AXI_ARREADY   = 0 ;
      assign S07_AXI_RID       = 0 ;
      assign S07_AXI_RDATA     = 0 ;
      assign S07_AXI_RRESP     = 0 ;
      assign S07_AXI_RLAST     = 0 ;
      assign S07_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 08) begin : gen_si08
      assign s_axi_aclk      [08 +: 1]                                                   = S08_AXI_ACLK      ;
      assign s_axi_awid      [08 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S08_AXI_AWID      ;
      assign s_axi_awaddr    [08 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S08_AXI_AWADDR    ;
      assign s_axi_awlen     [08 *8 +: 8]                                                = S08_AXI_AWLEN     ;
      assign s_axi_awsize    [08 *3 +: 3]                                                = S08_AXI_AWSIZE    ;
      assign s_axi_awburst   [08 *2 +: 2]                                                = S08_AXI_AWBURST   ;
      assign s_axi_awlock    [08 *2 +: 2]                                                = S08_AXI_AWLOCK    ;
      assign s_axi_awcache   [08 *4 +: 4]                                                = S08_AXI_AWCACHE   ;
      assign s_axi_awprot    [08 *3 +: 3]                                                = S08_AXI_AWPROT    ;
      assign s_axi_awqos     [08 *4 +: 4]                                                = S08_AXI_AWQOS     ;
      assign s_axi_awvalid   [08 +: 1]                                                   = S08_AXI_AWVALID   ;
      assign s_axi_wdata     [08 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S08_AXI_WDATA     ; 
      assign s_axi_wstrb     [08 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S08_AXI_WSTRB     ;    
      assign s_axi_wlast     [08 +: 1]                                                   = S08_AXI_WLAST     ;
      assign s_axi_wvalid    [08 +: 1]                                                   = S08_AXI_WVALID    ;
      assign s_axi_bready    [08 +: 1]                                                   = S08_AXI_BREADY    ;
      assign s_axi_arid      [08 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S08_AXI_ARID      ;
      assign s_axi_araddr    [08 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S08_AXI_ARADDR    ;
      assign s_axi_arlen     [08 *8 +: 8]                                                = S08_AXI_ARLEN     ;
      assign s_axi_arsize    [08 *3 +: 3]                                                = S08_AXI_ARSIZE    ;
      assign s_axi_arburst   [08 *2 +: 2]                                                = S08_AXI_ARBURST   ;
      assign s_axi_arlock    [08 *2 +: 2]                                                = S08_AXI_ARLOCK    ;
      assign s_axi_arcache   [08 *4 +: 4]                                                = S08_AXI_ARCACHE   ;
      assign s_axi_arprot    [08 *3 +: 3]                                                = S08_AXI_ARPROT    ;
      assign s_axi_arqos     [08 *4 +: 4]                                                = S08_AXI_ARQOS     ;
      assign s_axi_arvalid   [08 +: 1]                                                   = S08_AXI_ARVALID   ;
      assign s_axi_rready    [08 +: 1]                                                   = S08_AXI_RREADY    ;
                                                                                         
      assign S08_AXI_ARESET_OUT_N = s_axi_areset_out_n[08 +: 1]                                                   ;
      assign S08_AXI_AWREADY   = s_axi_awready   [08 +: 1]                                                   ;
      assign S08_AXI_WREADY    = s_axi_wready    [08 +: 1]                                                   ;
      assign S08_AXI_BID       = s_axi_bid       [08 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S08_AXI_BRESP     = s_axi_bresp     [08 *2 +: 2]                                                ;
      assign S08_AXI_BVALID    = s_axi_bvalid    [08 +: 1]                                                   ;
      assign S08_AXI_ARREADY   = s_axi_arready   [08 +: 1]                                                   ;
      assign S08_AXI_RID       = s_axi_rid       [08 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S08_AXI_RDATA     = s_axi_rdata     [08 *P_AXI_DATA_MAX_WIDTH +: C_S08_AXI_DATA_WIDTH]       ; 
      assign S08_AXI_RRESP     = s_axi_rresp     [08 *2 +: 2]                                                ;
      assign S08_AXI_RLAST     = s_axi_rlast     [08 +: 1]                                                   ;
      assign S08_AXI_RVALID    = s_axi_rvalid    [08 +: 1]                                                   ;
    end else begin : gen_no_si08
      assign S08_AXI_ARESET_OUT_N = 0 ;
      assign S08_AXI_AWREADY   = 0 ;
      assign S08_AXI_WREADY    = 0 ;
      assign S08_AXI_BID       = 0 ;
      assign S08_AXI_BRESP     = 0 ;
      assign S08_AXI_BVALID    = 0 ;
      assign S08_AXI_ARREADY   = 0 ;
      assign S08_AXI_RID       = 0 ;
      assign S08_AXI_RDATA     = 0 ;
      assign S08_AXI_RRESP     = 0 ;
      assign S08_AXI_RLAST     = 0 ;
      assign S08_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 09) begin : gen_si09
      assign s_axi_aclk      [09 +: 1]                                                   = S09_AXI_ACLK      ;
      assign s_axi_awid      [09 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S09_AXI_AWID      ;
      assign s_axi_awaddr    [09 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S09_AXI_AWADDR    ;
      assign s_axi_awlen     [09 *8 +: 8]                                                = S09_AXI_AWLEN     ;
      assign s_axi_awsize    [09 *3 +: 3]                                                = S09_AXI_AWSIZE    ;
      assign s_axi_awburst   [09 *2 +: 2]                                                = S09_AXI_AWBURST   ;
      assign s_axi_awlock    [09 *2 +: 2]                                                = S09_AXI_AWLOCK    ;
      assign s_axi_awcache   [09 *4 +: 4]                                                = S09_AXI_AWCACHE   ;
      assign s_axi_awprot    [09 *3 +: 3]                                                = S09_AXI_AWPROT    ;
      assign s_axi_awqos     [09 *4 +: 4]                                                = S09_AXI_AWQOS     ;
      assign s_axi_awvalid   [09 +: 1]                                                   = S09_AXI_AWVALID   ;
      assign s_axi_wdata     [09 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S09_AXI_WDATA     ; 
      assign s_axi_wstrb     [09 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S09_AXI_WSTRB     ;    
      assign s_axi_wlast     [09 +: 1]                                                   = S09_AXI_WLAST     ;
      assign s_axi_wvalid    [09 +: 1]                                                   = S09_AXI_WVALID    ;
      assign s_axi_bready    [09 +: 1]                                                   = S09_AXI_BREADY    ;
      assign s_axi_arid      [09 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S09_AXI_ARID      ;
      assign s_axi_araddr    [09 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S09_AXI_ARADDR    ;
      assign s_axi_arlen     [09 *8 +: 8]                                                = S09_AXI_ARLEN     ;
      assign s_axi_arsize    [09 *3 +: 3]                                                = S09_AXI_ARSIZE    ;
      assign s_axi_arburst   [09 *2 +: 2]                                                = S09_AXI_ARBURST   ;
      assign s_axi_arlock    [09 *2 +: 2]                                                = S09_AXI_ARLOCK    ;
      assign s_axi_arcache   [09 *4 +: 4]                                                = S09_AXI_ARCACHE   ;
      assign s_axi_arprot    [09 *3 +: 3]                                                = S09_AXI_ARPROT    ;
      assign s_axi_arqos     [09 *4 +: 4]                                                = S09_AXI_ARQOS     ;
      assign s_axi_arvalid   [09 +: 1]                                                   = S09_AXI_ARVALID   ;
      assign s_axi_rready    [09 +: 1]                                                   = S09_AXI_RREADY    ;
                                                                                         
      assign S09_AXI_ARESET_OUT_N = s_axi_areset_out_n[09 +: 1]                                                   ;
      assign S09_AXI_AWREADY   = s_axi_awready   [09 +: 1]                                                   ;
      assign S09_AXI_WREADY    = s_axi_wready    [09 +: 1]                                                   ;
      assign S09_AXI_BID       = s_axi_bid       [09 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S09_AXI_BRESP     = s_axi_bresp     [09 *2 +: 2]                                                ;
      assign S09_AXI_BVALID    = s_axi_bvalid    [09 +: 1]                                                   ;
      assign S09_AXI_ARREADY   = s_axi_arready   [09 +: 1]                                                   ;
      assign S09_AXI_RID       = s_axi_rid       [09 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S09_AXI_RDATA     = s_axi_rdata     [09 *P_AXI_DATA_MAX_WIDTH +: C_S09_AXI_DATA_WIDTH]       ; 
      assign S09_AXI_RRESP     = s_axi_rresp     [09 *2 +: 2]                                                ;
      assign S09_AXI_RLAST     = s_axi_rlast     [09 +: 1]                                                   ;
      assign S09_AXI_RVALID    = s_axi_rvalid    [09 +: 1]                                                   ;
    end else begin : gen_no_si09
      assign S09_AXI_ARESET_OUT_N = 0 ;
      assign S09_AXI_AWREADY   = 0 ;
      assign S09_AXI_WREADY    = 0 ;
      assign S09_AXI_BID       = 0 ;
      assign S09_AXI_BRESP     = 0 ;
      assign S09_AXI_BVALID    = 0 ;
      assign S09_AXI_ARREADY   = 0 ;
      assign S09_AXI_RID       = 0 ;
      assign S09_AXI_RDATA     = 0 ;
      assign S09_AXI_RRESP     = 0 ;
      assign S09_AXI_RLAST     = 0 ;
      assign S09_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 10) begin : gen_si10
      assign s_axi_aclk      [10 +: 1]                                                   = S10_AXI_ACLK      ;
      assign s_axi_awid      [10 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S10_AXI_AWID      ;
      assign s_axi_awaddr    [10 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S10_AXI_AWADDR    ;
      assign s_axi_awlen     [10 *8 +: 8]                                                = S10_AXI_AWLEN     ;
      assign s_axi_awsize    [10 *3 +: 3]                                                = S10_AXI_AWSIZE    ;
      assign s_axi_awburst   [10 *2 +: 2]                                                = S10_AXI_AWBURST   ;
      assign s_axi_awlock    [10 *2 +: 2]                                                = S10_AXI_AWLOCK    ;
      assign s_axi_awcache   [10 *4 +: 4]                                                = S10_AXI_AWCACHE   ;
      assign s_axi_awprot    [10 *3 +: 3]                                                = S10_AXI_AWPROT    ;
      assign s_axi_awqos     [10 *4 +: 4]                                                = S10_AXI_AWQOS     ;
      assign s_axi_awvalid   [10 +: 1]                                                   = S10_AXI_AWVALID   ;
      assign s_axi_wdata     [10 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S10_AXI_WDATA     ; 
      assign s_axi_wstrb     [10 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S10_AXI_WSTRB     ;    
      assign s_axi_wlast     [10 +: 1]                                                   = S10_AXI_WLAST     ;
      assign s_axi_wvalid    [10 +: 1]                                                   = S10_AXI_WVALID    ;
      assign s_axi_bready    [10 +: 1]                                                   = S10_AXI_BREADY    ;
      assign s_axi_arid      [10 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S10_AXI_ARID      ;
      assign s_axi_araddr    [10 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S10_AXI_ARADDR    ;
      assign s_axi_arlen     [10 *8 +: 8]                                                = S10_AXI_ARLEN     ;
      assign s_axi_arsize    [10 *3 +: 3]                                                = S10_AXI_ARSIZE    ;
      assign s_axi_arburst   [10 *2 +: 2]                                                = S10_AXI_ARBURST   ;
      assign s_axi_arlock    [10 *2 +: 2]                                                = S10_AXI_ARLOCK    ;
      assign s_axi_arcache   [10 *4 +: 4]                                                = S10_AXI_ARCACHE   ;
      assign s_axi_arprot    [10 *3 +: 3]                                                = S10_AXI_ARPROT    ;
      assign s_axi_arqos     [10 *4 +: 4]                                                = S10_AXI_ARQOS     ;
      assign s_axi_arvalid   [10 +: 1]                                                   = S10_AXI_ARVALID   ;
      assign s_axi_rready    [10 +: 1]                                                   = S10_AXI_RREADY    ;
                                                                                         
      assign S10_AXI_ARESET_OUT_N = s_axi_areset_out_n[10 +: 1]                                                   ;
      assign S10_AXI_AWREADY   = s_axi_awready   [10 +: 1]                                                   ;
      assign S10_AXI_WREADY    = s_axi_wready    [10 +: 1]                                                   ;
      assign S10_AXI_BID       = s_axi_bid       [10 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S10_AXI_BRESP     = s_axi_bresp     [10 *2 +: 2]                                                ;
      assign S10_AXI_BVALID    = s_axi_bvalid    [10 +: 1]                                                   ;
      assign S10_AXI_ARREADY   = s_axi_arready   [10 +: 1]                                                   ;
      assign S10_AXI_RID       = s_axi_rid       [10 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S10_AXI_RDATA     = s_axi_rdata     [10 *P_AXI_DATA_MAX_WIDTH +: C_S10_AXI_DATA_WIDTH]       ; 
      assign S10_AXI_RRESP     = s_axi_rresp     [10 *2 +: 2]                                                ;
      assign S10_AXI_RLAST     = s_axi_rlast     [10 +: 1]                                                   ;
      assign S10_AXI_RVALID    = s_axi_rvalid    [10 +: 1]                                                   ;
    end else begin : gen_no_si10
      assign S10_AXI_ARESET_OUT_N = 0 ;
      assign S10_AXI_AWREADY   = 0 ;
      assign S10_AXI_WREADY    = 0 ;
      assign S10_AXI_BID       = 0 ;
      assign S10_AXI_BRESP     = 0 ;
      assign S10_AXI_BVALID    = 0 ;
      assign S10_AXI_ARREADY   = 0 ;
      assign S10_AXI_RID       = 0 ;
      assign S10_AXI_RDATA     = 0 ;
      assign S10_AXI_RRESP     = 0 ;
      assign S10_AXI_RLAST     = 0 ;
      assign S10_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 11) begin : gen_si11
      assign s_axi_aclk      [11 +: 1]                                                   = S11_AXI_ACLK      ;
      assign s_axi_awid      [11 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S11_AXI_AWID      ;
      assign s_axi_awaddr    [11 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S11_AXI_AWADDR    ;
      assign s_axi_awlen     [11 *8 +: 8]                                                = S11_AXI_AWLEN     ;
      assign s_axi_awsize    [11 *3 +: 3]                                                = S11_AXI_AWSIZE    ;
      assign s_axi_awburst   [11 *2 +: 2]                                                = S11_AXI_AWBURST   ;
      assign s_axi_awlock    [11 *2 +: 2]                                                = S11_AXI_AWLOCK    ;
      assign s_axi_awcache   [11 *4 +: 4]                                                = S11_AXI_AWCACHE   ;
      assign s_axi_awprot    [11 *3 +: 3]                                                = S11_AXI_AWPROT    ;
      assign s_axi_awqos     [11 *4 +: 4]                                                = S11_AXI_AWQOS     ;
      assign s_axi_awvalid   [11 +: 1]                                                   = S11_AXI_AWVALID   ;
      assign s_axi_wdata     [11 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S11_AXI_WDATA     ; 
      assign s_axi_wstrb     [11 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S11_AXI_WSTRB     ;    
      assign s_axi_wlast     [11 +: 1]                                                   = S11_AXI_WLAST     ;
      assign s_axi_wvalid    [11 +: 1]                                                   = S11_AXI_WVALID    ;
      assign s_axi_bready    [11 +: 1]                                                   = S11_AXI_BREADY    ;
      assign s_axi_arid      [11 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S11_AXI_ARID      ;
      assign s_axi_araddr    [11 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S11_AXI_ARADDR    ;
      assign s_axi_arlen     [11 *8 +: 8]                                                = S11_AXI_ARLEN     ;
      assign s_axi_arsize    [11 *3 +: 3]                                                = S11_AXI_ARSIZE    ;
      assign s_axi_arburst   [11 *2 +: 2]                                                = S11_AXI_ARBURST   ;
      assign s_axi_arlock    [11 *2 +: 2]                                                = S11_AXI_ARLOCK    ;
      assign s_axi_arcache   [11 *4 +: 4]                                                = S11_AXI_ARCACHE   ;
      assign s_axi_arprot    [11 *3 +: 3]                                                = S11_AXI_ARPROT    ;
      assign s_axi_arqos     [11 *4 +: 4]                                                = S11_AXI_ARQOS     ;
      assign s_axi_arvalid   [11 +: 1]                                                   = S11_AXI_ARVALID   ;
      assign s_axi_rready    [11 +: 1]                                                   = S11_AXI_RREADY    ;
                                                                                         
      assign S11_AXI_ARESET_OUT_N = s_axi_areset_out_n[11 +: 1]                                                   ;
      assign S11_AXI_AWREADY   = s_axi_awready   [11 +: 1]                                                   ;
      assign S11_AXI_WREADY    = s_axi_wready    [11 +: 1]                                                   ;
      assign S11_AXI_BID       = s_axi_bid       [11 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S11_AXI_BRESP     = s_axi_bresp     [11 *2 +: 2]                                                ;
      assign S11_AXI_BVALID    = s_axi_bvalid    [11 +: 1]                                                   ;
      assign S11_AXI_ARREADY   = s_axi_arready   [11 +: 1]                                                   ;
      assign S11_AXI_RID       = s_axi_rid       [11 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S11_AXI_RDATA     = s_axi_rdata     [11 *P_AXI_DATA_MAX_WIDTH +: C_S11_AXI_DATA_WIDTH]       ; 
      assign S11_AXI_RRESP     = s_axi_rresp     [11 *2 +: 2]                                                ;
      assign S11_AXI_RLAST     = s_axi_rlast     [11 +: 1]                                                   ;
      assign S11_AXI_RVALID    = s_axi_rvalid    [11 +: 1]                                                   ;
    end else begin : gen_no_si11
      assign S11_AXI_ARESET_OUT_N = 0 ;
      assign S11_AXI_AWREADY   = 0 ;
      assign S11_AXI_WREADY    = 0 ;
      assign S11_AXI_BID       = 0 ;
      assign S11_AXI_BRESP     = 0 ;
      assign S11_AXI_BVALID    = 0 ;
      assign S11_AXI_ARREADY   = 0 ;
      assign S11_AXI_RID       = 0 ;
      assign S11_AXI_RDATA     = 0 ;
      assign S11_AXI_RRESP     = 0 ;
      assign S11_AXI_RLAST     = 0 ;
      assign S11_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 12) begin : gen_si12
      assign s_axi_aclk      [12 +: 1]                                                   = S12_AXI_ACLK      ;
      assign s_axi_awid      [12 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S12_AXI_AWID      ;
      assign s_axi_awaddr    [12 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S12_AXI_AWADDR    ;
      assign s_axi_awlen     [12 *8 +: 8]                                                = S12_AXI_AWLEN     ;
      assign s_axi_awsize    [12 *3 +: 3]                                                = S12_AXI_AWSIZE    ;
      assign s_axi_awburst   [12 *2 +: 2]                                                = S12_AXI_AWBURST   ;
      assign s_axi_awlock    [12 *2 +: 2]                                                = S12_AXI_AWLOCK    ;
      assign s_axi_awcache   [12 *4 +: 4]                                                = S12_AXI_AWCACHE   ;
      assign s_axi_awprot    [12 *3 +: 3]                                                = S12_AXI_AWPROT    ;
      assign s_axi_awqos     [12 *4 +: 4]                                                = S12_AXI_AWQOS     ;
      assign s_axi_awvalid   [12 +: 1]                                                   = S12_AXI_AWVALID   ;
      assign s_axi_wdata     [12 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S12_AXI_WDATA     ; 
      assign s_axi_wstrb     [12 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S12_AXI_WSTRB     ;    
      assign s_axi_wlast     [12 +: 1]                                                   = S12_AXI_WLAST     ;
      assign s_axi_wvalid    [12 +: 1]                                                   = S12_AXI_WVALID    ;
      assign s_axi_bready    [12 +: 1]                                                   = S12_AXI_BREADY    ;
      assign s_axi_arid      [12 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S12_AXI_ARID      ;
      assign s_axi_araddr    [12 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S12_AXI_ARADDR    ;
      assign s_axi_arlen     [12 *8 +: 8]                                                = S12_AXI_ARLEN     ;
      assign s_axi_arsize    [12 *3 +: 3]                                                = S12_AXI_ARSIZE    ;
      assign s_axi_arburst   [12 *2 +: 2]                                                = S12_AXI_ARBURST   ;
      assign s_axi_arlock    [12 *2 +: 2]                                                = S12_AXI_ARLOCK    ;
      assign s_axi_arcache   [12 *4 +: 4]                                                = S12_AXI_ARCACHE   ;
      assign s_axi_arprot    [12 *3 +: 3]                                                = S12_AXI_ARPROT    ;
      assign s_axi_arqos     [12 *4 +: 4]                                                = S12_AXI_ARQOS     ;
      assign s_axi_arvalid   [12 +: 1]                                                   = S12_AXI_ARVALID   ;
      assign s_axi_rready    [12 +: 1]                                                   = S12_AXI_RREADY    ;
                                                                                         
      assign S12_AXI_ARESET_OUT_N = s_axi_areset_out_n[12 +: 1]                                                   ;
      assign S12_AXI_AWREADY   = s_axi_awready   [12 +: 1]                                                   ;
      assign S12_AXI_WREADY    = s_axi_wready    [12 +: 1]                                                   ;
      assign S12_AXI_BID       = s_axi_bid       [12 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S12_AXI_BRESP     = s_axi_bresp     [12 *2 +: 2]                                                ;
      assign S12_AXI_BVALID    = s_axi_bvalid    [12 +: 1]                                                   ;
      assign S12_AXI_ARREADY   = s_axi_arready   [12 +: 1]                                                   ;
      assign S12_AXI_RID       = s_axi_rid       [12 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S12_AXI_RDATA     = s_axi_rdata     [12 *P_AXI_DATA_MAX_WIDTH +: C_S12_AXI_DATA_WIDTH]       ; 
      assign S12_AXI_RRESP     = s_axi_rresp     [12 *2 +: 2]                                                ;
      assign S12_AXI_RLAST     = s_axi_rlast     [12 +: 1]                                                   ;
      assign S12_AXI_RVALID    = s_axi_rvalid    [12 +: 1]                                                   ;
    end else begin : gen_no_si12
      assign S12_AXI_ARESET_OUT_N = 0 ;
      assign S12_AXI_AWREADY   = 0 ;
      assign S12_AXI_WREADY    = 0 ;
      assign S12_AXI_BID       = 0 ;
      assign S12_AXI_BRESP     = 0 ;
      assign S12_AXI_BVALID    = 0 ;
      assign S12_AXI_ARREADY   = 0 ;
      assign S12_AXI_RID       = 0 ;
      assign S12_AXI_RDATA     = 0 ;
      assign S12_AXI_RRESP     = 0 ;
      assign S12_AXI_RLAST     = 0 ;
      assign S12_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 13) begin : gen_si13
      assign s_axi_aclk      [13 +: 1]                                                   = S13_AXI_ACLK      ;
      assign s_axi_awid      [13 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S13_AXI_AWID      ;
      assign s_axi_awaddr    [13 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S13_AXI_AWADDR    ;
      assign s_axi_awlen     [13 *8 +: 8]                                                = S13_AXI_AWLEN     ;
      assign s_axi_awsize    [13 *3 +: 3]                                                = S13_AXI_AWSIZE    ;
      assign s_axi_awburst   [13 *2 +: 2]                                                = S13_AXI_AWBURST   ;
      assign s_axi_awlock    [13 *2 +: 2]                                                = S13_AXI_AWLOCK    ;
      assign s_axi_awcache   [13 *4 +: 4]                                                = S13_AXI_AWCACHE   ;
      assign s_axi_awprot    [13 *3 +: 3]                                                = S13_AXI_AWPROT    ;
      assign s_axi_awqos     [13 *4 +: 4]                                                = S13_AXI_AWQOS     ;
      assign s_axi_awvalid   [13 +: 1]                                                   = S13_AXI_AWVALID   ;
      assign s_axi_wdata     [13 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S13_AXI_WDATA     ; 
      assign s_axi_wstrb     [13 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S13_AXI_WSTRB     ;    
      assign s_axi_wlast     [13 +: 1]                                                   = S13_AXI_WLAST     ;
      assign s_axi_wvalid    [13 +: 1]                                                   = S13_AXI_WVALID    ;
      assign s_axi_bready    [13 +: 1]                                                   = S13_AXI_BREADY    ;
      assign s_axi_arid      [13 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S13_AXI_ARID      ;
      assign s_axi_araddr    [13 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S13_AXI_ARADDR    ;
      assign s_axi_arlen     [13 *8 +: 8]                                                = S13_AXI_ARLEN     ;
      assign s_axi_arsize    [13 *3 +: 3]                                                = S13_AXI_ARSIZE    ;
      assign s_axi_arburst   [13 *2 +: 2]                                                = S13_AXI_ARBURST   ;
      assign s_axi_arlock    [13 *2 +: 2]                                                = S13_AXI_ARLOCK    ;
      assign s_axi_arcache   [13 *4 +: 4]                                                = S13_AXI_ARCACHE   ;
      assign s_axi_arprot    [13 *3 +: 3]                                                = S13_AXI_ARPROT    ;
      assign s_axi_arqos     [13 *4 +: 4]                                                = S13_AXI_ARQOS     ;
      assign s_axi_arvalid   [13 +: 1]                                                   = S13_AXI_ARVALID   ;
      assign s_axi_rready    [13 +: 1]                                                   = S13_AXI_RREADY    ;
                                                                                         
      assign S13_AXI_ARESET_OUT_N = s_axi_areset_out_n[13 +: 1]                                                   ;
      assign S13_AXI_AWREADY   = s_axi_awready   [13 +: 1]                                                   ;
      assign S13_AXI_WREADY    = s_axi_wready    [13 +: 1]                                                   ;
      assign S13_AXI_BID       = s_axi_bid       [13 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S13_AXI_BRESP     = s_axi_bresp     [13 *2 +: 2]                                                ;
      assign S13_AXI_BVALID    = s_axi_bvalid    [13 +: 1]                                                   ;
      assign S13_AXI_ARREADY   = s_axi_arready   [13 +: 1]                                                   ;
      assign S13_AXI_RID       = s_axi_rid       [13 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S13_AXI_RDATA     = s_axi_rdata     [13 *P_AXI_DATA_MAX_WIDTH +: C_S13_AXI_DATA_WIDTH]       ; 
      assign S13_AXI_RRESP     = s_axi_rresp     [13 *2 +: 2]                                                ;
      assign S13_AXI_RLAST     = s_axi_rlast     [13 +: 1]                                                   ;
      assign S13_AXI_RVALID    = s_axi_rvalid    [13 +: 1]                                                   ;
    end else begin : gen_no_si13
      assign S13_AXI_ARESET_OUT_N = 0 ;
      assign S13_AXI_AWREADY   = 0 ;
      assign S13_AXI_WREADY    = 0 ;
      assign S13_AXI_BID       = 0 ;
      assign S13_AXI_BRESP     = 0 ;
      assign S13_AXI_BVALID    = 0 ;
      assign S13_AXI_ARREADY   = 0 ;
      assign S13_AXI_RID       = 0 ;
      assign S13_AXI_RDATA     = 0 ;
      assign S13_AXI_RRESP     = 0 ;
      assign S13_AXI_RLAST     = 0 ;
      assign S13_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 14) begin : gen_si14
      assign s_axi_aclk      [14 +: 1]                                                   = S14_AXI_ACLK      ;
      assign s_axi_awid      [14 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S14_AXI_AWID      ;
      assign s_axi_awaddr    [14 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S14_AXI_AWADDR    ;
      assign s_axi_awlen     [14 *8 +: 8]                                                = S14_AXI_AWLEN     ;
      assign s_axi_awsize    [14 *3 +: 3]                                                = S14_AXI_AWSIZE    ;
      assign s_axi_awburst   [14 *2 +: 2]                                                = S14_AXI_AWBURST   ;
      assign s_axi_awlock    [14 *2 +: 2]                                                = S14_AXI_AWLOCK    ;
      assign s_axi_awcache   [14 *4 +: 4]                                                = S14_AXI_AWCACHE   ;
      assign s_axi_awprot    [14 *3 +: 3]                                                = S14_AXI_AWPROT    ;
      assign s_axi_awqos     [14 *4 +: 4]                                                = S14_AXI_AWQOS     ;
      assign s_axi_awvalid   [14 +: 1]                                                   = S14_AXI_AWVALID   ;
      assign s_axi_wdata     [14 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S14_AXI_WDATA     ; 
      assign s_axi_wstrb     [14 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S14_AXI_WSTRB     ;    
      assign s_axi_wlast     [14 +: 1]                                                   = S14_AXI_WLAST     ;
      assign s_axi_wvalid    [14 +: 1]                                                   = S14_AXI_WVALID    ;
      assign s_axi_bready    [14 +: 1]                                                   = S14_AXI_BREADY    ;
      assign s_axi_arid      [14 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S14_AXI_ARID      ;
      assign s_axi_araddr    [14 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S14_AXI_ARADDR    ;
      assign s_axi_arlen     [14 *8 +: 8]                                                = S14_AXI_ARLEN     ;
      assign s_axi_arsize    [14 *3 +: 3]                                                = S14_AXI_ARSIZE    ;
      assign s_axi_arburst   [14 *2 +: 2]                                                = S14_AXI_ARBURST   ;
      assign s_axi_arlock    [14 *2 +: 2]                                                = S14_AXI_ARLOCK    ;
      assign s_axi_arcache   [14 *4 +: 4]                                                = S14_AXI_ARCACHE   ;
      assign s_axi_arprot    [14 *3 +: 3]                                                = S14_AXI_ARPROT    ;
      assign s_axi_arqos     [14 *4 +: 4]                                                = S14_AXI_ARQOS     ;
      assign s_axi_arvalid   [14 +: 1]                                                   = S14_AXI_ARVALID   ;
      assign s_axi_rready    [14 +: 1]                                                   = S14_AXI_RREADY    ;
                                                                                         
      assign S14_AXI_ARESET_OUT_N = s_axi_areset_out_n[14 +: 1]                                                   ;
      assign S14_AXI_AWREADY   = s_axi_awready   [14 +: 1]                                                   ;
      assign S14_AXI_WREADY    = s_axi_wready    [14 +: 1]                                                   ;
      assign S14_AXI_BID       = s_axi_bid       [14 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S14_AXI_BRESP     = s_axi_bresp     [14 *2 +: 2]                                                ;
      assign S14_AXI_BVALID    = s_axi_bvalid    [14 +: 1]                                                   ;
      assign S14_AXI_ARREADY   = s_axi_arready   [14 +: 1]                                                   ;
      assign S14_AXI_RID       = s_axi_rid       [14 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S14_AXI_RDATA     = s_axi_rdata     [14 *P_AXI_DATA_MAX_WIDTH +: C_S14_AXI_DATA_WIDTH]       ; 
      assign S14_AXI_RRESP     = s_axi_rresp     [14 *2 +: 2]                                                ;
      assign S14_AXI_RLAST     = s_axi_rlast     [14 +: 1]                                                   ;
      assign S14_AXI_RVALID    = s_axi_rvalid    [14 +: 1]                                                   ;
    end else begin : gen_no_si14
      assign S14_AXI_ARESET_OUT_N = 0 ;
      assign S14_AXI_AWREADY   = 0 ;
      assign S14_AXI_WREADY    = 0 ;
      assign S14_AXI_BID       = 0 ;
      assign S14_AXI_BRESP     = 0 ;
      assign S14_AXI_BVALID    = 0 ;
      assign S14_AXI_ARREADY   = 0 ;
      assign S14_AXI_RID       = 0 ;
      assign S14_AXI_RDATA     = 0 ;
      assign S14_AXI_RRESP     = 0 ;
      assign S14_AXI_RLAST     = 0 ;
      assign S14_AXI_RVALID    = 0 ;
    end
  endgenerate
  
  generate
    if (C_NUM_SLAVE_PORTS > 15) begin : gen_si15
      assign s_axi_aclk      [15 +: 1]                                                   = S15_AXI_ACLK      ;
      assign s_axi_awid      [15 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S15_AXI_AWID      ;
      assign s_axi_awaddr    [15 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S15_AXI_AWADDR    ;
      assign s_axi_awlen     [15 *8 +: 8]                                                = S15_AXI_AWLEN     ;
      assign s_axi_awsize    [15 *3 +: 3]                                                = S15_AXI_AWSIZE    ;
      assign s_axi_awburst   [15 *2 +: 2]                                                = S15_AXI_AWBURST   ;
      assign s_axi_awlock    [15 *2 +: 2]                                                = S15_AXI_AWLOCK    ;
      assign s_axi_awcache   [15 *4 +: 4]                                                = S15_AXI_AWCACHE   ;
      assign s_axi_awprot    [15 *3 +: 3]                                                = S15_AXI_AWPROT    ;
      assign s_axi_awqos     [15 *4 +: 4]                                                = S15_AXI_AWQOS     ;
      assign s_axi_awvalid   [15 +: 1]                                                   = S15_AXI_AWVALID   ;
      assign s_axi_wdata     [15 *P_AXI_DATA_MAX_WIDTH +: P_AXI_DATA_MAX_WIDTH]          = S15_AXI_WDATA     ; 
      assign s_axi_wstrb     [15 *P_AXI_DATA_MAX_WIDTH/8 +: P_AXI_DATA_MAX_WIDTH/8]      = S15_AXI_WSTRB     ;    
      assign s_axi_wlast     [15 +: 1]                                                   = S15_AXI_WLAST     ;
      assign s_axi_wvalid    [15 +: 1]                                                   = S15_AXI_WVALID    ;
      assign s_axi_bready    [15 +: 1]                                                   = S15_AXI_BREADY    ;
      assign s_axi_arid      [15 *P_AXI_ID_WIDTH +: P_AXI_ID_WIDTH]                      = S15_AXI_ARID      ;
      assign s_axi_araddr    [15 *C_AXI_ADDR_WIDTH +: C_AXI_ADDR_WIDTH]                  = S15_AXI_ARADDR    ;
      assign s_axi_arlen     [15 *8 +: 8]                                                = S15_AXI_ARLEN     ;
      assign s_axi_arsize    [15 *3 +: 3]                                                = S15_AXI_ARSIZE    ;
      assign s_axi_arburst   [15 *2 +: 2]                                                = S15_AXI_ARBURST   ;
      assign s_axi_arlock    [15 *2 +: 2]                                                = S15_AXI_ARLOCK    ;
      assign s_axi_arcache   [15 *4 +: 4]                                                = S15_AXI_ARCACHE   ;
      assign s_axi_arprot    [15 *3 +: 3]                                                = S15_AXI_ARPROT    ;
      assign s_axi_arqos     [15 *4 +: 4]                                                = S15_AXI_ARQOS     ;
      assign s_axi_arvalid   [15 +: 1]                                                   = S15_AXI_ARVALID   ;
      assign s_axi_rready    [15 +: 1]                                                   = S15_AXI_RREADY    ;
                                                                                         
      assign S15_AXI_ARESET_OUT_N = s_axi_areset_out_n[15 +: 1]                                                   ;
      assign S15_AXI_AWREADY   = s_axi_awready   [15 +: 1]                                                   ;
      assign S15_AXI_WREADY    = s_axi_wready    [15 +: 1]                                                   ;
      assign S15_AXI_BID       = s_axi_bid       [15 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S15_AXI_BRESP     = s_axi_bresp     [15 *2 +: 2]                                                ;
      assign S15_AXI_BVALID    = s_axi_bvalid    [15 +: 1]                                                   ;
      assign S15_AXI_ARREADY   = s_axi_arready   [15 +: 1]                                                   ;
      assign S15_AXI_RID       = s_axi_rid       [15 *P_AXI_ID_WIDTH +: C_THREAD_ID_PORT_WIDTH]              ;
      assign S15_AXI_RDATA     = s_axi_rdata     [15 *P_AXI_DATA_MAX_WIDTH +: C_S15_AXI_DATA_WIDTH]       ; 
      assign S15_AXI_RRESP     = s_axi_rresp     [15 *2 +: 2]                                                ;
      assign S15_AXI_RLAST     = s_axi_rlast     [15 +: 1]                                                   ;
      assign S15_AXI_RVALID    = s_axi_rvalid    [15 +: 1]                                                   ;
    end else begin : gen_no_si15
      assign S15_AXI_ARESET_OUT_N = 0 ;
      assign S15_AXI_AWREADY   = 0 ;
      assign S15_AXI_WREADY    = 0 ;
      assign S15_AXI_BID       = 0 ;
      assign S15_AXI_BRESP     = 0 ;
      assign S15_AXI_BVALID    = 0 ;
      assign S15_AXI_ARREADY   = 0 ;
      assign S15_AXI_RID       = 0 ;
      assign S15_AXI_RDATA     = 0 ;
      assign S15_AXI_RRESP     = 0 ;
      assign S15_AXI_RLAST     = 0 ;
      assign S15_AXI_RVALID    = 0 ;
    end
  endgenerate
  
      assign M00_AXI_AWLOCK         = m_axi_awlock_i[0];
      assign M00_AXI_ARLOCK         = m_axi_arlock_i[0];
      assign M00_AXI_WDATA          = m_axi_wdata[C_M00_AXI_DATA_WIDTH-1:0];
      assign M00_AXI_WSTRB          = m_axi_wstrb[C_M00_AXI_DATA_WIDTH/8-1:0];
      assign m_axi_rdata            = M00_AXI_RDATA;

ict106_axi_interconnect
  #(
    .C_BASEFAMILY                     (C_FAMILY),
    .C_NUM_SLAVE_SLOTS                (C_NUM_SLAVE_PORTS),
    .C_NUM_MASTER_SLOTS               (1),
    .C_AXI_ID_WIDTH                   (P_AXI_ID_WIDTH),
    .C_AXI_ADDR_WIDTH                 (C_AXI_ADDR_WIDTH),
    .C_AXI_DATA_MAX_WIDTH             (P_AXI_DATA_MAX_WIDTH),
    .C_S_AXI_DATA_WIDTH               (P_S_AXI_DATA_WIDTH),
    .C_M_AXI_DATA_WIDTH               (P_M_AXI_DATA_WIDTH),
    .C_INTERCONNECT_DATA_WIDTH        (C_INTERCONNECT_DATA_WIDTH),
    .C_S_AXI_PROTOCOL                 (0),
    .C_M_AXI_PROTOCOL                 (0),
    .C_M_AXI_BASE_ADDR                (P_M_AXI_BASE_ADDR),
    .C_M_AXI_HIGH_ADDR                (P_M_AXI_HIGH_ADDR),
    .C_S_AXI_BASE_ID                  (P_S_AXI_BASE_ID),
    .C_S_AXI_THREAD_ID_WIDTH          (P_S_AXI_THREAD_ID_WIDTH),
    .C_S_AXI_IS_INTERCONNECT          (0),
    .C_S_AXI_ACLK_RATIO               (P_S_AXI_ACLK_RATIO),
    .C_S_AXI_IS_ACLK_ASYNC            (P_S_AXI_IS_ACLK_ASYNC),
    .C_M_AXI_ACLK_RATIO               (P_M_AXI_ACLK_RATIO),
    .C_M_AXI_IS_ACLK_ASYNC            (C_M00_AXI_IS_ACLK_ASYNC),
    .C_INTERCONNECT_ACLK_RATIO        (K),
    .C_S_AXI_SUPPORTS_WRITE           (P_S_AXI_SUPPORTS_WRITE),
    .C_S_AXI_SUPPORTS_READ            (P_S_AXI_SUPPORTS_READ),
    .C_M_AXI_SUPPORTS_WRITE           (16'hFFFF),
    .C_M_AXI_SUPPORTS_READ            (16'hFFFF),
    .C_AXI_SUPPORTS_USER_SIGNALS      (0),
    .C_AXI_CONNECTIVITY               (32'hFFFFFFFF),
    .C_S_AXI_SINGLE_THREAD            (0),
    .C_S_AXI_WRITE_ACCEPTANCE         (P_S_AXI_WRITE_ACCEPTANCE),
    .C_S_AXI_READ_ACCEPTANCE          (P_S_AXI_READ_ACCEPTANCE),
    .C_M_AXI_WRITE_ISSUING            (P_M_AXI_WRITE_ISSUING),
    .C_M_AXI_READ_ISSUING             (P_M_AXI_READ_ISSUING),
    .C_S_AXI_ARB_PRIORITY             (P_S_AXI_ARB_PRIORITY),
    .C_M_AXI_SECURE                   (0),
    .C_S_AXI_WRITE_FIFO_DEPTH         (P_S_AXI_WRITE_FIFO_DEPTH),
    .C_S_AXI_READ_FIFO_DEPTH          (P_S_AXI_READ_FIFO_DEPTH),
    .C_M_AXI_WRITE_FIFO_DEPTH         (C_M00_AXI_WRITE_FIFO_DEPTH),
    .C_M_AXI_READ_FIFO_DEPTH          (C_M00_AXI_READ_FIFO_DEPTH),
    .C_S_AXI_WRITE_FIFO_DELAY         (P_S_AXI_WRITE_FIFO_DELAY),
    .C_S_AXI_READ_FIFO_DELAY          (P_S_AXI_READ_FIFO_DELAY),
    .C_M_AXI_WRITE_FIFO_DELAY         (C_M00_AXI_WRITE_FIFO_DELAY),
    .C_M_AXI_READ_FIFO_DELAY          (C_M00_AXI_READ_FIFO_DELAY),
    .C_S_AXI_AW_REGISTER              (P_S_AXI_REGISTER),
    .C_S_AXI_AR_REGISTER              (P_S_AXI_REGISTER),
    .C_S_AXI_W_REGISTER               (P_S_AXI_REGISTER),
    .C_S_AXI_R_REGISTER               (P_S_AXI_REGISTER),
    .C_S_AXI_B_REGISTER               (P_S_AXI_REGISTER),
    .C_M_AXI_AW_REGISTER              (P_M_AXI_REGISTER),
    .C_M_AXI_AR_REGISTER              (P_M_AXI_REGISTER),
    .C_M_AXI_W_REGISTER               (P_M_AXI_REGISTER),
    .C_M_AXI_R_REGISTER               (P_M_AXI_REGISTER),
    .C_M_AXI_B_REGISTER               (P_M_AXI_REGISTER),
    .C_INTERCONNECT_CONNECTIVITY_MODE (1),
    .C_USE_CTRL_PORT                  (0),
    .C_RANGE_CHECK                    (0),
    .C_DEBUG                          (0)
  ) axi_interconnect_inst (
    .INTERCONNECT_ACLK        (INTERCONNECT_ACLK  ),
    .INTERCONNECT_ARESETN     (INTERCONNECT_ARESETN ),
    .S_AXI_ARESET_OUT_N       (s_axi_areset_out_n ),
    .S_AXI_ACLK               (s_axi_aclk         ),
    .S_AXI_AWID               (s_axi_awid         ),
    .S_AXI_AWADDR             (s_axi_awaddr       ),
    .S_AXI_AWLEN              (s_axi_awlen        ),
    .S_AXI_AWSIZE             (s_axi_awsize       ),
    .S_AXI_AWBURST            (s_axi_awburst      ),
    .S_AXI_AWLOCK             (s_axi_awlock       ),
    .S_AXI_AWCACHE            (s_axi_awcache      ),
    .S_AXI_AWPROT             (s_axi_awprot       ),
    .S_AXI_AWQOS              (s_axi_awqos        ),
    .S_AXI_AWUSER             ({C_NUM_SLAVE_PORTS{1'b0}}),
    .S_AXI_AWVALID            (s_axi_awvalid      ),
    .S_AXI_AWREADY            (s_axi_awready      ),
    .S_AXI_WID                ({(P_AXI_ID_WIDTH*C_NUM_SLAVE_PORTS){1'b0}}),
    .S_AXI_WDATA              (s_axi_wdata        ),
    .S_AXI_WSTRB              (s_axi_wstrb        ),
    .S_AXI_WLAST              (s_axi_wlast        ),
    .S_AXI_WUSER              ({C_NUM_SLAVE_PORTS{1'b0}}),
    .S_AXI_WVALID             (s_axi_wvalid       ),
    .S_AXI_WREADY             (s_axi_wready       ),
    .S_AXI_BID                (s_axi_bid          ),
    .S_AXI_BRESP              (s_axi_bresp        ),
    .S_AXI_BUSER             (),
    .S_AXI_BVALID             (s_axi_bvalid       ),
    .S_AXI_BREADY             (s_axi_bready       ),
    .S_AXI_ARID               (s_axi_arid         ),
    .S_AXI_ARADDR             (s_axi_araddr       ),
    .S_AXI_ARLEN              (s_axi_arlen        ),
    .S_AXI_ARSIZE             (s_axi_arsize       ),
    .S_AXI_ARBURST            (s_axi_arburst      ),
    .S_AXI_ARLOCK             (s_axi_arlock       ),
    .S_AXI_ARCACHE            (s_axi_arcache      ),
    .S_AXI_ARPROT             (s_axi_arprot       ),
    .S_AXI_ARQOS              (s_axi_arqos        ),
    .S_AXI_ARUSER             ({C_NUM_SLAVE_PORTS{1'b0}}),
    .S_AXI_ARVALID            (s_axi_arvalid      ),
    .S_AXI_ARREADY            (s_axi_arready      ),
    .S_AXI_RID                (s_axi_rid          ),
    .S_AXI_RDATA              (s_axi_rdata        ),
    .S_AXI_RRESP              (s_axi_rresp        ),
    .S_AXI_RLAST              (s_axi_rlast        ),
    .S_AXI_RVALID             (s_axi_rvalid       ),
    .S_AXI_RUSER             (),
    .S_AXI_RREADY             (s_axi_rready       ),
    .M_AXI_ARESET_OUT_N       (M00_AXI_ARESET_OUT_N ),
    .M_AXI_ACLK               (M00_AXI_ACLK         ),
    .M_AXI_AWID               (M00_AXI_AWID         ),
    .M_AXI_AWADDR             (M00_AXI_AWADDR       ),
    .M_AXI_AWLEN              (M00_AXI_AWLEN        ),
    .M_AXI_AWSIZE             (M00_AXI_AWSIZE       ),
    .M_AXI_AWBURST            (M00_AXI_AWBURST      ),
    .M_AXI_AWLOCK             (m_axi_awlock_i     ),
    .M_AXI_AWCACHE            (M00_AXI_AWCACHE      ),
    .M_AXI_AWPROT             (M00_AXI_AWPROT       ),
    .M_AXI_AWQOS              (M00_AXI_AWQOS        ),
    .M_AXI_AWREGION           (),
    .M_AXI_AWUSER             (),
    .M_AXI_AWVALID            (M00_AXI_AWVALID      ),
    .M_AXI_AWREADY            (M00_AXI_AWREADY      ),
    .M_AXI_WID                (),
    .M_AXI_WDATA              (m_axi_wdata          ),
    .M_AXI_WSTRB              (m_axi_wstrb          ),
    .M_AXI_WLAST              (M00_AXI_WLAST        ),
    .M_AXI_WUSER             (),
    .M_AXI_WVALID             (M00_AXI_WVALID       ),
    .M_AXI_WREADY             (M00_AXI_WREADY       ),
    .M_AXI_BID                (M00_AXI_BID          ),
    .M_AXI_BRESP              (M00_AXI_BRESP        ),
    .M_AXI_BUSER              (1'b0               ),
    .M_AXI_BVALID             (M00_AXI_BVALID       ),
    .M_AXI_BREADY             (M00_AXI_BREADY       ),
    .M_AXI_ARID               (M00_AXI_ARID         ),
    .M_AXI_ARADDR             (M00_AXI_ARADDR       ),
    .M_AXI_ARLEN              (M00_AXI_ARLEN        ),
    .M_AXI_ARSIZE             (M00_AXI_ARSIZE       ),
    .M_AXI_ARBURST            (M00_AXI_ARBURST      ),
    .M_AXI_ARLOCK             (m_axi_arlock_i     ),
    .M_AXI_ARCACHE            (M00_AXI_ARCACHE      ),
    .M_AXI_ARPROT             (M00_AXI_ARPROT       ),
    .M_AXI_ARQOS              (M00_AXI_ARQOS        ),
    .M_AXI_ARREGION           (),
    .M_AXI_ARUSER             (),
    .M_AXI_ARVALID            (M00_AXI_ARVALID      ),
    .M_AXI_ARREADY            (M00_AXI_ARREADY      ),
    .M_AXI_RID                (M00_AXI_RID          ),
    .M_AXI_RDATA              (m_axi_rdata          ),
    .M_AXI_RRESP              (M00_AXI_RRESP        ),
    .M_AXI_RLAST              (M00_AXI_RLAST        ),
    .M_AXI_RUSER              (1'b0               ),
    .M_AXI_RVALID             (M00_AXI_RVALID       ),
    .M_AXI_RREADY             (M00_AXI_RREADY       ),
    .S_AXI_CTRL_AWADDR        (32'h0              ),
    .S_AXI_CTRL_AWVALID       (1'b0               ),
    .S_AXI_CTRL_WDATA         (32'h0              ),
    .S_AXI_CTRL_WVALID        (1'b0               ),
    .S_AXI_CTRL_BREADY        (1'b0               ),
    .S_AXI_CTRL_ARADDR        (32'h0              ),
    .S_AXI_CTRL_ARVALID       (1'b0               ),
    .S_AXI_CTRL_RREADY        (1'b0               ),
    .S_AXI_CTRL_AWREADY (),
    .S_AXI_CTRL_WREADY (),
    .S_AXI_CTRL_BRESP (),
    .S_AXI_CTRL_BVALID (),
    .S_AXI_CTRL_ARREADY (),
    .S_AXI_CTRL_RDATA (),
    .S_AXI_CTRL_RRESP (),
    .S_AXI_CTRL_RVALID (),
    .IRQ (),
    .INTERCONNECT_ARESET_OUT_N (),
    .DEBUG_AW_TRANS_SEQ (),
    .DEBUG_AW_ARB_GRANT (),
    .DEBUG_AR_TRANS_SEQ (),
    .DEBUG_AR_ARB_GRANT (),
    .DEBUG_AW_TRANS_QUAL (),
    .DEBUG_AW_ACCEPT_CNT (),
    .DEBUG_AW_ACTIVE_THREAD (),
    .DEBUG_AW_ACTIVE_TARGET (),
    .DEBUG_AW_ACTIVE_REGION (),
    .DEBUG_AW_ERROR (),
    .DEBUG_AW_TARGET (),
    .DEBUG_AR_TRANS_QUAL (),
    .DEBUG_AR_ACCEPT_CNT (),
    .DEBUG_AR_ACTIVE_THREAD (),
    .DEBUG_AR_ACTIVE_TARGET (),
    .DEBUG_AR_ACTIVE_REGION (),
    .DEBUG_AR_ERROR (),
    .DEBUG_AR_TARGET (),
    .DEBUG_B_TRANS_SEQ (),
    .DEBUG_R_BEAT_CNT (),
    .DEBUG_R_TRANS_SEQ (),
    .DEBUG_AW_ISSUING_CNT (),
    .DEBUG_AR_ISSUING_CNT (),
    .DEBUG_W_BEAT_CNT (),
    .DEBUG_W_TRANS_SEQ (),
    .DEBUG_BID_TARGET (),
    .DEBUG_BID_ERROR (),
    .DEBUG_RID_TARGET (),
    .DEBUG_RID_ERROR (),
    .DEBUG_SR_SC_ARADDR (),
    .DEBUG_SR_SC_ARADDRCONTROL (),
    .DEBUG_SR_SC_AWADDR (),
    .DEBUG_SR_SC_AWADDRCONTROL (),
    .DEBUG_SR_SC_BRESP (),
    .DEBUG_SR_SC_RDATA (),
    .DEBUG_SR_SC_RDATACONTROL (),
    .DEBUG_SR_SC_WDATA (),
    .DEBUG_SR_SC_WDATACONTROL (),
    .DEBUG_SC_SF_ARADDR (),
    .DEBUG_SC_SF_ARADDRCONTROL (),
    .DEBUG_SC_SF_AWADDR (),
    .DEBUG_SC_SF_AWADDRCONTROL (),
    .DEBUG_SC_SF_BRESP (),
    .DEBUG_SC_SF_RDATA (),
    .DEBUG_SC_SF_RDATACONTROL (),
    .DEBUG_SC_SF_WDATA (),
    .DEBUG_SC_SF_WDATACONTROL (),
    .DEBUG_SF_CB_ARADDR (),
    .DEBUG_SF_CB_ARADDRCONTROL (),
    .DEBUG_SF_CB_AWADDR (),
    .DEBUG_SF_CB_AWADDRCONTROL (),
    .DEBUG_SF_CB_BRESP (),
    .DEBUG_SF_CB_RDATA (),
    .DEBUG_SF_CB_RDATACONTROL (),
    .DEBUG_SF_CB_WDATA (),
    .DEBUG_SF_CB_WDATACONTROL (),
    .DEBUG_CB_MF_ARADDR (),
    .DEBUG_CB_MF_ARADDRCONTROL (),
    .DEBUG_CB_MF_AWADDR (),
    .DEBUG_CB_MF_AWADDRCONTROL (),
    .DEBUG_CB_MF_BRESP (),
    .DEBUG_CB_MF_RDATA (),
    .DEBUG_CB_MF_RDATACONTROL (),
    .DEBUG_CB_MF_WDATA (),
    .DEBUG_CB_MF_WDATACONTROL (),
    .DEBUG_MF_MC_ARADDR (),
    .DEBUG_MF_MC_ARADDRCONTROL (),
    .DEBUG_MF_MC_AWADDR (),
    .DEBUG_MF_MC_AWADDRCONTROL (),
    .DEBUG_MF_MC_BRESP (),
    .DEBUG_MF_MC_RDATA (),
    .DEBUG_MF_MC_RDATACONTROL (),
    .DEBUG_MF_MC_WDATA (),
    .DEBUG_MF_MC_WDATACONTROL (),
    .DEBUG_MC_MP_ARADDR (),
    .DEBUG_MC_MP_ARADDRCONTROL (),
    .DEBUG_MC_MP_AWADDR (),
    .DEBUG_MC_MP_AWADDRCONTROL (),
    .DEBUG_MC_MP_BRESP (),
    .DEBUG_MC_MP_RDATA (),
    .DEBUG_MC_MP_RDATACONTROL (),
    .DEBUG_MC_MP_WDATA (),
    .DEBUG_MC_MP_WDATACONTROL (),
    .DEBUG_MP_MR_ARADDR (),
    .DEBUG_MP_MR_ARADDRCONTROL (),
    .DEBUG_MP_MR_AWADDR (),
    .DEBUG_MP_MR_AWADDRCONTROL (),
    .DEBUG_MP_MR_BRESP (),
    .DEBUG_MP_MR_RDATA (),
    .DEBUG_MP_MR_RDATACONTROL (),
    .DEBUG_MP_MR_WDATA (),
    .DEBUG_MP_MR_WDATACONTROL ()
    );

endmodule

`default_nettype wire
