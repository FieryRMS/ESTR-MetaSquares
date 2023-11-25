import ctypes
from enum import Enum
import random
import traceback

from time import perf_counter, time

import pickle
import config
from func_timeout import func_set_timeout, FunctionTimedOut  # type: ignore
import logging
import google.cloud.logging
import multiprocessing as mp
from multiprocessing.pool import AsyncResult
import shutil
import subprocess

from pathlib import Path

gLogger = lambda *args, **kwargs: None  # type: ignore
gLogger.log_text = lambda *args, **kwargs: None  # type: ignore


class Player(Enum):
    EMPTY = 0
    BLUE = 1
    RED = 2


class State(Enum):
    INCOMPLETE = 0
    BLUE_WIN = 1
    RED_WIN = 2
    DRAW = 3
    RESTART = 4


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
            logging.debug("This is BLUE's turn")
        else:
            logging.debug("This is RED's turn")

        logging.debug("   1 2 3 4 5 6 7 8")
        for i in range(8):
            logging.debug(str(i + 1) + " ")
            for j in range(8):
                if self.board[i][j] == Player.EMPTY:
                    logging.debug(". ")
                elif self.board[i][j] == Player.BLUE:
                    logging.debug("# ")
                else:
                    logging.debug("0 ")
            logging.debug("\n")

    def make_move(self, p: Point):
        if not p.is_valid():
            logging.warning("Invalid move: Out of bounds")
            gLogger.log_text("Invalid move: Out of bounds", severity="WARNING")  # type: ignore
            return 0
        if not self.checkBoard(p, Player.EMPTY):
            logging.warning("Invalid move: Already occupied")
            gLogger.log_text("Invalid move: Already occupied", severity="WARNING")  # type: ignore
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
            gLogger.log_text("Invalid move", severity="ERROR")  # type: ignore
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
            if self.BlueScore - self.RedScore >= 15:
                return State.BLUE_WIN
            if self.RedScore - self.BlueScore >= 15:
                return State.RED_WIN

        if any(any(v == Player.EMPTY for v in row) for row in self.board):
            return State.INCOMPLETE

        if self.BlueScore > self.RedScore:
            return State.BLUE_WIN
        if self.RedScore > self.BlueScore:
            return State.RED_WIN
        return State.DRAW


