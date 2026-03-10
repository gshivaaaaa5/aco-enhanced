🚁 ACO-FANET: Intelligent Routing for Flying Ad-hoc Networks

<div align="center">

🏆 **Advanced Ant Colony Optimization Protocol for High-Speed Drone Networks**

A cutting-edge NS-3 simulation implementing bio-inspired algorithms for autonomous UAV swarm communication

📖 Documentation • 🚀 Quick Start • 📊 Results • 🤝 Contributing

</div>

---

# ✨ Highlights

### 🎯 Mission Critical Features

* ⚡ Real-time Adaptive Routing
* 🔄 Self-Healing Network Topology
* 📡 3D Spatial Awareness
* 🧬 Bio-Inspired Intelligence
* 📈 Scalable to 200+ Drones

### 🔬 Research Innovation

* 📐 True ACO Mathematics Implementation
* 🌊 Dynamic Pheromone Evaporation
* 🎮 Gauss-Markov 3D Mobility
* 📊 Multi-Metric Performance Analysis
* 🏭 Production-Ready Codebase

---

# 🏗️ Architecture Overview

```
UAV Swarm
   ↓
Forward Ants discover routes
   ↓
Backward Ants reinforce good paths
   ↓
Pheromone table updated
   ↓
Data packets follow strongest pheromone path
```

---

# 📁 Project Structure

```
ACO-FANET/

src/
 └── aco/model/
      ├── aco-routing-protocol.cc
      └── aco-routing-protocol.h

scratch/
 └── aco-test.cc

scripts/
 ├── run_experiments.sh
 ├── plot_fig7.py
 └── plot_pdr.py

results/
 └── simulation outputs and graphs
```

---

# 🚀 Quick Start

## 📋 Prerequisites

* NS-3 (v3.35+)
* GCC / Clang
* Python 3
* matplotlib
* numpy
* pandas

---

## ⚡ Installation

Clone into NS-3 source directory:

```
cd ~/ns-3-dev/src
git clone https://github.com/gshivaaaaa5/aco-enhanced.git aco
```

Build NS-3:

```
cd ~/ns-3-dev
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

Verify installation:

```
./ns3 run "aco-test --help"
```

---

# 🎮 Running Simulations

### Quick Test

```
./ns3 run "aco-test --nNodes=10"
```

### Full Experiment Suite

```
./run_experiments.sh
```

---

# 📊 Generate Graphs

```
python3 plot_fig7.py
python3 plot_fig10.py
python3 plot_path_time.py
python3 plot_pdr.py
```

Graphs generated:

* Congestion vs Nodes
* Reliability vs Ants
* Path Discovery Time
* Packet Delivery Ratio

---

# 🧬 ACO Algorithm Overview

### Pheromone Deposit

Δτ = Q / Delay

Higher pheromone is deposited on **low-delay paths**.

### Dynamic Evaporation

ρ = ρ_base + α × congestion

Congested paths lose pheromone faster, preventing routing through overloaded nodes.

### Pheromone Update

τ_new = (1 − ρ) τ_old + ΣΔτ

Balances **exploration vs exploitation**.

---

# 📊 Performance Metrics

| Metric                | Description             |
| --------------------- | ----------------------- |
| Packet Delivery Ratio | Reliability of routing  |
| Congestion Rate       | Network overload level  |
| Path Discovery Time   | Time to discover routes |
| Throughput            | Network data rate       |

---

# 🎓 Academic Context

This project explores **Ant Colony Optimization for routing in Flying Ad-hoc Networks (FANETs)** using NS-3 simulation.

Applications include:

* Drone swarm communication
* Disaster response networks
* Autonomous vehicle communication
* Industrial UAV coordination

---

# 👨‍💻 Authors

* Merin Babu
* Pranav Krishnan
* Shivadev G

Mar Athanasius College of Engineering

---

# 📄 License

MIT License

---

⭐ Star the repository if it helped you!

