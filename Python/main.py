import numpy as np
import torch
import os
import pandas as pd

from config import CORRELATION_FILE_PATH, HIGH_CORRELATION_FILE_PATH

# Needed to use the GPU
os.putenv("HSA_OVERRIDE_GFX_VERSION", "10.3.0")

def extract_tensors(device, stock_data_array_ptr):
    stock_data_array = stock_data_array_ptr.contents

    raw_stock_data_tensors = []
    for i in range(stock_data_array.raw_stock_data.contents.number_of_raw_stock_data_arrays):
        row_array = stock_data_array.raw_stock_data.contents.row_arrays[i]
        stock_symbol = row_array.stock_symbol.decode('utf-8')

        rows = row_array.rows[:row_array.data_size]
        row_tensor = torch.tensor(
            [(row.month, row.day, row.open, row.high, row.low, row.close, row.volume) for row in rows],
            dtype=torch.float32
        ).to(device)
        raw_stock_data_tensors.append((stock_symbol, row_tensor))

    direction_data_tensors = []
    for i in range(stock_data_array.directions.contents.data_size):
        direction_data_row = stock_data_array.directions.contents.direction_data_arrays[i]
        stock_symbol = direction_data_row.stock_symbol.decode('utf-8')

        direction_data = direction_data_row.direction_data[:direction_data_row.data_size]
        direction_tensor = torch.tensor(direction_data, dtype=torch.uint8).to(device)
        direction_data_tensors.append((stock_symbol, direction_tensor))

    direction_counts_tensors = []
    for i in range(stock_data_array.direction_counts.contents.data_size):
        direction_counts = stock_data_array.direction_counts.contents.direction_counts[i]
        stock_symbol = direction_counts.stock_symbol.decode('utf-8')

        counts_tensor = torch.tensor(list(direction_counts.direction_counts), dtype=torch.float32).to(device)
        direction_counts_tensors.append((stock_symbol, counts_tensor))

    return raw_stock_data_tensors, direction_data_tensors, direction_counts_tensors

def print_gpus():
    if torch.cuda.is_available():
        print(f"GPUs\n====")
        number_of_gpus = torch.cuda.device_count()
        for i in range(number_of_gpus):
            print(f"{i}: {torch.cuda.get_device_name(i)}")
    else:
        print("No GPUs available")


def save_correlations(device, raw_stock_data_tensors):
    stock_low_tensors = []
    was_profit_tensors = []
    for stock_name, tensor in raw_stock_data_tensors:
        stock_low_tensor = torch.tensor(tensor[:, 4], dtype=torch.float32, device=device)
        shifted_low_tensor = torch.cat([stock_low_tensor[1:], torch.full((1,), float('nan'), device=device)])
        stock_low_tensors.append((stock_name, stock_low_tensor))

        high_tensor = torch.tensor(tensor[:, 3], dtype=torch.float32, device=device)
        was_profit_tensor = (shifted_low_tensor - high_tensor) > 0
        was_profit_tensors.append(was_profit_tensor.float())

    was_profit = 0
    was_loss = 0
    correlation_results = []
    with open(HIGH_CORRELATION_FILE_PATH, mode='w') as f:
        for i, (stock1, target_tensor) in enumerate(stock_low_tensors):
            for j, (stock2, other_tensor) in enumerate(stock_low_tensors):
                if i != j:
                    was_profit_tensor = was_profit_tensors[j]

                    min_length = min(len(target_tensor), len(was_profit_tensor))
                    target_tensor_trimmed = target_tensor[-min_length:-2].to(device)
                    other_future_tensor_trimmed = was_profit_tensor[-min_length:-2].to(device)

                    # Calculate correlation using PyTorch (Pearson correlation)
                    # Pearson correlation: cov(X, Y) / (std(X) * std(Y))
                    target_mean = target_tensor_trimmed.mean()
                    target_std = target_tensor_trimmed.std()
                    other_mean = other_future_tensor_trimmed.mean()
                    other_std = other_future_tensor_trimmed.std()
                    covariance = ((target_tensor_trimmed - target_mean) * (other_future_tensor_trimmed - other_mean)).mean()
                    correlation = covariance / (target_std * other_std)

                    result = (f'Stock {stock1} vs Stock {stock2}', correlation.item())
                    if correlation.item() > 0.85 or correlation.item() < -0.85:
                        if correlation.item() > 0.85 and min_length > 6 and target_tensor_trimmed[-3] - target_tensor_trimmed[-4] > 0:
                            if other_future_tensor_trimmed[-1] > 0:
                                was_profit += 1
                            else:
                                was_loss += 1
                            print(f'Profit to loss: \n    {was_profit}:{was_loss}')
                        print(result)
                        f.write(f"{result}\n")

                    correlation_results.append(result)

        correlation_df = pd.DataFrame(correlation_results, columns=['Stock Pair', 'Correlation'])
        correlation_df.to_csv(HIGH_CORRELATION_FILE_PATH, index=False)
        print(correlation_df)


