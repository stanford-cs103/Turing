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
    void Write::accept(Visitor& v) {
        v.visit(*this);
    }
    void Move::accept(Visitor& v) {
        v.visit(*this);
    }
    void Return::accept(Visitor& v) {
        v.visit(*this);
    }
    void Goto::accept(Visitor& v) {
        v.visit(*this);
    }
    void If::accept(Visitor& v) {
        v.visit(*this);
    }

    /* Parsing. */

    Program::Program(istream& in) {
        parse(in);
        semanticAnalyze();
    }

    Program::Program(istream&& in) : Program(in) {

    }

    namespace {
        /* Removes comments and leading/trailing whitespace.
         * Comments begin with the # character. We have to be
         * careful when implementing this to make sure that we
         * don't treat the quoted string '#' as a comment.
         */
        void cleanLine(string& line) {
            /* Search for a comment. This will find the first # mark. It might be
             * in quotes, in which case we should ignore it
             * and search again.
             */
            size_t comment = line.find('#');
            if (comment != string::npos &&
                comment != 0 && comment + 1 != line.size() &&
                line[comment - 1] == '\'' &&
                line[comment + 1] == '\'') {
                /* Look for the next one. */
                comment = line.find(comment + 1);
            }

            /* Now if we have a hash mark, it's definitely a comment. */
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

            /* Parse the line. If that fails, record the error message. */
            try {
                auto stmt = Turing::parse(scan(line));

                /* Stash it for later. */
                statements_[rawLines_.size() - 1] = stmt;
            } catch (const exception& e) {
                errors_[rawLines_.size() - 1] = e.what();
            }
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
                    errors_[entry.first] = "Duplicate label; this was first defined at line " + std::to_string(result.first->second);
                }
            }
        }

        /* Make sure there's a start label. */
        if (!labels_.count(kStartLabel)) {
            /* Associate the error with Line 0, if there isn't already an error there. */
            if (!errors_.count(0)) {
                errors_[0] = "This program needs a " + string(kStartLabel) + " label so we know where to begin.";
            }
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
        set<string> jumpTargets;
        for (const auto& entry: statements_) {
            string label = LabelsUsed().calculate(*entry.second);
            if (label != "" && !labels_.count(label)) {
                errors_[entry.first] = "Goto statement references undefined label '" + label + "'.";
            }

            if (label != "") {
                jumpTargets.insert(label);
            }
        }
    }

    /* The program is valid if there are no errors. */
    bool Program::isValid() const {
        return errors_.empty();
    }

    /* See if we have a known error at the given line. */
    string Program::errorAtLine(size_t line) const {
        auto itr = errors_.find(line);
        return itr == errors_.end()? "" : itr->second;
    }

    /* As advertised. */
    size_t Program::numLines() const {
        return rawLines_.size();
    }

    string Program::line(size_t i) const {
        return rawLines_.at(i);
    }
    
    shared_ptr<Statement> Program::statement(size_t i) const {
        auto itr = statements_.find(i);
        return itr != statements_.end()? itr->second : nullptr;
    }
    
    size_t Program::lineForLabel(const string& label) const {
        auto itr = labels_.find(label);
        return itr != labels_.end()? itr->second : -1;
    }

    /* Interpreter setup. We need to do the following:
     *
     * 1. Set up the tape by copying over the initial contents.
     * 2. Figure out our starting line number by locating the Start label.
     */
    Interpreter::Interpreter(const Program& p, const vector<char32_t>& input) :
        p_(&p), tape_(input.begin(), input.end()) {
        /* If there are any errors in the program, abort because we can't run the
         * program.
         */
        if (!p.isValid()) {
            throw invalid_argument("Cannot interpret a program that contains errors.");
        }

        /* Place an extra blank cell at the end of the tape. This avoids edge cases by ensuring
         * that the tape position always points at something.
         */
        tape_.push_back(kBlankSymbol);

        /* Look up the start symbol and begin there. */
        lineNumber_ = p.labels_.at(kStartLabel);
    }

    /* As advertised. */
    Result Interpreter::state() const {
        return state_;
    }

    /* Change from deque coordinates to world coordinates. */
    int64_t Interpreter::tapeHeadPos() const {
        return int64_t(tapePos_) + dequeBase_;
    }
    
    /* Change from deque coordinates to world coordinates. */
    pair<int64_t, int64_t> Interpreter::usedTapeRange() const {
        return make_pair(dequeBase_, dequeBase_ + int64_t(tape_.size()));
    }

    /* Jump to the given label. */
    void Interpreter::jumpTo(const string& label) {
        lineNumber_ = p_->labels_.at(label);
    }

    /* Skip to next executable line. */
    void Interpreter::toNextLine() {
        /* Keep incrementing the line number until we walk off the end or find
         * a statement.
         */
        do {
            lineNumber_++;
        } while (lineNumber_ < p_->rawLines_.size() && !p_->statements_.count(lineNumber_));
    }

    char32_t Interpreter::tapeAt(int64_t index) const {
        /* Convert to deque index by subtracting out the deque base. */
        index -= dequeBase_;

        if (index >= 0 && size_t(index) < tape_.size()) {
            return tape_[size_t(index)];
        } else {
            return kBlankSymbol;
        }
    }

    /* Advance the simulation forward one step. */
    void Interpreter::step() {
        /* If we're not running, don't do anything. */
        if (state() != Result::RUNNING) return;

        /* Cache the current line; we'll need to do this to execute jumps. */
        size_t lineBefore = lineNumber_;
        auto stmt = p_->statements_.at(lineNumber_);

        /* Run the statement. If it terminated the program, stop. */
        auto result = execute(*stmt);
        if (result != Result::RUNNING) {
            state_ = result;
            return;
        }

        /* If we didn't execute a jump, move to the next line. */
        if (lineBefore == lineNumber_) {
            toNextLine();
        }

        /* If we're past the end of the program, reject.
         *
         * TODO: You may be able to safely remove this if you add in a requirement that
         * all programs must end with Accept, Reject, or Goto.
         */
        if (lineNumber_ == p_->numLines()) {
            state_ = Result::REJECT;
        }
    }

    /* Visitor that executes each command. */
    class StatementExecutor: public Visitor {
    public:
        StatementExecutor(Interpreter& me, Result& result) : me_(me), result_(result) {}

        /* Print writes a character. */
        void visit(Write& p) override {
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
                /* Nothing before me. Shift something new in. */
                else {
                    me_.tape_.push_front(kBlankSymbol);

                    /* The deque's starting position is now lower. */
                    me_.dequeBase_--;
                }
            } else throw runtime_error("Unknown direction?");
        }

        /* Goto executes a jump. */
        void visit(Goto& g) override {
            me_.jumpTo(g.label());
        }

        /* Halt stops the program. */
        void visit(Return& h) override {
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

    private /* data */:
        Interpreter& me_;
        Result& result_;
    };

    /* Execute a single command. */
    Result Interpreter::execute(Statement& stmt) {
        Result result = Result::RUNNING;

        StatementExecutor runner(*this, result);
        stmt.accept(runner);
        return result;
    }
}
