#ifndef AST_H
#define AST_H

#define TEMP_PROGRAM_CODE "VP4C_PROGRAM_CODE"
#define VIRT_TYPE_ID "VirtType"

#define VIRT_PACKET_IN_HEADER_TYPE "VirtPacketInHeaderType"
#define VIRT_PACKET_IN_HEADER_NAME "virtPacketInHeader"
#define VIRT_PACKET_IN_HEADER_PROG_ID_FIELD_NAME "programID"

#define VIRT_PACKET_OUT_HEADER_TYPE "VirtPacketOutHeaderType"
#define VIRT_PACKET_OUT_HEADER_PROG_ID_FIELD_NAME "programID"

#define VIRT_PREVENT_MERGE_ANNOTATION "VirtPreventMerge"

#define NULL_CLONE(a) (a == NULL ? NULL : a->Clone())
#define NULL_SET_PARENT(a, b) if(a != NULL) a->parent = b

#define DEFAULT_TABLE_SIZE 512
#define HEADER_STACK_PROGRAM_TYPE_SUFFIX " - Stack"

enum NodeType {NT_NONE, NT_PROGRAM, NT_EXPRESSION, NT_CONTROL_DECLARATION, NT_EXTERN_DECLARATION, NT_PARSER_DECLARATION, NT_CONTROL_TYPE_DECLARATION, NT_PARSER_TYPE_DECLARATION, NT_PACKAGE_TYPE_DECLARATION, NT_HEADER_TYPE_DECLARATION, NT_HEADER_UNION_DECLARATION, NT_STRUCT_TYPE_DECLARATION, NT_TYPEDEF_DECLARATION, NT_ENUM_DECLARATION, NT_ERROR_DECLARATION, NT_MATCH_KIND_DECLARATION, NT_CONTROL_LOCAL_DEC_LIST, NT_STAT_OR_DEC_LIST, 
               NT_ASSIGN_STATEMENT, NT_METHOD_CALL_STATEMENT, NT_CONDITIONAL_STATEMENT, NT_EXIT_STATEMENT, NT_SWITCH_STATEMENT, NT_SWITCH_CASE_LIST, NT_SWITCH_CASE, NT_RETURN_STATEMENT, NT_EMPTY_STATEMENT, NT_LITERAL, NT_IDENTIFIER_LIST, NT_BASE_TYPE, NT_TUPLE_TYPE, NT_TYPE_NAME, NT_STRUCT_FIELD_LIST, NT_PARAMETER_LIST, NT_PARAMETER, NT_ARGUMENT_LIST, NT_ARGUMENT, NT_DIRECTION, NT_STRUCT_FIELD,
               NT_VARIABLE_DECLARATION, NT_CONSTANT_DECLARATION, NT_ACTION_DECLARATION, NT_TABLE_DECLARATION, NT_PARSER_STATEMENT_LIST, NT_PARSER_LOCAL_ELEMENT_LIST, NT_PARSER_STATE, NT_PARSER_STATE_LIST, NT_TABLE_PROPERTY_LIST, NT_TABLE_PROPERTY, NT_ACTION_REF, NT_ACTION_LIST, NT_ENTRIES_LIST, NT_ENTRY, NT_KEY_ELEMENT_LIST, NT_KEY_ELEMENT, NT_INSTANTIATION, NT_OBJECT_DECLARATION_LIST,
               NT_SELECT_EXPRESSION, NT_MASK, NT_RANGE, NT_SIMPLE_KEYSET_DEFAULT, NT_SIMPLE_KEYSET_DONTCARE, NT_SIMPLE_EXPRESSION_LIST, NT_TUPLE_KEYSET_EXPRESSION, NT_SELECT_CASE_LIST, NT_SELECT_CASE, NT_NAME_STATE_EXPRESSION, NT_DIRECT_APPLICATION, NT_VALUE_SET_DECLARATION, NT_TYPEARG_VOID, NT_TYPEARG_DONTCARE, NT_TYPE_ARGUMENT_LIST, NT_TYPEARG_NONTYPENAME, NT_HEADER_STACK_TYPE,
               NT_EXPRESSION_LIST, NT_ANNOTATION_LIST, NT_ANNOTATION, NT_ANNOTATION_BODY, NT_ANNOTATION_TOKEN_LIST, NT_ANNOTATION_TOKEN, NT_METHOD_PROTOTYPE_LIST, NT_METHOD_PROTOTYPE, NT_CONSTRUCTOR_METHOD_PROTOTYPE, NT_FUNCTION_PROTOTYPE, NT_FUNCTION_DECLARATION, NT_ABSTRACT_FUNCTION_PROTOTYPE, NT_TYPE_OR_VOID, NT_TYPE_PARAMETER, NT_TYPE_PARAMETER_LIST, NT_SPECIFIED_IDENTIFIER_LIST, NT_SPECIFIED_IDENTIFIER};

