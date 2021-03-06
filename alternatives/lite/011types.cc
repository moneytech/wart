//// primitive datatypes: lists

cell* car(cell* x) {
  if (x->type != CONS) {
    RAISE << "car of non-cons: " << x << '\n';
    return nil;
  }
  return x->car;
}

cell* cdr(cell* x) {
  return x->cdr;
}

void set_car(cell* x, cell* y) {
  if (!is_cons(x)) {
    RAISE << "can't set car of " << x << '\n';
    return;
  }
  mkref(y);
  rmref(car(x));
  x->car = y;
}

void set_cdr(cell* x, cell* y) {
  if (!is_cons(x) && !is_table(x)) {
    RAISE << "can't set cdr of " << x << '\n';
    return;
  }
  mkref(y);
  rmref(cdr(x));
  x->cdr = y;
}

cell* new_cons(cell* car, cell* cdr) {
  cell* ans = new_cell();
  set_car(ans, car);
  set_cdr(ans, cdr);
  return ans;
}

cell* new_cons(cell* car) {
  return new_cons(car, nil);
}



//// numbers

map<long, cell*> Int_literals;

cell* new_num(long x) {
  if (Int_literals[x])
    return Int_literals[x];
  Int_literals[x] = new_cell();
  Int_literals[x]->car = (cell*)x;
  Int_literals[x]->type = INTEGER;
  return mkref(Int_literals[x]);
}

cell* new_num(int x) {  // just for integer literals
  return new_num((long)x);
}

cell* new_num(float x) {  // don't intern floats
  cell* result = new_cell();
  result->car = *(cell**)&x;
  result->type = FLOAT;
  return result;
}

cell* new_num(double x) {
  return new_num((float)x);
}

bool is_num(cell* x) {
  return x->type == INTEGER || x->type == FLOAT;
}

long to_int(cell* x) {
  // ignore endianness; cells are never persisted
  if (x->type == INTEGER)
    return (long)x->car;
  if (x->type == FLOAT)
    return (long)*((float*)&x->car);
  RAISE << "not a number: " << x << '\n';
  return 0;
}

float to_float(cell* x) {
  if (x->type == INTEGER)
    return (long)x->car;
  if (x->type == FLOAT)
    return *(float*)&x->car;
  RAISE << "not a number: " << x << '\n';
  return 0;
}

bool equal_floats(float x, float y) {
  return fabs(x-y) < 1e-6;
}



//// symbols

map<string, cell*> Sym_literals;

cell* new_sym(string x) {
  if (Sym_literals[x])
    return Sym_literals[x];
  Sym_literals[x] = new_cell();
  Sym_literals[x]->car = (cell*)new string(x);  // not aligned like cells; can fragment memory
  Sym_literals[x]->type = SYMBOL;
  return mkref(Sym_literals[x]);
}

bool is_sym(cell* x) {
  return x->type == SYMBOL;
}

cell* new_string(string x) {  // don't intern strings
  cell* result = new_cell();
  result->car = (cell*)new string(x);
  result->type = STRING;
  return result;
}

bool is_string(cell* x) {
  return x->type == STRING;
}

string to_string(cell* x) {
  if (!is_string(x) && !is_sym(x)) {
    RAISE << "can't convert to string: " << x << '\n';
    return "";
  }
  return *(string*)x->car;
}



//// associative arrays

cell* new_table() {
  cell* result = new_cell();
  result->type = TABLE;
  result->car = (cell*)new table();
  return result;
}

bool is_table(cell* x) {
  return x->type == TABLE;
}

table* to_table(cell* x) {
  if (!is_table(x)) return NULL;
  return (table*)x->car;
}

void put(cell* t, string k, cell* val) {
  unsafe_put(t, new_sym(k), val, true);
}

void put(cell* t, cell* k, cell* val) {
  unsafe_put(t, k, val, true);
}

cell* get(cell* t, cell* k) {
  cell* result = unsafe_get(t, k);
  if (!result) return nil;
  return result;
}

cell* get(cell* t, string k) {
  return get(t, new_sym(k));
}

void unsafe_put(cell* t, cell* key, cell* val, bool delete_nils) {
  if (!is_table(t)) {
    RAISE << "set on a non-table: " << t << '\n';
    return;
  }

  table& t2 = *(table*)(t->car);
  if (val == nil && delete_nils) {
    if (t2[key]) {
      rmref(t2[key]);
      t2[key] = NULL;
      rmref(key);
    }
    return;
  }

  if (val == t2[key]) return;

  if (!t2[key]) mkref(key);
  else rmref(t2[key]);
  t2[key] = mkref(val);
}

void unsafe_put(cell* t, string k, cell* val, bool delete_nils) {
  unsafe_put(t, new_sym(k), val, delete_nils);
}

cell* unsafe_get(cell* t, cell* key) {
  if (!is_table(t)) {
    RAISE << "get on a non-table\n";
    return nil;
  }
  return (*(table*)(t->car))[key];
}

cell* unsafe_get(cell* t, string k) {
  return unsafe_get(t, new_sym(k));
}



//// internals

void setup_cells() {
  setup_nil();
  Int_literals.clear();
  Sym_literals.clear();
  reset_heap(First_heap);
}

void teardown_cells() {
  if (nil->car != nil || nil->cdr != nil)
    RAISE << "nil was corrupted\n";

  for (map<long, cell*>::iterator p = Int_literals.begin(); p != Int_literals.end(); ++p) {
    if (p->second->nrefs > 1)
      RAISE << "couldn't unintern: " << p->first << ": " << (void*)p->second << " " << p->second->nrefs << '\n';
    free_cell(p->second);
  }

  for (map<string, cell*>::iterator p = Sym_literals.begin(); p != Sym_literals.end(); ++p) {
    if (p->first != "Curr_lexical_scope" && p->second->nrefs > 1)
      RAISE << "couldn't unintern: " << p->first << ": " << (void*)p->second << " " << p->second->nrefs << '\n';
    free_cell(p->second);
  }
}

// optimize lookups of common symbols
cell *sym_quote, *sym_backquote, *sym_unquote, *sym_splice, *sym_unquote_splice, *sym_already_evald;
cell *sym_false;
cell *sym_list, *sym_number, *sym_symbol, *sym_string, *sym_table;
cell *sym_object, *sym_Coercions;
cell *sym_function, *sym_name, *sym_sig, *sym_body, *sym_optimized_body, *sym_env, *sym_compiled, *sym_param_alias;
cell *sym_eval, *sym_caller_scope;
void setup_common_syms() {
  sym_quote = new_sym("'");
  sym_backquote = new_sym("`");
  sym_unquote = new_sym(",");
  sym_splice = new_sym("@");
  sym_unquote_splice = new_sym(",@");
  sym_already_evald = new_sym("''");

  sym_false = new_sym("false");

  sym_list = new_sym("list");
  sym_number = new_sym("number");
  sym_symbol = new_sym("symbol");
  sym_string = new_sym("string");
  sym_table = new_sym("table");

  sym_object = new_sym("object");
  sym_Coercions = new_sym("Coercions");

  sym_function = new_sym("function");
  sym_name = new_sym("name");
  sym_sig = new_sym("sig");
  sym_body = new_sym("body");
  sym_optimized_body = new_sym("optimized_body");
  sym_env = new_sym("env");
  sym_compiled = new_sym("compiled");
  sym_param_alias = new_sym("|");

  sym_eval = new_sym("eval");
  sym_caller_scope = new_sym("caller_scope");
}
