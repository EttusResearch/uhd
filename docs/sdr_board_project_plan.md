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

```
┌─────────────────────────────────────────────────────────────────┐
│                        Host x86 System                          │
│  ┌──────────────┐    ┌─────────────────────────────────────┐   │
│  │   OAI-UE     │    │           Custom UHD Driver         │   │
│  │   (gNB/UE)   │◄──►│  - PCIe XDMA Interface              │   │
│  └──────────────┘    │  - DMA Buffer Management            │   │
│  ┌──────────────┐    │  - RFNoC Integration                │   │
│  │    DPDK      │◄──►│  - Multi-channel Support            │   │
│  │  (Ethernet)  │    └───────────────┬─────────────────────┘   │
│  └──────────────┘                    │ PCIe Gen3/4 x8          │
└──────────────────────────────────────┼─────────────────────────┘
                                       │
┌──────────────────────────────────────┼─────────────────────────┐
│              Custom SDR Board        │                          │
│  ┌───────────────────────────────────▼───────────────────────┐ │
│  │                    Xilinx MPSoC                            │ │
│  │  ┌─────────────┐  ┌─────────────────────────────────────┐ │ │
│  │  │ ARM Cortex  │  │          Programmable Logic         │ │ │
│  │  │  A53 (PS)   │  │  ┌─────────┐  ┌─────────────────┐  │ │ │
│  │  │             │  │  │  RFNoC  │  │  PCIe DMA Core  │  │ │ │
│  │  │  - Linux    │  │  │  Core   │  │  (XDMA/QDMA)    │  │ │ │
│  │  │  - MPM      │  │  └────┬────┘  └────────┬────────┘  │ │ │
│  │  └─────────────┘  │       │                │           │ │ │
│  │                   │  ┌────┴────────────────┴────────┐  │ │ │
│  │                   │  │      JESD204B/C Interface    │  │ │ │
│  │                   │  └──────────────┬───────────────┘  │ │ │
│  └───────────────────┼─────────────────┼─────────────────┘ │ │
│                      │                 │                    │ │
│  ┌───────────────────┴─────────────────┴───────────────────┐ │
│  │                  TI AFE (AFE79xx)                        │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │ │
│  │  │ ADC Ch0  │  │ ADC Ch1  │  │ DAC Ch0  │  │ DAC Ch1  │ │ │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘ │ │
│  └───────┼─────────────┼─────────────┼─────────────┼───────┘ │
│          │             │             │             │         │
│  ┌───────┴─────────────┴─────────────┴─────────────┴───────┐ │
│  │                    RF Frontend                           │ │
│  │        RX0        RX1         TX0         TX1            │ │
│  └──────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

# MPSoC Selection

## Recommended Devices

### Option 1: ZU9EG (High Performance)
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
- **PS DDR4**: Up to 2400 MT/s

### Option 3: ZU15EG (Maximum Performance)
- **PL Logic Cells**: 747K
- **Block RAM**: 38.9 Mb
- **GTY Transceivers**: 32
- **DSP Slices**: 3,528

---

# TI AFE Selection

## AFE7950 (Recommended)

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

```
┌─────────────────────────────────────────────────────────────┐
│                    Clock Distribution                        │
│                                                              │
│  ┌──────────────┐                                           │
│  │  10 MHz Ref  │───┐                                       │
│  │  (External)  │   │                                       │
│  └──────────────┘   │   ┌────────────────────────────────┐ │
│                     │   │     Renesas 8A34001            │ │
│  ┌──────────────┐   │   │                                │ │
│  │   GPS/PPS    │───┼──►│  ┌──────┐  ┌──────────────┐   │ │
│  │   Input      │   │   │  │ DPLL │  │ Output Dividers │ │ │
│  └──────────────┘   │   │  └──┬───┘  └──────┬───────┘   │ │
│                     │   │     │             │           │ │
│  ┌──────────────┐   │   │  ┌──┴─────────────┴──┐       │ │
│  │   OCXO       │───┘   │  │    VCO Array      │       │ │
│  │  (Backup)    │       │  │  (Multiple PLLs)  │       │ │
│  └──────────────┘       │  └───────────────────┘       │ │
│                         └────────────┬─────────────────┘ │
│                                      │                    │
│  ┌───────────────────────────────────┼───────────────────┐│
│  │              Output Clocks        │                   ││
│  │  ┌────────┐  ┌────────┐  ┌───────┴┐  ┌────────────┐  ││
│  │  │ SYSREF │  │ JESD   │  │ Sample │  │ Reference  │  ││
│  │  │        │  │ Clock  │  │ Clock  │  │ Clock      │  ││
│  │  └───┬────┘  └───┬────┘  └───┬────┘  └─────┬──────┘  ││
│  │      │           │           │             │          ││
│  │      ▼           ▼           ▼             ▼          ││
│  │    AFE         AFE       MPSoC PL      MPSoC PS       ││
│  └───────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

