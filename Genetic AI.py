import ctypes
from enum import Enum
import random

from time import perf_counter
from datetime import datetime

import pickle
import config
import keyboard
from func_timeout import func_set_timeout, FunctionTimedOut  # type: ignore
import logging

from pathlib import Path

Path(config.TRANING_LOCATION).mkdir(parents=True, exist_ok=True)


LIB1 = ctypes.CDLL(config.DLLLOC1)
LIB1.ai_player.argtypes = [
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_int),
]
LIB1.ai_player.restype = ctypes.c_int
LIB1.reset_state.argtypes = [ctypes.POINTER(ctypes.c_double)]
LIB1.reset_state.restype = None

LIB2 = ctypes.CDLL(config.DLLLOC2)
LIB2.ai_player.argtypes = [
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_int),
]
LIB2.ai_player.restype = ctypes.c_int
LIB2.reset_state.argtypes = [ctypes.POINTER(ctypes.c_double)]
LIB2.reset_state.restype = None


class Player(Enum):
    EMPTY = 0
    BLUE = 1
    RED = 2


class State(Enum):
    INCOMPLETE = 0
    BLUE_WIN = 1
    RED_WIN = 2
    DRAW = 3


class Weight(Enum):
    POSSIBLE_SQUARES = 0
    POSSIBLE_SCORE = 1
    PATTERN_SQUARES = 2
    PATTERN_SCORE = 3
    MY_SQUARES = 4
    MY_SCORE = 5
    OPPONENT_SQUARES = 6
    OPPONENT_SCORE = 7
    KILLER_HEURISTICS = 8
    KILLER_DECAY_CONSTANT = 9
    DEPTH = 10
    DLOGB_CONSTANT = 11


class Point:
    def __init__(self, arg1: "Point | int", arg2: int | None = None):
        if arg2 == None:
            if type(arg1) is Point:
                self.x: int = arg1.x
                self.y: int = arg1.y
            elif type(arg1) is int and type(arg2) is int:
                self.x = arg1
                self.y = arg2
            elif type(arg1) == int:
                if arg1 > 88 or arg1 < 11:
                    self.x = -1
                    self.y = -1
                else:
                    self.x = arg1 // 10 - 1
                    self.y = arg1 % 10 - 1
        else:
            if type(arg1) == int and type(arg2) == int:
                self.x = arg1
                self.y = arg2

    def toNum(self) -> int:
        return (self.x + 1) * 10 + (self.y + 1)

    def print(self):
        print(self.x, self.y)

    def is_valid(self) -> bool:
        return 0 <= self.x <= 7 and 0 <= self.y <= 7

    def __str__(self) -> str:
        return "(" + str(self.x) + "," + str(self.y) + ")"


class Square:
    def __init__(
        self, p1: Point, p2: Point, p3: Point | None = None, p4: Point | None = None
    ):
        if p3 == None or p4 == None:
            dx = p2.x - p1.x
            dy = p2.y - p1.y
            self.p1 = Point(p1.x + dy, p1.y - dx)
            self.p2 = Point(p2.x + dy, p2.y - dx)
            self.p3 = p2
            self.p4 = p1
        else:
            self.p1 = p1
            self.p2 = p2
            self.p3 = p3
            self.p4 = p4

    def print(self):
        print(self.p1, self.p2, self.p3, self.p4)

    def is_valid(self):
        return (
            self.p1.is_valid()
            and self.p2.is_valid()
            and self.p3.is_valid()
            and self.p4.is_valid()
        )

    def get_score(self):
        mxRow = max(abs(self.p3.y - self.p1.y), abs(self.p4.y - self.p2.y)) + 1
        return mxRow * mxRow

    def __str__(self):
        return (
            "("
            + str(self.p1)
            + ","
            + str(self.p2)
            + ","
            + str(self.p3)
            + ","
            + str(self.p4)
            + ")"
        )


