import torch

def print_gpus():
    if torch.cuda.is_available():
        print(f"GPUs\n====")
        number_of_gpus = torch.cuda.device_count()
        for i in range(number_of_gpus):
            print(f"{i}: {torch.cuda.get_device_name(i)}")
    else:
        print("No GPUs available")

def main():
    print_gpus()
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")


if __name__ == '__main__':
    main()