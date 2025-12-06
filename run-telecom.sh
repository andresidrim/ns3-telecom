#!/usr/bin/env bash
set -e

# Opcional: limpa o CSV anterior antes de começar
# Descomente se quiser começar do zero
rm -f results.csv

echo "=== Variação de potência (intervalo fixo = 500 ms) ==="
./ns3 run "scratch/telecom-project.cc --txPower=5  --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=10 --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=15 --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=20 --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=25 --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=30 --packetIntervalMs=500"

echo
echo "=== Variação de intervalo (potência fixa = 16 dBm) ==="
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=1000"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=750"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=300"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=200"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=100"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=50"

echo
echo "=== Combinação potência × carga ==="
./ns3 run "scratch/telecom-project.cc --txPower=5  --packetIntervalMs=1000"
./ns3 run "scratch/telecom-project.cc --txPower=10 --packetIntervalMs=750"
./ns3 run "scratch/telecom-project.cc --txPower=15 --packetIntervalMs=500"
./ns3 run "scratch/telecom-project.cc --txPower=20 --packetIntervalMs=300"
./ns3 run "scratch/telecom-project.cc --txPower=25 --packetIntervalMs=200"
./ns3 run "scratch/telecom-project.cc --txPower=30 --packetIntervalMs=100"

echo
echo "=== Cenários extremos ==="
./ns3 run "scratch/telecom-project.cc --txPower=1  --packetIntervalMs=50"
./ns3 run "scratch/telecom-project.cc --txPower=30 --packetIntervalMs=2000"
./ns3 run "scratch/telecom-project.cc --txPower=16 --packetIntervalMs=10"

echo
echo "Simulações concluídas. Resultados em results.csv"

