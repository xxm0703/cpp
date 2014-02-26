/** This class implements a skeleton table driven LR parser.  In general,
 *  LR parsers are a form of bottom up shift-reduce parsers.  Shift-reduce
 *  parsers act by shifting input onto a parse stack until the Symbols
 *  matching the right hand side of a production appear on the top of the 
 *  stack.  Once this occurs, a reduce is performed.  This involves removing
 *  the Symbols corresponding to the right hand side of the production
 *  (the so called "handle") and replacing them with the non-terminal from
 *  the left hand side of the production.  <p>
 *
 *  To control the decision of whether to shift or reduce at any given point,
 *  the parser uses a state machine (the "viable prefix recognition machine" 
 *  built by the parser generator).  The current state of the machine is placed
 *  on top of the parse stack (stored as part of a Symbol object representing
 *  a terminal or non terminal).  The parse action table is consulted 
 *  (using the current state and the current lookahead Symbol as indexes) to 
 *  determine whether to shift or to reduce.  When the parser shifts, it 
 *  changes to a new state by pushing a new Symbol (containing a new state) 
 *  onto the stack.  When the parser reduces, it pops the handle (right hand 
 *  side of a production) off the stack.  This leaves the parser in the state
 *  it was in before any of those Symbols were matched.  Next the reduce-goto 
 *  table is consulted (using the new state and current lookahead Symbol as
 *  indexes) to determine a new state to go to.  The parser then shifts to
 *  this goto state by pushing the left hand side Symbol of the production
 *  (also containing the new state) onto the stack.<p>
 *
 *  This class actually provides four LR parsers.  The methods parse() and 
 *  debug_parse() provide two versions of the main parser (the only difference 
 *  being that debug_parse() emits debugging trace messages as it parses).
 *  In addition to these main parsers, the error recovery mechanism uses two 
 *  more.  One of these is used to simulate "parsing ahead" in the input 
 *  without carrying out actions (to verify that a potential error recovery 
 *  has worked), and the other is used to parse through buffered "parse ahead" 
 *  input in order to execute all actions and re-synchronize the actual parser 
 *  configuration.<p>
 *
 *  This is an abstract class which is normally filled out by a subclass
 *  generated by the JavaCup parser generator.  In addition to supplying
 *  the actual parse tables, generated code also supplies methods which 
 *  invoke various pieces of user supplied code, provide access to certain
 *  special Symbols (e.g., EOF and error), etc.  Specifically, the following
 *  abstract methods are normally supplied by generated code:
 *  <dl compact>
 *  <dt> short[][] production_table()
 *  <dd> Provides a reference to the production table (indicating the index of
 *       the left hand side non terminal and the length of the right hand side
 *       for each production in the grammar).
 *  <dt> short[][] action_table()
 *  <dd> Provides a reference to the parse action table.
 *  <dt> short[][] reduce_table()
 *  <dd> Provides a reference to the reduce-goto table.
 *  <dt> int start_state()      
 *  <dd> Indicates the index of the start state.
 *  <dt> int start_production() 
 *  <dd> Indicates the index of the starting production.
 *  <dt> int EOF_sym()
 *  <dd> Indicates the index of the EOF Symbol.
 *  <dt> int error_sym() 
 *  <dd> Indicates the index of the error Symbol.
 *  <dt> Symbol do_action() 
 *  <dd> Executes a piece of user supplied action code.  This always comes at 
 *       the point of a reduce in the parse, so this code also allocates and
 *       fills in the left hand side non terminal Symbol object that is to be 
 *       pushed onto the stack for the reduce.
 *  <dt> void init_actions()
 *  <dd> Code to initialize a special object that encapsulates user supplied
 *       actions (this object is used by do_action() to actually carry out the
 *       actions).
 *  </dl>
 *  
 *  In addition to these routines that <i>must</i> be supplied by the
 *  generated subclass there are also a series of routines that <i>may</i> 
 *  be supplied.  These include:
 *  <dl>
 *  <dt> Symbol scan()
 *  <dd> Used to get the next input Symbol from the scanner.
 *  <dt> Scanner getScanner()
 *  <dd> Used to provide a scanner for the default implementation of
 *       scan().
 *  <dt> int error_sync_size()
 *  <dd> This determines how many Symbols past the point of an error 
 *       must be parsed without error in order to consider a recovery to
 *       be valid.  This defaults to 3.  Values less than 2 are not
 *       recommended.
 *  <dt> void report_error(String message, Object info)
 *  <dd> This method is called to report an error.  The default implementation
 *       simply prints a message to System.err and where the error occurred.
 *       This method is often replaced in order to provide a more sophisticated
 *       error reporting mechanism.
 *  <dt> void report_fatal_error(String message, Object info)
 *  <dd> This method is called when a fatal error that cannot be recovered from
 *       is encountered.  In the default implementation, it calls
 *       report_error() to emit a message, then throws an exception.
 *  <dt> void syntax_error(Symbol cur_token)
 *  <dd> This method is called as soon as syntax error is detected (but
 *       before recovery is attempted).  In the default implementation it
 *       invokes: report_error("Syntax error", null);
 *  <dt> void unrecovered_syntax_error(Symbol cur_token)
 *  <dd> This method is called if syntax error recovery fails.  In the default
 *       implementation it invokes:<br>
 *         report_fatal_error("Couldn't repair and continue parse", null);
 *  </dl>
 *
 * @see     java_cup.runtime.Symbol
 * @see     java_cup.runtime.Symbol
 * @see     java_cup.runtime.virtual_parse_stack
 * @version last updated: 7/3/96
 * @author  Frank Flannery
 */
