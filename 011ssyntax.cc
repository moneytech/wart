//// simple syntax shorthands transforming syms into forms

Cell* transform_ssyntax(Cell* input) {
  if (isSym(input)) {
    string var = toString(input);
    // avoid detecting floats as ssyntax
    if (var.find_first_not_of("0123456789.:!~&") == NOT_FOUND)
      ;
    else if (var[0] == '!')
      input = expandNot(var);
    else if (find(var, '.') || var.find('!') < var.length()-1)
      input = expandCall(var);
    else if (var[0] != ':' && find(var, ':'))
      input = expandCompose(var);
    else if (find(var, '&'))
      input = expandAndf(var);
    else if (var[0] == '~')
      input = expandComplement(var);
  }

  if (!isCons(input)) return input;   // no tables or compiledFns in static code
  setCar(input, transform_ssyntax(car(input)));
  setCdr(input, transform_ssyntax(cdr(input)));
  return input;
}



// internals

Cell* expandNot(string var) {
  var.replace(0, 1, "not ");
  return nextRawCell(stream(var));
}

Cell* expandCompose(string var) {
  var.replace(var.rfind(':'), 1, " ");
  return nextRawCell(stream("compose "+var));
}

Cell* expandCall(string var) {
  size_t end = var.length()-1;
  if (var.rfind('.') == end)
    return newCons(newSym(var.substr(0, end)));

  size_t dot = var.rfind('.');
  size_t bang = end > 0 ? var.rfind('!', end-1) : var.rfind('!');
  if (bang != NOT_FOUND && (dot == NOT_FOUND || bang > dot))
    var.replace(bang, 1, " '");
  else
    var.replace(dot, 1, " ");
  return nextRawCell(stream(var));
}

Cell* expandAndf(string var) {
  var.replace(var.rfind('&'), 1, " ");
  return nextRawCell(stream("andf "+var));
}

Cell* expandComplement(string var) {
  var.replace(0, 1, "complement ");
  return nextRawCell(stream(var));
}