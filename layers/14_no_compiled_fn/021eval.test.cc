bool contains(const list<Cell*>& v, string sym) {
  return find(v.begin(), v.end(), newSym(sym)) != v.end();
}

void test_evalBindAll_handles_unquoted_param() {
  Cell* params = read("(x)");
  Cell* args = read("(a)");
  newDynamicScope("a", newNum(3));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  CHECK_EQ(varsBound.size(), 1);
  CHECK(contains(varsBound, "x"));
  CHECK_EQ(lookup("x"), newNum(3));
  endDynamicScope("x");
  endDynamicScope("a");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_binds_missing_params() {
  Cell* params = read("(x y)");
  Cell* args = read("(a)");
  newDynamicScope("a", newNum(3));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  CHECK_EQ(lookup("x"), newNum(3));
  CHECK_EQ(lookup("y"), nil);
  CHECK_EQ(varsBound.size(), 2);
  CHECK(contains(varsBound, "x"));
  CHECK(contains(varsBound, "y"));
  endDynamicScope("x");
  endDynamicScope("y");
  endDynamicScope("a");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_handles_quoted_param() {
  Cell* params = read("('x)");
  Cell* args = read("(a)");
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  CHECK_EQ(varsBound.size(), 1);
  CHECK(contains(varsBound, "x"));
  CHECK_EQ(lookup("x"), newSym("a"));
  endDynamicScope("x");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_handles_varargs_param() {
  Cell* params = read("x");
  Cell* args = read("(a b)");
  newDynamicScope("a", newNum(3));
  newDynamicScope("b", newNum(4));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  CHECK_EQ(varsBound.size(), 1);
  CHECK(contains(varsBound, "x"));
  // {x: (3 4)}
  CHECK_EQ(car(lookup("x")), newNum(3));
  CHECK_EQ(car(cdr(lookup("x"))), newNum(4));
  CHECK_EQ(cdr(cdr(lookup("x"))), nil);
  endDynamicScope("x");
  endDynamicScope("a");
  endDynamicScope("b");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_handles_quoted_varargs_param() {
  Cell* params = read("'x");
  Cell* args = read("(a b)");
  newDynamicScope("a", newNum(3));
  newDynamicScope("b", newNum(4));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  CHECK_EQ(varsBound.size(), 1);
  CHECK(contains(varsBound, "x"));
  // {x: (a b)}
  CHECK_EQ(car(lookup("x")), newSym("a"));
  CHECK_EQ(car(cdr(lookup("x"))), newSym("b"));
  CHECK_EQ(cdr(cdr(lookup("x"))), nil);
  endDynamicScope("x");
  endDynamicScope("a");
  endDynamicScope("b");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_handles_rest_param() {
  Cell* params = read("(x ... y)");
  Cell* args = read("(a b)");
  newDynamicScope("a", newNum(3));
  newDynamicScope("b", newNum(4));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  // {x: 3, y: (4)}
  CHECK_EQ(lookup("x"), newNum(3));
  CHECK_EQ(car(lookup("y")), newNum(4));
  CHECK_EQ(cdr(lookup("y")), nil);
  endDynamicScope("x");
  endDynamicScope("y");
  endDynamicScope("a");
  endDynamicScope("b");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_handles_quoted_rest_param() {
  Cell* params = read("(x ... 'y)");
  Cell* args = read("(a b)");
  newDynamicScope("a", newNum(3));
  newDynamicScope("b", newNum(4));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  // {x: 3, y: (b)}
  CHECK_EQ(lookup("x"), newNum(3));
  CHECK_EQ(car(lookup("y")), newSym("b"));
  CHECK_EQ(cdr(lookup("y")), nil);
  endDynamicScope("x");
  endDynamicScope("y");
  endDynamicScope("a");
  endDynamicScope("b");
  rmref(args);
  rmref(params);
}

void test_evalBindAll_handles_destructured_params() {
  Cell* params = read("((a b))");
  Cell* args = read("((cons x (cons y)))");
  newDynamicScope("x", newNum(3));
  newDynamicScope("y", newNum(4));
  list<Cell*> varsBound;
  evalBindAll(params, args, varsBound);
  // {a: 3, b: 4}
  CHECK_EQ(lookup("a"), newNum(3));
  CHECK_EQ(lookup("b"), newNum(4));
  endDynamicScope("x");
  endDynamicScope("y");
  endDynamicScope("a");
  endDynamicScope("b");
  rmref(args);
  rmref(params);
}



void test_nil_evals_to_itself() {
  Cell* expr = read("()");
  Cell* result = eval(expr);
  CHECK_EQ(result, nil);
  rmref(result);
  rmref(expr);
}

void test_num_evals_to_itself() {
  Cell* expr = read("34");
  Cell* result = eval(expr);
  CHECK_EQ(result, expr);
  rmref(result);
  rmref(expr);
}

void test_colonsym_evals_to_itself() {
  Cell* expr = read(":abc");
  Cell* result = eval(expr);
  CHECK_EQ(result, expr);
  rmref(result);
  rmref(expr);
}

void test_colon_evals() {
  Cell* expr = read(":");
  newDynamicScope(":", nil);
  Cell* result = eval(expr);
  CHECK_EQ(result, nil);
  endDynamicScope(":");
  rmref(expr);
}

void test_string_evals_to_itself() {
  Cell* expr = read("\"ac bd\"");
  Cell* result = eval(expr);
  CHECK_EQ(result, expr);
  rmref(result);
  rmref(expr);
}

void test_sym_evals_to_value() {
  newDynamicScope("a", newNum(34));
  Cell* expr = read("a");
  Cell* result = eval(expr);
  CHECK_EQ(result, newNum(34));
  rmref(result);
  rmref(expr);
  endDynamicScope("a");
}

void test_sym_evals_to_itself() {
  newDynamicScope("a", newSym("a"));
  Cell* expr = read("a");
  Cell* result = eval(expr);
  CHECK_EQ(result, expr);
  rmref(result);
  rmref(expr);
  endDynamicScope("a");
}

void test_object_expr_evals_to_itself() {
  Cell* expr = read("(object foo 4)");
  Cell* result = eval(expr);
  CHECK_EQ(result, expr);
  rmref(result);
  rmref(expr);
}

void test_eval_handles_quoted_atoms() {
  Cell* expr = read("'a");
  Cell* result = eval(expr);
  CHECK_EQ(result, newSym("a"));
  rmref(result);
  rmref(expr);

  expr = read("'34");
  result = eval(expr);
  CHECK_EQ(result, newNum(34));
  rmref(result);
  rmref(expr);
}

void test_eval_handles_quoted_lists() {
  Cell* expr = read("'(a b)");
  Cell* result = eval(expr);
  // (a b)
  CHECK_EQ(car(result), newSym("a"));
  CHECK_EQ(car(cdr(result)), newSym("b"));
  CHECK_EQ(cdr(cdr(result)), nil);
  rmref(result);
  rmref(expr);
}

void test_eval_handles_quoted_destructured_params() {
  Cell* call = read("((fn ('(a b)) b) (1 2))");
  Cell* result = eval(call);
  CHECK(isNum(result));
  CHECK_EQ(toInt(result), 2);
  rmref(result);
  rmref(call);
}

void test_eval_handles_rest_params() {
  Cell* call = read("((fn (a b ... c) c) 1 2 3 4 5)");
  Cell* result = eval(call);
  CHECK(isCons(result));
  CHECK(isNum(car(result)));
  // (3 4 5)
  CHECK_EQ(toInt(car(result)), 3);
  CHECK(isNum(car(cdr(result))));
  CHECK_EQ(toInt(car(cdr(result))), 4);
  CHECK_EQ(toInt(car(cdr(cdr(result)))), 5);
  CHECK_EQ(cdr(cdr(cdr(result))), nil);
  rmref(result);
  rmref(call);
}

void test_eval_handles_fn_calls() {
  Cell* call = read("((fn () 34))");
  Cell* result = eval(call);
  CHECK_EQ(result, newNum(34));
  rmref(result);
  rmref(call);
}

void test_eval_expands_syms_in_fn_bodies() {
  Cell* fn = read("((fn () a))");
  newDynamicScope("a", newNum(34));
  Cell* result = eval(fn);
  CHECK_EQ(result, newNum(34));
  endDynamicScope("a");
  rmref(result);
  rmref(fn);
}

void test_eval_handles_assigned_fn_calls() {
  Cell* fn = read("(fn () 34)");
  Cell* f = eval(fn);
  newDynamicScope("f", f);
    Cell* call = read("(f)");
    Cell* result = eval(call);
    CHECK_EQ(result, newNum(34));
  endDynamicScope("f");
  rmref(result);
  rmref(call);
  rmref(f);
  rmref(fn);
}

void test_eval_handles_quoted_param_list() {
  newDynamicScope("a", newNum(23));
  Cell* fn = read("(fn '(arg1) arg1)");
  Cell* f = eval(fn);
  newDynamicScope("f", f);
  Cell* call = read("(f a)");
  Cell* result = eval(call);
  CHECK_EQ(result, newSym("a"));
  rmref(result);
  rmref(call);
  rmref(f);
  rmref(fn);
  endDynamicScope("f");
  endDynamicScope("a");
}

void test_eval_handles_multiple_args() {
  Cell* fn = read("(fn (a b) b)");
  Cell* f = eval(fn);
  newDynamicScope("f", f);
  Cell* call = read("(f 1 2)");
  Cell* result = eval(call);
  CHECK_EQ(result, newNum(2));
  rmref(result);
  rmref(call);
  rmref(f);
  rmref(fn);
  endDynamicScope("f");
}

void test_eval_handles_multiple_body_exprs() {
  Cell* fn = read("(fn () 1 2)");
  Cell* f = eval(fn);
  newDynamicScope("f", f);
  Cell* call = read("(f)");
  Cell* result = eval(call);
  CHECK_EQ(result, newNum(2));
  rmref(result);
  rmref(call);
  rmref(f);
  rmref(fn);
  endDynamicScope("f");
}

void test_eval_handles_vararg_param() {
  Cell* call = read("((fn args args) 1)");
  Cell* result = eval(call);
  CHECK(isCons(result));
  CHECK_EQ(car(result), newNum(1));
  rmref(result);
  rmref(call);
}

void test_eval_evals_args() {
  Cell* call = read("((fn (f) (f)) (fn () 34))");
  Cell* result = eval(call);
  CHECK(isNum(result));
  CHECK_EQ(toInt(result), 34);
  rmref(result);
  rmref(call);
}

void test_eval_doesnt_leak_body_evals() {
  Cell* call = read("((fn (f) (f) (f)) (fn () 34))");
  Cell* result = eval(call);
  CHECK(isNum(result));
  CHECK_EQ(toInt(result), 34);
  rmref(result);
  rmref(call);
}

void test_eval_handles_destructured_params() {
  Cell* call = read("((fn ((a b)) b) '(1 2))");
  Cell* result = eval(call);
  CHECK(isNum(result));
  CHECK_EQ(toInt(result), 2);
  rmref(result);
  rmref(call);
}