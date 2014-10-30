#pragma once
#include "Tokenizer.h"

class CppName;

void skipTemplate(Tokenizer &tok);
String parsePkg(Tokenizer &tok);
CppName findSuper(Tokenizer &tok);
