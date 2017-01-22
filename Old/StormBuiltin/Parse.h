#pragma once
#include "Tokenizer.h"

class CppSuper;

void skipTemplate(Tokenizer &tok);
String parsePkg(Tokenizer &tok);
CppSuper findSuper(Tokenizer &tok);
