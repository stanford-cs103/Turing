#include "TuringParser.h"
#include "TuringScanner.h"
#include <queue>
#include <stack>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
using namespace std;

#define PARSER_IS_VERBOSE (false)

namespace Turing {
    namespace {
      template <typename T> T popFrom(stack<T>& s) {
        if (s.empty()) throw runtime_error("Pop from empty stack.");
        T result = std::move(s.top());
        s.pop();
        return result;
      }

      enum class Nonterminal {
        COMMAND,
    DIRECTION,
    STATEMENT,
    SYMBOL,
    _parserInternalStart,

      };

      /* Type representing an item that can be on the parsing stack (either a
       * terminal or a nonterminal).
       */
      struct Symbol {
        bool isToken;
        union {
          TokenType   tokenType;
          Nonterminal nonterminalType;
        };

        Symbol(TokenType t) {
          isToken = true;
          tokenType = t;
        }
        Symbol(Nonterminal t) {
          isToken = false;
          nonterminalType = t;
        }
      };

      bool operator< (const Symbol& lhs, const Symbol& rhs) {
        if (lhs.isToken != rhs.isToken) return lhs.isToken;
        if (lhs.isToken) return lhs.tokenType < rhs.tokenType;
        return lhs.nonterminalType < rhs.nonterminalType;
      }

      /* Type representing auxiliary data to store in each stack entry. */
      struct AuxData {
        Direction field1;
    char32_t field2;
    std::shared_ptr<Statement> field0;

      };

      /* Type representing data aggregated so far. */
      struct StackData {
        Token     token;  // Only active if the item is a terminal
        AuxData   data;   // Only active if the item is a nonterminal
      };

      /* Type representing an item on the stack. */
      struct StackItem {
        size_t    state;
        StackData data;
      };

      /* Base type for actions. */
      struct Action {
        virtual ~Action() = default;
      };

      struct ShiftAction: Action {
        size_t target;

        ShiftAction(int target) : target(target) {}
      };

      struct HaltAction: Action {

      };

      struct ReduceAction: Action {
        Nonterminal reduceTo;

        /* Does the reduction. */
        virtual void reduce(stack<StackItem>& s) = 0;

        ReduceAction(Nonterminal t) : reduceTo(t) {}
      };

      /* Helper template functions that fire off callbacks with the arguments
       * expanded out into a list.
       */
      template <size_t N> struct DoReduction {
        template <typename Callback, typename... Args>
        static AuxData invoke(stack<StackItem>& s, Callback c, const Args&... args) {
          /* Build arguments up in reverse order, since items are coming off of the stack! */
          auto nextArg = popFrom(s);
          return DoReduction<N - 1>::invoke(s, c, nextArg.data, args...);
        }
      };
      template <> struct DoReduction<0> {
        template <typename Callback, typename... Args>
        static AuxData invoke(stack<StackItem>&, Callback c, const Args&... args) {
          return c(args...);
        }
      };

      /* Utility metafunction that maps from a number N to a callback type that accepts
       * N arguments.
       */
      template <size_t M> struct CallbackType {
        template <size_t N, typename... Args> struct Helper {
          using type = typename Helper<N - 1, Args..., StackData>::type;
        };
        template <typename... Args> struct Helper<0, Args...> {
          using type = std::function<AuxData (Args...)>;
        };
        using type = typename Helper<M>::type;
      };

      /* Use template system so we know how many arguments to forward. */
      template <size_t N> struct ReduceActionN: ReduceAction {
        typename CallbackType<N>::type callback; // Function to invoke with reduced items

        void reduce(stack<StackItem>& s) override;

        ReduceActionN(Nonterminal n, typename CallbackType<N>::type c) : ReduceAction(n), callback(c) {}
      };

      /* Unused argument type. */
      struct _unused_ {};

      Direction reduce_DIRECTION_from_LEFT(const std::string&);
  Direction reduce_DIRECTION_from_RIGHT(const std::string&);
  char32_t reduce_SYMBOL_from_BLANK(const std::string&);
  char32_t reduce_SYMBOL_from_CHAR(const std::string& _parserArg1);
  std::shared_ptr<Statement> reduce_COMMAND_from_ACCEPT(const std::string&);
  std::shared_ptr<Statement> reduce_COMMAND_from_GOTO_LABEL(const std::string&, const std::string& _parserArg2);
  std::shared_ptr<Statement> reduce_COMMAND_from_MOVE_DIRECTION(const std::string&, Direction _parserArg2);
  std::shared_ptr<Statement> reduce_COMMAND_from_PRINT_SYMBOL(const std::string&, char32_t _parserArg2);
  std::shared_ptr<Statement> reduce_COMMAND_from_REJECT(const std::string&);
  std::shared_ptr<Statement> reduce_STATEMENT_from_COMMAND(std::shared_ptr<Statement> _parserArg1);
  std::shared_ptr<Statement> reduce_STATEMENT_from_IF_NOT_SYMBOL_COMMAND(const std::string&, const std::string&, char32_t _parserArg3, std::shared_ptr<Statement> _parserArg4);
  std::shared_ptr<Statement> reduce_STATEMENT_from_IF_SYMBOL_COMMAND(const std::string&, char32_t _parserArg2, std::shared_ptr<Statement> _parserArg3);
  std::shared_ptr<Statement> reduce_STATEMENT_from_LABEL_COLON(const std::string& _parserArg1, const std::string&);


