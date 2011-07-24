#include<cstdio>
#include<cstring>
#include<vector>
using std::vector;
#include<list>
using std::list;
#include<stack>
using std::stack;
#include<ext/hash_map>
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;
#include<ext/hash_set>
using __gnu_cxx::hash_set;
#include<exception>



// unicode strings everywhere

#include<string>
typedef std::wstring string;

#include<iostream>
typedef std::wistream istream;
typedef std::wostream ostream;
#define cin std::wcin
#define cout std::wcout
#define cerr std::wcerr
using std::endl;

#include<sstream>
typedef std::wstringstream stringstream;
typedef std::wostringstream ostringstream;

#include<fstream>
typedef std::wifstream ifstream;
typedef std::wofstream ofstream;

typedef char ascii;
#define char wchar_t // must come after all system includes



int debug = 0;
#define dbg if(debug == 1) cerr
#define dbg2 if(debug == 2) cerr

#define __unused__ __attribute__((unused))

class Die {};
ostream& operator<<(ostream& os __unused__, Die die __unused__) {
  exit(1);
}
Die DIE;



bool runningTests = false;
int numFailures = 0;

#define check(X) if (!(X)) { \
    ++numFailures; \
    cerr << endl << "F " << __FUNCTION__ << ": " << #X << endl; \
  } \
  else { cerr << "."; fflush(stderr); }

#define check_eq(X, Y) if ((X) != (Y)) { \
    ++numFailures; \
    cerr << endl << "F " << __FUNCTION__ << ": " << #X << " == " << #Y << endl; \
    cerr << "  got " << (X) << endl; /* BEWARE: multiple eval */ \
  } \
  else { cerr << "."; fflush(stderr); }

stringstream& teststream(string s) {
  stringstream& result = *new stringstream(s);
  result << std::noskipws;
  return result;
}

void checkState();



bool interactive = false;

// bootstrapping phases

#include "boot_list" // cc files containing 'boot'

// compiled primitive ops

#define COMPILE_PRIM_FUNC(name, params, body) \
  Cell* primFunc_##name() { \
    Cell* result = nil; /* implicit result */ \
    body \
    return result; \
  }

#include "op_list" // cc files not containing 'boot'

struct PrimFuncMetadata {
  string name;
  string params;
  PrimFunc impl;
};

const PrimFuncMetadata primFuncs[] = {
  #include "prim_func_list"
};

void setupPrimFuncs() {
  for (unsigned int i=0; i < sizeof(primFuncs)/sizeof(primFuncs[0]); ++i) {
    Cell* impl = newCell();
    setCar(impl, newPrimFunc(primFuncs[i].impl));
    setCdr(impl, buildCells(parse(parenthesize(tokenize(teststream(L"("+primFuncs[i].params+L")"))))).front());
    newDynamicScope(primFuncs[i].name, impl);
  }
}

void teardownPrimFuncs() {
  for (unsigned int i=0; i < sizeof(primFuncs)/sizeof(primFuncs[0]); ++i)
    endDynamicScope(primFuncs[i].name);
}



typedef void (*testfunc)(void);

const testfunc tests[] = {
  #include "test_list"
};

void runTests() {
  runningTests = true; // never reset
  for (unsigned int i=0; i < sizeof(tests)/sizeof(tests[0]); ++i)
    (*tests[i])();

  loadFiles(".wart"); // after GC tests
  loadFiles(".test");

  cerr << endl;
  if (numFailures == 0) return;
  cerr << numFailures << " failure";
      if (numFailures > 1) cerr << "s";
      cerr << endl;
}



// post-test teardown

