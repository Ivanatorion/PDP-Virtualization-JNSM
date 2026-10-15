%{
#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"

void yyerror (char const *s);
int yylex();
int get_line_number();
std::string get_file_name();

AST *ast;

std::vector<std::pair<std::string, int>> includeStack;

std::unordered_set<std::string> parserTypeIdentifiers;
std::vector<std::unordered_set<std::string>> parserTypeIdentifierStack;

//Constants
std::vector<std::unordered_map<std::string, AST*>> constantsStack = {{}};

void CSClear(){
  constantsStack[0].clear();
}

void CSAdd(const std::string& c, AST *exp){
  if(constantsStack[constantsStack.size() - 1].find(c) == constantsStack[constantsStack.size() - 1].end())
    constantsStack[constantsStack.size() - 1].emplace(c, exp);
}

AST* CSFind(const std::string& key){
  int i = (int) constantsStack.size() - 1;
  while(i >= 0){
    auto it = constantsStack[i].find(key);
    if(it != constantsStack[i].end())
      return it->second;
    i--;
  }
  return NULL;
}

void CSSubstitute(AST **ast){
  if((*ast)->getChildren().size() == 0){
    AST *sub = CSFind((*ast)->getValue());
    if(sub != NULL){
      delete (*ast);
      *ast = sub->Clone();
    }
  }
}

//Add an annotation to indicate from what included header a declaration comes from
void CheckAndAddIncludeAnnotation(AST *ast){
  if(includeStack.size() > 0){
    const std::string includeFile = includeStack[0].first;
    Assert(ast->getChildren().size() > 0);
    if(ast->getChildren()[0] == NULL){
      ast->RemoveChild(NULL);
      ast->addChildStart(new AST("", NT_ANNOTATION_LIST));
    }

    AST *annotation = new AST("VirtIncludedDeclaration", NT_ANNOTATION);
    AST *annotationTokenList = new AST("", NT_ANNOTATION_TOKEN_LIST);
    AST *annotationToken = new AST("\"" + includeFile + "\"", NT_ANNOTATION_TOKEN);

    annotation->addChild(annotationTokenList);
    annotationTokenList->addChild(annotationToken);

    ast->getChildren()[0]->addChild(annotation);
  }
}

void CSPush(){
  constantsStack.push_back({});
}

void CSPop(){
  constantsStack.pop_back();
}

inline std::string LexValPointerToString(LexVal *lv){
  if(lv == NULL)
    return "";

  std::string result = std::string(*lv);
  delete lv;
  return result;
}

%}

%define parse.error verbose
%define parse.trace

//%language "c++"

%token TYPEDEF
%token IDENTIFIER
%token TYPE_IDENTIFIER
%token STRING_LITERAL
%token INTEGER
%token HEADER
%token HEADER_UNION
%token BOOL
%token INT
%token ERROR
%token STRING
%token BIT
%token VARBIT
%token CONST
%token CONTROL
%token APPLY
%token IN
%token OUT
%token INOUT
%token IF
%token ELSE
%token MATCH_KIND
%token KEY
%token ACTION
%token ACTIONS
%token STATE
%token ENTRIES
%token TYPE
%token DONTCARE
%token VOID
%token PRAGMA
%token TRUE
%token FALSE
%token THIS
%token PREFIX
%token RETURN
%token EXIT
%token TABLE
%token DEFAULT
%token MASK
%token RANGE
%token COMMA
%token SWITCH
%token SELECT
%token STRUCT
%token VALUESET
%token PARSER
%token PACKAGE
%token ENUM
%token TRANSITION
%token TUPLE
%token EXTERN
%token ABSTRACT

%token TK_OC_LE
%token TK_OC_GE
%token TK_OC_EQ
%token TK_OC_NE
%token TK_OC_AND
%token TK_OC_OR
%token TK_OC_PLUSPLUS
%token TK_OC_SAT_ADD
%token TK_OC_SAT_SUB
%token TOKEN_ERROR
%start p4program

%left ','
%nonassoc '?'
%nonassoc ':'
%left TK_OC_OR
%left TK_OC_AND
%left TK_OC_EQ TK_OC_NE
%left '<' '>' TK_OC_LE TK_OC_GE
%left '|'
%left '^'
%left '&'
%left TK_OC_PLUSPLUS '+' '-' TK_OC_SAT_ADD TK_OC_SAT_SUB
%left '*' '/' MODULUS
%right PREFIX
%nonassoc ']' '(' '['
%left '.'

%precedence ')'

%nonassoc "then"
%nonassoc ELSE

%code requires {
  #include "../include/AST.h"
}

%union {
  LexVal* lexval;
  AST* ast;
}

%type<ast> p4program declaration optAnnotations annotation annotations annotationBody controlDeclaration controlBody controlLocalDeclarations controlLocalDeclaration constantDeclaration externDeclaration tableDeclaration
actionDeclaration parserDeclaration typeDeclaration instantiation errorDeclaration matchKindDeclaration functionDeclaration blockStatement tableProperty tablePropertyList
statOrDeclList optConstructorParameters statementOrDeclaration statement conditionalStatement exitStatement returnStatement switchStatement expression initializer optInitializer variableDeclaration assignmentOrMethodCallStatement
lvalue controlTypeDeclaration baseType typedefDeclaration typeRef headerTypeDeclaration headerUnionDeclaration derivedTypeDeclaration enumDeclaration structTypeDeclaration
structField structFieldList parserTypeDeclaration packageTypeDeclaration nonEmptyParameterList parameter parameterList direction argument argumentList nonEmptyArgList
entry entriesList actionList actionRef keyElementList keyElement identifierList parserLocalElements parserLocalElement parserBlockStatement parserState parserStates parserStatement parserStatements transitionStatement
selectExpression selectCase selectCaseList keysetExpression reducedSimpleKeysetExpression tupleKeysetExpression simpleExpressionList simpleKeysetExpression directApplication stateExpression valueSetDeclaration
tupleType emptyStatement typeArg typeArgumentList realTypeArg realTypeArgumentList expressionList typeOrVoid namedType specializedType functionPrototype methodPrototype methodPrototypes typeParameterList typeParameters optTypeParameters specifiedIdentifier specifiedIdentifierList
switchCase switchCases switchLabel nonBraceExpression objDeclaration objDeclarations objInitializer headerStackType

%type<lexval> nonTypeName prefixedNonTypeName name annotationToken member dotPrefix typeName prefixedType nonTableKwName
IDENTIFIER APPLY KEY ACTIONS STATE TYPE_IDENTIFIER THIS ENTRIES TYPE INTEGER STRING_LITERAL FALSE TRUE TK_OC_AND TK_OC_OR TK_OC_LE TK_OC_GE TK_OC_EQ TK_OC_NE TK_OC_PLUSPLUS TK_OC_SAT_ADD TK_OC_SAT_SUB IF ELSE EXIT RETURN SWITCH
CONTROL BIT BOOL ERROR STRING INT TYPEDEF HEADER STRUCT IN OUT INOUT CONST ACTION TABLE DONTCARE EXTERN PARSER PACKAGE VALUESET TRANSITION TUPLE VOID SELECT DEFAULT
MASK RANGE ABSTRACT MATCH_KIND ENUM HEADER_UNION PRAGMA VARBIT
MODULUS '?' '~' '^' '!' '&' '|' '_' '+' '-' '=' '*' '/' '(' ')' '{' '}' ';' '.' '>' '<' ',' '[' ']' ':' '@'

