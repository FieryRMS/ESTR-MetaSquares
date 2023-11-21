import pickle
from pathlib import Path
from enum import Enum
import logging
import config
import plotly.graph_objects as go # type: ignore

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
    # weights, score, states
    dataset = restore_dataset(config.START_GENERATION)

    if len(dataset) == 0:
        logging.error("Dataset is empty")
        exit(1)

    colLabels = [str(i) for i in range(len(dataset))]
    colLabels.insert(0, " ")
    colLabels.extend(["Wins", "Losses", "Draws", "Win %", "Score", "Time Saved"])
    rowLabels = [str(i) for i in range(len(dataset))]
    rowLabels.extend(["Wins", "Losses", "Draws", "Win %", "Score", "Time Saved"])
    cell_text = [["" for _ in range(len(dataset) + 6)] for _ in range(len(dataset) + 6)]
    cell_colours = [
        ["white" for _ in range(len(dataset) + 6)] for _ in range(len(dataset) + 6)
    ]
    col_win = [0 for _ in range(len(dataset))]
    col_loss = [0 for _ in range(len(dataset))]
    col_draw = [0 for _ in range(len(dataset))]

    cell_sz = 16
    for i in range(len(dataset)):
        row_win = 0
        row_loss = 0
        row_draw = 0
        for j in range(len(dataset)):
            if i == j:
                cell_text[i][j] = "X"
                cell_colours[i][j] = "black"
                continue

            match dataset[i][2][j]:
                case State.INCOMPLETE:
                    cell_text[i][j] = "I"
                    cell_colours[i][j] = "grey"
                case State.BLUE_WIN:
                    cell_text[i][j] = "B"
                    cell_colours[i][j] = "blue"
                    row_win += 1
                    col_loss[j] += 1
                case State.RED_WIN:
                    cell_text[i][j] = "R"
                    cell_colours[i][j] = "red"
                    row_loss += 1
                    col_win[j] += 1
                case State.DRAW:
                    cell_text[i][j] = "D"
                    cell_colours[i][j] = "lightgrey"
                    row_draw += 1
                    col_draw[j] += 1

        cell_text[i][len(dataset)] = str(row_win)
        cell_text[i][len(dataset) + 1] = str(row_loss)
        cell_text[i][len(dataset) + 2] = str(row_draw)
        cell_text[i][len(dataset) + 3] = "{:.2f}".format(
            row_win / (row_win + row_loss + row_draw)
        )
        cell_text[i][len(dataset) + 4] = "{:.2f}".format(dataset[i][1][0])
        cell_text[i][len(dataset) + 5] = "{:.2f}".format(dataset[i][1][1])

    for i in range(len(dataset)):
        cell_text[len(dataset)][i] = str(col_win[i])
        cell_text[len(dataset) + 1][i] = str(col_loss[i])
        cell_text[len(dataset) + 2][i] = str(col_draw[i])
        cell_text[len(dataset) + 3][i] = "{:.2f}".format(
            col_win[i] / (col_win[i] + col_loss[i] + col_draw[i])
        )
        cell_text[len(dataset) + 4][i] = "{:.2f}".format(dataset[i][1][0])
        cell_text[len(dataset) + 5][i] = "{:.2f}".format(dataset[i][1][1])

    cols: list[list[str]] = [[rowLabels[i] for i in range(len(dataset))]]
    col_colours: list[list[str]] = [["white" for _ in range(len(dataset))]]
    for i in range(len(dataset) + 6):
        col: list[str] = []
        col_colour: list[str] = []
        for j in range(len(dataset) + 6):
            col.append(cell_text[j][i])
            col_colour.append(cell_colours[j][i])
        cols.append(col)
        col_colours.append(col_colour)

    fig = go.Figure(
        data=[
            go.Table(
                header=dict(
                    values=colLabels,
                    fill_color="paleturquoise",
                    align="center",
                    font=dict(color="black", size=3),
                ),
                cells=dict(
                    values=cols,
                    fill_color=col_colours,
                    align="center",
                    font=dict(color="black", size=3),
                    height=cell_sz,
                ),
            )
        ]
    )
    fig.update_layout( # type: ignore
        autosize=False,
        width=cell_sz * (len(dataset) + 6),
        height=cell_sz * (len(dataset) + 10),
        margin=dict(l=0, r=0, b=0, t=0, pad=0),
    )

    fig.show() # type: ignore
