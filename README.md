<p align="center">
  <img src="Files\Images\app_icon.png" alt="PacketForge Logo" width="140"/>
</p>

<h1 align="center">⚡ PacketForge</h1>

<p align="center">
  <em>Your Ultimate Multi-Protocol Communication Workbench</em>
</p>

<p align="center">
  <a href="#-features"><img src="https://img.shields.io/badge/Features-12+-blueviolet?style=for-the-badge" alt="Features"/></a>
  <a href="#-protocols"><img src="https://img.shields.io/badge/Protocols-4-success?style=for-the-badge" alt="Protocols"/></a>
  <a href="https://riteshdpandit.vercel.app"><img src="https://img.shields.io/badge/Author-Ritesh%20D.%20Pandit-ff6b6b?style=for-the-badge" alt="Author"/></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6.x-41CD52?style=flat-square&logo=qt&logoColor=white" alt="Qt 6"/>
  <img src="https://img.shields.io/badge/C++-26-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++26"/>
  <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows&logoColor=white" alt="Windows"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=flat-square" alt="License"/>
</p>

---

<p align="center">
  <b>PacketForge</b> is a sleek, modern desktop utility for testing and debugging <b>Serial</b>, <b>TCP</b>, and <b>UDP</b> communication. Designed for embedded engineers, IoT developers, and protocol testers, it provides a unified cockpit to transmit, receive, and analyze data packets in real-time.
</p>

---

## � Table of Contents

<details open>
<summary><b>Click to expand</b></summary>

