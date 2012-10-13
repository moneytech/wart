//// run unit tests or interactive interpreter

// The design of lisp seems mindful of the following:
//  keep programs as short as possible
//  as programs grow, allow them to be decomposed into functions
//  functions are defined with a recipe for work, and invoked to perform that work
//  functions are parameterized with vars (parameters) and invoked with data (arguments)
//  new functions can be defined at any time
//  functions can invoke other functions before they've been defined
//  functions are data; they can be passed to other functions
//  when an invocation contains multiple functions, make the one being invoked easy to identify
//    almost always the first symbol in the invocation
//  there have to be *some* bedrock primitives, but any primitive can be user-defined in principle
//
// These weren't the reasons lisp was created; they're the reasons I attribute
// to its power.

// To satisfy these properties, lisp is designed around function invocation as
// the core activity of the language; features in other languages are
// definable as functions. Even large programs are structured as a single
// function that performs all the work when invoked.
//
// Setting up such a function involves two stages:
//  - setting up the data for it from disk to memory, including subsidiary functions (read)
//  - invoking this function by repeatedly invoking its subsidiary functions (eval)
// A function can be read once into memory, and eval'd any number of times
// once read.
//
// Read is the time for optimizations, when subsidiary functions can be
// specialized to a specific call-site.

// track indent state when reading code from disk
struct CodeStream {
  istream& fd;
  long currIndent;
  CodeStream(istream& in) :fd(in), currIndent(-1) { fd >> std::noskipws; }
  bool eof() { return fd.eof(); }
};
CodeStream STDIN(cin);

int main(int argc, unused char* argv[]) {
  if (argc > 1) {
    runTests();
    return 0;
  }

  // Interpreter loop: prompt, read, eval, print
  interactive_setup();
  while (true) {
    prompt("wart> ");
    Cell* form = read(STDIN);
    if (STDIN.eof()) break;
    Cell* result = eval(form);
    cout << result << endl;

    rmref(result);
    rmref(form);
    reset(STDIN);
  }
  return 0;
}

// read: tokenize, parenthesize, parse, transform infix, build cells, transform $vars
Cell* read(CodeStream& c) {
  return mkref(transformDollarVars(nextRawCell(c)));
}



// test harness

bool runningTests = false;
long numFailures = 0;

typedef void (*TestFn)(void);

const TestFn tests[] = {
  #include "test_list"
};

void runTests() {
  runningTests = true;
  pretendRaise = true;  // for death tests
  time_t t; time(&t);
  cerr << "C tests: " << ctime(&t);
  for (unsigned long i=0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
    setup();
    (*tests[i])();
    verify();
  }

  pretendRaise = false;
  setup();
  loadFiles(".wart");   // after GC tests
  loadFiles(".test");

  cerr << endl;
  if (numFailures == 0) return;
  cerr << numFailures << " failure";
      if (numFailures > 1) cerr << "s";
      cerr << endl;
}

void verify() {
  if (raiseCount != 0) cerr << raiseCount << " errors encountered" << endl;
  teardownStreams();
  teardownCompiledFns();
  teardownCells();

  if (numUnfreed() > 0) {
    RAISE << "Memory leak!\n";
    dumpUnfreed();
  }
}

void setup() {
  setupCells();
  setupScopes();
  setupCompiledFns();
  setupStreams();
  raiseCount = 0;
}



// misc

bool interactive = false;   // trigger eval on empty lines
void interactive_setup() {
  setup();
  loadFiles(".wart");
  interactive = true;
  catchCtrlC();
}

void prompt(string msg) {
  extern unsigned long numAllocs;
  extern long numUnfreed();
  cout << numUnfreed() << " " << numAllocs << " " << msg;
}

void reset(CodeStream& in) {
  in.fd.get();
  if (interactive) in.fd.get();
}

// helper to read from string
stringstream& stream(string s) {
  stringstream& result = *new stringstream(s);
  result << std::noskipws;
  return result;
}