#ifndef lr_parser_h
#define lr_parser_h

#include <iostream.h>
#include <assert.h>
#include "lr_symbol.h"
#include "scanner.h"

#define public    public:
#define protected protected:
#define private   private:
#define abstract  =0

#ifndef MAX_ERROR_SYNC_SIZE
# define MAX_ERROR_SYNC_SIZE 8
#endif


class lr_parser
{
  /*-----------------------------------------------------------*/
  /*--- Constructor(s) ----------------------------------------*/
  /*-----------------------------------------------------------*/
  /** Instance variables initialization routine */
  protected void init();

  /** Simple constructor. */
  public lr_parser();

  /** Constructor that sets the default scanner. [CSA/davidm] */
  public lr_parser(scanner* s);

  /** Just a placeholder for a virtual destructor */
  public virtual ~lr_parser() {;}

  /** Exception that is thrown from within the parse() method
      when fatal error is encountered */
  public struct xfatal
    {
      const char* msg;
      xfatal(const char* m)   : msg(m)     {;}
      xfatal(const xfatal& x) : msg(x.msg) {;}
    };

  /** Inner class to implement the parser stack */
  public class stack
    {
      private lr_symbol**  base;
      private lr_symbol**  top;
      private lr_symbol**  end;

      enum { DEFAULT_INIT_SIZE = 128 };

      public stack(int init_size=DEFAULT_INIT_SIZE);
            ~stack();

      private void  grow();

      public void  push(lr_symbol* sym)
        {
          if (top == end)
            grow();
          *top++ = sym;
        }

      public void  pop()
        {
          assert(top != base);
          --top;
        }

      public void  npop(int n)
        {
          top -= n;
          assert(n >= 0);
          assert(base <= top);
        }

      public int  size() const
        {
          return top - base;
        }

      public bool  empty() const
        {
          return top == base;
        }

      public void  remove_all_elements()
        {
          top = base;
        }

      public lr_symbol* element_at(int idx)
        {
          assert(0 <= idx && idx < size());
          return base[idx];
        }

      public lr_symbol* peek()
        {
          return top[-1];
        }

      public lr_symbol** top_ptr()
        {
          return top;
        }
    };
  public typedef class stack  stack_type;

  public class action_executor
    {
      public virtual ~action_executor() {;}

      /** Perform a bit of user supplied action code (supplied by generated
       *  subclass).  Actions are indexed by an internal action number assigned
       *  at parser generation time.
       *
       * @param act_num   the internal index of the action to be performed.
       * @param parser    the parser object we are acting for.
       * @param stack_top the parser's stack top.
       */
      public virtual lr_symbol* do_action(int         act,
                                          lr_parser&  parser,
                                          lr_symbol** stack_top) abstract;
    };

  /** Each production entry has two parts, the index of the non-terminal on
   *  the left hand side of the production, and the number of Symbols
   *  on the right hand side.
   */
  public struct prod_entry
    {
      short lhs_sym;
      short rhs_size;
    };

  /*-----------------------------------------------------------*/
  /*--- (Access to) Instance Variables ------------------------*/
  /*-----------------------------------------------------------*/

