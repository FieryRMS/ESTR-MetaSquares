import pickle
from pathlib import Path
from enum import Enum
import logging
import config


class State(Enum):
    INCOMPLETE = 0
    BLUE_WIN = 1
    RED_WIN = 2
    DRAW = 3


def restore_dataset(
    gen: int,
) -> list[tuple[list[float], tuple[float, float], list[State]]]:
    if gen == -1:
        return []
    try:
        # get latest file
        files = sorted(
            Path.glob(
                Path(config.TRANING_LOCATION), "gen_{}_dataset_*.pickle".format(gen)
            )
        )
        if len(files) == 0:
            raise FileNotFoundError
        logging.info("Restoring datset from {}".format(files[-1]))
        with open(files[-1], "rb") as f:
            data = pickle.load(f)
    except FileNotFoundError:
        logging.error("Generation {} not found\n\n".format(gen))
        exit(1)

    return data


if __name__ == "__main__":
    dataset = restore_dataset(9)
    for i in range(100):
        for j in range(100):
            print(dataset[i][2][j].value, end=" ")
        print()
