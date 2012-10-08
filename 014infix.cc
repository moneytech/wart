//// transform infix expressions into prefix

const string extraSymChars = "$?!_";  // besides letters and digits

AstNode transformInfix(AstNode n) {
  // special-case: ellipses are for dotted lists, not infix
  if (n.isAtom() && n.atom.token == "...")
    return n;

  if (n.isAtom() && n.atom.token[0] == '\"')
    return n;

  if (n.isAtom() && parseableAsFloat(n.atom.token))
    return n;

  if (n.isAtom() && !containsInfixChar(n.atom.token))
    return n;

  if (n.isAtom())
    n = tokenizeInfix(n);
  if (n.isAtom())
    return n;

  if (isQuoteOrUnquote(n.elems.front())) {
    list<AstNode>::iterator p = n.elems.begin();
    while (isQuoteOrUnquote(*p)) {
      ++p;
      if (p == n.elems.end()) return n;
    }
    AstNode result = (p == --n.elems.end())
        ? transformInfix(*p)
        : transformInfix(AstNode(list<AstNode>(p, n.elems.end())));
    if (result.isAtom()) {
      n.elems.pop_back();
      n.elems.push_back(result);
      return n;
    }
    else {
      result.elems.insert(result.elems.begin(), n.elems.begin(), p);
      return result;
    }
  }

  if (n.elems.front() != Token("("))
    return n;

  if (n.elems.size() == 2)  // ()
    return n;

  if (infixOpCalledWithoutArgs(n))
    return *++n.elems.begin();  // (++) => ++

  int oldsize = n.elems.size();

  // now n is guaranteed to have at least 3 ops
  // slide a window of 3, pinching into s-exprs when middle elem is an op
  auto prev = n.elems.begin();
  auto curr=prev; ++curr;
  auto next=curr; ++next;
  for (; next != n.elems.end(); ++prev, ++curr, ++next) {
    if (curr->atom.token == "...") continue;

    if (!isInfixOp(*curr)) {
      *curr = transformInfix(*curr);
      continue;
    }
    if (*next == ")") {  // postfix op
      *curr = transformInfix(*curr);
      continue;
    }

    list<AstNode> tmp;
    tmp.push_back(transformInfix(*curr));
    if (prev == n.elems.begin()) {
      auto oldnext = next;
      // prefix op; grab as many non-ops as you can
      while (!isInfixOp(*next)) {
        tmp.push_back(transformInfix(*next));
        ++next;
        if (next == --n.elems.end()) break;
      }

      // update next
      n.elems.erase(oldnext, next);
      next=curr; ++next;
    }
    else {
      // infix op; switch to prefix
      tmp.push_back(*prev);
      tmp.push_back(transformInfix(*next));

      // update both prev and next
      n.elems.erase(prev);
      prev=curr; --prev;
      n.elems.erase(next);
      next=curr; ++next;
    }
    // wrap in parens
    tmp.push_front(AstNode(Token("(")));
    tmp.push_back(AstNode(Token(")")));
    // insert the new s-expr
    *curr = AstNode(tmp);
  }

  // (a + b) will have become ((+ a b)); strip out one pair of parens
  if (n.elems.size() == 3 && oldsize > 3)
    return *++n.elems.begin();

  return n;
}

AstNode tokenizeInfix(AstNode n) {
  const char* var = n.atom.token.c_str();

  // special-case: :sym is never infix
  if (var[0] == ':') return n;
  // special-case: $var is never infix
  if (var[0] == '$') return n;

  string out;
  for (size_t x=0; var[x] != '\0'; ++x) {
    if (isdigit(var[x]) && (x == 0 || isInfixChar(var[x-1]))) {
      const char* next = skipFloat(&var[x]);
      if (next != &var[x]) {
        out += " ";
        while (var[x] != '\0' && &var[x] != next) {
          out += var[x];
          ++x;
        }
        --x;
        continue;
      }
    }

    if ((x > 0)
          && ((isInfixChar(var[x]) && isRegularChar(var[x-1]))
              || (isRegularChar(var[x]) && isInfixChar(var[x-1])))) {
      out += " ";
    }
    out += var[x];
  }
  CodeStream cs(stream(out));
  return nextAstNode(cs);
}



bool isInfixOp(AstNode n) {
  if (n.isList()) return false;
  string s = n.atom.token;
  string::iterator p = s.begin();
  if (*p != '$' && !isInfixChar(*p))
    return false;
  for (++p; p != s.end(); ++p)
    if (!isInfixChar(*p))
      return false;
  return true;
}

bool containsInfixChar(string name) {
  for (string::iterator p = name.begin(); p != name.end(); ++p) {
    if (p == name.begin() && *p == '-')
      continue;

    if (isInfixChar(*p)) return true;
  }
  return false;
}

bool isInfixChar(char c) {
  return !find(punctuationChars, c)
      && !find(quoteAndUnquoteChars, c)
      && !isalnum(c) && !find(extraSymChars, c);
}

bool isRegularChar(char c) {
  return isalnum(c) || find(extraSymChars, c);
}

bool infixOpCalledWithoutArgs(AstNode n) {
  if (!n.isList() || n.elems.size() != 3) return false;
  list<AstNode>::iterator p = n.elems.begin();
  if (*p != Token("(")) return false;
  ++p;
  return isInfixOp(*p);
}

bool parseableAsFloat(string s) {
  errno = 0;
  const char* end = skipFloat(s.c_str());
  return *end == '\0' && errno == 0;
}

const char* skipFloat(const char* s) {
  char* end = NULL;
  strtof(s, &end);
  return end;
}
