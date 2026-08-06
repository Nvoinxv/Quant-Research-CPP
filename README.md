# Quant Research C++

A C++ based quantitative research framework for backtesting trading strategies, evaluating statistical indicators, and integrated portfolio management. This project is built to explore and validate systematic trading ideas using historical data (Candles/OHLCV) before deploying them in a live or paper trading environment.

> **Status:** Active development (Work in Progress). The backtesting engine has been recently fully integrated with the strategy execution pipeline.

---

## Table of Contents

- [Key Features](#key-features)
- [Project Structure](#project-structure)
- [Module Architecture](#module-architecture)
- [Build & Run Instructions](#build--run-instructions)
- [Workflow](#workflow)
- [Roadmap & Known Issues](#roadmap--known-issues)
- [Contributing](#contributing)
- [License](#license)

---

## Key Features

- **Backtesting Engine** — Complete simulation of order execution, broker simulation, and portfolio tracking using historical data.
- **Statistical/Quant Indicators** — Implementations of core indicators like EWMA, RSI, and Volatility as a basis for trading signals.
- **Modular Trading Strategies** — Examples of systematic strategies (Breakout, EMA Cross, Mean Reversion) that serve as templates for new ideas.
- **Performance Evaluation Metrics** — Calculation of Sharpe Ratio, Maximum Drawdown, Expectancy, and other risk-adjusted metrics.
- **Flexible Data Loader** — Supports reading data from local CSV files and fetching OHLCV data directly from the Binance API.
- **Visualization** — Plotting modules for candlestick charts, equity curves, and indicator overlays.

---

## Project Structure

```text
Quant_Research_C++/
├── headers/                    # Header files (.hpp) — module interfaces and contracts
│   ├── core/                   # Core data structures (candle, dataset, timeseries)
│   ├── data/                   # Data loader interfaces (CSV, Binance)
│   └── indicator/              # Statistical indicator interfaces
├── src/                        # Implementation files (.cpp)
│   ├── backtest/               # Simulation engine, broker, orders, and portfolio
│   ├── data/                   # Data loader implementations
│   ├── indicator/              # Indicator implementations
│   ├── metrics/                # Performance metrics calculations
│   ├── plot/                   # Backtest result visualizations
│   └── strategy/               # Trading strategy implementations
├── main.cpp                    # Application entry point
├── CMakeLists.txt              # CMake build configuration
├── quant_research              # Build output binary
└── README.md
```

---

## Module Architecture

### `headers/core/`
Fundamental data structures used throughout the system:
- `candle.hpp` — Representation of a single Candle/OHLCV data point.
- `dataset.hpp` — Collection of market data (time series).
- `timeseries.hpp` — Generic time series data structure.

### `headers/data/` & `src/data/`
Data acquisition layer:
- `csv_loader.hpp/.cpp` & `csv_reader.hpp/.cpp` — Read historical data from CSV files.
- `binance_loader.hpp/.cpp` — Fetch OHLCV data directly from the Binance API.

### `headers/indicator/` & `src/indicator/`
Statistical indicators used for generating signals:
- `ewma.hpp/.cpp` — Exponentially Weighted Moving Average.
- `rsi.hpp/.cpp` — Relative Strength Index.
- `volatility.hpp/.cpp` — Volatility measurements (historical/realized volatility).
- `choppiness_index.hpp/.cpp` — Market trend and choppiness measurement.

### `src/backtest/`
The core of the backtesting engine:
- `engine.cpp` — Main simulation orchestrator. Handles the complete flow: Candle → Strategy → Signal → Broker → Order → Portfolio.
- `broker.cpp` — Order execution simulation (handles slippage, fees).
- `order.cpp` — Order representation (Market/Limit, Buy/Sell).
- `portofolio.cpp` — Tracking positions, cash balances, and equity.

### `src/metrics/`
Strategy performance evaluation:
- `sharpe.cpp` — Sharpe Ratio.
- `max_drawdown.cpp` — Maximum Drawdown calculation.
- `expectancy.cpp` — Trade expectancy per execution.
- `sortino_ratio.hpp/.cpp` — Risk-adjusted return metric based on downside deviation.

### `src/plot/`
Result visualization:
- `candlestick.cpp` — Candlestick charting.
- `equity_curver.cpp` — Equity curve visualization over time.
- `indicator_plot.cpp` — Overlaying indicators on price charts.

### `src/strategy/`
Ready-to-use trading strategies acting as templates:
- `breakout.cpp` — Breakout strategy.
- `ema_cross.cpp` — Exponential Moving Average crossover strategy (Golden/Death cross).
- `mean_reversion.cpp` — Mean reversion trading strategy.

---

## Build & Run Instructions

The project uses CMake for its build system.

### Prerequisites
- A C++ compiler with C++17 support (e.g., `g++`, `clang++`, or MSVC).
- CMake installed.
- (Optional) `libcurl` for the Binance API loader.

### Building the Project
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Running the Backtester
```bash
./quant_research
```
*(Note: On Windows, the executable may be named `quant_research.exe`)*

---

## Workflow

The conceptual flow of the framework when running a backtest:

1. **Load Data** — Fetch historical data via `csv_reader` or `binance_loader` and build a `Dataset`.
2. **Calculate Indicators** — Apply `EWMA`, `RSI`, `Volatility`, etc., onto the dataset.
3. **Strategy Execution** — The Strategy logic (e.g., `breakout`, `ema_cross`) analyzes the dataset and generates a series of buy/sell `Signal`s based on indicators.
4. **Backtest Engine** — The `Engine` iterates through the dataset candle by candle, matching signals, routing them to the `Broker` for execution, and recording `Order`s into the `Portfolio`.
5. **Evaluate Performance** — Calculate `Sharpe`, `Max Drawdown`, and other risk-adjusted metrics based on the final equity curve.
6. **Visualization** — Display results using candlestick charts, equity curves, and indicator plots.

---

## Roadmap & Known Issues

- [x] **Integrate Backtest Engine Pipeline**: The `Engine` is now fully wired up with `Strategy`, `Broker`, `Order`, and `Portfolio`.
- [x] **Fix Header Typos**: Resolved typo in `max_drawdown.hpp`.
- [x] **Add CMakeLists.txt**: CMake build configuration is now available in the repository.
- [ ] **Missing Headers**: Complete missing `.hpp` interface files for several modules in `src/metrics/`, `src/plot/`, and `src/strategy/` to ensure clean architectural boundaries.
- [ ] **Unit Testing**: Add comprehensive unit tests (e.g., Catch2 or GoogleTest) to validate indicator math, statistics, and edge cases.
- [ ] **Sample Data**: Include a small `sample_data/` directory so users can test the project without connecting to the Binance API.
- [ ] **Documentation**: Document strategy parameters and indicator thresholds directly in the code/wiki.

---

## Contributing

Contributions, feature requests, and bug reports are highly welcome. Please open an issue or submit a pull request. For major architectural changes, please open an issue first to discuss your proposed changes to maintain structural consistency.

---

## License

*(License yet to be determined — Please add a `LICENSE` file according to the project's preference, such as MIT, Apache 2.0, or Proprietary).*