      AuxData reduce_COMMAND_from_ACCEPT__thunk(StackData a0) {
    AuxData result;
    result.field0 = reduce_COMMAND_from_ACCEPT(a0.token.data);
    return result;
  }

  AuxData reduce_COMMAND_from_GOTO_LABEL__thunk(StackData a0, StackData a1) {
    AuxData result;
    result.field0 = reduce_COMMAND_from_GOTO_LABEL(a0.token.data, a1.token.data);
    return result;
  }

  AuxData reduce_COMMAND_from_MOVE_DIRECTION__thunk(StackData a0, StackData a1) {
    AuxData result;
    result.field0 = reduce_COMMAND_from_MOVE_DIRECTION(a0.token.data, a1.data.field1);
    return result;
  }

  AuxData reduce_COMMAND_from_PRINT_SYMBOL__thunk(StackData a0, StackData a1) {
    AuxData result;
    result.field0 = reduce_COMMAND_from_PRINT_SYMBOL(a0.token.data, a1.data.field2);
    return result;
  }

  AuxData reduce_COMMAND_from_REJECT__thunk(StackData a0) {
    AuxData result;
    result.field0 = reduce_COMMAND_from_REJECT(a0.token.data);
    return result;
  }

  AuxData reduce_DIRECTION_from_LEFT__thunk(StackData a0) {
    AuxData result;
    result.field1 = reduce_DIRECTION_from_LEFT(a0.token.data);
    return result;
  }

  AuxData reduce_DIRECTION_from_RIGHT__thunk(StackData a0) {
    AuxData result;
    result.field1 = reduce_DIRECTION_from_RIGHT(a0.token.data);
    return result;
  }

  AuxData reduce_STATEMENT_from_COMMAND__thunk(StackData a0) {
    AuxData result;
    result.field0 = reduce_STATEMENT_from_COMMAND(a0.data.field0);
    return result;
  }

  AuxData reduce_STATEMENT_from_IF_NOT_SYMBOL_COMMAND__thunk(StackData a0, StackData a1, StackData a2, StackData a3) {
    AuxData result;
    result.field0 = reduce_STATEMENT_from_IF_NOT_SYMBOL_COMMAND(a0.token.data, a1.token.data, a2.data.field2, a3.data.field0);
    return result;
  }

  AuxData reduce_STATEMENT_from_IF_SYMBOL_COMMAND__thunk(StackData a0, StackData a1, StackData a2) {
    AuxData result;
    result.field0 = reduce_STATEMENT_from_IF_SYMBOL_COMMAND(a0.token.data, a1.data.field2, a2.data.field0);
    return result;
  }

  AuxData reduce_STATEMENT_from_LABEL_COLON__thunk(StackData a0, StackData a1) {
    AuxData result;
    result.field0 = reduce_STATEMENT_from_LABEL_COLON(a0.token.data, a1.token.data);
    return result;
  }

  AuxData reduce_SYMBOL_from_BLANK__thunk(StackData a0) {
    AuxData result;
    result.field2 = reduce_SYMBOL_from_BLANK(a0.token.data);
    return result;
  }

  AuxData reduce_SYMBOL_from_CHAR__thunk(StackData a0) {
    AuxData result;
    result.field2 = reduce_SYMBOL_from_CHAR(a0.token.data);
    return result;
  }