class MetaSquaresBoard:
    def __init__(self):
        self.board = [[Player.EMPTY for _ in range(8)] for __ in range(8)]
        self.linear_board = [Player.EMPTY.value for _ in range(89)]
        self.current_player = Player.BLUE
        self.BlueScore = 0
        self.RedScore = 0
        self.BluePointList: list[Point] = []
        self.RedPointList: list[Point] = []

    def print(self):
        if self.current_player == Player.BLUE:
            print("This is BLUE's turn")
        else:
            print("This is RED's turn")

        print("   1 2 3 4 5 6 7 8")
        for i in range(8):
            print(i + 1, end=" ")
            for j in range(8):
                if self.board[i][j] == Player.EMPTY:
                    print(".", end=" ")
                elif self.board[i][j] == Player.BLUE:
                    print("#", end=" ")
                else:
                    print("0", end=" ")
            print()

    def make_move(self, p: Point):
        if not p.is_valid():
            print("Invalid move: Out of bounds")
            return 0
        if not self.checkBoard(p, Player.EMPTY):
            print("Invalid move: Already occupied")
            return 0

        self.board[p.x][p.y] = self.current_player
        self.linear_board[p.toNum()] = self.current_player.value

        if self.current_player == Player.BLUE:
            self.BluePointList.append(p)
            self.BlueScore += self.new_squares_score(p)
            self.current_player = Player.RED
        else:
            self.RedPointList.append(p)
            self.RedScore += self.new_squares_score(p)
            self.current_player = Player.BLUE

        return 1

    def currPointList(self):
        if self.current_player == Player.BLUE:
            return self.BluePointList
        return self.RedPointList

    def checkBoard(self, arg: Point | Square, player: Player) -> bool:
        if type(arg) is Point:
            return self.board[arg.x][arg.y] == player
        if type(arg) is Square:
            return (
                self.board[arg.p1.x][arg.p1.y] == player
                and self.board[arg.p2.x][arg.p2.y] == player
                and self.board[arg.p3.x][arg.p3.y] == player
                and self.board[arg.p4.x][arg.p4.y] == player
            )
        return False

    def new_squares_score(self, p: Point):
        totalScore = 0
        if not p.is_valid():
            logging.error("Invalid move")
            return 0

        for p1 in self.currPointList():
            if p1 == p:
                continue
            sq = Square(p, p1)
            if sq.is_valid() and self.checkBoard(sq, self.current_player):
                totalScore += sq.get_score()
        return totalScore

    def check_state(self):
        if self.BlueScore > 150 or self.RedScore > 150:
            if self.BlueScore - self.RedScore > 15:
                return State.BLUE_WIN
            if self.RedScore - self.BlueScore > 15:
                return State.RED_WIN

        if any(any(v == Player.EMPTY for v in row) for row in self.board):
            return State.INCOMPLETE

        if self.BlueScore > self.RedScore:
            return State.BLUE_WIN
        if self.RedScore > self.BlueScore:
            return State.RED_WIN
        return State.DRAW