/*
enum LIT_TYPE {LIT_NONE = -1, LIT_INTEGER = 0, LIT_STRING = 1, LIT_BOOL = 2};

typedef struct lexval{
  LIT_TYPE litType;
  std::string svalue;
} LexVal;
*/

typedef struct {
  int program;
  std::string deparserName;
  int emitNum;
} ExceptionWrongEmitOrder;

typedef std::string LexVal;

class AST{
    public:
        AST();
        //AST(LexVal *v, const NodeType nType);
        AST(const std::string& svalue, const NodeType nType);
        ~AST();

        AST* Clone();

        AST* getRoot();
        std::vector<AST*> getChildren();
        AST* getChildNT(const NodeType nt) const;
        LexVal getValue() const;
        LexVal getValueExpression();
        std::string getProgramType() const;
        NodeType getNodeType() const;
        /*
        Same as value.c_str()
        */
        const char *str() const;

        void setNodeType(NodeType nt);
        void setValue(LexVal value);
        void setProgramType(const std::string& programType);

        void addChild(AST* child);
        void addChildStart(AST* child);
        void RemoveChild(AST* child);

        void Print();
        void GenDot(FILE *fp);
        void GenDot(const std::string& filePath);

        /* Checks for some illegal declarations / statements defined in the P4 language specification */
        bool CheckLegal(bool printErrors, const std::string programName);

        void InferTypes();

        void AddPreventMergeAnnotationBasedOnMergedEmit(int emitNum, const std::string& deparserName);

        //Changes stuff such as "9 % 8" to 1
        bool SimplifyConstExpressions();

        void UnifyErrorMKDeclarations();
        void RemoveErrorAndMKDeclarations();
        void RemoveEmptyIfElses();
        void RemoveEmptySwitches();
        void RemoveUnusedBlocks();
        void RemoveUnusedHeaders();
        void RemoveDuplicateHeaders();
        //Remove states with no statements and simple transitions
        void RemoveSimpleParserStates();
        //Calls CheckParserStates for all children
        void CheckParsersStates();
        void SubstituteTypedef();
        void SubstituteIdentifierWithType(std::string oldID, const std::string& newID, const std::string& type, bool skipMemberAccesses);
        void SubstituteIdentifierWithType(std::string oldID, const std::string& newID, const std::string& type);

        void SubstituteStructFieldIdentifier(std::string oldID, const std::string& newID, const std::string& structTypeName);

        void ExpandEmits();

        void RemoveDeclarationsFromIncludesByAnotation(const std::vector<std::string>& includes);

        //Changes switch statements to if statements
        void RemoveSwitchStatements();

        //Remove all instantiations on parsers / controls
        void RemoveInstantiations();

        //Remove AST nodes that are lists and have no children
        void RemoveEmptyLists();

        //Simply removes the declarations (assumes already substituted).
        void RemoveConstantDeclarations();

        /*
        Tries to remove the prefixes that merge introduces on names
        */
        void RemoveMergePrefixes();

        /*
        Renames the unused fields of the structs / headers
        */
        void FixUnusedStructFieldsNames();

        /*
        If this is a parser declaration, writes the state graph to a file.
        */
        void WriteParserDOT(const std::string& fileName);

        void writeCode(FILE *fp) const;
        void writeCode(const std::string& fileName) const;

        /*
        Generate, for each header, a new header with the unused fields unified.
        Returns a list of pairs with the original header and the new header (which is allocated in this function).
        */
        std::vector<std::pair<AST*, AST*>> GetHeadersWithRemovedUnusedFields();

