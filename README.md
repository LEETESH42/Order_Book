# Order Book Implementation in C++

This project implements a **limit order book** for simulating trading scenarios, where buy and sell orders are matched based on price and time priority. It includes functionalities for order addition, cancellation, matching, and execution, supporting different order types like **Good-Till-Cancel** and **Fill-and-Kill**. 

## Features
- **Order Matching**: Automatically matches buy and sell orders based on the best available prices.
- **Order Types**:
  - *Good-Till-Cancel*: Orders remain in the book until explicitly canceled or matched.
  - *Fill-and-Kill*: Orders are matched immediately; any unfilled portion is canceled.
- **Trade Execution**: Tracks trade details, including order IDs, prices, and quantities.
- **Partial Fills**: Supports scenarios where orders are partially filled.
- **Order Cancellation**: Allows for the cancellation of orders by ID.
- **Order Book Levels**: Provides a summary of bid and ask levels with aggregated quantities.

## Code Highlights
- **Classes**:
  - `Order`: Represents an individual buy or sell order.
  - `OrderBook`: Manages the order book, including order matching and level aggregation.
  - `Trade`: Captures details of executed trades.
  - `OrderModify`: Handles modifications of existing orders.
- **Efficient Data Structures**:
  - `std::map`: Used for storing orders with price-based sorting (bids in descending order, asks in ascending order).
  - `std::unordered_map`: For fast lookups of orders by ID.
- **Error Handling**: Ensures logical consistency with checks and exceptions for invalid operations.
- **Unit Tests**: Includes test cases for basic operations, partial fills, and Fill-and-Kill scenarios.

## How to Use
1. Clone the repository and compile the code with a C++ compiler that supports C++20.
2. Run the `main()` function to execute predefined test cases for the order book functionality.

## Example Test Scenarios
- Add and cancel an order.
- Match buy and sell orders with complete or partial fills.
- Simulate Fill-and-Kill orders.
- Query order book levels for bid and ask summaries.

## Requirements
- C++20 or higher
- Standard Template Library (STL)

## Potential Enhancements
- Extend to support additional order types like *Market Orders* and *Stop Orders*.
- Integrate with real-time trading data for simulations.
- Optimize data structures for high-frequency trading scenarios.