import csv

def print_correlation_file(file_name):
    try:
        with open(file_name, 'r') as file:
            reader = csv.DictReader(file)

            print("Stock Pair\tCorrelation:")
            for row in reader:
                stock_pair = row['Stock Pair']
                correlation = row['Correlation']
                print(f"{stock_pair}\t{correlation}")

    except FileNotFoundError:
        print("The file does not exist. Please check the file name and path.")


def load_stock_data_array():
    import ctypes
    # TODO
    lib = ctypes.CDLL('../cmake-build-debug/libarray_to_python.so')
    class Row(ctypes.Structure):
        _fields_ = [
            ("month", ctypes.c_uint8),
            ("day", ctypes.c_uint8),
            ("open", ctypes.c_double),
            ("high", ctypes.c_double),
            ("low", ctypes.c_double),
            ("close", ctypes.c_double),
            ("volume", ctypes.c_double)
        ]

    class RowArray(ctypes.Structure):
        _fields_ = [
            ("stock_symbol", ctypes.c_char_p),
            ("rows", ctypes.POINTER(Row)),
            ("data_size", ctypes.c_size_t)
        ]

    class RawStockDataArray(ctypes.Structure):
        _fields_ = [
            ("row_arrays", ctypes.POINTER(RowArray)),
            ("number_of_raw_stock_data_arrays", ctypes.c_size_t)
        ]

    class DirectionDataRowArray(ctypes.Structure):
        _fields_ = [
            ("stock_symbol", ctypes.c_char_p),
            ("direction_data", ctypes.POINTER(ctypes.c_uint8)),
            ("data_size", ctypes.c_size_t)
        ]

    class DirectionDataArray(ctypes.Structure):
        _fields_ = [
            ("direction_data_arrays", ctypes.POINTER(DirectionDataRowArray)),
            ("data_size", ctypes.c_size_t)
        ]

    # DirectionCounts structure
    class DirectionCounts(ctypes.Structure):
        _fields_ = [
            ("stock_symbol", ctypes.c_char_p),
            ("direction_counts", ctypes.c_double * 512)
        ]

    class DirectionCountsArray(ctypes.Structure):
        _fields_ = [
            ("direction_counts", ctypes.POINTER(DirectionCounts)),
            ("data_size", ctypes.c_size_t)
        ]

    class StockDataArray(ctypes.Structure):
        _fields_ = [
            ("raw_stock_data", ctypes.POINTER(RawStockDataArray)),
            ("directions", ctypes.POINTER(DirectionDataArray)),
            ("direction_counts", ctypes.POINTER(DirectionCountsArray)),
            ("size", ctypes.c_size_t)
        ]


    lib.load_stock_data.restype = ctypes.POINTER(StockDataArray)
    lib.free_stock_data.argtypes = [ctypes.POINTER(StockDataArray)]
    stock_data_array = lib.load_stock_data()
    return stock_data_array


import ast
def read_correlation_file_to_map(file_path):
    correlation_map = {}

    try:
        with open(file_path, 'r') as file:
            for line in file:
                try:
                    stock_pair, correlation = ast.literal_eval(line.strip())
                    correlation_map[stock_pair] = correlation
                except (ValueError, SyntaxError) as e:
                    print(f"Error parsing line: {line.strip()} - {e}")

    except FileNotFoundError:
        print(f"File not found: {file_path}")

    return correlation_map

def main():
    print_gpus()
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    # if os.path.exists(HIGH_CORRELATION_FILE_PATH):
    #     stock_pair_to_correlation_map = read_correlation_file_to_map(HIGH_CORRELATION_FILE_PATH)
    if os.path.exists(CORRELATION_FILE_PATH):
        print_correlation_file(CORRELATION_FILE_PATH)
    else:
        stock_data_array = load_stock_data_array()
        raw_stock_data_tensors, direction_data_tensors, direction_counts_tensors = extract_tensors(device, stock_data_array)
        save_correlations(device, raw_stock_data_tensors)
    print()





if __name__ == '__main__':
    main()