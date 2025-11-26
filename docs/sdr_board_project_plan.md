# Custom SDR Board Project Plan
## X410 Clone with MPSoC Architecture

---

# Project Overview

## Goal
X410 기반 Custom SDR 보드 개발

## Key Differences from X410
| Feature | X410 | Custom Board |
|---------|------|--------------|
| SoC | RF-SoC (ZU28DR) | MPSoC |
| ADC/DAC | Integrated | External (TI AFE) |
| Host Interface | Ethernet | PCIe |
| PLL | LMK04832 | Renesas 8A34001 |

---

# System Architecture

```mermaid
flowchart TB
    subgraph HOST["Host x86 System"]
        OAI["OAI-UE<br/>(gNB/UE)"]
        DPDK["DPDK<br/>(Ethernet)"]
        subgraph UHD["Custom UHD Driver"]
            XDMA_IF["PCIe XDMA Interface"]
            DMA_BUF["DMA Buffer Management"]
            RFNOC_INT["RFNoC Integration"]
        end
        OAI <--> UHD
        DPDK <--> UHD
    end

    subgraph SDR["Custom SDR Board"]
        subgraph MPSOC["Xilinx MPSoC"]
            subgraph PS["ARM Cortex A53 (PS)"]
                LINUX["Linux"]
                MPM["MPM"]
            end
            subgraph PL["Programmable Logic"]
                RFNOC["RFNoC Core"]
                PCIE_DMA["PCIe DMA Core<br/>(XDMA/QDMA)"]
                JESD["JESD204B/C<br/>Interface"]
            end
        end

        subgraph AFE["TI AFE (AFE79xx)"]
            ADC0["ADC Ch0"]
            ADC1["ADC Ch1"]
            DAC0["DAC Ch0"]
            DAC1["DAC Ch1"]
        end

        subgraph RF["RF Frontend"]
            RX0["RX0"]
            RX1["RX1"]
            TX0["TX0"]
            TX1["TX1"]
        end
    end

    UHD <-->|"PCIe Gen3/4 x8"| PCIE_DMA
    PCIE_DMA <--> RFNOC
    RFNOC <--> JESD
    JESD <--> AFE
    ADC0 <--> RX0
    ADC1 <--> RX1
    DAC0 <--> TX0
    DAC1 <--> TX1
```

---

# MPSoC Selection

## Recommended Devices

```mermaid
graph LR
    subgraph Options["MPSoC Options"]
        ZU7["ZU7EV<br/>Balanced"]
        ZU9["ZU9EG<br/>High Performance"]
        ZU15["ZU15EG<br/>Maximum"]
    end

    ZU7 --> |"504K LUTs<br/>16 GTY"| SPEC1["Good for 2T2R"]
    ZU9 --> |"599K LUTs<br/>24 GTY"| SPEC2["Recommended for 4T4R"]
    ZU15 --> |"747K LUTs<br/>32 GTY"| SPEC3["Future Expansion"]
```

### Option 1: ZU9EG (High Performance) - Recommended
- **PL Logic Cells**: 599K
- **Block RAM**: 32.1 Mb
- **GTY Transceivers**: 24
- **DSP Slices**: 2,520
- **PS DDR4**: Up to 2400 MT/s

### Option 2: ZU7EV (Balanced)
- **PL Logic Cells**: 504K
- **Block RAM**: 26.1 Mb
- **GTY Transceivers**: 16
- **DSP Slices**: 1,728

### Option 3: ZU15EG (Maximum Performance)
- **PL Logic Cells**: 747K
- **Block RAM**: 38.9 Mb
- **GTY Transceivers**: 32
- **DSP Slices**: 3,528

---

# TI AFE Selection

## AFE7950 (Recommended)