class AI_Agent:
    LIMITS: list[tuple[float, float]] = [
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
        (4, 26),
        (0, 8),
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
        if weights is not None:
            self.weights = weights.copy()
        self.score: tuple[float, float] = (0.0, 0.0)

    def add_score(self, score: tuple[float, float]):
        self.score = (self.score[0] + score[0], self.score[1] + score[1])

    def reset_score(self):
        self.score = (0.0, 0.0)

    def randomize_weights(self):
        for i in range(len(self.LIMITS)):
            self.weights[i] = random.uniform(self.LIMITS[i][0], self.LIMITS[i][1])

        self.check_and_fix_limits()

    def mutate_weight(self, i: int):
        delta = random.uniform(-1, 1) * self.weights[i] * self.MutationRate
        self.weights[i] += delta

    def check_and_fix_limits(self):
        for i in range(len(self.LIMITS)):
            self.weights[i] = max(self.weights[i], self.LIMITS[i][0])
            self.weights[i] = min(self.weights[i], self.LIMITS[i][1])

    def mutate_weights(self):
        for i in range(len(self.LIMITS)):
            if random.random() <= self.MutationChance:
                self.mutate_weight(i)

    def breed(self, other: "AI_Agent"):
        child = AI_Agent()
        for i in range(len(self.LIMITS)):
            child.weights[i] = random.choice([self.weights[i], other.weights[i]])
        return child

    def asexual_baby1(self):
        child = self.copy()
        for i in range(len(self.LIMITS)):
            if random.uniform(0, 1) <= self.MutationChance:
                child.weights[i] = random.uniform(self.LIMITS[i][0], self.LIMITS[i][1])
            else:
                child.mutate_weight(i)
        return child

    def asexual_baby2(self):
        child = self.copy()
        child.mutate_weights()
        return child

    def copy(self):
        return AI_Agent(self.weights)

    def set_LIB_state(self, LIB: ctypes.CDLL):
        self.check_and_fix_limits()
        c_test_weights = (ctypes.c_double * len(self.weights))(*self.weights)
        LIB.reset_state(c_test_weights)

    def get_move(self, board: MetaSquaresBoard, player: Player, LIB: ctypes.CDLL):
        c_board = (ctypes.c_int * len(board.linear_board))(*board.linear_board)
        x = LIB.ai_player(player.value, c_board)

        return Point(x)

    def __str__(self) -> str:
        # return as C style array
        return "{" + ", ".join([str(i) for i in self.weights]) + "}"


class MetaSquares:
    time_limit = config.TIME_LIMIT

    def __init__(
        self, AI1: AI_Agent, AI2: AI_Agent, LIB1: ctypes.CDLL, LIB2: ctypes.CDLL
    ):
        self.agent1 = AI1
        self.agent2 = AI2
        self.board = MetaSquaresBoard()
        self.Lib1 = LIB1
        self.Lib2 = LIB2
        self.agent1.set_LIB_state(self.Lib1)
        self.agent2.set_LIB_state(self.Lib2)
        self.move_count = 0
        self.time_saved_AI1 = 0
        self.time_saved_AI2 = 0
        self.gameState = State.INCOMPLETE
        self.is_first_log = True

    @func_set_timeout(time_limit)  # type: ignore
    def request_move(self, AI: AI_Agent, player: Player, LIB: ctypes.CDLL):
        if self.board is None:
            return Point(-1, -1)
        return AI.get_move(self.board, player, LIB)

    def log_status(self):
        if self.board is None:
            return
        if not self.is_first_log:
            logging.info("\033[A                             \033[A")

        logging.info(
            "Move: {}/64 | Blue Score: {} | Red Score: {}".format(
                self.move_count, self.board.BlueScore, self.board.RedScore
            )
        )
        self.is_first_log = False

    def game_loop(self):
        if (
            self.Lib1 is None
            or self.Lib2 is None
            or self.agent1 is None
            or self.agent2 is None
            or self.board is None
        ):
            logging.error("Libraries not initialized")
            gLogger.log_text("Libraries not initialized", severity="ERROR")  # type: ignore
            exit(1)
        while self.board.check_state() == State.INCOMPLETE:
            if config.PROCESS_COUNT == 1:  # type: ignore
                self.log_status()
            try:
                start_time = perf_counter()
                if self.board.current_player == Player.BLUE:
                    p = self.request_move(self.agent1, Player.BLUE, self.Lib1)
                else:
                    p = self.request_move(self.agent2, Player.RED, self.Lib2)
                end_time = perf_counter()
                delta = end_time - start_time
                if delta > self.time_limit:
                    raise FunctionTimedOut
            except FunctionTimedOut:
                logging.info(
                    "AI took too long to respond, decreasing DLOGB_CONSTANT and restarting..."
                )
                self.gameState = State.RESTART
                if self.board.current_player == Player.BLUE:
                    logging.info(
                        "AI1: " + str(self.agent1.weights[Weight.DLOGB_CONSTANT.value])
                    )
                    self.agent1.weights[Weight.DLOGB_CONSTANT.value] -= 0.1
                else:
                    logging.info(
                        "AI2: " + str(self.agent2.weights[Weight.DLOGB_CONSTANT.value])
                    )
                    self.agent2.weights[Weight.DLOGB_CONSTANT.value] -= 0.1
                break
            except Exception as e:
                logging.error("ERROR: {}".format(e))
                logging.error(traceback.format_exc())
                gLogger.log_text("ERROR: {}".format(e), severity="ERROR")  # type: ignore
                raise e
            if self.board.current_player == Player.BLUE:
                self.time_saved_AI1 += 5 - delta
            else:
                self.time_saved_AI2 += 5 - delta

            if self.board.make_move(p):
                self.move_count += 1
            else:
                logging.error("\n")
                logging.error("AI made an invalid move: {}".format(p))
                if self.board.current_player == Player.BLUE:
                    logging.error("OFFENDER: AI1")
                else:
                    logging.error("OFFENDER: AI2")
                logging.error("DATA:")
                logging.error("AI1: " + str(self.agent1))
                logging.error("AI2: " + str(self.agent2))
                gLogger.log_text("AI made an invalid move: {}".format(p), severity="ERROR")  # type: ignore
                exit(1)

        if self.gameState == State.INCOMPLETE:
            self.gameState = self.board.check_state()
            if config.PROCESS_COUNT == 1:  # type: ignore
                self.log_status()

        self.Lib1 = None
        self.Lib2 = None
        self.agent1 = None
        self.agent2 = None
        self.board = None

    def getScore(self, player: Player):
        if (player == Player.BLUE and self.gameState == State.RED_WIN) or (
            player == Player.RED and self.gameState == State.BLUE_WIN
        ):
            return (-10.0, 0.0)

        score = 10.0
        if self.gameState == State.DRAW:
            score = 0.0
        if self.move_count == 0:
            return (score, 0.0)

        if player == Player.BLUE:
            avg_time_saved = self.time_saved_AI1 / self.move_count
        else:
            avg_time_saved = self.time_saved_AI2 / self.move_count

        return (score, avg_time_saved)


def restore_agents(gen: int) -> list[AI_Agent]:
    if gen == -1:
        return []
    try:
        # get latest file
        files = sorted(
            Path.glob(
                Path(config.TRANING_LOCATION),
                "gen_{}_dataset_[0-9]*.pickle".format(gen),
            )
        )
        if len(files) == 0:
            raise FileNotFoundError
        logging.info("Restoring agents from {}".format(files[-1]))
        with open(files[-1], "rb") as f:
            data: list[
                tuple[list[float], tuple[float, float], list[State]]
            ] = pickle.load(f)
    except FileNotFoundError:
        logging.warning("Generation {} not found, starting fresh...\n\n".format(gen))
        return []

    agents: list[AI_Agent] = []
    for i in data:
        agent = AI_Agent(i[0])
        agent.score = i[1]
        agents.append(agent)
    agents.sort(key=lambda x: x.score, reverse=True)
    agents = agents[: config.PERSISTENT_AGENTS]
    return agents


def calc_time(Ts: float):
    if Ts <= 0:
        Ts = 0
    Tm = Ts // 60
    Ts = Ts - Tm * 60
    Th = Tm // 60
    Tm = Tm - Th * 60
    return (round(Th), round(Tm), round(Ts))


def setup_LIB(LIBLOC: str):
    LIB = ctypes.CDLL(LIBLOC)
    LIB.ai_player.argtypes = [
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_int),
    ]
    LIB.ai_player.restype = ctypes.c_int
    LIB.reset_state.argtypes = [ctypes.POINTER(ctypes.c_double)]
    LIB.reset_state.restype = None
    return LIB