      /* Action table. */
      const vector<map<Symbol, Action*>> kActionTable = {
      {
  {    TokenType::ACCEPT, new ShiftAction{18} },
  {    Nonterminal::COMMAND, new ShiftAction{22} },
  {    TokenType::GOTO, new ShiftAction{15} },
  {    TokenType::IF, new ShiftAction{13} },
  {    TokenType::LABEL, new ShiftAction{11} },
  {    TokenType::MOVE, new ShiftAction{7} },
  {    TokenType::PRINT, new ShiftAction{3} },
  {    TokenType::REJECT, new ShiftAction{2} },
  {    Nonterminal::STATEMENT, new ShiftAction{1} },
},
{
  {    TokenType::SCAN_EOF, new HaltAction() },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::COMMAND, reduce_COMMAND_from_REJECT__thunk) },
},
{
  {    TokenType::BLANK, new ShiftAction{6} },
  {    TokenType::CHAR, new ShiftAction{5} },
  {    Nonterminal::SYMBOL, new ShiftAction{4} },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<2>(Nonterminal::COMMAND, reduce_COMMAND_from_PRINT_SYMBOL__thunk) },
},
{
  {    TokenType::ACCEPT, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_CHAR__thunk) },
  {    TokenType::GOTO, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_CHAR__thunk) },
  {    TokenType::MOVE, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_CHAR__thunk) },
  {    TokenType::PRINT, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_CHAR__thunk) },
  {    TokenType::REJECT, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_CHAR__thunk) },
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_CHAR__thunk) },
},
{
  {    TokenType::ACCEPT, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_BLANK__thunk) },
  {    TokenType::GOTO, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_BLANK__thunk) },
  {    TokenType::MOVE, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_BLANK__thunk) },
  {    TokenType::PRINT, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_BLANK__thunk) },
  {    TokenType::REJECT, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_BLANK__thunk) },
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::SYMBOL, reduce_SYMBOL_from_BLANK__thunk) },
},
{
  {    Nonterminal::DIRECTION, new ShiftAction{10} },
  {    TokenType::LEFT, new ShiftAction{9} },
  {    TokenType::RIGHT, new ShiftAction{8} },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::DIRECTION, reduce_DIRECTION_from_RIGHT__thunk) },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::DIRECTION, reduce_DIRECTION_from_LEFT__thunk) },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<2>(Nonterminal::COMMAND, reduce_COMMAND_from_MOVE_DIRECTION__thunk) },
},
{
  {    TokenType::COLON, new ShiftAction{12} },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<2>(Nonterminal::STATEMENT, reduce_STATEMENT_from_LABEL_COLON__thunk) },
},
{
  {    TokenType::BLANK, new ShiftAction{6} },
  {    TokenType::CHAR, new ShiftAction{5} },
  {    TokenType::NOT, new ShiftAction{19} },
  {    Nonterminal::SYMBOL, new ShiftAction{14} },
},
{
  {    TokenType::ACCEPT, new ShiftAction{18} },
  {    Nonterminal::COMMAND, new ShiftAction{17} },
  {    TokenType::GOTO, new ShiftAction{15} },
  {    TokenType::MOVE, new ShiftAction{7} },
  {    TokenType::PRINT, new ShiftAction{3} },
  {    TokenType::REJECT, new ShiftAction{2} },
},
{
  {    TokenType::LABEL, new ShiftAction{16} },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<2>(Nonterminal::COMMAND, reduce_COMMAND_from_GOTO_LABEL__thunk) },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<3>(Nonterminal::STATEMENT, reduce_STATEMENT_from_IF_SYMBOL_COMMAND__thunk) },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::COMMAND, reduce_COMMAND_from_ACCEPT__thunk) },
},
{
  {    TokenType::BLANK, new ShiftAction{6} },
  {    TokenType::CHAR, new ShiftAction{5} },
  {    Nonterminal::SYMBOL, new ShiftAction{20} },
},
{
  {    TokenType::ACCEPT, new ShiftAction{18} },
  {    Nonterminal::COMMAND, new ShiftAction{21} },
  {    TokenType::GOTO, new ShiftAction{15} },
  {    TokenType::MOVE, new ShiftAction{7} },
  {    TokenType::PRINT, new ShiftAction{3} },
  {    TokenType::REJECT, new ShiftAction{2} },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<4>(Nonterminal::STATEMENT, reduce_STATEMENT_from_IF_NOT_SYMBOL_COMMAND__thunk) },
},
{
  {    TokenType::SCAN_EOF, new ReduceActionN<1>(Nonterminal::STATEMENT, reduce_STATEMENT_from_COMMAND__thunk) },
},

      };

      template <size_t N>
        void ReduceActionN<N>::reduce(stack<StackItem>& s) {
          #if PARSER_IS_VERBOSE
            cout << "  Should pop " << N << " items from the stack." << endl;
            cout << "    Before: " << s.size() << endl;
          #endif
          /* Invoke the callback on the proper arguments, getting the AuxData
           * to push onto the stack.
           */
          auto nextItem = DoReduction<N>::invoke(s, callback);
          #if PARSER_IS_VERBOSE
            cout << "    After: " << s.size() << endl;
          #endif

          int state = s.top().state;
          #if PARSER_IS_VERBOSE
            cout << "  Top state is " << state << endl;
          #endif

          /* Look up the shift destination for this nonterminal. */
          auto* action = static_cast<ShiftAction*>(kActionTable[state].at(reduceTo));
          #if PARSER_IS_VERBOSE
            cout << "  Should shift to state " << action->target << endl;
          #endif

          /* Push this onto the stack. */
          s.push({ action->target, { { }, nextItem } });
      }

      /* Internal parsing routine */
      AuxData parseInternal(queue<Token>& tokens) {
        stack<StackItem> s;

        /* Seed the stack with the initial state. */
        s.push({ 0, {} });

        /* Run the parser! */
        while (!tokens.empty()) {
          /* Look at the next token. We only consume it in a shift. */
          auto curr  = tokens.front();
          int  state = s.top().state;

          #if PARSER_IS_VERBOSE
            cout << "Current state: " << state << endl;
            cout << "  Symbol: " << curr.data << endl;
            cout << endl;
          #endif

          /* Look up the action to take; if there is no action, it's an error. */
          if (!kActionTable[state].count(curr.type)) {
            if (curr.type == TokenType::SCAN_EOF) {
              throw runtime_error("End of formula encountered unexpectedly. (Are you missing a close parenthesis?)");
            }
            throw runtime_error("Found \"" + to_string(curr) + "\" where it wasn't expected.");
          }

          /* See what action to take. */
          auto action = kActionTable[state].at(curr.type);
          if (auto* shift = dynamic_cast<ShiftAction*>(action)) {
            #if PARSER_IS_VERBOSE
              cout << "  Action: Shift to " << shift->target << endl;
            #endif
            s.push( { shift->target, {curr, {}} } ); // No special data.
            tokens.pop();
          } else if (auto* reduce = dynamic_cast<ReduceAction*>(action)) {
            #if PARSER_IS_VERBOSE
              cout << "  Action: Reduce" << endl;
            #endif
            reduce->reduce(s);
          } else if (dynamic_cast<HaltAction*>(action)) {
            #if PARSER_IS_VERBOSE
              cout << "  Action: Halt" << endl;
            #endif
            return s.top().data.data;
          } else {
            throw runtime_error("Unknown action.");
          }
        }

        throw runtime_error("Out of tokens, but parser hasn't finished.");
      }

      Direction reduce_DIRECTION_from_LEFT(const std::string&) {
    Direction _parserArg0;
    _parserArg0 = Direction::LEFT;
    return _parserArg0;
  }

  Direction reduce_DIRECTION_from_RIGHT(const std::string&) {
    Direction _parserArg0;
    _parserArg0 = Direction::RIGHT;
    return _parserArg0;
  }

  char32_t reduce_SYMBOL_from_BLANK(const std::string&) {
    char32_t _parserArg0;
    _parserArg0 = kBlankSymbol;
    return _parserArg0;
  }

  char32_t reduce_SYMBOL_from_CHAR(const std::string& _parserArg1) {
    char32_t _parserArg0;
    _parserArg0 = fromUTF8(_parserArg1);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_COMMAND_from_ACCEPT(const std::string&) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<Halt>(true);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_COMMAND_from_GOTO_LABEL(const std::string&, const std::string& _parserArg2) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<Goto>(_parserArg2);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_COMMAND_from_MOVE_DIRECTION(const std::string&, Direction _parserArg2) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<Move>(_parserArg2);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_COMMAND_from_PRINT_SYMBOL(const std::string&, char32_t _parserArg2) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<Print>(_parserArg2);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_COMMAND_from_REJECT(const std::string&) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<Halt>(false);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_STATEMENT_from_COMMAND(std::shared_ptr<Statement> _parserArg1) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = _parserArg1;
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_STATEMENT_from_IF_NOT_SYMBOL_COMMAND(const std::string&, const std::string&, char32_t _parserArg3, std::shared_ptr<Statement> _parserArg4) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<If>(true, _parserArg3, _parserArg4);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_STATEMENT_from_IF_SYMBOL_COMMAND(const std::string&, char32_t _parserArg2, std::shared_ptr<Statement> _parserArg3) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<If>(false, _parserArg2, _parserArg3);
    return _parserArg0;
  }

  std::shared_ptr<Statement> reduce_STATEMENT_from_LABEL_COLON(const std::string& _parserArg1, const std::string&) {
    std::shared_ptr<Statement> _parserArg0;
    _parserArg0 = make_shared<Label>(_parserArg1);
    return _parserArg0;
  }


    }

    /* Public parsing routine. */
    std::shared_ptr<Statement> parse(queue<Token>& q) {
      return parseInternal(q).field0;
    }
    std::shared_ptr<Statement> parse(queue<Token>&& q) {
      return parseInternal(q).field0;
    }
}