```mermaid
flowchart LR
    subgraph AFE7950["AFE7950 Block Diagram"]
        direction TB
        subgraph RX["RX Path"]
            RF_RX["RF Input<br/>1-8 GHz"] --> LNA["LNA"]
            LNA --> MIX_RX["Mixer"]
            MIX_RX --> ADC["ADC<br/>3 GSPS"]
        end

        subgraph TX["TX Path"]
            DAC["DAC<br/>9 GSPS"] --> MIX_TX["Mixer"]
            MIX_TX --> PA["Driver"]
            PA --> RF_TX["RF Output"]
        end

        subgraph DSP["On-chip DSP"]
            NCO["NCO"]
            DDC["DDC"]
            DUC["DUC"]
        end

        ADC --> DDC
        DUC --> DAC
        NCO --> MIX_RX
        NCO --> MIX_TX
    end

    JESD["JESD204C<br/>Interface"] <--> DSP
```

### Specifications
| Parameter | Value |
|-----------|-------|
| ADC Channels | 4 |
| DAC Channels | 4 |
| ADC Sample Rate | 3 GSPS |
| DAC Sample Rate | 9 GSPS |
| RF Bandwidth | 400 MHz |
| Interface | JESD204C |

### Features
- Direct RF sampling up to 8 GHz
- Integrated NCO per channel
- On-chip DSP (DDC/DUC)
- Internal LO synthesis

## Alternatives
- **AFE7903**: 2T2R, lower cost
- **AFE7920**: 4T4R, higher integration

---

# Clock Architecture

## Renesas 8A34001 Network Synchronizer

```mermaid
flowchart TB
    subgraph INPUTS["Clock Inputs"]
        EXT["10 MHz External<br/>Reference"]
        GPS["GPS/PPS<br/>Input"]
        OCXO["OCXO<br/>(Backup)"]
    end

    subgraph PLL["Renesas 8A34001"]
        direction TB
        DPLL["DPLL<br/>(4 Independent)"]
        VCO["VCO Array"]
        DIV["Output Dividers"]

        DPLL --> VCO
        VCO --> DIV
    end

    subgraph OUTPUTS["Output Clocks"]
        SYSREF["SYSREF<br/>(AFE Sync)"]
        JESD_CLK["JESD Clock<br/>(Lane Clock)"]
        SAMPLE["Sample Clock<br/>(Converter)"]
        REF["Reference Clock<br/>(System)"]
    end

    EXT --> DPLL
    GPS --> DPLL
    OCXO --> DPLL

    DIV --> SYSREF
    DIV --> JESD_CLK
    DIV --> SAMPLE
    DIV --> REF

    SYSREF --> AFE_DEST["TI AFE"]
    JESD_CLK --> AFE_DEST
    SAMPLE --> MPSOC_PL["MPSoC PL"]
    REF --> MPSOC_PS["MPSoC PS"]
```

### Key Features
- 4 independent DPLLs
- Sub-ns jitter performance (<100 fs RMS)
- IEEE 1588/SyncE support
- Programmable via I2C/SPI

---

# PCIe Interface Design

## XDMA Architecture

```mermaid
flowchart TB
    subgraph HOST["Host System"]
        CPU["x86 CPU"]
        MEM["System Memory"]
        CPU <--> MEM
    end

    subgraph PCIE["PCIe Gen3/4 x8"]
        PCIE_IF["PCIe Interface<br/>7.88-15.75 GB/s"]
    end

    subgraph XDMA["Xilinx XDMA Core"]
        direction TB
        H2C["H2C DMA<br/>(TX Path)"]
        C2H["C2H DMA<br/>(RX Path)"]
        AXI_LITE["AXI-Lite<br/>Control"]

        subgraph DESC["Descriptor Engine"]
            H2C_DESC["H2C Descriptors<br/>Ring Buffer"]
            C2H_DESC["C2H Descriptors<br/>Ring Buffer"]
        end
    end

    subgraph AXI["AXI Interconnect"]
        RFNOC_CORE["RFNoC Core"]
        DDR4["DDR4 Controller"]
        REGS["Registers<br/>(GPIO, CLK)"]
    end

    CPU <--> PCIE_IF
    PCIE_IF <--> H2C
    PCIE_IF <--> C2H
    PCIE_IF <--> AXI_LITE

    H2C --> H2C_DESC
    C2H --> C2H_DESC

    H2C --> RFNOC_CORE
    C2H --> RFNOC_CORE
    AXI_LITE --> DDR4
    AXI_LITE --> REGS
```

