#include "Turing.h"
#include "TuringScanner.h"
#include "TuringParser.h"
#include "StrUtils/StrUtils.h"
#include "Utilities/Unicode.h"
#include <deque>
#include <sstream>
#include <iostream>
#include <string>
#include <set>
#include <climits>
using namespace std;

namespace Turing {
    /* Visitor implementations */
    void Label::accept(Visitor& v) {
        v.visit(*this);
    }
    void Print::accept(Visitor& v) {
        v.visit(*this);
    }
    void Move::accept(Visitor& v) {
        v.visit(*this);
    }
    void Halt::accept(Visitor& v) {
        v.visit(*this);
    }
    void Goto::accept(Visitor& v) {
        v.visit(*this);
    }
    void If::accept(Visitor& v) {
        v.visit(*this);
    }

    /* Pretty-printers. */
    namespace {
        /* Character -> String, in a pretty-printy way. */
        std::string toString(char32_t ch) {
            if (ch == Turing::kBlankSymbol) return "Blank";

            return "'" + toUTF8(ch) + "'";
        }

        /* Direction -> String */
        std::string toString(Direction d) {
            if (d == Direction::LEFT) return "Left";
            if (d == Direction::RIGHT) return "Right";
            return "<unknown direction>";
        }

        /* Expression -> String, in a pretty-printy way. */
        std::string toString(std::shared_ptr<Statement> stmt) {
            class Printer: public Calculator<string> {
                string handle(Label& l) override {
                    return l.label() + ":";
                }
                string handle(Move& m) override {
                    return "Move " + toString(m.direction());
                }
                string handle(Halt& h) override {
                    return h.isAccepting()? "Accept" : "Reject";
                }
                string handle(Goto& g) override {
                    return "Goto " + g.label();
                }
                string handle(Print& p) override {
                    return "Print " + toString(p.ch());
                }
                string handle(If& i, const string& expr) override {
                    return string("If ") + (i.isNegated()? "Not " : "") + toString(i.ch()) + " " + expr;
                }
            };

            return Printer().calculate(*stmt);
        }
    }

    /* Parsing. */

    Program::Program(istream& in) {
        parse(in);
        semanticAnalyze();
    }

    namespace {
        /* Removes comments and leading/trailing whitespace. */
        void cleanLine(string& line) {
            size_t comment = line.find('#');
            if (comment != string::npos) {
                line.erase(comment);
            }

            line = Utilities::trim(line);
        }
    }

    /* Read the program one line at a time, stripping out comments and handing off the
     * line to the parser for AST assembly.
     */
    void Program::parse(istream& in) {
        for (string line; getline(in, line); ) {
            /* Store the line for later. */
            rawLines_.push_back(line);

            /* Remove comments and whitespace, if any. */
            cleanLine(line);

            /* Blank? Skip it. */
            if (line.empty()) continue;

            /* Parse the line. */
            auto stmt = Turing::parse(scan(line));
            //cout << toString(stmt) << endl;

            /* Stash it for later. */
            statements_[rawLines_.size() - 1] = stmt;
        }
    }

    /* Semantic analysis phase. We need to ensure that all labels are known, that no duplicated
     * labels exist, and that the label targets are all valid.
     */
    void Program::semanticAnalyze() {
        /* Find all labels and write them down. */
        for (const auto& entry: statements_) {
            if (auto label = dynamic_pointer_cast<Label>(entry.second)) {
                /* Add it, failing if it already exists. */
                auto result = labels_.insert(make_pair(label->label(), entry.first));
                if (!result.second) {
                    throw runtime_error("Duplicate label: " + label->label());
                }
            }
        }

        /* Make sure there's a start label. */
        if (!labels_.count(kStartLabel)) {
            throw runtime_error("No start label was found.");
        }

        /* Find all used labels and make sure they exist. */

        /* Empty string corresponds to "no label." */
        class LabelsUsed: public Calculator<string> {
            string handle(Goto& g) override {
                return g.label();
            }
            string handle(If&, const string& expr) override {
                return expr;
            }
        };
        for (const auto& entry: statements_) {
            string label = LabelsUsed().calculate(*entry.second);
            if (label != "" && !labels_.count(label)) {
                throw runtime_error("Goto statement references undefined label '" + label + "'.");
            }
        }
    }

    /* Interpretation (the Universal Turing Machine, kinda sorta. ^_^) */
    class Interpreter {
    public:
        Interpreter(const Program& p, const vector<char32_t>& input);

        /* Current line number. */
        size_t lineNumber() const {
            return lineNumber_;
        }

        /* Returns the tape character the indicated number of steps away from the
         * head, which can be in either a positive or negative direction.
         */
        char32_t tapeAt(int relSteps) const;

        /* Advances one step forward, returning a Result indicating the
         * current state. Calling this function after a halt instruction
         * has been executed is a Bad Thing.
         */
        Result step();

        /* Friendship with Program requires us to export these for
         * other types that need them.
         */
        void jumpTo(const string& label);

    private /* data */:
        /* Reference program. */
        const Program& p_;

        /* Which line to execute next. */
        size_t lineNumber_;

        /* Tape contents and tape head position. */
        deque<char32_t> tape_;
        size_t tapePos_ = 0;

    private /* helpers */:
        /* Steps the program counter forward to the next line after the
         * current one, skipping comments.
         */
        void toNextLine();