        void SubstituteAndDeleteChildren(const std::string& oldCName, AST *newC);

        static AST* MergeAST(const std::vector<AST*> programs, const std::vector<std::set<int>>& virtualSwitchPorts, const P4Architecture architecture, const int cpuPort);

        static std::string NodeTypeToString(NodeType nt);
        static std::string BaseTypeToString(AST *ast);

        static std::string RemoveMergePrefix(const std::string& str);

        static bool isP4Keyword(const std::string& word);

        static int GetSizeOfFieldInBits(AST *field);
        static int CalculateStructSize(AST *program, const std::string& structName);
        static int CalculateStructSize(const std::vector<AST*>& structs, const std::string& structName);

        friend class TofinoMerge;
        friend class V1Merge;
        friend class CFG;
        friend class PreProcessor;
        friend class ASTParser;
        friend class ASTApplyBlockOptimizer;
        friend class ASTSimplifier;
    private:
        AST *parent;
        std::vector<AST*> children;
        LexVal value;
        NodeType nType;
        std::string programType;
        int lineNumber;

        static std::string GetVirtInstanceP4InfoName(std::string virtInstanceName);

        void SubstituteTypedefRec(std::unordered_map<std::string, AST*> *table);
        void RemoveTypedefRec(std::unordered_map<std::string, AST*> *table);

        void ChangeIdsAfterMergeHeaderListsRec(const std::string& oldsf, const std::string& newsf, const std::string& type);
        void ChangeIdsAfterMergeHeaderLists(AST *oldHeader, AST *newHeader);
        void RenameProgramTypes(const std::string& oldPT, const std::string& newPT);

        void InferTypesAnnotationTokenList(std::map<std::string, std::string>& nameToTypes, std::map<std::string, std::map<std::string, std::string>>& nameToList, const std::string& programName); 
        void InferTypesRec(std::map<std::string, std::string>& nameToTypes, std::map<std::string, std::map<std::string, std::string>>& nameToList, const std::string& programName);

        void ExpandEmitsInControlBlock(const std::unordered_map<std::string, AST*>& structs);

        /*
        Substitute every value and type of "oldValue" to "newValue"
        */
        void SubstituteValue(const std::string& oldValue, const std::string& newValue);

        void SubstituteHeaderType(AST *oldHeader, AST *newHeader, const std::vector<AST*>& programs);
        void SubstituteTypeName(const std::string& oldName, const std::string& newName);

        AST* CreateControlInstanceApplyStatements(AST *argumentList);
        AST* CreateParserInstanceApplyStateList(const std::string& prefix, const std::string& suffix, const std::string& acceptState, AST *argumentList);
        void SubstituteParameter(const std::string& paramName, AST *newAST);
        static AST* CreateParserInstance(AST *instantiation, const std::string& prefix);
        static AST* CreateControlInstance(AST *root, AST *instantiation, const std::string& prefix);
        void RemoveInstantiationsInParser(AST *root);
        static void RemoveInstantiationsInControlRec(AST *root, const std::unordered_map<std::string, AST*>& instanceMap, AST *applyStatements);
        void RemoveInstantiationsInControl(AST *root);

        void RemoveHeaderFieldsPrefixRec(const std::string headerName, const std::string longestP, const AST* parent);
        void RemoveHeaderFieldsPrefix(AST *header);
        /*
        Changes every "PX_Y" to "Y"
        */
        void RemoveHeaderPrefix(const std::string headerName);

        void ChangeHeaderFieldName(const std::string headerName, const std::string oldName, const std::string newName, AST *parent);

        void RemoveControllerHeaderAnnotations(const int cpuPort);

        //Adds padding struct fields to all headers/structs so that they are multiple of 8 bits
        void AddHeaderPaddings();

        /*
        Finds the children with the given value.
        validTypes contains the node types that are a valid match (empty for all types).
        Returns NULL if not found.
        */
        AST* FindChildrenByValue(const LexVal& value, const std::unordered_set<NodeType>& validTypes);

        /*
        Finds the descendant with the given value (leftmost).
        validTypes contains the node types that are a valid match (empty for all types).
        Returns NULL if not found.
        */
        AST* FindDescendantByValue(const LexVal& value, const std::unordered_set<NodeType>& validTypes);