## Performance Targets
| Metric | Gen3 x8 | Gen4 x8 |
|--------|---------|---------|
| Theoretical BW | 7.88 GB/s | 15.75 GB/s |
| Effective BW | ~6.5 GB/s | ~13 GB/s |
| Latency | < 10 μs | < 5 μs |

---

# Software Architecture

## Custom UHD Driver Stack

```mermaid
flowchart TB
    subgraph APP["Application Layer"]
        OAI["OAI-UE<br/>(5G NR Stack)"]
        GR["GNU Radio<br/>Applications"]
    end

    subgraph UHD["Custom UHD Implementation"]
        direction TB
        DISC["Device Discovery<br/>(PCIe Enumeration + Sysfs)"]

        subgraph RADIO["Radio Control Block"]
            FREQ["Frequency Control"]
            GAIN["Gain Control"]
            RATE["Sample Rate Config"]
            ANT["Antenna Selection"]
        end

        subgraph DMA["DMA Manager"]
            XDMA_DEV["XDMA Char Device"]
            ZERO["Zero-copy Buffers"]
            SG["Scatter-Gather DMA"]
        end
    end

    subgraph KERNEL["Kernel Driver Layer"]
        subgraph XDMA_MOD["XDMA Kernel Module"]
            H2C_DEV["/dev/xdma0_h2c_0<br/>(Host to Card)"]
            C2H_DEV["/dev/xdma0_c2h_0<br/>(Card to Host)"]
            USER_DEV["/dev/xdma0_user<br/>(Registers)"]
        end
    end

    subgraph HW["Hardware"]
        FPGA["FPGA<br/>(XDMA + RFNoC)"]
    end

    OAI --> UHD
    GR --> UHD
    UHD --> KERNEL
    H2C_DEV --> FPGA
    C2H_DEV --> FPGA
    USER_DEV --> FPGA
```

---

# FPGA Architecture

## Top-Level Block Diagram

```mermaid
flowchart TB
    subgraph PL["MPSoC FPGA (PL)"]
        PCIE_HARD["PCIe Hard Block<br/>(Gen3/4 x8)"]

        subgraph XDMA["XDMA Core"]
            DMA_ENGINE["DMA/Bridge<br/>Subsystem"]
        end

        subgraph RFNOC["RFNoC Framework"]
            TRANSPORT["Transport<br/>Adapter"]
            CHDR["CHDR<br/>Crossbar"]
            CTRL["Control<br/>Interface"]

            subgraph BLOCKS["RFNoC Blocks"]
                RADIO["Radio<br/>Block"]
                DDC["DDC<br/>Block"]
                DUC["DUC<br/>Block"]
                REPLAY["Replay<br/>Block"]
            end
        end

        subgraph JESD["JESD204B/C Core"]
            TX_PATH["TX Path<br/>(DAC Interface)"]
            RX_PATH["RX Path<br/>(ADC Interface)"]
        end

        DDR4_CTRL["DDR4<br/>Controller"]
    end

    subgraph AFE["TI AFE"]
        DAC_IF["DAC"]
        ADC_IF["ADC"]
    end

    PCIE_HARD <-->|"AXI4-Stream"| DMA_ENGINE
    DMA_ENGINE <--> TRANSPORT
    TRANSPORT <--> CHDR
    CHDR <--> CTRL
    CHDR <--> BLOCKS

    RADIO <--> TX_PATH
    RADIO <--> RX_PATH
    REPLAY <--> DDR4_CTRL

    TX_PATH <-->|"SERDES<br/>Lanes"| DAC_IF
    RX_PATH <-->|"SERDES<br/>Lanes"| ADC_IF
```

---