### Key Features
- 4 independent DPLLs
- Sub-ns jitter performance
- IEEE 1588/SyncE support
- Programmable via I2C/SPI

---

# PCIe Interface Design

## XDMA Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     PCIe Gen3/4 x8                          │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │                  Xilinx XDMA Core                       │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  │ │
│  │  │   H2C DMA   │  │   C2H DMA   │  │  AXI-Lite      │  │ │
│  │  │  (TX Path)  │  │  (RX Path)  │  │  Control       │  │ │
│  │  │             │  │             │  │                │  │ │
│  │  │ Descriptors │  │ Descriptors │  │ Register Map   │  │ │
│  │  │ Ring Buffer │  │ Ring Buffer │  │                │  │ │
│  │  └──────┬──────┘  └──────┬──────┘  └───────┬────────┘  │ │
│  └─────────┼────────────────┼─────────────────┼───────────┘ │
│            │                │                 │              │
│  ┌─────────┴────────────────┴─────────────────┴───────────┐ │
│  │                    AXI Interconnect                     │ │
│  │                                                         │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐  │ │
│  │  │   RFNoC     │  │    DDR4     │  │   Registers    │  │ │
│  │  │   Core      │  │  Controller │  │   (GPIO,CLK)   │  │ │
│  │  └─────────────┘  └─────────────┘  └────────────────┘  │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Performance Targets
- **PCIe Gen3 x8**: 7.88 GB/s theoretical
- **PCIe Gen4 x8**: 15.75 GB/s theoretical
- **Latency Target**: < 10 μs round-trip

---

# Software Architecture

## Custom UHD Driver Stack

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  ┌─────────────────┐  ┌─────────────────┐                   │
│  │     OAI-UE      │  │   GNU Radio     │                   │
│  │  (5G NR Stack)  │  │  Applications   │                   │
│  └────────┬────────┘  └────────┬────────┘                   │
│           └──────────┬─────────┘                             │
│                      │ UHD API                               │
│  ┌───────────────────┴────────────────────────────────────┐ │
│  │              Custom UHD Implementation                  │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │              Device Discovery                     │  │ │
│  │  │         (PCIe Enumeration + Sysfs)               │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │            Radio Control Block                   │  │ │
│  │  │  - Frequency/Gain Control via Register Access    │  │ │
│  │  │  - Sample Rate Configuration                     │  │ │
│  │  │  - Antenna Selection                             │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │              DMA Manager                          │  │ │
│  │  │  - XDMA Character Device Interface               │  │ │
│  │  │  - Zero-copy Buffer Management                   │  │ │
│  │  │  - Scatter-Gather DMA                            │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  └─────────────────────────────────────────────────────────┘ │
│                      │                                       │
│  ┌───────────────────┴────────────────────────────────────┐ │
│  │               Kernel Driver Layer                       │ │
│  │  ┌──────────────────────────────────────────────────┐  │ │
│  │  │            XDMA Kernel Module                     │  │ │
│  │  │  - /dev/xdma0_h2c_0 (Host to Card DMA)           │  │ │
│  │  │  - /dev/xdma0_c2h_0 (Card to Host DMA)           │  │ │
│  │  │  - /dev/xdma0_user  (Register Access)            │  │ │
│  │  └──────────────────────────────────────────────────┘  │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

# FPGA Architecture

## Top-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        MPSoC FPGA (PL)                          │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    PCIe Hard Block                        │   │
│  │                    (Gen3/4 x8)                            │   │
│  └────────────────────────┬─────────────────────────────────┘   │
│                           │ AXI4-Stream                          │
│  ┌────────────────────────┴─────────────────────────────────┐   │
│  │                      XDMA Core                            │   │
│  │              (Xilinx DMA/Bridge Subsystem)                │   │
│  └────────────────────────┬─────────────────────────────────┘   │
│                           │                                      │
│  ┌────────────────────────┴─────────────────────────────────┐   │
│  │                    RFNoC Framework                        │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │   │
│  │  │  Transport  │  │   CHDR      │  │    Control      │   │   │
│  │  │   Adapter   │  │  Crossbar   │  │    Interface    │   │   │
│  │  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘   │   │
│  │         │                │                  │             │   │
│  │  ┌──────┴────────────────┴──────────────────┴──────┐     │   │
│  │  │              RFNoC Blocks                        │     │   │
│  │  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐ │     │   │
│  │  │  │ Radio  │  │  DDC   │  │  DUC   │  │ Replay │ │     │   │
│  │  │  │ Block  │  │ Block  │  │ Block  │  │ Block  │ │     │   │
│  │  │  └───┬────┘  └────────┘  └────────┘  └────────┘ │     │   │
│  │  └──────┼──────────────────────────────────────────┘     │   │
│  └─────────┼────────────────────────────────────────────────┘   │
│            │                                                     │
│  ┌─────────┴────────────────────────────────────────────────┐   │
│  │               JESD204B/C Core (8 Lanes)                   │   │
│  │  ┌─────────────────────┐  ┌─────────────────────────┐    │   │
│  │  │      TX Path        │  │       RX Path           │    │   │
│  │  │  (DAC Interface)    │  │  (ADC Interface)        │    │   │
│  │  └──────────┬──────────┘  └──────────┬──────────────┘    │   │
│  └─────────────┼─────────────────────────┼──────────────────┘   │
│                │                         │                       │
└────────────────┼─────────────────────────┼───────────────────────┘
                 │  SERDES Lanes           │
                 ▼                         ▼
          ┌──────────────────────────────────┐
          │            TI AFE                │
          └──────────────────────────────────┘
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

