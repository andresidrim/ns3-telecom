import csv
import matplotlib.pyplot as plt

FILENAME = "results.csv"

def load_results():
    results = []
    with open(FILENAME, newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            r["txPower_dBm"] = float(r["txPower_dBm"])
            r["packetInterval_ms"] = float(r["packetInterval_ms"])
            r["sentPkts"] = int(r["sentPkts"])
            r["recvPkts"] = int(r["recvPkts"])
            r["pdr"] = float(r["pdr"])
            r["avgDelay_s"] = float(r["avgDelay_s"])
            r["throughput_bps"] = float(r["throughput_bps"])
            results.append(r)
    return results


def plot_metric(x, y, xlabel, ylabel, title):
    plt.figure()
    plt.plot(x, y, marker='o')
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.grid(True)
    plt.show()


def plot_txpower(results, intervalo_fixo):
    subset = [r for r in results if r["packetInterval_ms"] == intervalo_fixo]
    subset = sorted(subset, key=lambda r: r["txPower_dBm"])

    x = [r["txPower_dBm"] for r in subset]

    plot_metric(x, [r["pdr"] * 100 for r in subset],
                "TxPower (dBm)", "PDR (%)",
                "PDR vs TxPower")

    plot_metric(x, [r["avgDelay_s"] for r in subset],
                "TxPower (dBm)", "Delay (s)",
                "Delay vs TxPower")

    plot_metric(x, [r["throughput_bps"] / 1e3 for r in subset],
                "TxPower (dBm)", "Throughput (kbps)",
                "Throughput vs TxPower")


def plot_interval(results, tx_fixo):
    subset = [r for r in results if r["txPower_dBm"] == tx_fixo]
    subset = sorted(subset, key=lambda r: r["packetInterval_ms"])

    x = [r["packetInterval_ms"] for r in subset]

    plot_metric(x, [r["pdr"] * 100 for r in subset],
                "Intervalo (ms)", "PDR (%)",
                "PDR vs Intervalo")

    plot_metric(x, [r["avgDelay_s"] for r in subset],
                "Intervalo (ms)", "Delay (s)",
                "Delay vs Intervalo")

    plot_metric(x, [r["throughput_bps"] / 1e3 for r in subset],
                "Intervalo (ms)", "Throughput (kbps)",
                "Throughput vs Intervalo")


def main():
    results = load_results()
    print(f"Carregadas {len(results)} linhas de {FILENAME}")

    intervalo_fixo = 500
    tx_fixo = 16

    print("\nGerando gráficos de TxPower...")
    plot_txpower(results, intervalo_fixo)

    print("\nGerando gráficos de Intervalo...")
    plot_interval(results, tx_fixo)


if __name__ == "__main__":
    main()