# Bill of Materials (BOM) - Key Components

## Main Components

| Component | Part Number | Qty | Description |
|-----------|-------------|-----|-------------|
| MPSoC | XCZU9EG-2FFVB1156I | 1 | Zynq UltraScale+ |
| AFE | AFE7950 | 1 | RF Transceiver |
| PLL | 8A34001 | 1 | Clock Generator |
| DDR4 (PS) | MT40A512M16TB | 2 | 16Gb DDR4 SDRAM |
| DDR4 (PL) | MT40A256M16GE | 4 | 8Gb DDR4 SDRAM |
| Flash | MT25QU256ABA | 1 | 256Mb QSPI |
| eMMC | MTFC8GAKAJCN | 1 | 8GB eMMC |
| PCIe Conn | 10018783-10111TLF | 1 | PCIe x8 Edge |
| Oscillator | SIT5356AI | 1 | 100MHz TCXO |
| Power | TPS650861 | 1 | PMIC |

---

# Power Architecture

## Power Rails

```mermaid
flowchart TB
    subgraph INPUT["Power Input"]
        PCIe_12V["12V from PCIe<br/>(75W Max)"]
    end

    subgraph PMIC["TPS650861 PMIC"]
        direction LR
        VCC_INT["0.85V<br/>VCC_INT<br/>(30A)"]
        VCC_AUX["1.8V<br/>VCC_AUX<br/>(3A)"]
        VCC_IO["3.3V<br/>VCC_IO<br/>(2A)"]
        DDR4_V["1.2V<br/>DDR4<br/>(5A)"]
    end

    subgraph MPSOC_PWR["MPSoC Power"]
        CORE["Core"]
        IO["I/O"]
        DDR["DDR4 Memory"]
    end

    subgraph AFE_PWR["AFE Power Rails"]
        AVDD["1.0V AVDD<br/>(2A)"]
        DVDD["1.8V DVDD<br/>(1A)"]
        IOVDD["3.3V IOVDD<br/>(0.5A)"]
    end

    PCIe_12V --> PMIC
    VCC_INT --> CORE
    VCC_AUX --> IO
    VCC_IO --> PERIPH["Peripherals"]
    DDR4_V --> DDR

    PCIe_12V --> AFE_PWR
```

### Power Budget
| Rail | Current | Power |
|------|---------|-------|
| MPSoC Core (0.85V) | 30A | 25.5W |
| MPSoC Aux (1.8V) | 3A | 5.4W |
| DDR4 (1.2V) | 5A | 6W |
| AFE Total | - | 15W |
| **Total** | - | **~55W** |

---

# Development Phases

## Project Timeline

```mermaid
gantt
    title SDR Board Development Timeline
    dateFormat  YYYY-MM

    section Phase 1: PoC
    MPSoC Dev Board Eval     :p1a, 2024-01, 1M
    TI AFE EVM Testing       :p1b, 2024-01, 2M
    JESD204C IP Validation   :p1c, 2024-02, 2M
    Basic XDMA Driver        :p1d, 2024-02, 2M

    section Phase 2: PCB
    Schematic Capture        :p2a, 2024-04, 2M
    PCB Layout (16L)         :p2b, 2024-05, 2M
    SI/PI Simulation         :p2c, 2024-06, 1M
    Prototype Fabrication    :p2d, 2024-07, 1M

    section Phase 3: FPGA
    RFNoC Port to XDMA       :p3a, 2024-08, 2M
    JESD204C Integration     :p3b, 2024-09, 2M
    Clock Distribution       :p3c, 2024-10, 1M
    Timing Closure           :p3d, 2024-11, 2M

    section Phase 4: Software
    Custom UHD Driver        :p4a, 2025-01, 2M
    XDMA Kernel Adaptation   :p4b, 2025-01, 1M
    OAI-UE Integration       :p4c, 2025-02, 2M
    System Testing           :p4d, 2025-03, 1M
```

## Phase Details

