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
            r["simTime_s"] = float(r["simTime_s"])
            r["sentPkts"] = int(r["sentPkts"])
            r["recvPkts"] = int(r["recvPkts"])
            r["pdr"] = float(r["pdr"])
            r["avgDelay_s"] = float(r["avgDelay_s"])
            r["throughput_bps"] = float(r["throughput_bps"])
            results.append(r)
    return results

def plot_param(results, xKey, titlePrefix=""):
    results = sorted(results, key=lambda r: r[xKey])

    x = [r[xKey] for r in results]
    pdr = [r["pdr"] * 100 for r in results]
    delay = [r["avgDelay_s"] for r in results]
    thr = [r["throughput_bps"] / 1e3 for r in results]

    plt.figure()
    plt.plot(x, pdr, marker='o')
    plt.xlabel(xKey)
    plt.ylabel("PDR (%)")
    plt.title(f"{titlePrefix} PDR vs {xKey}")
    plt.grid(True)
    plt.show()

    plt.figure()
    plt.plot(x, delay, marker='o')
    plt.xlabel(xKey)
    plt.ylabel("Delay (s)")
    plt.title(f"{titlePrefix} Delay vs {xKey}")
    plt.grid(True)
    plt.show()

    plt.figure()
    plt.plot(x, thr, marker='o')
    plt.xlabel(xKey)
    plt.ylabel("Throughput (kbps)")
    plt.title(f"{titlePrefix} Throughput vs {xKey}")
    plt.grid(True)
    plt.show()


def main():
    results = load_results()
    print(f"Carregadas {len(results)} linhas de {FILENAME}")

    txPower_results = [r for r in results if r["dataMode"] == "HtMcs7" and r["packetInterval_ms"] == 500]
    plot_param(txPower_results, "txPower_dBm", titlePrefix="[TxPower]")

    dataMode_results = [r for r in results if r["txPower_dBm"] == 16 and r["packetInterval_ms"] == 500]
    plot_param(dataMode_results, "dataMode", titlePrefix="[DataMode]")

    interval_results = [r for r in results if r["txPower_dBm"] == 16 and r["dataMode"] == "HtMcs7"]
    plot_param(interval_results, "packetInterval_ms", titlePrefix="[Intervalo]")


if __name__ == "__main__":
    main()