        /*
        Finds a state with the given name in this state list
        Returns NULL if not found
        */
        AST *FindStateInList(const std::string name);

        //Check if the parser has transitions to undeclared states and removes unreacheable states
        void CheckParserStates(const std::string& programName);

        //Checks the program to verify if a given field from a header is ever used
        bool isFieldUsed(const std::string& fieldName, const std::string& headerType);

        //Used to write idented code
        static std::string TAB_SPACES;
        static void IncreaseTabSpaces();
        static void DecreaseTabSpaces();

        //Generates a new VirtType
        static std::string NextVirtTypeID();

        static bool CanMergeStructFieldListByNames(const std::vector<AST*>& headersDec, const std::vector<std::string>& names);
        static AST* MergeStructFieldListByNames(const std::vector<AST*>& headersDec, const std::vector<std::string>& names);

        static void RenameTypeDeclarations(const std::vector<AST *>& programs);
        
        static void AddGlobalActionsToCFG(const std::vector<AST *>& programs);

        static void _RemoveDependencyGraphDummies(AST *graph, std::set<AST*>& dummys);
        static std::vector<std::string> GetDependencyGraphOrder(AST *graph);
        static std::vector<std::string> GetTableOrderFromLinearize(AST *applyBlock);
        static AST* GetSharedTablesDependencyGraph(AST *applyBlock);

        static std::vector<AST*> Linearize(AST *sList);
        static std::vector<AST*> SplitApply(AST* apply, const std::vector<std::string>& sharedTNs, int *maxIfDepth);
        static bool MergeRange(AST *r1, AST *r2);
        
        static std::vector<AST*> MergeParsers(std::vector<AST*> programs, std::vector<AST*> parsers, const std::vector<AST*>& headersDec, const std::string& newParserName, const std::vector<std::set<int>>& virtualSwitchPorts, AST* IG_PORT_EXP);
        static std::vector<AST*> MergeControls(const std::vector<AST*>& programs, const std::vector<AST*>& controls, const std::vector<AST*>& headersDec, const std::string& newControlName, const std::vector<std::set<int>>& virtualSwitchPorts);
        static std::vector<AST*> MergeParsers(std::vector<AST*> programs, std::vector<AST*> parsers, const std::vector<AST*>& headersDec, const std::string& newParserName, const std::vector<std::set<int>>& virtualSwitchPorts, AST* IG_PORT_EXP, const bool printErrors);
        static std::vector<AST*> MergeControls(const std::vector<AST*>& programs, const std::vector<AST*>& controls, const std::vector<AST*>& headersDec, const std::string& newControlName, const std::vector<std::set<int>>& virtualSwitchPorts, const bool printErrors);
        

        static std::vector<AST*> MergeUnifyHeaders(const std::vector<AST*>& programs);
        static std::vector<AST*> MergeUnifyExterns(const std::vector<AST*>& programs);

        static bool TablesMergeable(AST *t1, AST *t2);
        static AST* TablesMerge(AST *t1, AST *t2, int P1, int P2);

        static bool hasVirtPreventMergeAnnotation(AST *ast);
        static bool hasFieldWithVirtPreventMergeAnnotation(AST *SFL);

        /*
        Attempt to allign unused fields in the headers
        If successfull, modifies the headers and returns true
        */
        static bool MakeHeaderEqual(AST *h1, AST *h2);

        static bool Equals(AST *a1, AST *a2);
        static bool EqualsParserDeclaration(AST *a1, AST *a2, AST *p1, AST *p2, const std::vector<AST*>& headersDec);
        static bool EqualsControlDeclaration(AST *a1, AST *a2, AST *c1, AST *c2, const std::vector<AST*>& headersDec);
        static bool EqualActionDeclarations(AST *a1, AST *a2);

        static std::vector<std::pair<int, int>> GetContinuous(const std::set<int>& iSet);

        static void RemoveEqualSwitchCases(AST *switchStatement);

        static bool isConditionVirtProgramIDCheck(AST *condition);
};

int get_line_number();

inline AST::AST(){
  Assert(false); //TODO: Delete this?
  
  this->nType = NT_NONE;
  this->parent = NULL;

  this->lineNumber = get_line_number();
}

