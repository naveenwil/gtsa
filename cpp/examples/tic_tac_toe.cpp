#include "../gtsa.hpp"

const int SIDE = 3;
const char PLAYER_1 = 'X';
const char PLAYER_2 = 'O';
const char EMPTY = '_';

struct TicTacToeMove : public Move<TicTacToeMove> {
    unsigned x;
    unsigned y;

    TicTacToeMove() { }

    TicTacToeMove(unsigned x, unsigned y) : x(x), y(y) { }

    void read(istream &stream = cin) override {
        if (&stream == &cin) {
            cout << "Enter space separated X and Y of your move: ";
        }
        stream >> x >> y;
    }

    ostream &to_stream(ostream &os) const override {
        return os << x << " " << y;
    }

    bool operator==(const TicTacToeMove &rhs) const override {
        return x == rhs.x && y == rhs.y;
    }

    size_t hash() const override {
        using boost::hash_value;
        using boost::hash_combine;
        size_t seed = 0;
        hash_combine(seed, hash_value(x));
        hash_combine(seed, hash_value(y));
        return seed;
    }
};

const vector<vector<TicTacToeMove>> LINES = [] {
    vector<vector<TicTacToeMove>> lines;
    for (int y = 0; y < SIDE; ++y) {
        lines.emplace_back(); // add a new line to back()
        for (int x = 0; x < SIDE; ++x)
            lines.back().emplace_back(x, y); // add a new coord to that line
    }
    for (int x = 0; x < SIDE; ++x) {
        lines.emplace_back();
        for (int y = 0; y < SIDE; ++y)
            lines.back().emplace_back(x, y);
    }
    lines.emplace_back();
    for (int i = 0; i < SIDE; ++i) {
        lines.back().emplace_back(i, i);
    }
    lines.emplace_back();
    for (int i = 0; i < SIDE; ++i) {
        lines.back().emplace_back(SIDE - i - 1, i);
    }
    return lines;
}();

const unsigned long LINES_SIZE = 2 * SIDE + 2;

struct TicTacToeState : public State<TicTacToeState, TicTacToeMove> {

    vector<char> board;

    TicTacToeState() : State(PLAYER_1) { }

    TicTacToeState(const string &init_string) : State(PLAYER_1) {
        const unsigned long length = init_string.length();
        const unsigned long correct_length = SIDE * SIDE;
        if (length != correct_length) {
            throw invalid_argument("Initialization string length must be " + to_string(correct_length));
        }
        for (int i = 0; i < length; i++) {
            const char c = init_string[i];
            if (c != PLAYER_1 && c != PLAYER_2 && c != EMPTY) {
                throw invalid_argument(string("Undefined symbol used: '") + c + "'");
            }
        }
        board = vector<char>(init_string.begin(), init_string.end());
    }

    TicTacToeState clone() const override {
        TicTacToeState clone = TicTacToeState();
        clone.board = board;
        clone.player_to_move = player_to_move;
        return clone;
    }

    int get_goodness() const override {
        int goodness = 0;
        const auto &counts = count_players_on_lines(player_to_move);
        for (int i = 0; i < LINES_SIZE; ++i) {
            const int player_places = counts[2 * i];
            const int enemy_places = counts[2 * i + 1];
            if (player_places == SIDE) {
                goodness += SIDE * SIDE;
            }
            else if (enemy_places == SIDE) {
                goodness -= SIDE * SIDE;
            }
            else if (player_places == SIDE - 1 and enemy_places == 0) {
                goodness += SIDE;
            }
            else if (enemy_places == SIDE - 1 and player_places == 0) {
                goodness -= SIDE;
            }
            else if (player_places == SIDE - 2 and enemy_places == 0) {
                ++goodness;
            }
            else if (enemy_places == SIDE - 2 and player_places == 0) {
                --goodness;
            }
        }
        return goodness;
    }

    vector<TicTacToeMove> get_legal_moves(int max_moves = INF) const override {
        int available_moves = SIDE * SIDE;
        if (max_moves > available_moves) {
            max_moves = available_moves;
        }
        vector<TicTacToeMove> moves(max_moves);
        int i = 0;
        for (int y = 0; y < SIDE; ++y) {
            for (int x = 0; x < SIDE; ++x) {
                if (board[y * SIDE + x] == EMPTY) {
                    moves[i++] = TicTacToeMove(x, y);
                    if (i >= max_moves) {
                        return moves;
                    }
                }
            }
        }
        moves.resize(i);
        return moves;
    }

    char get_enemy(char player) const override {
        return (player == PLAYER_1) ? PLAYER_2 : PLAYER_1;
    }

    bool is_terminal() const override {
        if (!has_empty_space()) {
            return true;
        }
        const auto &counts = count_players_on_lines(player_to_move);
        for (int i = 0; i < LINES_SIZE; ++i) {
            if (counts[2 * i] == SIDE || counts[2 * i + 1] == SIDE) {
                return true;
            }
        }
        return false;
    }

    bool is_winner(char player) const override {
        const auto &counts = count_players_on_lines(player);
        for (int i = 0; i < LINES_SIZE; ++i) {
            if (counts[2 * i] == SIDE) {
                return true;
            }
        }
        return false;
    }

    void make_move(const TicTacToeMove &move) override {
        board[move.y * SIDE + move.x] = player_to_move;
        player_to_move = get_enemy(player_to_move);
    }

    void undo_move(const TicTacToeMove &move) override {
        board[move.y * SIDE + move.x] = EMPTY;
        player_to_move = get_enemy(player_to_move);
    }

    bool has_empty_space() const {
        for (unsigned y = 0; y < SIDE; ++y) {
            for (unsigned x = 0; x < SIDE; ++x) {
                if (board[y * SIDE + x] == EMPTY) {
                    return true;
                }
            }
        }
        return false;
    }

    vector<int> count_players_on_lines(char player) const {
        vector<int> counts(2 * LINES_SIZE);
        char enemy = get_enemy(player);
        for (int i = 0; i < LINES_SIZE; ++i) {
            int player_places = 0;
            int enemy_places = 0;
            for (int j = 0; j < SIDE; ++j) {
                const TicTacToeMove &coord = LINES[i][j];
                const int board_index = coord.y * SIDE + coord.x;
                if (board[board_index] == player) {
                    ++player_places;
                }
                else if (board[board_index] == enemy) {
                    ++enemy_places;
                }
            }
            counts[2 * i] = player_places;
            counts[2 * i + 1] = enemy_places;
        }
        return counts;
    }

    ostream &to_stream(ostream &os) const override {
        for (int y = 0; y < SIDE; ++y) {
            for (int x = 0; x < SIDE; ++x) {
                os << board[y * SIDE + x];
            }
            os << "\n";
        }
        os << player_to_move << endl;
        return os;
    }

    bool operator==(const TicTacToeState &other) const {
        return board == other.board;
    }

    size_t hash() const {
        using boost::hash_value;
        using boost::hash_combine;
        size_t seed = 0;
        hash_combine(seed, hash_value(board));
        return seed;
    }
};