def worker(
    agent1: AI_Agent,
    agent2: AI_Agent,
    LIBLOC1: str,
    LIBLOC2: str,
    i: int,
    j: int,
    lib: int,
):
    try:
        LIB1 = setup_LIB(LIBLOC1)
        LIB2 = setup_LIB(LIBLOC2)
        game = MetaSquares(agent1, agent2, LIB1, LIB2)
        while 1:
            game.game_loop()
            if game.gameState != State.RESTART:
                break
            game = MetaSquares(agent1, agent2, LIB1, LIB2)
    except Exception as e:
        logging.error("ERROR: {}".format(e))
        logging.error(traceback.format_exc())
        gLogger.log_text("ERROR: {}".format(e), severity="ERROR")  # type: ignore
        raise e
    return game, i, j, lib, agent1.weights, agent2.weights


def get_eta(games_played: float, total_games: float, start_time: float):
    elapsed_time = perf_counter() - start_time
    speed = games_played / elapsed_time
    eta = (total_games - games_played) / speed
    return eta


def get_agent(agents: list[AI_Agent]):
    total_score: float = 0
    for i in agents:
        if i.score[0] < 0:
            continue
        total_score += i.score[0] + i.score[1]

    if total_score == 0:
        return random.choice(agents)
    r = random.uniform(0, total_score)
    for i in agents:
        if i.score[0] < 0:
            continue
        r -= i.score[0] + i.score[1]
        if r <= 0:
            return i
    return random.choice(agents)