```
┌─────────────────────────────────────────────────────────────┐
│                    12V Input (from PCIe)                    │
│                            │                                 │
│  ┌─────────────────────────┴─────────────────────────────┐  │
│  │                    TPS650861 PMIC                      │  │
│  │                                                        │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐   │  │
│  │  │  0.85V  │  │  1.8V   │  │  3.3V   │  │  1.2V   │   │  │
│  │  │ VCC_INT │  │ VCC_AUX │  │ VCC_IO  │  │ DDR4    │   │  │
│  │  │  (30A)  │  │  (3A)   │  │  (2A)   │  │  (5A)   │   │  │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘   │  │
│  └───────┼───────────┼───────────┼───────────┼──────────┘  │
│          │           │           │           │              │
│          ▼           ▼           ▼           ▼              │
│       MPSoC       MPSoC      Peripherals    DDR4            │
│       Core        I/O                                       │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  AFE Power Rails                       │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐                │  │
│  │  │  1.0V   │  │  1.8V   │  │  3.3V   │                │  │
│  │  │ AVDD    │  │ DVDD    │  │ IOVDD   │                │  │
│  │  │ (2A)    │  │ (1A)    │  │ (0.5A)  │                │  │
│  │  └─────────┘  └─────────┘  └─────────┘                │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  Total Power Budget: ~75W (PCIe Slot Limit)                 │
└─────────────────────────────────────────────────────────────┘
```

---

# Development Phases

## Phase 1: Proof of Concept (3 months)
- [ ] MPSoC development board evaluation
- [ ] TI AFE evaluation module testing
- [ ] JESD204B/C IP core validation
- [ ] Basic XDMA driver development

## Phase 2: Schematic & PCB (4 months)
- [ ] Schematic capture (Altium/OrCAD)
- [ ] PCB layout (16+ layers)
- [ ] SI/PI simulation
- [ ] BOM optimization

## Phase 3: FPGA Development (5 months)
- [ ] RFNoC port to XDMA
- [ ] JESD204C integration
- [ ] Clock distribution logic
- [ ] Timing closure

## Phase 4: Software Development (4 months)
- [ ] Custom UHD driver (PCIe backend)
- [ ] XDMA kernel module adaptation
- [ ] MPM porting (if needed)
- [ ] OAI-UE integration testing

---

# Risk Assessment

## High Risk
| Risk | Impact | Mitigation |
|------|--------|------------|
| JESD204C timing | System failure | Early validation on eval kit |
| Power integrity | Noise/instability | Extensive SI/PI simulation |
| Thermal management | Performance degradation | Proper heatsink design |

## Medium Risk
| Risk | Impact | Mitigation |
|------|--------|------------|
| UHD driver complexity | Development delay | Modular approach |
| Component availability | Schedule slip | Second source planning |

## Low Risk
| Risk | Impact | Mitigation |
|------|--------|------------|
| Documentation | Knowledge gap | Regular documentation |

---

# Cost Estimate

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

```
┌───────────────────┬─────────────────────┬─────────────────────┐
│    Feature        │       X410          │    Custom Board     │
├───────────────────┼─────────────────────┼─────────────────────┤
│ SoC Type          │ RF-SoC (ZU28DR)     │ MPSoC (ZU9EG)       │
│ ADC/DAC           │ Integrated          │ External (TI AFE)   │
│ Host Interface    │ 2x QSFP28 (100GbE)  │ PCIe Gen3/4 x8      │
│ Latency           │ ~1ms (Network)      │ ~10μs (PCIe)        │
│ Bandwidth         │ 200 Gbps            │ 15.75 GB/s          │
│ PLL               │ LMK04832            │ Renesas 8A34001     │
│ RF Bandwidth      │ 400 MHz             │ 400 MHz             │
│ Channels          │ 4T4R                │ 4T4R (or 2T2R)      │
│ Power             │ ~100W               │ ~75W (PCIe limit)   │
│ Form Factor       │ Standalone          │ PCIe Card           │
│ Price             │ ~$15,000            │ ~$4,400             │
└───────────────────┴─────────────────────┴─────────────────────┘
```

---

# Target Applications

## Primary Use Cases

### 5G NR Development
- OAI gNB/UE testing
- Low-latency requirements
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

```
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│         Custom SDR Board Project                             │
│         X410 Clone with MPSoC Architecture                   │
│                                                              │
│         Host ─── PCIe ─── MPSoC ─── TI AFE ─── RF           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```
