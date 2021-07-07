/******************************************************************************
 * Turing programs. This is modeled after Hao Wang's B-machine, but due to the
 * similarity with how TMs work we're calling them "Turing Programs."
 *
 * Programs consist of several lines. Each line can be one of the following:
 *
 * # Comment
 * Name of a label:
 * Move Left
 * Move Right
 * Print [ch]
 * Goto Label
 * Accept
 * Reject
 * If [ch] Then [non-if-statement]
 * If Not [ch] Then [non-if-statement]
 *
 * Here, [ch] represents a character and can be one of 'single-letter' or Blank
 *
 */
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <istream>

namespace Turing {
    class Statement;
    class Interpreter;

    /* Blank character. */
    const char32_t kBlankSymbol = 0;

    /* Result of running the interpreter. */
    enum class Result {
        ACCEPT, REJECT, RUNNING
    };

    constexpr char kStartLabel[] = "Start";

    class Program {
    public:
        Program(std::istream& in);

        /* Accessors. */
        size_t numLines() const;
        std::string line(size_t lineNo) const;

        /* Runs the program for the given number of steps. */
        Result runFor(const std::vector<char32_t>& input, size_t numSteps) const;

    private /* state */:
        /* Raw lines of the program. */
        std::vector<std::string> rawLines_;

        /* Map from lines to statements. */
        std::map<size_t, std::shared_ptr<Statement>> statements_;

        /* Map from labels to line numbers. */
        std::map<std::string, size_t> labels_;

        friend class Interpreter;


    private /* helpers */:
        void parse(std::istream& in);
        void semanticAnalyze();
    };

    class Visitor;

    class Statement {
    public:
        virtual ~Statement() = default;

        /* Executes this line of the program. */
        virtual void accept(Visitor& v) = 0;
    };


    /*** Statement Types ***/

    class Label: public Statement {
    public:
        Label(const std::string& label) : label_(label) {}

        std::string label() const {
            return label_;
        }

        virtual void accept(Visitor& v) override;

    private:
        std::string label_;
    };

    enum class Direction {
        LEFT, RIGHT
    };

    class Move: public Statement {
    public:
        Move(Direction direction) : direction_(direction) {}

        Direction direction() const {
            return direction_;
        }

        virtual void accept(Visitor& v) override;

    private:
        Direction direction_;
    };

    class Print: public Statement {
    public:
        Print(char32_t ch) : ch_(ch) {};

        char32_t ch() const {
            return ch_;
        }

        virtual void accept(Visitor& v) override;

    private:
        char32_t ch_;
    };

    class Goto: public Statement {
    public:
        Goto(const std::string& label) : label_(label) {}

        std::string label() const {
            return label_;
        }

        virtual void accept(Visitor& v) override;

    private:
        std::string label_;
    };

    class Halt: public Statement {
    public:
        Halt(bool isAccept) : isAccept_(isAccept) {}

        virtual void accept(Visitor& v) override;

        bool isAccepting() const {
            return isAccept_;
        }

    private:
        bool isAccept_;
    };

    class If: public Statement {
    public:
        If(bool isNegated, char32_t ch, std::shared_ptr<Statement> stmt) : isNegated_(isNegated), ch_(ch), stmt_(stmt) {}

        bool isNegated() const {
            return isNegated_;
        }

        char32_t ch() const {
            return ch_;
        }

        std::shared_ptr<Statement> stmt() const {
            return stmt_;
        }

        virtual void accept(Visitor& v) override;

    private:
        bool isNegated_;
        char32_t ch_;
        std::shared_ptr<Statement> stmt_;
    };

    /* Visitor type. */
    class Visitor {
    public:
        virtual ~Visitor() = default;

        virtual void visit(Label&) {}
        virtual void visit(Move&) {}
        virtual void visit(Print&) {}
        virtual void visit(Halt&) {}
        virtual void visit(If&) {}
        virtual void visit(Goto&) {}
    };

    template <typename T> class Calculator: public Visitor {
    public:
        T calculate(Statement& stmt) {
            stmt.accept(*this);
            return result_;
        }

        virtual T handle(Label&) { return T(); }
        virtual T handle(Move&) { return T(); }
        virtual T handle(Print&) { return T(); }
        virtual T handle(Halt&) { return T(); }
        virtual T handle(If&, const T&) { return T(); }
        virtual T handle(Goto&) { return T(); }

    private:
        virtual void visit(Label& l) override final {
            result_ = handle(l);
        }

        virtual void visit(Move& m) override final {
            result_ = handle(m);
        }
        virtual void visit(Print& p) override final {
            result_ = handle(p);
        }
        virtual void visit(Halt& h) override final {
            result_ = handle(h);
        }
        virtual void visit(If& i) override final {
            i.stmt()->accept(*this);
            T temp = result_; // Avoid reference shenanigans
            result_ = handle(i, temp);
        }
        virtual void visit(Goto& g) override final {
            result_ = handle(g);
        }

        T result_;
    };
}