/*
inline AST::AST(LexVal *v, const NodeType nType){
  this->parent = NULL;
  this->nType = nType;

  if(v != NULL){
    this->value = *v;
    delete v;
  }

  this->lineNumber = get_line_number();
}
*/

inline AST::AST(const std::string& svalue, const NodeType nType){
  this->parent = NULL;
  this->nType = nType;
  this->value = svalue;

  this->lineNumber = get_line_number();
}

inline AST::~AST(){
  for(AST* c : this->children)
    if(c != NULL)
      delete c;
}

inline AST* AST::Clone(){
  AST* newAST = new AST(this->value, this->nType);
  newAST->programType = this->programType;
  newAST->lineNumber = lineNumber;

  newAST->children.resize(this->children.size(), NULL);
  for(int i = 0; i < (int) this->children.size(); i++){
      if(this->children[i] != NULL){
          newAST->children[i] = this->children[i]->Clone();
      }
  }

  return newAST;
}

inline std::vector<AST*> AST::getChildren(){return this->children;}

inline AST* AST::getChildNT(const NodeType nt) const{
    int i = 0;
    bool found = false;
    while(i < (int) this->children.size() && !found){
        if(this->children[i] != NULL && this->children[i]->nType == nt)
            found = true;
        else
            i++;
    }
    if(found)
        return this->children[i];
    return NULL;
}

inline LexVal AST::getValue() const{return this->value;}

inline LexVal AST::getValueExpression() {
    if(this->value == "."){
        if(this->children.size() == 2){
          return this->children[0]->getValueExpression() + "." + this->children[1]->getValueExpression();
        }
        else if(this->children.size() == 3){
          return this->children[0]->getValueExpression() + "." + this->children[1]->getValueExpression() + "()";
        }
    }
    else if(this->value == "[]"){
      if(this->children.size() == 2){
        return this->children[0]->getValueExpression() + "[" + this->children[1]->getValueExpression() + "]";
      }
    }
    else if(this->value == ":"){
      if(this->children.size() == 2){
        return this->children[0]->getValueExpression() + ":" + this->children[1]->getValueExpression();
      }
    }
    return this->value;
}

inline std::string AST::getProgramType() const{return this->programType;}

inline NodeType AST::getNodeType() const{return this->nType;}

inline void AST::setNodeType(NodeType nt){this->nType = nt;}

inline void AST::setValue(LexVal value){this->value = value;}

inline void AST::setProgramType(const std::string& programType){this->programType = programType;}

inline void AST::addChild(AST* child){this->children.push_back(child); NULL_SET_PARENT(child, this);}

inline void AST::addChildStart(AST* child){
  NULL_SET_PARENT(child, this);
  this->children.push_back(child);
  for(int i = (int) this->children.size() - 1; i > 0; i--)
      this->children[i] = this->children[i - 1];
  this->children[0] = child;
}

inline void AST::SubstituteAndDeleteChildren(const std::string& oldCName, AST *newC){
  for(size_t i = 0; i < this->children.size(); i++)
    if(this->children[i]->value == oldCName){
      delete this->children[i];
      this->children[i] = newC;
      this->children[i]->parent = this;
    }
}

inline void AST::RemoveChild(AST* child){
  for(size_t i = 0; i < this->children.size(); i++){
      if(this->children[i] == child){
          this->children.erase(this->children.begin() + i);
          NULL_SET_PARENT(child, this);
          return;
      }
  }

  Assert(false); 
}

inline const char* AST::str() const {return this->value.c_str();}

inline AST* AST::FindStateInList(const std::string name){
  AST *state = NULL;

  for(AST *s : this->children)
    if(s->value == name)
      state = s;

  return state;
}

inline std::vector<std::pair<int, int>> AST::GetContinuous(const std::set<int>& iSet){
  std::vector<std::pair<int, int>> result;
  int lastP = -1;
    int lastX = -1;
    for(int x : iSet){
        if(lastP == -1){
            lastP = x;
        }
        else if(x != lastX + 1){
            result.push_back(std::make_pair(lastP, lastX));
            lastP = x;
        }
        lastX = x;
    }
    if(lastP != -1)
        result.push_back(std::make_pair(lastP, lastX));
  return result;
}

#endif