### Phase 1: Proof of Concept
- [ ] MPSoC development board evaluation
- [ ] TI AFE evaluation module testing
- [ ] JESD204B/C IP core validation
- [ ] Basic XDMA driver development

### Phase 2: Schematic & PCB
- [ ] Schematic capture (Altium/OrCAD)
- [ ] PCB layout (16+ layers)
- [ ] SI/PI simulation
- [ ] BOM optimization

### Phase 3: FPGA Development
- [ ] RFNoC port to XDMA
- [ ] JESD204C integration
- [ ] Clock distribution logic
- [ ] Timing closure

### Phase 4: Software Development
- [ ] Custom UHD driver (PCIe backend)
- [ ] XDMA kernel module adaptation
- [ ] MPM porting (if needed)
- [ ] OAI-UE integration testing

---

# Risk Assessment

## Risk Matrix

```mermaid
quadrantChart
    title Risk Assessment Matrix
    x-axis Low Impact --> High Impact
    y-axis Low Probability --> High Probability
    quadrant-1 Monitor
    quadrant-2 High Priority
    quadrant-3 Low Priority
    quadrant-4 Medium Priority

    JESD204C Timing: [0.8, 0.7]
    Power Integrity: [0.75, 0.5]
    Thermal: [0.6, 0.6]
    Driver Complexity: [0.5, 0.4]
    Component Supply: [0.4, 0.5]
    Documentation: [0.2, 0.3]
```

## Risk Details

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| JESD204C timing | High | High | Early validation on eval kit |
| Power integrity | High | Medium | Extensive SI/PI simulation |
| Thermal management | Medium | Medium | Proper heatsink design |
| UHD driver complexity | Medium | Medium | Modular approach |
| Component availability | Medium | Medium | Second source planning |
| Documentation gaps | Low | Low | Regular documentation |

---

# Cost Estimate

## Cost Breakdown

```mermaid
pie showData
    title Development Cost Distribution
    "Engineering Labor" : 200000
    "PCB Prototype" : 15000
    "MPSoC Dev Kit" : 5000
    "Vivado License" : 3500
    "TI AFE EVM" : 3000
    "Other" : 3500
```

## Development Costs (One-time)

| Category | Cost (USD) |
|----------|------------|
| MPSoC Dev Kit | $5,000 |
| TI AFE EVM | $3,000 |
| Vivado License | $3,500/yr |
| PCB Prototype (5 units) | $15,000 |
| Engineering Labor | $200,000 |
| **Total Development** | **~$230,000** |

## Unit Cost (Production)

```mermaid
pie showData
    title Unit Cost Breakdown ($4,400)
    "MPSoC (ZU9EG)" : 2500
    "TI AFE" : 800
    "PCB + Assembly" : 500
    "Other Components" : 400
    "DDR4 Memory" : 200
```

| Component | Cost (USD) |
|-----------|------------|
| MPSoC (ZU9EG) | $2,500 |
| TI AFE | $800 |
| DDR4 Memory | $200 |
| PCB + Assembly | $500 |
| Other Components | $400 |
| **Total Unit Cost** | **~$4,400** |

---

# Comparison: X410 vs Custom Board

```mermaid
flowchart LR
    subgraph X410["X410 (Ettus)"]
        direction TB
        X_SOC["RF-SoC<br/>ZU28DR"]
        X_IF["2x QSFP28<br/>100GbE"]
        X_ADC["Integrated<br/>ADC/DAC"]
        X_PLL["LMK04832"]
        X_PWR["~100W"]
        X_PRICE["~$15,000"]
    end

    subgraph CUSTOM["Custom Board"]
        direction TB
        C_SOC["MPSoC<br/>ZU9EG"]
        C_IF["PCIe<br/>Gen3/4 x8"]
        C_ADC["External<br/>TI AFE"]
        C_PLL["Renesas<br/>8A34001"]
        C_PWR["~75W"]
        C_PRICE["~$4,400"]
    end

    X410 ---|"vs"| CUSTOM
```

## Detailed Comparison