class AI_Agent:
    LIMITS = [
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 500),
        (0, 1),
        # (1, 64),
        # (0, 115.59),
    ]
    MutationRate = config.MUTATION_RATE
    MutationChance = config.MUTATION_CHANCE

    def __init__(self, weights: list[float] | None = None):
        self.weights = [
            1.0,
            1.0,
            1.0,
            1.0,
            1.0,
            1.0,
            50.0,
            50.0,
            100.0,
            0.8,
            6.0,
            7.81,
        ]
        if weights != None:
            self.weights = weights
        self.player = Player.BLUE
        self.lib: ctypes.CDLL | None = None

    def play_as(self, player: Player):
        self.player = player

    def randomize_weights(self):
        for i in range(len(self.LIMITS)):  # ignore depth and dlogb for now
            self.weights[i] = random.uniform(self.LIMITS[i][0], self.LIMITS[i][1])

    def mutate_weights(self):
        for i in range(len(self.LIMITS)):
            if random.random() <= self.MutationChance:
                delta = random.uniform(-1, 1) * self.weights[i] * self.MutationRate
                self.weights[i] += delta
                self.weights[i] = max(self.weights[i], self.LIMITS[i][0])
                self.weights[i] = min(self.weights[i], self.LIMITS[i][1])

    def breed(self, other: "AI_Agent"):
        child = AI_Agent()
        for i in range(len(self.LIMITS)):
            child.weights[i] = random.choice([self.weights[i], other.weights[i]])
        return child

    def reset_state(self, LIB: ctypes.CDLL):
        self.lib = LIB
        c_test_weights = (ctypes.c_double * len(self.weights))(*self.weights)
        self.lib.reset_state(c_test_weights)

    def get_move(self, board: MetaSquaresBoard):
        if self.lib == None:
            logging.error("ERROR: AI not initialized")
            exit(1)
        c_board = (ctypes.c_int * len(board.linear_board))(*board.linear_board)
        x = self.lib.ai_player(self.player.value, c_board)

        return Point(x)

    def __str__(self) -> str:
        return str(self.weights)


class MetaSquares:
    time_limit = config.TIME_LIMIT
    time_multiplier = config.SAVED_TIME_MULTIPLIER

    def __init__(self, AI1: AI_Agent, AI2: AI_Agent) -> None:
        self.AI1 = AI1
        self.AI2 = AI2
        self.board = MetaSquaresBoard()
        self.AI1.play_as(Player.BLUE)
        self.AI2.play_as(Player.RED)
        self.AI1.reset_state(LIB1)
        self.AI2.reset_state(LIB2)
        self.move_count = 0
        self.time_saved_AI1 = 0
        self.time_saved_AI2 = 0
        self.gameState = State.INCOMPLETE
        self.is_first_log = True

    @func_set_timeout(time_limit)  # type: ignore
    def request_move(self, AI: AI_Agent):
        return AI.get_move(self.board)

    def log_status(self):
        if not self.is_first_log:
            logging.info("\033[A                             \033[A")

        print(
            "Move: {}/64 | Blue Score: {} | Red Score: {}".format(
                self.move_count, self.board.BlueScore, self.board.RedScore
            )
        )
        self.is_first_log = False

    def game_loop(self):
        while self.board.check_state() == State.INCOMPLETE:
            self.log_status()
            try:
                start_time = perf_counter()
                if self.board.current_player == Player.BLUE:
                    p = self.request_move(self.AI1)
                else:
                    p = self.request_move(self.AI2)
                end_time = perf_counter()
                delta = end_time - start_time
                if delta > self.time_limit:
                    raise FunctionTimedOut
            except FunctionTimedOut:
                print("AI took too long to respond")
                if self.board.current_player == Player.BLUE:
                    self.gameState = State.RED_WIN
                else:
                    self.gameState = State.BLUE_WIN
                break
            if self.board.current_player == Player.BLUE:
                self.time_saved_AI1 += 5 - delta
            else:
                self.time_saved_AI2 += 5 - delta

            if keyboard.is_pressed("ctrl+alt+shift+q"):
                print("Game terminated")
                exit(0)

            if self.board.make_move(p):
                self.move_count += 1
            else:
                print()
                print("AI made an invalid move: {}".format(p))
                if self.board.current_player == Player.BLUE:
                    print("OFFENDER: AI1")
                else:
                    print("OFFENDER: AI2")
                print("DATA:")
                print("AI1: " + str(self.AI1))
                print("AI2: " + str(self.AI2))
                exit(1)

        if self.gameState == State.INCOMPLETE:
            self.gameState = self.board.check_state()
            self.log_status()

    def getScore(self, player: Player):
        if player == Player.BLUE and self.gameState == State.RED_WIN:
            return -100
        if player == Player.RED and self.gameState == State.RED_WIN:
            return -100

        score = 100
        if self.gameState == State.DRAW:
            score = 0
        if self.move_count == 0:
            return score

        avg_time_saved = self.time_saved_AI1 / self.move_count

        return score + avg_time_saved * self.time_multiplier