void checkLiteralTables() {
  for (hash_map<long, Cell*>::iterator p = numLiterals.begin(); p != numLiterals.end(); ++p) {
    if (p->second->nrefs > 1)
      cerr << "couldn't unintern: " << p->first << ": " << (void*)p->second << " " << (long)p->second->car << " " << p->second->nrefs << endl;
    if (p->second->nrefs > 0)
      rmref(p->second);
  }
  for (StringMap<Cell*>::iterator p = stringLiterals.begin(); p != stringLiterals.end(); ++p) {
    if (p->first == L"currLexicalScope") continue; // memory leak
    if (p->second->nrefs > 1)
      cerr << "couldn't unintern: " << p->first << ": " << (void*)p->second << " " << *(string*)p->second->car << " " << p->second->nrefs << endl;
    if (p->second->nrefs > 0)
      rmref(p->second);
  }
}

                                  void mark_all_cells(Cell* x, hash_map<long, long>& mark) {
                                    if (x == nil) return;
                                    ++mark[(long)x];
                                    switch (x->type) {
                                    case NUM:
                                    case STRING:
                                    case SYM:
                                      break;
                                    case CONS:
                                      mark_all_cells(car(x), mark); break;
                                    case TABLE: {
                                      Table* t = (Table*)x->car;
                                      for (hash_map<long, Cell*>::iterator p = t->table.begin(); p != t->table.end(); ++p) {
                                        mark_all_cells((Cell*)p->first, mark);
                                        mark_all_cells(p->second, mark);
                                      }
                                      break;
                                    }
                                    case PRIM_FUNC:
                                      break;
                                    default:
                                      cerr << "Can't mark type " << x->type << endl << DIE;
                                    }
                                    mark_all_cells(cdr(x), mark);
                                  }

void dumpUnfreed() {
  hash_map<long, long> numRefsRemaining;
  for (Cell* x = heapStart; x < currCell; ++x) {
    if (!x->car) continue;
    mark_all_cells(x, numRefsRemaining);
  }
  for (Cell* x = heapStart; x < currCell; ++x) {
    if (!x->car) continue;
    if (x == newSym(L"currLexicalScope")) continue;
    if (numRefsRemaining[(long)x] > 1) continue;
    cerr << "unfreed: " << (void*)x << " " << x << endl;
  }
}

void checkUnfreed() {
  int n = currCell-heapStart-1; // ignore empty currLexicalScopes
  for (; freelist; freelist = freelist->cdr)
    --n;
  check_eq(n, 0);
  if (n > 0) dumpUnfreed();
}

void resetState() {
  numLiterals.clear();
  stringLiterals.clear();
  freelist = NULL;
  for(Cell* curr=currCell; curr >= heapStart; --curr)
    curr->init();
  currCell = heapStart;
  dynamics.clear(); // leaks memory for strings and tables
}

void setupState() {
  setupNil();
  setupLexicalScope();
  setupPrimFuncs();
}

void checkState() {
  teardownPrimFuncs();
  checkLiteralTables();
  checkUnfreed();

  resetState();
  setupState();
}



int main(int argc, ascii* argv[]) {
  setupState();

  int pass = 0;
  if (argc > 1) {
    std::string arg1(argv[1]);
    if (arg1 == "test") {
      runTests();
      return 0;
    }
    else if (arg1[0] >= L'0' || arg1[0] <= L'9') {
      pass = atoi(arg1.c_str());
    }
  }

  loadFiles(".wart");

  switch (pass) {
  case 1:
    cout << tokenize(cin); break;
  case 2:
    cout << parenthesize(tokenize(cin)); break;
  case 3:
    cout << parse(parenthesize(tokenize(cin))); break;
  case 4:
    cout << buildCells(parse(parenthesize(tokenize(cin)))); break;
  case 5:
    cout << eval(buildCells(parse(parenthesize(tokenize(cin)))).front()) << endl; break;
  default:
    // no unit tests for interactive repl, so manual QA:
    //   expr that doesn't start with paren should eval on empty line
    //   expr that starts with paren should eval on close
    interactive = true;
    while (!cin.eof()) {
      cout << "wart> ";
      list<Cell*> form = buildCells(parse(parenthesize(tokenize(cin))));
      if (form.empty()) continue;
      Cell* result = eval(form.front());
      cout << result << endl;
      rmref(result);
    }
  }
  return 0;
}

// style:
//  wide unicode strings everywhere
//  no function prototypes
//  immutable objects; copy everywhere; no references or pointers except Cell*
