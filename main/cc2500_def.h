#pragma once

// Header bits
#define HDR_READ  0x80
#define HDR_BURST 0x40

// Registers
#define REG_IOCFG2   0x00 // GDO2 output pin configuration
#define REG_IOCFG1   0x01 // GDO1 output pin configuration
#define REG_IOCFG0   0x02 // GDO0 output pin configuration
#define REG_FIFOTHR  0x03 // RX FIFO and TX FIFO thresholds
#define REG_SYNC1    0x04 // Sync word, high byte
#define REG_SYNC0    0x05 // Sync word, low byte
#define REG_PKTLEN   0x06 // Packet length
#define REG_PKTCTRL1 0x07 // Packet automation control
#define REG_PKTCTRL0 0x08 // Packet automation control
#define REG_ADDR     0x09 // Device address
#define REG_CHANNR   0x0A // Channel number
#define REG_FSCTRL1  0x0B // Frequency synthesizer control
#define REG_FSCTRL0  0x0C // Frequency synthesizer control
#define REG_FREQ2    0x0D // Frequency control word, high byte
#define REG_FREQ1    0x0E // Frequency control word, middle byte
#define REG_FREQ0    0x0F // Frequency control word, low byte
#define REG_MDMCFG4  0x10 // Modem configuration
#define REG_MDMCFG3  0x11 // Modem configuration
#define REG_MDMCFG2  0x12 // Modem configuration
#define REG_MDMCFG1  0x13 // Modem configuration
#define REG_MDMCFG0  0x14 // Modem configuration
#define REG_DEVIATN  0x15 // Modem deviation setting
#define REG_MCSM2    0x16 // Main Radio Control State Machine configuration
#define REG_MCSM1    0x17 // Main Radio Control State Machine configuration
#define REG_MCSM0    0x18 // Main Radio Control State Machine configuration
#define REG_FOCCFG   0x19 // Frequency Offset Compensation configuration
#define REG_BSCFG    0x1A // Bit Synchronization configuration
#define REG_AGCTRL2  0x1B // AGC control
#define REG_AGCTRL1  0x1C // AGC control
#define REG_AGCTRL0  0x1D // AGC control
#define REG_WOREVT1  0x1E // High byte Event 0 timeout
#define REG_WOREVT0  0x1F // Low byte Event 0 timeout
#define REG_WORCTRL  0x20 // Wake On Radio control
#define REG_FREND1   0x21 // Front end RX configuration
#define REG_FREND0   0x22 // Front end TX configuration
#define REG_FSCAL3   0x23 // Frequency synthesizer calibration
#define REG_FSCAL2   0x24 // Frequency synthesizer calibration
#define REG_FSCAL1   0x25 // Frequency synthesizer calibration
#define REG_FSCAL0   0x26 // Frequency synthesizer calibration
#define REG_RCCTRL1  0x27 // RC oscillator configuration
#define REG_RCCTRL0  0x28 // RC oscillator configuration
#define REG_FSTEST   0x29 // Frequency synthesizer calibration control
#define REG_PTEST    0x2A // Production test
#define REG_AGCTEST  0x2B // AGC test
#define REG_TEST2    0x2C // Various test settings
#define REG_TEST1    0x2D // Various test settings
#define REG_TEST0    0x2E // Various test settings

// Multi-byte registers
#define REG_PATABLE		0x3E	// Power output table
#define REG_FIFO		0x3F	// Transmit / Receive FIFO

// Status registers
#define STATUS_PARTNUM        0x30 // CC2500 part number
#define STATUS_VERSION        0x31 // Current version number
#define STATUS_FREQEST        0x32 // Frequency offset estimate
#define STATUS_LQI            0x33 // Demodulator estimate for Link Quality
#define STATUS_RSSI           0x34 // Received signal strength indication
#define STATUS_MARCSTATE      0x35 // Control state machine state
#define STATUS_WORTIME1       0x36 // High byte of WOR timer
#define STATUS_WORTIME0       0x37 // Low byte of WOR timer
#define STATUS_PKTSTATUS      0x38 // Current GDOx status and packet status
#define STATUS_VCO_VC_DAC     0x39 // Current setting from PLL calibration module
#define STATUS_TXBYTES        0x3A // Underflow and number of bytes in the TX FIFO
#define STATUS_RXBYTES        0x3B // Overflow and number of bytes in the RX FIFO
#define STATUS_RCCTRL1_STATUS 0x3C // Last RC oscillator calibration result
#define STATUS_RCCTRL0_STATUS 0x3D // Last RC oscillator calibration result

// Command strobes
#define CMD_SRES    0x30 // Reset chip.
#define CMD_SFSTXON 0x31 // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1). If in RX (with CCA): Go to a wait state where only the synthesizer is running (for quick RX / TX turnaround).
#define CMD_SXOFF   0x32 // Turn off crystal oscillator.
#define CMD_SCAL    0x33 // Calibrate frequency synthesizer and turn it off. SCAL can be strobed from IDLE mode without setting manual calibration mode (MCSM0.FS_AUTOCAL=0)
#define CMD_SRX     0x34 // Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1.
#define CMD_STX     0x35 // In IDLE state: Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled: Only go to TX if channel is clear.
#define CMD_SIDLE   0x36 // Exit RX / TX, turn off frequency synthesizer and exit Wake-On-Radio mode if applicable.
#define CMD_SWOR    0x38 // Start automatic RX polling sequence (Wake-on-Radio) as described in Section 19.5 if WORCTRL.RC_PD=0.
#define CMD_SPWD    0x39 // Enter power down mode when CSn goes high.
#define CMD_SFRX    0x3A // Flush the RX FIFO buffer. Only issue SFRX in IDLE or RXFIFO_OVERFLOW states.
#define CMD_SFTX    0x3B // Flush the TX FIFO buffer. Only issue SFTX in IDLE or TXFIFO_UNDERFLOW states.
#define CMD_SWORRST 0x3C // Reset real time clock to Event1 value.
#define CMD_SNOP    0x3D // No operation. May be used to get access to the chip status byte

// Possible values for the state bits in the Chip Status Byte
#define STATE_IDLE             0x0 // Idle state (Also reported for some transitional states instead of SETTLING or CALIBRATE)
#define STATE_RX               0x1 // Receive mode
#define STATE_TX               0x2 // Transmit mode
#define STATE_FSTXON           0x3 // Frequency synthesizer is on, ready to start transmitting
#define STATE_CALIBRATE        0x4 // Frequency synthesizer calibration is running
#define STATE_SETTLING         0x5 // PLL is settling
#define STATE_RXFIFO_OVERFLOW  0x6 // RX FIFO has overflowed. Read out any useful data, then flush the FIFO with SFRX
#define STATE_TXFIFO_UNDERFLOW 0x7 // TX FIFO has underflowed. Acknowledge with SFTX

#define GET_STATE(status) ((status >> 4) & 0x07)