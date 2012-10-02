//// construct parse tree out of cells

Cell* nextRawCell(CodeStream c) {
  c.fd.peek();
  if (c.fd.eof()) return nil;
  return buildCell(transformInfix(nextAstNode(c)));
}

Cell* buildCell(AstNode n) {
  if (n.isNil())
    return nil;
  if (n.isList() && n.elems.front() == ")") {
    if (n.elems.size() > 1) RAISE << "Syntax error: ) not at end of expr" << endl << DIE;
    return nil;
  }

  if (n.isAtom()) {
    errno = 0;
    char* end;
    long v = strtol(n.atom.token.c_str(), &end, 0);
    if (*end == '\0' && errno == 0)
      return newNum(v);

    if (errno == ERANGE || errno == EOVERFLOW)
      RAISE << "dropping precision for bignum " << n.atom.token << endl;

    float f = strtof(n.atom.token.c_str(), &end);
    if (*end == '\0') {
      if (n.atom.token.substr(0, 2) == "-.")
        RAISE << "assuming '" << n.atom.token << "' is a float; to remove this warning say '-0" << n.atom.token.substr(1) << "'.\n"
            << "If you mean to negate an int, skip the ssyntax: '-" << n.atom.token.substr(2) << "'.\n";
      return newNum(f);
    }

    if (n.atom.token.c_str()[0] == '"')
      return newString(n.atom.token.substr(1, n.atom.token.length()-2));

    return newSym(n.atom.token);
  }

  list<AstNode>::iterator first = n.elems.begin();
  if (*first == "(") {
    n.elems.pop_front();
    return buildCell(n);
  }

  Cell* newForm = newCell();
  setCar(newForm, buildCell(n.elems.front()));

  list<AstNode>::iterator next = first; ++next;
  if (*next == ".") {
    if (n.elems.size() == 2) RAISE << "Syntax error: . can't terminate expr" << endl << DIE;
    setCdr(newForm, buildCell(*++next));  // dotted pair
  }
  else if (isQuoteOrUnquote(*first) && n.elems.size() == 2) {
    setCdr(newForm, buildCell(*next));  // dotted pair
  }
  else {
    n.elems.pop_front();
    if (n.elems.empty())
      RAISE << "Error in parsing " << n << endl << DIE;
    setCdr(newForm, buildCell(n));
  }

  return newForm;
}

bool isQuoteOrUnquote(AstNode n) {
  return n == "'" || n == "`" || n == "," || n == ",@" || n == "@";
}