- [🔌 Protocols](#-protocols)
- [🎯 Features](#-features)
- [🧩 Modules](#-modules)
- [🖼️ Screenshots](#️-screenshots)
- [🛠️ Installation](#️-installation)
- [📂 Project Structure](#-project-structure)
- [🤝 Contributing](#-contributing)
- [📄 License](#-license)
- [👤 Author](#-author)

</details>

---

## 🔌 Protocols

<table align="center">
  <tr>
    <td align="center"><img src="https://img.icons8.com/fluency/48/serial-tasks.png" width="40"/><br/><b>Serial</b><br/><sub>RS-232/485</sub></td>
    <td align="center"><img src="https://img.icons8.com/fluency/48/cloud-sync.png" width="40"/><br/><b>TCP Client</b><br/><sub>Connect to Server</sub></td>
    <td align="center"><img src="https://img.icons8.com/fluency/48/server.png" width="40"/><br/><b>TCP Server</b><br/><sub>Host Connections</sub></td>
    <td align="center"><img src="https://img.icons8.com/fluency/48/broadcast.png" width="40"/><br/><b>UDP</b><br/><sub>Datagram Mode</sub></td>
  </tr>
</table>

|  Protocol   | Baud/Port Config | Data Bits |    Parity     | Stop Bits | Flow Control |
| :---------: | :--------------: | :-------: | :-----------: | :-------: | :----------: |
| **Serial**  |     300 — 3M     |  5/6/7/8  | None/Odd/Even |  1/1.5/2  |  None/HW/SW  |
| **TCP/UDP** |    IP + Port     |     —     |       —       |     —     |      —       |

---

## 🎯 Features

<details>
<summary><b>🗂️ Tabbed Multi-Session Interface</b></summary>

> Manage **unlimited independent connections** in separate tabs. Each tab maintains its own protocol, settings, and data buffers.

</details>

<details>
<summary><b>⚡ Macro Quick-Send System</b></summary>

> Configure up to **12 macro buttons** for instant packet transmission. Supports:
>
> - Custom label and hex payload
> - Auto-repeat with configurable interval
> - One-click activation

</details>

<details>
<summary><b>🔁 Auto-Send / Repeat</b></summary>

> Continuously transmit packets at intervals as low as **1 millisecond** for stress testing and timing validation.

</details>

<details>
<summary><b>📄 HEX & ASCII Display Modes</b></summary>

> Toggle between:
>
> - `HEX` — View raw byte values (e.g., `A5 4F 00`)
> - `ASCII` — Human-readable text with escape sequences

</details>

<details>
<summary><b>📝 Line Ending Options</b></summary>

> Choose from `None`, `CR`, `LF`, or `CRLF` to match your device's protocol expectations.

</details>

<details>
<summary><b>📁 Send File</b></summary>

> Transmit entire binary files (firmware, hex dumps, scripts) directly over the active connection.

</details>

<details>
<summary><b>📊 RX/TX Byte Counters</b></summary>

> Live counters display total bytes sent and received per session.

</details>

<details>
<summary><b>📜 File Logging</b></summary>

> Automatically log all traffic to a timestamped `.log` file for post-session analysis.

</details>

<details>
<summary><b>🌗 Dark / Light Themes</b></summary>

> Switch between a sleek **dark mode** and a clean **light mode** with one click.

</details>

<details>
<summary><b>📌 Stay on Top</b></summary>

> Pin PacketForge above all windows for constant visibility during debugging.

</details>

---

## 🧩 Modules

PacketForge includes **4 powerful analysis modules** accessible from the sidebar:

<table>
<tr>
<td width="50%">

### 📡 Modbus Client

Full **Modbus RTU** (Serial) and **Modbus TCP** master implementation.

- Read Holding Registers
- Auto-polling at configurable intervals
- Slave ID and address configuration

</td>
<td width="50%">

### 📈 Oscilloscope

Real-time **waveform visualization** of incoming byte values.

- Adjustable timebase
- Pause/Resume
- Grid overlay

</td>
</tr>
<tr>
<td width="50%">

### 📋 Traffic Monitor

Detailed **packet log** with:

- Timestamps (ms precision)
- Direction indicators (TX/RX)
- Export to `.txt` or `.pcap`

</td>
<td width="50%">

### 💡 Byte Visualizer

**LED-style binary display** showing each bit of the last received byte.

- Bit position labels (7→0)
- Green ON / Dark OFF

</td>
</tr>
</table>

---

## �️ Screenshots

> 🚧 _Add your screenshots here!_

```
📷 Main Window     📷 Dark Theme     📷 Modbus Client     📷 Oscilloscope
```

---

## 🛠️ Installation

### Prerequisites

| Requirement  | Version                                                    |
| ------------ | ---------------------------------------------------------- |
| **Qt**       | 6.x (with `serialport`, `network`, `serialbus`, `widgets`) |
| **Compiler** | C++26 compatible (MinGW 13+, MSVC 2022)                    |

### Quick Start

```bash
# 1. Clone
git clone https://github.com/Riteshp2001/PacketForge.git
cd packetforge

# 2. Open in Qt Creator
#    → Open PacketTransmitter.pro from the ROOT directory

# 3. Build & Run
#    → Select Kit (e.g., Desktop Qt 6.x MinGW 64-bit)
#    → Build (Ctrl+B) → Run (Ctrl+R)
```

> 💡 **Tip:** The compiled binary is placed in the `bin/` folder.

---

## 📂 Project Structure

```
📦 packetforge/
├── 📄 PacketTransmitter.pro      # Project file (open this!)
├── 📁 bin/                       # Compiled binary output
├── 📁 build/                     # Build artifacts
├── 📁 Files/                     # Resources (icons, QRC)
├── 📁 scripts/                   # Deployment scripts
└── 📁 src/                       # Source code
    ├── 📁 core/                  # Paths, utilities
    ├── 📁 macros/                # Macro structures
    ├── 📁 modules/
    │   ├── 📁 modbus/            # Modbus RTU/TCP widget
    │   ├── 📁 oscilloscope/      # Waveform plotter
    │   ├── 📁 traffic/           # Packet log viewer
    │   └── 📁 visualizer/        # Bit LED panel
    ├── 📁 network/               # Serial, TCP, UDP handlers
    └── 📁 ui/                    # MainWindow, ConnectionTab UIs
```

---

## 🤝 Contributing

Contributions are welcome! Feel free to:

1. 🍴 Fork the repository
2. 🌿 Create a feature branch (`git checkout -b feature/amazing-feature`)
3. 💾 Commit your changes (`git commit -m 'Add amazing feature'`)
4. 📤 Push to the branch (`git push origin feature/amazing-feature`)
5. 🔃 Open a Pull Request

---

## 📄 License

This project is licensed under the **MIT License**.

```
MIT License © 2026 Ritesh D. Pandit
```

---

## 👤 Author

<p align="center">
  <a href="https://riteshdpandit.vercel.app">
    <img src="https://img.shields.io/badge/🌐_Website-riteshdpandit.vercel.app-4A90D9?style=for-the-badge" alt="Website"/>
  </a>
  <a href="https://github.com/Riteshp2001">
    <img src="https://img.shields.io/badge/GitHub-@Riteshp2001-181717?style=for-the-badge&logo=github" alt="GitHub"/>
  </a>
</p>

<p align="center">
  <b>Ritesh D. Pandit</b><br/>
  <em>Embedded Systems | IoT | Desktop Applications</em>
</p>

---

<p align="center">
  <sub>Built with 💚 using <b>Qt</b> & <b>C++</b></sub>
</p>