def get_agents(agents: list[AI_Agent]):
    return (get_agent(agents), get_agent(agents))


if __name__ == "__main__":
    Path(config.TRANING_LOCATION).mkdir(parents=True, exist_ok=True)

    LIBS: list[str] = []

    for i in range(config.PROCESS_COUNT * 2):
        dst = config.DLLLOC + config.DLLNAME.split(".")[0] + str(i) + ".dll"
        shutil.copyfile(config.DLLLOC + config.DLLNAME, dst)
        LIBS.append(dst)

    with open("restart.bool", "w+") as f:
        f.write("false")

    file_handler = logging.FileHandler(
        config.TRANING_LOCATION + "genetic_errors_{}.log".format(int(time()))
    )
    file_handler.setLevel(logging.WARNING)
    logging.basicConfig(
        level=logging.INFO,
        format="%(levelname)s %(asctime)s:\t %(message)s",
        datefmt="%H:%M:%S",
        handlers=[logging.StreamHandler(), file_handler],
    )
    try:
        client = google.cloud.logging.Client()
        gLogger = client.logger("Genetic_AI")  # type: ignore
    except Exception as e:
        logging.warning("Could not connect to Google Cloud Logging: {}".format(e))

    generation = 0
    sample_size = config.SAMPLE_SIZE
    persistent_agents = config.PERSISTENT_AGENTS
    agents: list[AI_Agent] = restore_agents(config.START_GENERATION)
    if len(agents) != 0:
        generation = config.START_GENERATION
        logging.info(
            "Restored {} agents from genetaion {}".format(len(agents), generation)
        )

    while 1:
        try:
            generation += 1
            logging.info(("#" * config.HEADER_SIZE))
            logging.info("Generation: {}".format(generation).center(config.HEADER_SIZE))
            logging.info(("#" * config.HEADER_SIZE))
            gLogger.log_text("GENERATION {} STARTED".format(generation))  # type: ignore

            logging.info("Breeding agents...")

            if len(agents) == 0:
                logging.info("Adding random agents to fill array...\n")
                while len(agents) < sample_size:
                    agent = AI_Agent()
                    agent.randomize_weights()
                    agents.append(agent)
            else:
                for i in agents:
                    i.weights[Weight.DLOGB_CONSTANT.value] *= 1.2

            as1_agents = (config.SAMPLE_SIZE - len(agents) + 1) // 3
            s_agents = (config.SAMPLE_SIZE - len(agents) - as1_agents) // 2
            as2_agents = config.SAMPLE_SIZE - len(agents) - as1_agents - s_agents
            for _ in range(as1_agents):
                agent = get_agent(agents).asexual_baby1()
                agent.weights[Weight.DLOGB_CONSTANT.value] *= 0.8
                agents.append(agent)

            for _ in range(as2_agents):
                agent = get_agent(agents).asexual_baby2()
                agent.weights[Weight.DLOGB_CONSTANT.value] *= 0.8
                agents.append(agent)

            for _ in range(s_agents):
                (agent1, agent2) = get_agents(agents)
                agent = agent1.breed(agent2)
                agent.mutate_weights()
                agent.weights[Weight.DLOGB_CONSTANT.value] *= 0.8
                agents.append(agent)

            for i in agents:
                i.reset_score()
                i.check_and_fix_limits()

            logging.info("Agents: {}\n".format(len(agents)))

            if len(agents) > sample_size:
                logging.info("Removing agents to fit array...\n")
                while len(agents) > sample_size:
                    agents.pop(random.randint(0, len(agents) - 1))

            logging.info("Playing games...")
            games_played = 0
            win_loss_table = [
                [State.DRAW for _ in range(sample_size)] for __ in range(sample_size)
            ]
            total_games = sample_size * sample_size - sample_size
            start_time = perf_counter()
            with mp.Pool(processes=config.PROCESS_COUNT) as pool:

                def task_gen():
                    for i in range(len(agents)):
                        for j in range(len(agents)):
                            if i == j:
                                continue
                            yield i, j

                nxt_task = task_gen()
                result_list: list[
                    AsyncResult[
                        tuple[MetaSquares, int, int, int, list[float], list[float]]
                    ]
                ] = []

                def add_task(lib: int, i: int, j: int):
                    Libloc1 = LIBS[2 * lib]
                    Libloc2 = LIBS[2 * lib + 1]
                    result_list.append(
                        pool.apply_async(
                            worker,
                            args=[
                                agents[i],
                                agents[j],
                                Libloc1,
                                Libloc2,
                                i,
                                j,
                                lib,
                            ],
                            callback=call_back,
                        )
                    )

                def call_back(
                    args: tuple[MetaSquares, int, int, int, list[float], list[float]]
                ):
                    try:
                        (i, j) = next(nxt_task)
                    except StopIteration:
                        return
                    add_task(args[3], i, j)

                for lib in range(config.PROCESS_COUNT):
                    try:
                        (i, j) = next(nxt_task)
                    except StopIteration:
                        break
                    add_task(lib, i, j)

                while len(result_list) > 0:
                    try:
                        agent1W: list[float]
                        agent2W: list[float]
                        (game, i, j, lib, agent1W, agent2W) = result_list.pop(0).get()
                    except Exception as e:
                        logging.error("ERROR: {}".format(e))
                        logging.error(traceback.format_exc())
                        gLogger.log_text("ERROR: {}".format(e), severity="ERROR")  # type: ignore
                        exit(1)
                    agents[i].add_score(game.getScore(Player.BLUE))
                    agents[j].add_score(game.getScore(Player.RED))
                    agents[i].weights = agent1W
                    agents[j].weights = agent2W
                    games_played += 1
                    win_loss_table[i][j] = game.gameState
                    eta = get_eta(games_played, total_games, start_time)
                    (Th, Tm, Ts) = calc_time(eta)
                    logging.info(
                        "GEN{:<2} ETA: {:>2}h {:>2}m {:>2}s | Game complete {:>5}/{:<5}: {:>3} vs {:<3} = {:<8} in {:<2} moves".format(
                            generation,
                            round(Th),
                            round(Tm),
                            round(Ts),
                            games_played,
                            total_games,
                            i,
                            j,
                            game.gameState.name,
                            game.move_count,
                        )
                    )
                    gLogger.log_text(  # type: ignore
                        "GEN{:<2} ETA: {:>2}h {:>2}m {:>2}s | Game complete {:>5}/{:<5}: {:>3} vs {:<3} = {:<8} in {:<2} moves".format(
                            generation,
                            round(Th),
                            round(Tm),
                            round(Ts),
                            games_played,
                            total_games,
                            i,
                            j,
                            game.gameState.name,
                            game.move_count,
                        )
                    )

            elapsed_time = perf_counter() - start_time

            logging.info("Games Complete")

            logging.info("Calculating best of BLUE and RED and adding bonuses...")
            best_blue = -1
            best_red = -1
            best_blue_index = 0
            best_red_index = 0
            for i in range(len(agents)):
                blue_score = 0
                red_score = 0
                for j in range(len(agents)):
                    if i == j:
                        continue
                    if win_loss_table[i][j] == State.BLUE_WIN:
                        blue_score += 1
                    if win_loss_table[j][i] == State.RED_WIN:
                        red_score += 1
                if blue_score > best_blue or (
                    blue_score == best_blue
                    and agents[i].score > agents[best_blue_index].score
                ):
                    best_blue = blue_score
                    best_blue_index = i
                if red_score > best_red or (
                    red_score == best_red
                    and agents[i].score > agents[best_red_index].score
                ):
                    best_red = red_score
                    best_red_index = i
            agents[best_blue_index].add_score((155.0, 0.0))
            agents[best_red_index].add_score((166.0, 0.0))
            logging.info(
                "Best Blue: {} ".format(str(agents[best_blue_index]))
                + str(agents[best_blue_index].score)
            )
            logging.info(
                "Best Red: {} ".format(str(agents[best_red_index]))
                + str(agents[best_red_index].score)
            )

            logging.info("Dumping complete dataset...")
            with open(
                config.TRANING_LOCATION
                + "gen_{}_dataset_{}.pickle".format(generation, int(time())),
                "wb+",
            ) as f:
                data: list[tuple[list[float], tuple[float, float], list[State]]] = []
                for i in range(len(agents)):
                    data.append((agents[i].weights, agents[i].score, win_loss_table[i]))
                pickle.dump(data, f)

            temp = [(i, agents[i].score) for i in range(len(agents))]
            temp.sort(key=lambda x: x[1], reverse=True)

            logging.info("Top Agents:")
            with open(
                config.TRANING_LOCATION
                + "gen_{}_stats_{}.log".format(generation, int(time())),
                "w+",
            ) as f:
                for i, _ in temp:
                    wins = 0
                    losses = 0
                    draws = 0
                    for j in range(sample_size):
                        if i == j:
                            continue
                        match win_loss_table[i][j]:
                            case State.BLUE_WIN:
                                wins += 1
                            case State.RED_WIN:
                                losses += 1
                            case State.DRAW:
                                draws += 1
                            case _:
                                pass
                        match win_loss_table[j][i]:
                            case State.BLUE_WIN:
                                losses += 1
                            case State.RED_WIN:
                                wins += 1
                            case State.DRAW:
                                draws += 1
                            case _:
                                pass
                    logging.info(str(agents[i]))
                    logging.info(
                        "Wins: {}, Losses: {}, Draws: {} Score: {}\n".format(
                            wins, losses, draws, agents[i].score
                        )
                    )
                    f.write(str(agents[i]) + "\n")
                    f.write(
                        "Wins: {}, Losses: {}, Draws: {} Score: {}\n\n".format(
                            wins, losses, draws, agents[i].score
                        )
                    )

            agents = [agents[i] for i, _ in temp[:persistent_agents]]

            (Th, Tm, Ts) = calc_time(elapsed_time)

            logging.info(("#" * config.HEADER_SIZE))
            logging.info(
                "GENERATION {} COMPLETE".format(generation).center(config.HEADER_SIZE)
            )
            logging.info(
                "TIME ELAPSED: {}h {}m {}s".format(round(Th), round(Tm), round(Ts))
            )
            logging.info(("#" * config.HEADER_SIZE) + "\n\n\n")
            gLogger.log_text(  # type: ignore
                "GENERATION {} COMPLETE | TIME ELAPSED: {}h {}m {}s".format(
                    generation, round(Th), round(Tm), round(Ts)
                )
            )
            with open("restart.bool", "r") as f:
                if f.read().lower().strip() == "true":
                    logging.info("Restarting...")
                    gLogger.log_text("Restarting...")  # type: ignore
                    subprocess.run("nohup ./restart.sh &", shell=True)
                    exit(0)

        except KeyboardInterrupt:
            logging.info("Keyboard Interrupt")
            break
        except Exception as e:
            logging.error("ERROR: {}".format(e))
            logging.error(traceback.format_exc())
            gLogger.log_text("ERROR: {}".format(e), severity="ERROR")  # type: ignore
            break