        /* Executes the given command. */
        Result execute(Statement& stmt);
    };

    namespace {
        void printTape(const Interpreter& interpreter) {
            for (int i = -20; i <= 20; i++) {
                char32_t ch = interpreter.tapeAt(i);

                if (ch != kBlankSymbol) cout << toUTF8(ch);
                else cout << ' ';
            }
            cout << '\n';
            for (int i = -20; i < 0; i++) {
                cout << ' ';
            }
            cout << '^' << '\n';
        }
    }

    /* The runFor function just steps the interpreter forward the appropriate number of times. */
    Result Program::runFor(const std::vector<char32_t>& input, size_t numSteps) const {
        Interpreter interpreter(*this, input);
        //printTape(interpreter);

        for (size_t i = 0; i < numSteps; i++) {
            if (i % 1000000 == 0) {
                cout << i << endl;
            }
            //cout << setw(4) << interpreter.lineNumber() << ": " << rawLines_[interpreter.lineNumber()] << '\n';

            auto result = interpreter.step();
            if (result != Result::RUNNING) return result;

            //printTape(interpreter);
        }

        return Result::RUNNING;
    }

    /* Interpreter setup. We need to do the following:
     *
     * 1. Set up the tape by copying over the initial contents.
     * 2. Figure out our starting line number by locating the Start label.
     */
    Interpreter::Interpreter(const Program& p, const vector<char32_t>& input) :
        p_(p), tape_(input.begin(), input.end()) {
        /* Place an extra blank cell at the end of the tape. This avoids edge cases by ensuring
         * that the tape position always points at something.
         */
        tape_.push_back(kBlankSymbol);

        /* Look up the start symbol and begin there. */
        lineNumber_ = p.labels_.at(kStartLabel);
    }

    /* Jump to the given label. */
    void Interpreter::jumpTo(const string& label) {
        lineNumber_ = p_.labels_.at(label);
    }

    /* Skip to next executable line. */
    void Interpreter::toNextLine() {
        /* Keep incrementing the line number until we walk off the end or find
         * a statement.
         */
        do {
            lineNumber_++;
        } while (lineNumber_ < p_.rawLines_.size() && !p_.statements_.count(lineNumber_));
    }

    char32_t Interpreter::tapeAt(int relSteps) const {
        if (relSteps >= 0) {
            size_t index = tapePos_ + relSteps;
            return index < tape_.size()? tape_[index] : kBlankSymbol;
        } else {
            return tapePos_ < -relSteps? kBlankSymbol : tape_[tapePos_ + relSteps];
        }
    }

    /* Advance the simulation forward one step. */
    Result Interpreter::step() {
        /* If the current line is past the end of the program, reject. */
        if (lineNumber_ == p_.rawLines_.size()) {
            return Result::REJECT;
        }

        /* Cache the current line; we'll need to do this to execute jumps. */
        size_t lineBefore = lineNumber_;
        auto stmt = p_.statements_.at(lineNumber_);

        /* Run the statement. If it terminated the program, stop. */
        auto result = execute(*stmt);
        if (result != Result::RUNNING) {
            return result;
        }

        /* If we didn't execute a jump, move to the next line. */
        if (lineBefore == lineNumber_) {
            toNextLine();

            /* Stop running if that was the last line. */
            if (lineNumber_ == p_.rawLines_.size()) {
                return Result::REJECT;
            }
        }

        return Result::RUNNING;
    }

    /* Execute a single command. */
    Result Interpreter::execute(Statement& stmt) {
        Result result = Result::RUNNING;

        /* How to execute each command. */
        class Runner: public Visitor {
        public:
            Runner(Interpreter& me, Result& result) : me_(me), result_(result) {}
            Interpreter& me_;
            Result& result_;

            /* Print writes a character. */
            void visit(Print& p) override {
                me_.tape_[me_.tapePos_] = p.ch();
            }

            /* Move moves the tape head, handling boundaries as appropriate. */
            void visit(Move& m) override {
                if (m.direction() == Direction::RIGHT) {
                    me_.tapePos_++;
                    if (me_.tapePos_ == me_.tape_.size()) {
                        me_.tape_.push_back(kBlankSymbol);
                    }
                } else if (m.direction() == Direction::LEFT) {
                    /* Something before me? Back up. */
                    if (me_.tapePos_ > 0) {
                        me_.tapePos_ --;
                    }
                    /* Nothign before me. Shift something new in. */
                    else {
                        me_.tape_.push_front(kBlankSymbol);
                    }
                } else throw runtime_error("Unknown direction?");
            }

            /* Goto executes a jump. */
            void visit(Goto& g) override {
                me_.jumpTo(g.label());
            }

            /* Halt stops the program. */
            void visit(Halt& h) override {
                result_ = h.isAccepting()? Result::ACCEPT : Result::REJECT;
            }

            /* If statements check the condition and react appropriately. */
            void visit(If& h) override {
                /* Check whether the tape symbol matches/doesn't match, then fire off
                 * the command if we're supposed to.
                 */
                if ((me_.tape_[me_.tapePos_] == h.ch()) != h.isNegated()) {
                    h.stmt()->accept(*this);
                }
            }

        } runner(*this, result);

        stmt.accept(runner);
        return result;
    }
}