  /** Table of production information (supplied by generated subclass).
   *  This table contains one entry per production and is indexed by
   *  the negative-encoded values (reduce actions) in the action_table.
   *  Each entry has two parts, the index of the non-terminal on the
   *  left hand side of the production, and the number of Symbols
   *  on the right hand side.
   */
  public virtual prod_entry* production_table() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The action table (supplied by generated subclass).  This table is
   *  indexed by state and terminal number indicating what action is to
   *  be taken when the parser is in the given state (i.e., the given state
   *  is on top of the stack) and the given terminal is next on the input.
   *  States are indexed using the first dimension, however, the entries for
   *  a given state are compacted and stored in adjacent index, value pairs
   *  which are searched for rather than accessed directly (see get_action()).
   *  The actions stored in the table will be either shifts, reduces, or
   *  errors.  Shifts are encoded as positive values (one greater than the
   *  state shifted to).  Reduces are encoded as negative values (one less
   *  than the production reduced by).  Error entries are denoted by zero.
   *
   * @see java_cup.runtime.lr_parser#get_action
   */
  public virtual short** action_table() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The reduce-goto table (supplied by generated subclass).  This
   *  table is indexed by state and non-terminal number and contains
   *  state numbers.  States are indexed using the first dimension, however,
   *  the entries for a given state are compacted and stored in adjacent
   *  index, value pairs which are searched for rather than accessed
   *  directly (see get_reduce()).  When a reduce occurs, the handle
   *  (corresponding to the RHS of the matched production) is popped off
   *  the stack.  The new top of stack indicates a state.  This table is
   *  then indexed by that state and the LHS of the reducing production to
   *  indicate where to "shift" to.
   *
   * @see java_cup.runtime.lr_parser#get_reduce
   */
  public virtual short** reduce_table() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The index of the start state (supplied by generated subclass). */
  public virtual int start_state() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The index of the start production (supplied by generated subclass). */
  public virtual int start_production() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The index of the end of file terminal Symbol (supplied by generated
   *  subclass).
   */
  public virtual int EOF_sym() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The index of the special error Symbol (supplied by generated subclass). */
  public virtual int error_sym() abstract;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
  /* Global parse state shared by parse(), error recovery, and
   * debugging routines */
  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The current lookahead Symbol. */
  protected lr_symbol* cur_token;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The parse stack itself. */
  protected class stack stack;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Direct reference to the production table. */
  protected prod_entry* production_tab;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Direct reference to the action table. */
  protected short** action_tab;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Direct reference to the reduce-goto table. */
  protected short** reduce_tab;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The stream that error messages are printed to. */
  protected ostream* _error_os;

  /** Simple accessor method to set the error stream */
  public void  set_error_os(ostream& os) { _error_os = &os; }

  /** Simple accessor method to get the error stream */
  public ostream&  error_os() { return *_error_os; }

  /** The stream that debug messages are printed to. */
  protected ostream* _debug_os;

  /** Simple accessor method to set the error stream */
  public void  set_debug_os(ostream& os) { _debug_os = &os; }

  /** Simple accessor method to get the error stream */
  public ostream&  debug_os() { return *_debug_os; }

  /** This is the action_executor object used by the default implementation
   *  of parse() to execute actions.  To avoid name conflicts with existing
   *  code, this field is private.
   */
  private action_executor* _action_executor;

  protected lr_symbol* do_action(int act)
    {
      return _action_executor->do_action(act, *this, stack.top_ptr());
    }

  /**
   * Simple accessor method to set the default action executor.
   */
  public void  set_action_executor(action_executor* ae)
    {
      _action_executor = ae;
    }

  /**
   * Simple accessor method to get the default action executor.
   */
  public action_executor* get_action_executor()
    {
      return _action_executor;
    }

  /** This is the scanner object used by the default implementation
   *  of scan() to get Symbols.  To avoid name conflicts with existing
   *  code, this field is private. [CSA/davidm]
   */
  private scanner* _scanner;

  /**
   * Simple accessor method to set the default scanner.
   */
  public void set_scanner(scanner* s) { _scanner = s; }

  /**
   * Simple accessor method to get the default scanner.
   */
  public scanner* get_scanner() { return _scanner; }