def restore_agents(gen: int) -> list[AI_Agent]:
    if gen == -1:
        return []
    try:
        with open(config.TRANING_LOCATION + "gen_{}.pickle".format(gen), "rb") as f:
            data = pickle.load(f)
    except FileNotFoundError:
        print("Generation {} not found, starting fresh...\n\n".format(gen))
        return []

    agents: list[AI_Agent] = []
    for i in data:
        agents.append(AI_Agent(i))
    return agents


if __name__ == "__main__":
    generation = 0
    sample_size = config.SAMPLE_SIZE
    persistent_agents = config.PERSISTENT_AGENTS
    agents: list[AI_Agent] = restore_agents(config.START_GENERATION)
    if len(agents) == 0:
        agents.append(AI_Agent())  # baseline agent
    else:
        generation = config.START_GENERATION
        print("Restored {} agents from genetaion {}".format(len(agents), generation))

    while 1:
        start_time = perf_counter()
        generation += 1
        print("#" * config.HEADER_SIZE)
        print("Generation: {}".format(generation).center(config.HEADER_SIZE))
        print(
            "Time: {}".format(datetime.now().strftime("%d/%m/%y %H:%M:%S")).center(
                config.HEADER_SIZE
            )
        )
        print("#" * config.HEADER_SIZE)

        win_loss_table = [
            [State.DRAW for _ in range(sample_size)] for __ in range(sample_size)
        ]
        score = [0.0 for _ in range(sample_size)]

        print("Breeding agents...")
        for i in range(len(agents) - 1):
            for j in range(i + 1, len(agents)):
                child: AI_Agent = agents[i].breed(agents[j])
                child.mutate_weights()
                agents.append(child)

        print("Agents: {}".format(len(agents)))

        print("Adding random agents to fill array...")
        while len(agents) < sample_size:
            agent = AI_Agent()
            agent.randomize_weights()
            agents.append(agent)

        print("Playing games...")
        games_played = 0
        total_games = sample_size * sample_size - sample_size
        for i in range(len(agents)):
            for j in range(len(agents)):
                if i == j:
                    continue
                print(
                    "Playing game {}/{}: {} vs {}".format(
                        games_played, total_games, i, j
                    )
                )
                game = MetaSquares(agents[i], agents[j])
                game.game_loop()
                score[i] += game.getScore(agents[i].player)
                score[j] += game.getScore(agents[j].player)

                win_loss_table[i][j] = game.gameState
                games_played += 1
                print("State: {}\n".format(game.gameState.name))

        print("Games Complete")
        temp = [(i, score[i]) for i in range(len(score))]
        temp.sort(key=lambda x: x[1], reverse=True)

        print("Top Agents:")
        for i, _ in temp:
            wins = 0
            losses = 0
            draws = 0
            for j in range(sample_size):
                if i == j:
                    continue
                if win_loss_table[i][j] == State.BLUE_WIN:
                    wins += 1
                elif win_loss_table[i][j] == State.RED_WIN:
                    losses += 1
                else:
                    draws += 1
            print(agents[i])
            print(
                "Wins: {}, Losses: {}, Draws: {} Score: {}".format(
                    wins, losses, draws, score[i]
                )
            )

        agents = [agents[i] for i, _ in temp[:persistent_agents]]

        print("Saving agents...")
        dump = [i.weights for i in agents]
        with open(
            config.TRANING_LOCATION + "gen_{}.pickle".format(generation), "wb+"
        ) as f:
            pickle.dump(dump, f)

        elapsed_time = perf_counter() - start_time
        print("#" * config.HEADER_SIZE)
        print("GENERATION {} COMPLETE".format(generation).center(config.HEADER_SIZE))
        print(
            "TIME ELAPSED: {}".format(round(elapsed_time, 2)).center(config.HEADER_SIZE)
        )
        print("#" * config.HEADER_SIZE)
        print("\n\n")