//%type<lexval> '%'

%%

p4program
: /* empty */ {$$ = NULL;}
| p4program declaration {if($1 == NULL) {$$ = new AST("", NT_PROGRAM); $$->addChild($2);} else {$$ = $1; $$->addChild($2);} ast = $$;}
| p4program ';' {delete $2; $$ = $1;}
/* empty declaration */
;

declaration
: constantDeclaration {$$ = $1;}
| externDeclaration {$$ = $1; CheckAndAddIncludeAnnotation($$);}
| actionDeclaration {$$ = $1; CheckAndAddIncludeAnnotation($$);}
| parserDeclaration {$$ = $1; CheckAndAddIncludeAnnotation($$->getChildren()[0]);}
| typeDeclaration {$$ = $1; CheckAndAddIncludeAnnotation($$);}
| controlDeclaration {$$ = $1; CheckAndAddIncludeAnnotation($$->getChildren()[0]);}
| instantiation {$$ = $1;}
| errorDeclaration {$$ = $1;}
| matchKindDeclaration {$$ = $1;}
| functionDeclaration {$$ = $1;}
;

externDeclaration
: optAnnotations EXTERN nonTypeName optTypeParameters {std::unordered_set<std::string> newTypeIds; parserTypeIdentifiers.emplace(*$3); if($4 != NULL) {
    for(AST* child : $4->getChildren()){
      parserTypeIdentifiers.emplace(child->getValue());
    } 
  } parserTypeIdentifierStack.push_back(newTypeIds);
} '{' methodPrototypes '}' {parserTypeIdentifierStack.pop_back(); delete $2; delete $6; delete $8; $$ = new AST(LexValPointerToString($3), NT_EXTERN_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($7);}
| optAnnotations EXTERN functionPrototype ';' {delete $2; delete $4; $$ = new AST($3->getValue(), NT_EXTERN_DECLARATION); $$->addChild($1); $$->addChild($3);}
;

methodPrototypes
: /* empty */ {$$ = NULL;}
| methodPrototypes methodPrototype {if($1 == NULL) {$$ = new AST("", NT_METHOD_PROTOTYPE_LIST);} else {$$ = $1;} $$->addChild($2);}
;

methodPrototype
: optAnnotations functionPrototype ';' {delete $3; $$ = new AST("", NT_METHOD_PROTOTYPE); $$->addChild($1); $$->addChild($2);}
| optAnnotations TYPE_IDENTIFIER '(' parameterList ')' ';' {delete $3; delete $5; delete $6; $$ = new AST("", NT_METHOD_PROTOTYPE); AST *c = new AST(LexValPointerToString($2), NT_CONSTRUCTOR_METHOD_PROTOTYPE); c->addChild($4); $$->addChild($1); $$->addChild(c);} //constructor
| optAnnotations ABSTRACT functionPrototype ';' {delete $2; delete $4; $$ = new AST("", NT_METHOD_PROTOTYPE); $$->addChild($1); $$->addChild($3); $3->setNodeType(NT_ABSTRACT_FUNCTION_PROTOTYPE);}
;

typedefDeclaration
: optAnnotations TYPEDEF typeRef name ';' {delete $2; delete $5; $$ = new AST(LexValPointerToString($4), NT_TYPEDEF_DECLARATION); $$->addChild($1); $$->addChild($3);}
| optAnnotations TYPEDEF derivedTypeDeclaration name ';' {delete $2; delete $5; $$ = new AST(LexValPointerToString($4), NT_TYPEDEF_DECLARATION); $$->addChild($1); $$->addChild($3);}
;

typeDeclaration
: derivedTypeDeclaration {$$ = $1; parserTypeIdentifiers.emplace($1->getValue());}
| typedefDeclaration {$$ = $1; parserTypeIdentifiers.emplace($1->getValue());}
| parserTypeDeclaration ';' {delete $2; $$ = $1;}
| controlTypeDeclaration ';' {delete $2; $$ = $1;}
| packageTypeDeclaration ';' {delete $2; $$ = $1;}
;

parserTypeDeclaration
: optAnnotations PARSER name optTypeParameters {std::unordered_set<std::string> newTypeIds; if($4 != NULL) {
  for(AST* child : $4->getChildren()){
    newTypeIds.emplace(child->getValue());
  }
} parserTypeIdentifierStack.push_back(newTypeIds);} '(' parameterList ')' {parserTypeIdentifierStack.pop_back(); delete $2; delete $6; delete $8; $$ = new AST(LexValPointerToString($3), NT_PARSER_TYPE_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($7); parserTypeIdentifiers.emplace($$->getValue());}
;

parserDeclaration
: parserTypeDeclaration optConstructorParameters '{' {CSPush();} parserLocalElements parserStates '}' {CSPop(); delete $3; delete $7; $$ = new AST("", NT_PARSER_DECLARATION); $$->addChild($1); $$->addChild($2); $$->addChild($5); $$->addChild($6); $$->setValue($1->getValue());}
;

packageTypeDeclaration
: optAnnotations PACKAGE name optTypeParameters '(' parameterList ')' {delete $2; delete $5; delete $7; $$ = new AST(LexValPointerToString($3), NT_PACKAGE_TYPE_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($6); parserTypeIdentifiers.emplace($$->getValue());}
;

derivedTypeDeclaration
: headerTypeDeclaration {$$ = $1;}
| headerUnionDeclaration {$$ = $1;}
| structTypeDeclaration {$$ = $1;}
| enumDeclaration {$$ = $1;}
;

enumDeclaration
: optAnnotations ENUM name '{' identifierList '}' {delete $2; delete $4; delete $6; $$ = new AST(LexValPointerToString($3), NT_ENUM_DECLARATION); $$->addChild($1); $$->addChild(NULL); $$->addChild($5);}
| optAnnotations ENUM name '{' identifierList ',' '}' {delete $2; delete $4; delete $6; delete $7; $$ = new AST(LexValPointerToString($3), NT_ENUM_DECLARATION); $$->addChild($1); $$->addChild(NULL); $$->addChild($5);}
| optAnnotations ENUM typeRef name '{' specifiedIdentifierList '}' {delete $2; delete $5; delete $7; $$ = new AST(LexValPointerToString($4), NT_ENUM_DECLARATION); $$->addChild($1); $$->addChild($3); $$->addChild($6);}
| optAnnotations ENUM typeRef name '{' specifiedIdentifierList ',' '}' {delete $2; delete $5; delete $7; delete $8; $$ = new AST(LexValPointerToString($4), NT_ENUM_DECLARATION); $$->addChild($1); $$->addChild($3); $$->addChild($6);}
;

specifiedIdentifierList
: specifiedIdentifier {$$ = new AST("", NT_SPECIFIED_IDENTIFIER_LIST); $$->addChild($1);}
| specifiedIdentifierList ',' specifiedIdentifier {delete $2; $$ = $1; $$->addChild($3);}
;

specifiedIdentifier
: name '=' initializer {delete $2; $$ = new AST(LexValPointerToString($1), NT_SPECIFIED_IDENTIFIER); $$->addChild($3);}
;

parserLocalElements
: /* empty */ {$$ = NULL;}
| parserLocalElements parserLocalElement {if($1 == NULL) {$$ = new AST("", NT_PARSER_LOCAL_ELEMENT_LIST);} else {$$ = $1;} $$->addChild($2);}
;

parserStates
: parserState {$$ = new AST("", NT_PARSER_STATE_LIST); $$->addChild($1);}
| parserStates parserState {$$ = $1; $$->addChild($2);}
;

parserLocalElement
: constantDeclaration {$$ = $1;}
| variableDeclaration {$$ = $1;}
| valueSetDeclaration {$$ = $1;}
| instantiation {$$ = $1;}
;

parserState
: optAnnotations STATE name '{' {CSPush();} parserStatements transitionStatement '}' {CSPop(); delete $2; delete $4; delete $8; $$ = new AST(LexValPointerToString($3), NT_PARSER_STATE); $$->addChild($1); $$->addChild($6); $$->addChild($7);}
;

parserStatements
: /* empty */ {$$ = NULL;}
| parserStatements parserStatement  {if($1 == NULL) {$$ = new AST("", NT_PARSER_STATEMENT_LIST); $$->addChild($2);} else {$$ = $1; $$->addChild($2);}}
;

parserStatement
: assignmentOrMethodCallStatement {$$ = $1;}
| directApplication {$$ = $1;}
| parserBlockStatement {$$ = $1;}
| constantDeclaration {$$ = $1;}
| variableDeclaration {$$ = $1;}
| emptyStatement {$$ = $1;}
| conditionalStatement {$$ = $1;}
;

parserBlockStatement
: '{' {CSPush();} parserStatements '}' {CSPop(); delete $1; delete $4; $$ = $3;}
/* : optAnnotations '{' parserStatements '}' */
;

directApplication
: typeName '.' APPLY '(' argumentList ')' ';' {delete $2; delete $3; delete $4; delete $6; delete $7; $$ = new AST(LexValPointerToString($1), NT_DIRECT_APPLICATION); $$->addChild($5);}
;

transitionStatement
: /* empty */ {$$ = new AST("reject", NT_NAME_STATE_EXPRESSION);} //Explicit reject trasition
| TRANSITION stateExpression {delete $1; $$ = $2;}
;

stateExpression
: name ';' {delete $2; $$ = new AST(LexValPointerToString($1), NT_NAME_STATE_EXPRESSION);}
| selectExpression {$$ = $1;}
;

valueSetDeclaration
: optAnnotations VALUESET '<' baseType '>' '(' expression ')' name ';' {delete $2; delete $3; delete $5; delete $6; delete $8; delete $10; $$ = new AST(LexValPointerToString($9), NT_VALUE_SET_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($7);}
| optAnnotations VALUESET '<' tupleType '>' '(' expression ')' name ';' {delete $2; delete $3; delete $5; delete $6; delete $8; delete $10; $$ = new AST(LexValPointerToString($9), NT_VALUE_SET_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($7);}
| optAnnotations VALUESET '<' typeName '>' '(' expression ')' name ';' {delete $2; delete $3; delete $5; delete $6; delete $8; delete $10; $$ = new AST(LexValPointerToString($9), NT_VALUE_SET_DECLARATION); $$->addChild($1); $$->addChild(new AST(LexValPointerToString($4), NT_TYPE_NAME)); $$->addChild($7);}
;

tupleType
: TUPLE '<' typeArgumentList '>' {delete $2; delete $4; $$ = new AST(LexValPointerToString($1), NT_TUPLE_TYPE); $$->addChild($3);}
;

errorDeclaration
: ERROR '{' identifierList '}' {delete $2; delete $4; $$ = new AST(LexValPointerToString($1), NT_ERROR_DECLARATION); $$->addChild($3);}
;

matchKindDeclaration
: MATCH_KIND '{' identifierList '}' {delete $2; delete $4; $$ = new AST(LexValPointerToString($1), NT_MATCH_KIND_DECLARATION); $$->addChild($3);}
| MATCH_KIND '{' identifierList ',' '}' {delete $2; delete $4; delete $5; $$ = new AST(LexValPointerToString($1), NT_MATCH_KIND_DECLARATION); $$->addChild($3);}
;

identifierList
: name {$$ = new AST("", NT_IDENTIFIER_LIST); $$->addChild(new AST(LexValPointerToString($1), NT_NONE));}
| identifierList ',' name {delete $2; $$ = $1; $$->addChild(new AST(LexValPointerToString($3), NT_NONE));}
;

headerTypeDeclaration
: optAnnotations HEADER name optTypeParameters '{' structFieldList '}' {delete $2; delete $5; delete $7; $$ = new AST(LexValPointerToString($3), NT_HEADER_TYPE_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($6);}
;

headerUnionDeclaration
: optAnnotations HEADER_UNION name optTypeParameters '{' structFieldList '}' {delete $2; delete $5; delete $7; $$ = new AST(LexValPointerToString($3), NT_HEADER_UNION_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($6);}
;

structTypeDeclaration
: optAnnotations STRUCT name optTypeParameters '{' structFieldList '}' {delete $2; delete $5; delete $7; $$ = new AST(LexValPointerToString($3), NT_STRUCT_TYPE_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($6);}
;

structFieldList
: /* empty */ {$$ = NULL;}
| structFieldList structField {if($1 == NULL) {$$ = new AST("", NT_STRUCT_FIELD_LIST); $$->addChild($2);} else {$$ = $1; $$->addChild($2);}}
;

structField
: optAnnotations typeRef name ';' {delete $4; $$ = new AST(LexValPointerToString($3), NT_STRUCT_FIELD); $$->addChild($1); $$->addChild($2);}
;

constantDeclaration
: optAnnotations CONST typeRef name '=' initializer ';' {delete $2; delete $5; delete $7; $$ = new AST(LexValPointerToString($4), NT_CONSTANT_DECLARATION); $$->addChild($1); $$->addChild($3); $$->addChild($6); CSAdd($$->getValue(), $6);}
;

controlDeclaration
: controlTypeDeclaration optConstructorParameters
/* no type parameters allowed in controlTypeDeclaration */
'{' {CSPush();} controlLocalDeclarations APPLY controlBody '}' {CSPop(); delete $3; delete $6; delete $8; $$ = new AST("", NT_CONTROL_DECLARATION); $$->addChild($1); $$->addChild($2); $$->addChild($5); $$->addChild($7); $$->setValue($1->getValue());}
;

controlTypeDeclaration
: optAnnotations CONTROL name optTypeParameters {std::unordered_set<std::string> newTypeIds; if($4 != NULL) {
  for(AST* child : $4->getChildren()){
    newTypeIds.emplace(child->getValue());
  }
} parserTypeIdentifierStack.push_back(newTypeIds);} '(' parameterList ')' {parserTypeIdentifierStack.pop_back(); delete $2; delete $6; delete $8; $$ = new AST(LexValPointerToString($3), NT_CONTROL_TYPE_DECLARATION); $$->addChild($1); $$->addChild($4); $$->addChild($7); parserTypeIdentifiers.emplace($$->getValue());}
;

optConstructorParameters
: /* empty */ {$$ = NULL;}
| '(' parameterList ')' {delete $1; delete $3; $$ = $2;}
;

initializer
: expression {$$ = $1;}
;

expression
: INTEGER {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| TRUE {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| FALSE {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| THIS {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| STRING_LITERAL {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| nonTypeName {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); CSSubstitute(&($$));}
//| dotPrefix nonTypeName
| expression '[' expression ']' {delete $2; delete $4; $$ = new AST("[]", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '[' expression ':' expression ']' {delete $2; delete $6; $$ = new AST("[]", NT_EXPRESSION); $$->addChild($1); AST *rhs = new AST(LexValPointerToString($4), NT_EXPRESSION); rhs->addChild($3); rhs->addChild($5); $$->addChild(rhs);}
| '{' expressionList '}' {delete $1; delete $3; $$ = $2;}
//| '{' kvList '}'
| '(' expression ')' {delete $1; delete $3; $$ = $2;}
| '!' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| '~' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| '-' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| '+' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| typeName '.' member {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild(new AST(LexValPointerToString($1), NT_NONE)); $$->addChild(new AST(LexValPointerToString($3), NT_NONE));}
//| ERROR '.' member
| expression '.' member {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild(new AST(LexValPointerToString($3), NT_NONE));}
| expression '*' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '/' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression MODULUS expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '+' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '-' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_SAT_ADD expression {delete $2; $$ = new AST("|+|", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_SAT_SUB expression {delete $2; $$ = new AST("|-|", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_LE expression {delete $2; $$ = new AST("<=", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_GE expression {delete $2; $$ = new AST(">=", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '<' '<' expression {delete $2; delete $3; $$ = new AST("<<", NT_EXPRESSION); $$->addChild($1); $$->addChild($4);}
| expression '>' '>' expression {delete $2; delete $3; $$ = new AST(">>", NT_EXPRESSION); $$->addChild($1); $$->addChild($4);}
| expression '<' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '>' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_NE expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_EQ expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '&' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '^' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '|' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_PLUSPLUS expression {delete $2; $$ = new AST("++", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_AND expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression TK_OC_OR expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| expression '?' expression ':' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); AST *rhs = new AST(LexValPointerToString($4), NT_EXPRESSION); rhs->addChild($3); rhs->addChild($5); $$->addChild(rhs);}
| expression '<' realTypeArgumentList '>' '(' argumentList ')' {delete $2; delete $4; delete $5; delete $7; $$ = $1; $$->addChild($3); $$->addChild($6);}
| expression '(' argumentList ')' {delete $2; delete $4; $$ = $1; $$->addChild($3);}
| namedType '(' argumentList ')' {delete $2; delete $4; $$ = $1; $$->setNodeType(NT_EXPRESSION); $$->addChild($3);}
| '(' typeRef ')' expression {delete $1; delete $3; $$ = new AST("TYPE_CAST", NT_EXPRESSION); $$->addChild($2); $$->addChild($4);}
;

member
: name {$$ = $1;}
;

parameterList
: /* empty */ {$$ = NULL;}
| nonEmptyParameterList {$$ = $1;}
;

nonEmptyParameterList
: parameter {$$ = new AST("", NT_PARAMETER_LIST); $$->addChild($1);}
| nonEmptyParameterList ',' parameter {delete $2; $$ = $1; $$->addChild($3);}
;

parameter
: optAnnotations direction typeRef name {$$ = new AST(LexValPointerToString($4), NT_PARAMETER); $$->addChild($1); $$->addChild($2); $$->addChild($3);}
| optAnnotations direction typeRef name '=' expression {delete $5; $$ = new AST(LexValPointerToString($4), NT_PARAMETER); $$->addChild($1); $$->addChild($2); $$->addChild($3); $$->addChild($6);}
;

direction
: IN {delete $1; $$ = new AST("in", NT_DIRECTION);}
| OUT {delete $1; $$ = new AST("out", NT_DIRECTION);}
| INOUT {delete $1; $$ = new AST("inout", NT_DIRECTION);}
| /* empty */ {$$ = NULL;}
;

controlLocalDeclarations
: /* empty */ {$$ = NULL;}
| controlLocalDeclarations controlLocalDeclaration {if($1 == NULL) {$$ = new AST("", NT_CONTROL_LOCAL_DEC_LIST); $$->addChild($2);} else {$$ = $1; $$->addChild($2);}}
;

controlLocalDeclaration
: constantDeclaration {$$ = $1;}
| variableDeclaration {$$ = $1;}
| actionDeclaration {$$ = $1;}
| tableDeclaration {$$ = $1;}
| instantiation {$$ = $1;}
;

actionDeclaration
: optAnnotations ACTION name '(' parameterList ')' blockStatement {delete $2; delete $4; delete $6; $$ = new AST(LexValPointerToString($3), NT_ACTION_DECLARATION); $$->addChild($1); $$->addChild($5); $$->addChild($7);}
;

tableDeclaration
: optAnnotations TABLE name '{' tablePropertyList '}' {delete $2; delete $4; delete $6; $$ = new AST(LexValPointerToString($3), NT_TABLE_DECLARATION); $$->addChild($1); $$->addChild($5);}
;

tablePropertyList
: tableProperty {$$ = new AST("", NT_TABLE_PROPERTY_LIST); $$->addChild($1);}
| tablePropertyList tableProperty {$$ = $1; $$->addChild($2);}
;

tableProperty
: KEY '=' '{' keyElementList '}' {delete $2; delete $3; delete $5; $$ = new AST(LexValPointerToString($1), NT_TABLE_PROPERTY); $$->addChild($4);}
| ACTIONS '=' '{' actionList '}' {delete $2; delete $3; delete $5; $$ = new AST(LexValPointerToString($1), NT_TABLE_PROPERTY); $$->addChild($4);}
| CONST ENTRIES '=' '{' entriesList '}' {delete $1; delete $2; delete $3; delete $4; delete $6; $$ = new AST("const entries", NT_TABLE_PROPERTY); $$->addChild($5);}
| CONST nonTableKwName '=' initializer ';' {delete $1; delete $3; delete $5; *($2) = "const " + *($2); $$ = new AST(LexValPointerToString($2), NT_TABLE_PROPERTY); $$->addChild($4);}
| nonTableKwName '=' initializer ';' {delete $2; delete $4; $$ = new AST(LexValPointerToString($1), NT_TABLE_PROPERTY); $$->addChild($3);}
/*
| optAnnotations CONST ENTRIES '=' '{' entriesList '}'
| optAnnotations CONST nonTableKwName '=' initializer ';'
| optAnnotations nonTableKwName '=' initializer ';'
*/
;

actionList
: /* empty */ {$$ = NULL;}
| actionList optAnnotations actionRef ';' {delete $4; if($1 == NULL) {$$ = new AST("", NT_ACTION_LIST); $$->addChild($2); $$->addChild($3);} else {$$ = $1; $$->addChild($2); $$->addChild($3);}}
;

actionRef
: prefixedNonTypeName {$$ = new AST(LexValPointerToString($1), NT_ACTION_REF); $$->addChild(NULL);}
| prefixedNonTypeName '(' argumentList ')' {delete $2; delete $4; $$ = new AST(LexValPointerToString($1), NT_ACTION_REF); $$->addChild($3);}
;

entriesList
: entry {$$ = new AST("", NT_ENTRIES_LIST); $$->addChild($1);}
| entriesList entry {$$ = $1; $$->addChild($2);}
;

entry
: keysetExpression ':' actionRef optAnnotations ';' {delete $2; delete $5; $$ = new AST("", NT_ENTRY); $$->addChild($1); $$->addChild($3); $$->addChild($4);}
;

nonTableKwName
: IDENTIFIER {$$ = $1;}
| TYPE_IDENTIFIER {$$ = $1;}
| APPLY {$$ = $1;}
| STATE {$$ = $1;}
| TYPE {$$ = $1;}
;

keyElementList
: /* empty */ {$$ = NULL;}
| keyElementList keyElement {if($1 == NULL) {$$ = new AST("", NT_KEY_ELEMENT_LIST); $$->addChild($2);} else {$$ = $1; $$->addChild($2);}}
;

keyElement
: expression ':' name optAnnotations ';' {delete $2; delete $5; $$ = new AST(LexValPointerToString($3), NT_KEY_ELEMENT); $$->addChild($1); $$->addChild($4);}
;

controlBody
: blockStatement {$$ = $1;}
;

instantiation
: typeRef '(' argumentList ')' name ';' {delete $2; delete $4; delete $6; $$ = new AST(LexValPointerToString($5), NT_INSTANTIATION); $$->addChild(NULL); $$->addChild($1); $$->addChild($3);}
| annotations typeRef '(' argumentList ')' name ';' {delete $3; delete $5; delete $7; $$ = new AST(LexValPointerToString($6), NT_INSTANTIATION); $$->addChild($1); $$->addChild($2); $$->addChild($4);}
| annotations typeRef '(' argumentList ')' name '=' objInitializer ';' {delete $3; delete $5; delete $7; delete $9; $$ = new AST(LexValPointerToString($6), NT_INSTANTIATION); $$->addChild($1); $$->addChild($2); $$->addChild($4); $$->addChild($8);}
| typeRef '(' argumentList ')' name '=' objInitializer ';' {delete $2; delete $4; delete $6; delete $8; $$ = new AST(LexValPointerToString($5), NT_INSTANTIATION); $$->addChild(NULL); $$->addChild($1); $$->addChild($3); $$->addChild($7);}
;

objInitializer
: '{' objDeclarations '}' {delete $1; delete $3; $$ = $2;}
;

objDeclarations
: /* empty */ {$$ = NULL;}
| objDeclarations objDeclaration {if($1 == NULL) {$$ = new AST("", NT_OBJECT_DECLARATION_LIST);} else {$$ = $1;} $$->addChild($2);}
;

objDeclaration
: functionDeclaration {$$ = $1;}
| instantiation {$$ = $1;}
;

typeRef
: baseType {$$ = $1;}
| typeName {$$ = new AST(LexValPointerToString($1), NT_TYPE_NAME);}
| specializedType {$$ = $1;}
| headerStackType {$$ = $1;}
| tupleType {$$ = $1;}
;

headerStackType
: typeName '[' expression ']' {delete $2; delete $4; $$ = new AST("[]", NT_HEADER_STACK_TYPE); $$->addChild(new AST(LexValPointerToString($1), NT_TYPE_NAME)); $$->addChild($3);}
| specializedType '[' expression ']' {delete $2; delete $4; $$ = new AST("[]", NT_HEADER_STACK_TYPE); $$->addChild($1); $$->addChild($3);}
;

dotPrefix
: '.' {$$ = $1;}
;

namedType
: typeName {$$ = new AST(LexValPointerToString($1), NT_TYPE_NAME);}
| specializedType {$$ = $1;}
;

prefixedType
: TYPE_IDENTIFIER {$$ = $1;}
| dotPrefix TYPE_IDENTIFIER
;

typeName 
: prefixedType {$$ = $1;}
;

specializedType
: prefixedType '<' typeArgumentList '>' {delete $2; delete $4; $$ = new AST(LexValPointerToString($1), NT_TYPE_NAME); $$->addChild($3);}
;

typeArgumentList
: /* empty */ {$$ = NULL;}
| typeArg {$$ = new AST("", NT_TYPE_ARGUMENT_LIST); $$->addChild($1);}
| typeArgumentList ',' typeArg {delete $2; if($1 == NULL) {$$ = new AST("", NT_TYPE_ARGUMENT_LIST);} else {$$ = $1;} $$->addChild($3);}
;

typeArg
: DONTCARE {$$ = new AST(LexValPointerToString($1), NT_TYPEARG_DONTCARE);}
| typeRef {$$ = $1;}
| nonTypeName {$$ = new AST(LexValPointerToString($1), NT_TYPEARG_NONTYPENAME);}
| VOID {$$ = new AST(LexValPointerToString($1), NT_TYPEARG_VOID);}
;

baseType
: BOOL {$$ = new AST(LexValPointerToString($1), NT_BASE_TYPE);}
| ERROR {$$ = new AST(LexValPointerToString($1), NT_BASE_TYPE);}
| STRING {$$ = new AST(LexValPointerToString($1), NT_BASE_TYPE);}
| INT {$$ = new AST(LexValPointerToString($1), NT_BASE_TYPE);}
| BIT {$$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); AST *c = new AST("1", NT_LITERAL); $$->addChild(c);} //This is the same as bit<1>
| BIT '<' INTEGER '>' {delete $2; delete $4; AST *c = new AST(LexValPointerToString($3), NT_LITERAL); $$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); $$->addChild(c);}
| INT '<' INTEGER '>' {delete $2; delete $4; AST *c = new AST(LexValPointerToString($3), NT_LITERAL); $$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); $$->addChild(c);}
| VARBIT '<' INTEGER '>' {delete $2; delete $4; AST *c = new AST(LexValPointerToString($3), NT_LITERAL); $$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); $$->addChild(c);}
| BIT '<' '(' expression ')' '>' {delete $2; delete $3; delete $5; delete $6; AST *c = $4; c->SimplifyConstExpressions(); $$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); $$->addChild(c);}
| INT '<' '(' expression ')' '>' {delete $2; delete $3; delete $5; delete $6; AST *c = $4; c->SimplifyConstExpressions(); $$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); $$->addChild(c);}
| VARBIT '<' '(' expression ')' '>' {delete $2; delete $3; delete $5; delete $6; AST *c = $4; c->SimplifyConstExpressions(); $$ = new AST(LexValPointerToString($1), NT_BASE_TYPE); $$->addChild(c);}
;

nonTypeName 
: IDENTIFIER {$$ = $1;}
| APPLY {$$ = $1;}
| KEY {$$ = $1;}
| ACTIONS {$$ = $1;}
| STATE {$$ = $1;}
| ENTRIES {$$ = $1;}
| TYPE {$$ = $1;}
;

name 
: nonTypeName {$$ = $1;}
| TYPE_IDENTIFIER {$$ = $1;}
;

optTypeParameters
: /* empty */ {$$ = NULL;}
| typeParameters {$$ = $1;}
;

typeParameters
: '<' typeParameterList '>' {delete $1; delete $3; $$ = $2;}
;

typeParameterList
: name {$$ = new AST("", NT_TYPE_PARAMETER_LIST); $$->addChild(new AST(LexValPointerToString($1), NT_TYPE_PARAMETER));}
| typeParameterList ',' name {delete $2; $$ = $1; $$->addChild(new AST(LexValPointerToString($3), NT_TYPE_PARAMETER));}
;

realTypeArg
: DONTCARE {$$ = new AST(LexValPointerToString($1), NT_TYPEARG_DONTCARE);}
| typeRef {$$ = $1;}
| VOID {$$ = new AST(LexValPointerToString($1), NT_TYPEARG_VOID);}
;

functionDeclaration
: functionPrototype blockStatement {$$ = new AST("", NT_FUNCTION_DECLARATION); $$->addChild($1); $$->addChild($2);}
;

functionPrototype
: typeOrVoid name optTypeParameters {std::unordered_set<std::string> newTypeIds; if($3 != NULL) {
  for(AST* child : $3->getChildren()){
    newTypeIds.emplace(child->getValue());
  }
} parserTypeIdentifierStack.push_back(newTypeIds); } '(' parameterList ')' {parserTypeIdentifierStack.pop_back(); delete $5; delete $7; $$ = new AST(LexValPointerToString($2), NT_FUNCTION_PROTOTYPE); $$->addChild($1); $$->addChild($3); $$->addChild($6);}
;

typeOrVoid
: typeRef {$$ = $1;}
| VOID {$$ = new AST(LexValPointerToString($1), NT_TYPE_OR_VOID);}
| IDENTIFIER {$$ = new AST(LexValPointerToString($1), NT_TYPE_OR_VOID);}
// may be a type variable
;

argumentList 
: /* empty */ {$$ = NULL;}
| nonEmptyArgList {$$ = $1;}
;

nonEmptyArgList
: argument {$$ = new AST("", NT_ARGUMENT_LIST); $$->addChild($1);}
| nonEmptyArgList ',' argument {delete $2; $$ = $1; $$->addChild($3);}
;

argument
: expression {$$ = new AST("", NT_ARGUMENT); $$->addChild($1);}
| name '=' expression {delete $2; $$ = new AST(LexValPointerToString($1), NT_ARGUMENT); $$->addChild($3);}
| DONTCARE {delete $1; $$ = new AST("_", NT_ARGUMENT); $$->addChild(NULL);}
;

/*
kvList
: kvPair {printf("Not implemented!\n"); exit(1);}
| kvList ',' kvPair {printf("Not implemented!\n"); exit(1);}
;

kvPair
: name '=' expression {printf("Not implemented!\n"); exit(1);}
;
*/

expressionList
: /* empty */ {$$ = NULL;}
| expression {$$ = new AST("", NT_EXPRESSION_LIST); $$->addChild($1);}
| expressionList ',' expression {delete $2; if($1 == NULL) {$$ = new AST("", NT_EXPRESSION_LIST);} else {$$ = $1;} $$->addChild($3);}
;

statement
: assignmentOrMethodCallStatement {$$ = $1;}
| conditionalStatement {$$ = $1;}
| emptyStatement {$$ = $1;}
| blockStatement {$$ = $1;}
| exitStatement {$$ = $1;}
| returnStatement {$$ = $1;}
| switchStatement {$$ = $1;}
;

conditionalStatement
: IF '(' expression ')' statement {delete $1; delete $2; delete $4; $$ = new AST("IF", NT_CONDITIONAL_STATEMENT); $$->addChild($3); $$->addChild($5); $$->addChild(NULL);} %prec "then"
| IF '(' expression ')' statement ELSE statement {delete $1; delete $2; delete $4; delete $6; $$ = new AST("IF", NT_CONDITIONAL_STATEMENT); $$->addChild($3); $$->addChild($5); if($7->getNodeType() != NT_STAT_OR_DEC_LIST) {AST *s = $7; $7 = new AST("", NT_STAT_OR_DEC_LIST); $7->addChild(s);} $$->addChild($7);}
;

switchStatement
: SWITCH '(' expression ')' '{' switchCases '}' {delete $1; delete $2; delete $4; delete $5; delete $7; $$ = new AST("", NT_SWITCH_STATEMENT); $$->addChild($3); $$->addChild($6);}
;

switchCases
: /* empty */ {$$ = NULL;}
| switchCases switchCase {if($1 == NULL) {$$ = new AST("", NT_SWITCH_CASE_LIST);} else {$$ = $1;} $$->addChild($2);}
;

switchCase
: switchLabel ':' blockStatement {delete $2; $$ = new AST("", NT_SWITCH_CASE); $$->addChild($1); $$->addChild($3);}
| switchLabel ':' {delete $2; $$ = new AST("", NT_SWITCH_CASE); $$->addChild($1);}
;

switchLabel
: DEFAULT {$$ = new AST(LexValPointerToString($1), NT_SIMPLE_KEYSET_DEFAULT);}
| nonBraceExpression {$$ = $1;}
;

nonBraceExpression
: INTEGER {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| STRING_LITERAL {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| TRUE {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| FALSE {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| THIS {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION);}
| nonTypeName {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); CSSubstitute(&($$));}
//| dotPrefix nonTypeName
| nonBraceExpression '[' expression ']' {delete $2; delete $4; $$ = new AST("[]", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '[' expression ':' expression ']' {delete $2; delete $6; $$ = new AST("[]", NT_EXPRESSION); $$->addChild($1); AST *rhs = new AST(LexValPointerToString($4), NT_EXPRESSION); rhs->addChild($3); rhs->addChild($5); $$->addChild(rhs);}
| '(' expression ')' {delete $1; delete $3; $$ = $2;}
| '!' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| '~' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| '-' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| '+' expression %prec PREFIX {$$ = new AST(LexValPointerToString($1), NT_EXPRESSION); $$->addChild($2);}
| typeName '.' member {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild(new AST(LexValPointerToString($1), NT_NONE)); $$->addChild(new AST(LexValPointerToString($3), NT_NONE));}
//| ERROR '.' member
| nonBraceExpression '.' member {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild(new AST(LexValPointerToString($3), NT_NONE));}
| nonBraceExpression '*' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '/' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression MODULUS expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '+' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '-' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_SAT_ADD expression {delete $2; $$ = new AST("|+|", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_SAT_SUB expression {delete $2; $$ = new AST("|-|", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_LE expression {delete $2; $$ = new AST("<=", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_GE expression {delete $2; $$ = new AST(">=", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '<' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '>' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_NE expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_EQ expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '<' '<' expression {delete $2; delete $3; $$ = new AST("<<", NT_EXPRESSION); $$->addChild($1); $$->addChild($4);}
| nonBraceExpression '>' '>' expression {delete $2; delete $3; $$ = new AST(">>", NT_EXPRESSION); $$->addChild($1); $$->addChild($4);}
| nonBraceExpression '&' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '^' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '|' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_PLUSPLUS expression {delete $2; $$ = new AST("++", NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_AND expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression TK_OC_OR expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); $$->addChild($3);}
| nonBraceExpression '?' expression ':' expression {$$ = new AST(LexValPointerToString($2), NT_EXPRESSION); $$->addChild($1); AST *rhs = new AST(LexValPointerToString($4), NT_EXPRESSION); rhs->addChild($3); rhs->addChild($5); $$->addChild(rhs);}
| nonBraceExpression '<' realTypeArgumentList '>' '(' argumentList ')' {delete $2; delete $4; delete $5; delete $7; $$ = $1; $$->addChild($3); $$->addChild($6);}
| nonBraceExpression '(' argumentList ')' {delete $2; delete $4; $$ = $1; $$->setNodeType(NT_EXPRESSION); $$->addChild($3);}
| namedType '(' argumentList ')' {delete $2; delete $4; $$ = $1; $$->setNodeType(NT_EXPRESSION); $$->addChild($3);}
| '(' typeRef ')' expression {delete $1; delete $3; $$ = new AST("TYPE_CAST", NT_EXPRESSION); $$->addChild($2); $$->addChild($4);}
;

realTypeArgumentList
: realTypeArg {$$ = new AST("", NT_TYPE_ARGUMENT_LIST); $$->addChild($1);}
| realTypeArgumentList ',' typeArg {delete $2; if($1 == NULL) {$$ = new AST("", NT_TYPE_ARGUMENT_LIST);} else {$$ = $1;} $$->addChild($3);}
;

assignmentOrMethodCallStatement
: lvalue '(' argumentList ')' ';' {delete $2; delete $4; delete $5; $$ = new AST("", NT_METHOD_CALL_STATEMENT); $$->addChild($1); $$->addChild(NULL); $$->addChild($3);}
| lvalue '<' typeArgumentList '>' '(' argumentList ')' ';' {delete $2; delete $4; delete $5; delete $7; delete $8; $$ = new AST("", NT_METHOD_CALL_STATEMENT); $$->addChild($1); $$->addChild($3); $$->addChild($6);}
| lvalue '=' expression ';' {delete $4; $$ = new AST(LexValPointerToString($2), NT_ASSIGN_STATEMENT); $$->addChild($1); $$->addChild($3);}
;

emptyStatement
: ';' {delete $1; $$ = new AST("", NT_EMPTY_STATEMENT);}
;

blockStatement
: '{' {CSPush();} statOrDeclList '}' {CSPop(); delete $1; delete $4; $$ = $3;}
/* : optAnnotations '{' statOrDeclList '}' */
;

statOrDeclList
: /* empty */ {$$ = NULL;}
| statOrDeclList statementOrDeclaration {if($1 == NULL) {$$ = new AST("", NT_STAT_OR_DEC_LIST); $$->addChild($2);} else {$$ = $1; $$->addChild($2);}}
;

statementOrDeclaration
: variableDeclaration {$$ = $1;}
| constantDeclaration {$$ = $1;}
| statement {$$ = $1;}
//| instantiation
;

lvalue
: prefixedNonTypeName {$$ = new AST(LexValPointerToString($1), NT_NONE);}
| THIS {$$ = new AST(LexValPointerToString($1), NT_NONE);}
| lvalue '.' member  {$$ = new AST(LexValPointerToString($2), NT_NONE); $$->addChild($1); $$->addChild(new AST(LexValPointerToString($3), NT_NONE));}
| lvalue '[' expression ']' {delete $2; delete $4; $$ = new AST("[]", NT_NONE); $$->addChild($1); $$->addChild($3);}
| lvalue '[' expression ':' expression ']' {delete $2; delete $6; $$ = new AST("[]", NT_NONE); $$->addChild($1); AST *rhs = new AST(LexValPointerToString($4), NT_EXPRESSION); rhs->addChild($3); rhs->addChild($5); $$->addChild(rhs);}
;

prefixedNonTypeName
: nonTypeName {$$ = $1;}
//| dotPrefix nonTypeName
;

returnStatement
: RETURN ';' {delete $2; $$ = new AST(LexValPointerToString($1), NT_RETURN_STATEMENT);}
| RETURN expression ';' {delete $3; $$ = new AST(LexValPointerToString($1), NT_RETURN_STATEMENT); $$->addChild($2);}
;

exitStatement
: EXIT ';' {delete $2; $$ = new AST(LexValPointerToString($1), NT_EXIT_STATEMENT);}
;

variableDeclaration
: typeRef name optInitializer ';' {delete $4; $$ = new AST(LexValPointerToString($2), NT_VARIABLE_DECLARATION); $$->addChild(NULL); $$->addChild($1); $$->addChild($3);}
| annotations typeRef name optInitializer ';' {delete $5; $$ = new AST(LexValPointerToString($3), NT_VARIABLE_DECLARATION); $$->addChild($1); $$->addChild($2); $$->addChild($4);}
;

optInitializer
: /* empty */ {$$ = NULL;}
| '=' initializer {delete $1; $$ = $2;}
;

selectExpression
: SELECT '(' expressionList ')' '{' selectCaseList '}' {delete $2; delete $4; delete $5; delete $7; $$ = new AST(LexValPointerToString($1), NT_SELECT_EXPRESSION); $$->addChild($3); $$->addChild($6);}
;

selectCaseList
: /* empty */ {$$ = NULL;}
| selectCaseList selectCase {if($1 == NULL) {$$ = new AST("", NT_SELECT_CASE_LIST); $$->addChild($2);} else {$$ = $1; $$->addChild($2);}}
;

selectCase
: keysetExpression ':' name ';' {delete $2; delete $4; $$ = new AST(LexValPointerToString($3), NT_SELECT_CASE); $$->addChild($1);}
;

keysetExpression
: tupleKeysetExpression {$$ = $1;}
| simpleKeysetExpression {$$ = $1;}
;

tupleKeysetExpression
: '(' simpleKeysetExpression ',' simpleExpressionList ')' {delete $1; delete $3; delete $5; $$ = new AST("", NT_TUPLE_KEYSET_EXPRESSION); $4->addChildStart($2); $$->addChild($4);}
| '(' reducedSimpleKeysetExpression ')' {delete $1; delete $3; $$ = new AST("", NT_TUPLE_KEYSET_EXPRESSION); $$->addChild($2);}
;

simpleExpressionList
: simpleKeysetExpression {$$ = new AST("", NT_SIMPLE_EXPRESSION_LIST); $$->addChild($1);}
| simpleExpressionList ',' simpleKeysetExpression {delete $2; $$ = $1; $$->addChild($3);}
;

reducedSimpleKeysetExpression
: DEFAULT {$$ = new AST(LexValPointerToString($1), NT_SIMPLE_KEYSET_DEFAULT);}
| DONTCARE {$$ = new AST(LexValPointerToString($1), NT_SIMPLE_KEYSET_DONTCARE);}
/*
: expression '&&&' expression
| expression '..' expression
*/
;

simpleKeysetExpression
: expression {$$ = $1;}
| DEFAULT {$$ = new AST(LexValPointerToString($1), NT_SIMPLE_KEYSET_DEFAULT);}
| DONTCARE {$$ = new AST(LexValPointerToString($1), NT_SIMPLE_KEYSET_DONTCARE);}
| expression MASK expression {$$ = new AST(LexValPointerToString($2), NT_MASK); $$->addChild($1); $$->addChild($3);}
| expression RANGE expression {$$ = new AST(LexValPointerToString($2), NT_RANGE); $$->addChild($1); $$->addChild($3);}
;

optAnnotations
: /* empty */ {$$ = NULL;}
| annotations {$$ = $1;}
;

annotations
: annotation {$$ = new AST("", NT_ANNOTATION_LIST); $$->addChild($1);}
| annotations annotation {$$ = $1; $$->addChild($2);}
;

annotation
: '@' name {delete $1; $$ = new AST(LexValPointerToString($2), NT_ANNOTATION); $$->addChild(NULL);}
| '@' name '(' annotationBody ')' {delete $1; delete $3; delete $5; $$ = new AST(LexValPointerToString($2), NT_ANNOTATION); $$->addChild($4);}
/*| '@' name '[' structuredAnnotationBody ']'*/ 
;

annotationBody
: /* empty */ {$$ = NULL;}
| annotationBody '(' annotationBody ')' {delete $2; delete $4; $$ = new AST("", NT_ANNOTATION_BODY); $$->addChild($1); $$->addChild($3);}
| annotationBody annotationToken {if($1 == NULL) {$$ = new AST("", NT_ANNOTATION_TOKEN_LIST); AST *c = new AST(LexValPointerToString($2), NT_ANNOTATION_TOKEN); $$->addChild(c);} else { $$ = $1; AST *c = new AST(LexValPointerToString($2), NT_ANNOTATION_TOKEN); $$->addChild(c);}}
;

//structuredAnnotationBody
//: expressionList
//| kvList
//;

annotationToken
: ABSTRACT {$$ = $1;}
| ACTION {$$ = $1;}
| ACTIONS {$$ = $1;}
| APPLY {$$ = $1;}
| BOOL {$$ = $1;}
| BIT {$$ = $1;}
| CONST {$$ = $1;}
| CONTROL {$$ = $1;}
| DEFAULT {$$ = $1;}
| ELSE {$$ = $1;}
| ENTRIES {$$ = $1;}
| ENUM {$$ = $1;}
| ERROR {$$ = $1;}
| EXIT {$$ = $1;}
| EXTERN {$$ = $1;}
| FALSE {$$ = $1;}
| HEADER {$$ = $1;}
| HEADER_UNION {$$ = $1;}
| IF {$$ = $1;}
| IN {$$ = $1;}
| INOUT {$$ = $1;}
| INT {$$ = $1;}
| KEY {$$ = $1;}
| MATCH_KIND {$$ = $1;}
| TYPE {$$ = $1;}
| OUT {$$ = $1;}
| PARSER {$$ = $1;}
| PACKAGE {$$ = $1;}
| PRAGMA {$$ = $1;}
| RETURN {$$ = $1;}
| SELECT {$$ = $1;}
| STATE {$$ = $1;}
| STRING {$$ = $1;}
| STRUCT {$$ = $1;}
| SWITCH {$$ = $1;}
| TABLE {$$ = $1;}
| TRANSITION {$$ = $1;}
| TRUE {$$ = $1;}
| TUPLE {$$ = $1;}
| TYPEDEF {$$ = $1;}
| VARBIT {$$ = $1;}
| VALUESET {$$ = $1;}
| VOID {$$ = $1;}
| '_' {$$ = $1;}
| IDENTIFIER {$$ = $1;}
| TYPE_IDENTIFIER {$$ = $1;}
| STRING_LITERAL {$$ = $1;}
| INTEGER {$$ = $1;}
/* | "&&&" */
/* | ".." */
| TK_OC_AND {$$ = $1;}
| TK_OC_OR {$$ = $1;}
| TK_OC_EQ {$$ = $1;}
| TK_OC_NE {$$ = $1;}
| TK_OC_GE {$$ = $1;}
| TK_OC_LE {$$ = $1;}
/* | '++' */
| '+' {$$ = $1;}
/* | '|+|' */
| '-' {$$ = $1;}
/* | '|-|' */
| '*' {$$ = $1;}
| '/' {$$ = $1;}
| MODULUS {$$ = $1;}
| '|' {$$ = $1;}
| '&' {$$ = $1;}
| '^' {$$ = $1;}
| '~' {$$ = $1;}
| '[' {$$ = $1;}
| ']' {$$ = $1;}
| '{' {$$ = $1;}
| '}' {$$ = $1;}
| '<' {$$ = $1;}
| '>' {$$ = $1;}
| '!' {$$ = $1;}
| ':' {$$ = $1;}
| ',' {$$ = $1;}
| '?' {$$ = $1;}
| '.' {$$ = $1;}
| '=' {$$ = $1;}
| ';' {$$ = $1;}
| '@' {$$ = $1;}
/*| UNKNOWN_TOKEN*/
;

%%

void yyerror (char const *s) {
  printf("Error at \"%s\" line: %d:\n", get_file_name().c_str(), get_line_number());
  printf("---> %s\n", s);

  if(includeStack.size() > 0){
    printf("\n");
    for(int x = (int) includeStack.size() - 1; x >= 0; x--){
      printf("...in file \"%s\" included at line %d\n", includeStack[x].first.c_str(), includeStack[x].second);
    }
  }
}