  /*-----------------------------------------------------------*/
  /*--- General Methods ---------------------------------------*/
  /*-----------------------------------------------------------*/

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** User code for initialization inside the parser.  Typically this
   *  initializes the scanner.  This is called before the parser requests
   *  the first Symbol.  Here this is just a placeholder for subclasses that
   *  might need this and we perform no action.   This method is normally
   *  overridden by the generated code using this contents of the "init with"
   *  clause as its body.
   */
  public virtual void user_init() {;}

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Initialize the action object.  This is called before the parser does
   *  any parse actions. This is filled in by generated code to create
   *  an object that encapsulates all action code.
   */
  public virtual void init_actions() {;}

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Get the next Symbol from the input (supplied by generated subclass).
   *  Once end of file has been reached, all subsequent calls to scan
   *  should return an EOF Symbol (which is Symbol number 0).  By default
   *  this method returns getScanner().next_token(); this implementation
   *  can be overriden by the generated parser using the code declared in
   *  the "scan with" clause.  Do not recycle objects; every call to
   *  scan() should return a fresh object.
   */
  public lr_symbol* scan()
    {
      return _scanner->next_token();
    }

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Report a fatal error.  This method takes a  message string and an
   *  additional object (to be used by specializations implemented in
   *  subclasses).  Here in the base class a very simple implementation
   *  is provided which reports the error then throws an exception.
   *
   * @param message an error message.
   * @param info    an extra object reserved for use by specialized subclasses.
   */
  public virtual void report_fatal_error(const char* message, lr_symbol* nfo=0);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Report a non fatal error (or warning).  This method takes a message
   *  string and an additional object (to be used by specializations
   *  implemented in subclasses).  Here in the base class a very simple
   *  implementation is provided which simply prints the message to
   *  System.err.
   *
   * @param message an error message.
   * @param info    an extra object reserved for use by specialized subclasses.
   */
  public virtual void report_error(const char* message, lr_symbol* info=0);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** This method is called when a syntax error has been detected and recovery
   *  is about to be invoked.  Here in the base class we just emit a
   *  "Syntax error" error message.
   *
   * @param cur_token the current lookahead Symbol.
   */
  public virtual void syntax_error(lr_symbol* cur_token);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** This method is called if it is determined that syntax error recovery
   *  has been unsuccessful.  Here in the base class we report a fatal error.
   *
   * @param cur_token the current lookahead Symbol.
   */
  public virtual void unrecovered_syntax_error(lr_symbol* cur_token);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Fetch an action from the action table.  The table is broken up into
   *  rows, one per state (rows are indexed directly by state number).
   *  Within each row, a list of index, value pairs are given (as sequential
   *  entries in the table), and the list is terminated by a default entry
   *  (denoted with a Symbol index of -1).  To find the proper entry in a row
   *  we do a linear or binary search (depending on the size of the row).
   *
   * @param state the state index of the action being accessed.
   * @param sym   the Symbol index of the action being accessed.
   */
  protected short get_action(int state, int sym);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Fetch a state from the reduce-goto table.  The table is broken up into
   *  rows, one per state (rows are indexed directly by state number).
   *  Within each row, a list of index, value pairs are given (as sequential
   *  entries in the table), and the list is terminated by a default entry
   *  (denoted with a Symbol index of -1).  To find the proper entry in a row
   *  we do a linear search.
   *
   * @param state the state index of the entry being accessed.
   * @param sym   the Symbol index of the entry being accessed.
   */
  protected short get_reduce(int state, int sym);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** This method provides the main parsing routine.  It returns only when
   *  accept  has been commited or fatal error has been reported)
   *  See the header documentation for the class regarding how
   *  shift/reduce parsers operate and how the various tables are used.
   */
  public lr_symbol* parse();

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Dump the parse stack for debugging purposes. */
  public void dump_stack();

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Do debug output for a reduce.
   *
   * @param prod_num  the production we are reducing with.
   * @param nt_num    the index of the LHS non terminal.
   * @param rhs_size  the size of the RHS.
   */
  public void debug_reduce(int prod_num, int nt_num, int rhs_size);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Do debug output for shift.
   *
   * @param shift_tkn the Symbol being shifted onto the stack.
   */
  public void debug_shift(const lr_symbol* shift_tkn);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Do debug output for stack state. [CSA]
   */
  public void debug_stack();

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Perform a parse with debugging output.  This does exactly the
   *  same things as parse(), except that it calls debug_shift() and
   *  debug_reduce() when shift and reduce moves are taken by the parser
   *  and produces various other debugging messages.
   */
  //public lr_symbol* debug_parse();

  /** Error recovery status enum */
  protected enum ers_t {
    ERS_FAIL,
    ERS_SUCCESS,
    ERS_ACCEPT
  };

  /** Attempt to recover from a syntax error.  This returns false if recovery
   *  fails, true if it succeeds.  Recovery happens in 4 steps.  First we
   *  pop the parse stack down to a point at which we have a shift out
   *  of the top-most state on the error symbol.  This represents the
   *  initial error recovery configuration.  If no such configuration is
   *  found, then we fail.  Next a small number of "lookahead" or "parse
   *  ahead" Symbols are read into a buffer.  The size of this buffer is
   *  determined by error_sync_size() and determines how many Symbols beyond
   *  the error must be matched to consider the recovery a success.  Next,
   *  we begin to discard Symbols in attempt to get past the point of error
   *  to a point where we can continue parsing.  After each symbol, we attempt
   *  to "parse ahead" though the buffered lookahead Symbols.  The "parse ahead"
   *  process simulates that actual parse, but does not modify the real
   *  parser's configuration, nor execute any actions. If we can  parse all
   *  the stored Symbols without error, then the recovery is considered a
   *  success.  Once a successful recovery point is determined, we do an
   *  actual parse over the stored input -- modifying the real parse
   *  configuration and executing all actions.  Finally, we return the the
   *  normal parser to continue with the overall parse.
   *
   * @param debug should we produce debugging messages as we parse.
   */
  protected virtual ers_t  error_recovery(bool debug);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/
  /* Error recovery code */
  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** This class implements a temporary or "virtual" parse stack that
   *  replaces the top portion of the actual parse stack (the part that
   *  has been changed by some set of operations) while maintaining its
   *  original contents.  This data structure is used when the parse needs
   *  to "parse ahead" to determine if a given error recovery attempt will
   *  allow the parse to continue far enough to consider it successful.  Once
   *  success or failure of parse ahead is determined the system then
   *  reverts to the original parse stack (which has not actually been
   *  modified).  Since parse ahead does not execute actions, only parse
   *  state is maintained on the virtual stack, not full lr_symbol objects.
   *
   * @see       java_cup.runtime.lr_parser
   * @version   last updated: 7/3/96
   * @author    Frank Flannery
   * @modified  Stanislav Jordanov 11/21/99
   */

  public class virtual_stack
    {
      private typedef stack_type  stack;
      /*-----------------------------------------------------------*/
      /*--- Constructor(s) ----------------------------------------*/
      /*-----------------------------------------------------------*/

      /** Constructor to build a virtual stack out of a real stack. */
      public virtual_stack(stack& shadowing_stack)
        : real_stack(shadowing_stack), real_next(0)
        {
          /* get one element onto the virtual portion of the stack */
          get_from_real();
        }

      /*-----------------------------------------------------------*/
      /*--- (Access to) Instance Variables ------------------------*/
      /*-----------------------------------------------------------*/

      /** The real stack that we shadow.  This is accessed when we move off
       *  the bottom of the virtual portion of the stack, but is always left
       *  unmodified.
       */
      protected stack& real_stack;

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

      /** Top of stack indicator for where we leave off in the real stack.
       *  This is measured from top of stack, so 0 would indicate that no
       *  elements have been "moved" from the real to virtual stack.
       */
      protected int real_next;

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

      /** The virtual top portion of the stack.  This stack contains Integer
       *  objects with state numbers.  This stack shadows the top portion
       *  of the real stack within the area that has been modified (via
       *  operations on the virtual stack).  When this portion of the stack
       *  becomes empty we transfer elements from the underlying stack onto
       *  this stack.
       */
      protected stack  vstack;

      /*-----------------------------------------------------------*/
      /*--- General Methods ---------------------------------------*/
      /*-----------------------------------------------------------*/

      /** Transfer an element from the real to the virtual stack.
       *  This assumes that the virtual stack is currently empty.
       */
      protected void get_from_real()
        {
          lr_symbol* stack_sym;

          /* don't transfer if the real stack is empty */
          if (real_next >= real_stack.size())
            return;

          /* get a copy of the first symbol we have not transfered */
          stack_sym = real_stack.element_at(real_stack.size()-1-real_next);

          /* record the transfer */
          real_next++;

          /* put the state number from the symbol onto the virtual stack */
          vstack.push((lr_symbol*)stack_sym->get_parse_state());
        }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

      /** Indicate whether the stack is empty. */
      public bool empty()
        {
          /* if vstack is empty then we were unable to transfer onto it and
             the whole thing is empty. */
          return vstack.empty();
        }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

      /** Return value on the top of the stack (without popping it). */
      public int top()
        {
          assert(!vstack.empty());
          return (int)vstack.peek();
        }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

      /** Pop the stack. */
      public void pop()
        {
          assert(!vstack.empty());

          /* pop it */
          vstack.pop();

          /* if we are now empty transfer an element (if there is one) */
          if (vstack.empty())
            get_from_real();
        }

      /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

      /** Push a state number onto the stack. */
      public void push(int state_num)
        {
          vstack.push((lr_symbol*)state_num);
        }

      /*-----------------------------------------------------------*/
    };

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The default number of Symbols after an error we much match to consider
   *  it recovered from.
   */
  protected int _error_sync_size;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** The number of Symbols after an error we much match to consider it
   *  recovered from.
   */
  protected virtual int  error_sync_size() const { return _error_sync_size; }

  /** Set the number of Symbols after an error we much match to consider it
   *  recovered from.
   */
  protected void  set_error_sync_size(int ess) { _error_sync_size = ess; }

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Determine if we can shift under the special error symbol out of the
   *  state currently on the top of the (real) parse stack.
   */
  protected bool shift_under_error()
    {
      /* is there a shift under error symbol */
      return get_action(stack.peek()->parse_state, error_sym()) > 0;
    }

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Put the (real) parse stack into error recovery configuration by
   *  popping the stack down to a state that can shift on the special
   *  error symbol, then doing the shift.  If no suitable state exists on
   *  the stack we return false
   *
   * @param debug should we produce debugging messages as we parse.
   */
  protected bool find_recovery_config(bool debug);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Lookahead Symbols used for attempting error recovery "parse aheads". */
  protected lr_symbol* lookahead[MAX_ERROR_SYNC_SIZE];

  /** Position in lookahead input buffer used for "parse ahead". */
  protected int   lookahead_pos;

  /** # of tokens read in the lookahead input buffer */
  protected int   lookahead_len;

  /** When the EOF token is received for first time this flag is set to
      prevent furher calls to scanner */
  protected bool  got_eof;

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Read from input to establish our buffer of "parse ahead" lookahead
   *  Symbols.
   */
  protected void read_lookahead();

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Return the current lookahead in our error "parse ahead" buffer. */
  protected lr_symbol* cur_err_token() { return lookahead[lookahead_pos]; }

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Advance to next "parse ahead" input symbol. Return true if we have
   *  input to advance to, false otherwise.
   */
  protected bool advance_lookahead()
    {
      /* advance the input location  &
         return true if we didn't go off the end */
      return ++lookahead_pos < lookahead_len;
    }

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Reset the parse ahead input to one symbol past where we started error
   *  recovery (this consumes one new symbol from the real input).
   */
  protected void restart_lookahead();

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Gives the derived class a chance to destroy symbols popped during
   *  error recovery stack unwind.
   */
  protected virtual void dispose_of(lr_symbol* sym) { delete sym; }

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Do a simulated parse forward (a "parse ahead") from the current
   *  stack configuration using stored lookahead input and a virtual parse
   *  stack.  Return true if we make it all the way through the stored
   *  lookahead input without error. This basically simulates the action of
   *  parse() using only our saved "parse ahead" input, and not executing any
   *  actions.
   *
   * @param debug should we produce debugging messages as we parse.
   */
  protected bool try_parse_ahead(bool debug);

  /*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . .*/

  /** Parse forward using stored lookahead Symbols.  In this case we have
   *  already verified that parsing will make it through the stored lookahead
   *  Symbols and we are now getting back to the point at which we can hand
   *  control back to the normal parser.  Consequently, this version of the
   *  parser performs all actions and modifies the real parse configuration.
   *  This returns once we have consumed all the stored input or we accept.
   *
   * @param   debug should we produce debugging messages as we parse.
   * @return  ERS_SUCCESS or ERS_FAIL
   */
  protected ers_t  parse_lookahead(bool debug);

  /*-----------------------------------------------------------*/
};

#undef public
#undef protected
#undef private
#undef abstract

#endif //lr_parser.h