| Feature | X410 | Custom Board |
|---------|------|--------------|
| SoC Type | RF-SoC (ZU28DR) | MPSoC (ZU9EG) |
| ADC/DAC | Integrated | External (TI AFE) |
| Host Interface | 2x QSFP28 (100GbE) | PCIe Gen3/4 x8 |
| Latency | ~1ms (Network) | **~10μs (PCIe)** |
| Bandwidth | 200 Gbps | 15.75 GB/s |
| PLL | LMK04832 | Renesas 8A34001 |
| RF Bandwidth | 400 MHz | 400 MHz |
| Channels | 4T4R | 4T4R (or 2T2R) |
| Power | ~100W | ~75W (PCIe limit) |
| Form Factor | Standalone | PCIe Card |
| Price | ~$15,000 | **~$4,400** |

---

# Target Applications

## Use Case Diagram

```mermaid
flowchart TB
    SDR["Custom SDR Board"]

    subgraph 5G["5G NR Development"]
        OAI_GNB["OAI gNB"]
        OAI_UE["OAI UE"]
        DPDK_APP["DPDK Apps"]
    end

    subgraph RESEARCH["Research & Academia"]
        PHY["Custom PHY"]
        MIMO["MIMO Experiments"]
        SPECTRUM["Spectrum Sensing"]
    end

    subgraph PRIVATE["Private Networks"]
        IOT["Industrial IoT"]
        FACTORY["Smart Factory"]
        CAMPUS["Campus Networks"]
    end

    SDR --> 5G
    SDR --> RESEARCH
    SDR --> PRIVATE
```

## Primary Use Cases

### 5G NR Development
- OAI gNB/UE testing
- Low-latency requirements (<10μs)
- DPDK integration for packet processing

### Research & Academia
- Custom PHY algorithms
- MIMO experiments
- Spectrum sensing

### Private Networks
- Industrial IoT
- Smart factory
- Campus networks

---

# Next Steps

## Action Items

```mermaid
flowchart LR
    subgraph PROCUREMENT["1. Procurement"]
        P1["Order ZCU106/ZCU111"]
        P2["Order AFE79xx EVM"]
        P3["Get 8A34001 Samples"]
    end

    subgraph VALIDATION["2. Technical Validation"]
        V1["JESD204C Link Bring-up"]
        V2["XDMA Loopback Test"]
        V3["Clock Jitter Measurement"]
    end

    subgraph DESIGN["3. Design Kickoff"]
        D1["Create Git Repository"]
        D2["Define Register Map"]
        D3["Start Schematic"]
    end

    PROCUREMENT --> VALIDATION
    VALIDATION --> DESIGN
```

## Immediate Actions

1. **Procurement**
   - Order MPSoC evaluation kit (ZCU106/ZCU111)
   - Order TI AFE79xx EVM
   - Obtain Renesas 8A34001 samples

2. **Technical Validation**
   - JESD204C link bring-up
   - XDMA loopback testing
   - Clock jitter measurement

3. **Design Kickoff**
   - Create project repository
   - Define register map
   - Start schematic design

---

# Contact & Resources

## Project Resources
- **UHD Repository**: github.com/EttusResearch/uhd
- **X410 Documentation**: files.ettus.com/manual
- **TI AFE Resources**: ti.com/product/AFE7950

## References
- X410 Hardware Manual
- JESD204B/C Specification
- Xilinx XDMA Product Guide (PG195)
- Renesas 8A34001 Datasheet

---

# Thank You

## Questions?

```mermaid
flowchart LR
    HOST["Host x86"]
    PCIe["PCIe"]
    MPSOC["MPSoC"]
    AFE["TI AFE"]
    RF["RF"]

    HOST <-->|"Custom UHD"| PCIe
    PCIe <-->|"XDMA"| MPSOC
    MPSOC <-->|"JESD204C"| AFE
    AFE <-->|"Analog"| RF
```

### Custom SDR Board Project
### X410 Clone with MPSoC Architecture
