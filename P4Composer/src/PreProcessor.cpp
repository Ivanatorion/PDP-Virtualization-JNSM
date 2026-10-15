#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/AST.h"
#include "../include/PreProcessor.h"

PreProcessor::PreProcessor(const std::string& filePath, const std::vector<std::string>& p4includePaths, const std::vector<std::string>& defines, PreProcessorVerbosity verbosity){
    this->m_p4architecture = P4A_UNDEFINED;
    this->m_filePath = filePath;
    this->m_p4includePaths = p4includePaths;
    this->m_fileLines = PreProcessor::readFileLines(filePath);

    this->m_verboseLevel = verbosity;
    
    for(const std::string& define : defines){
        std::vector<std::string> splitS = IPLBase::split(define);
        splitS.insert(splitS.begin(), "define");
        splitS.insert(splitS.begin(), "#");
        this->ProcessDefine(-1, splitS);
    }
}

void PreProcessor::Process(){
    for(const auto& it : this->m_defineMacros)
        this->ReplaceDefine(0, it.second);

    typedef struct _if_data {
        int line;
        bool isTrue;
        bool wasTrue;
    } IfData;

    std::vector<IfData> ifstack;
    std::vector<std::string> currentFilePathStack = {this->m_filePath};

    std::vector<std::pair<int, int>> lineBreaks;

    this->ProcessStripComments();
    int i = 0;

    while(i < (int) this->m_fileLines.size()){
        //Delete blanks at the beginning
        while(this->m_fileLines[i].size() > 0 && isblank(this->m_fileLines[i][0]))
            this->m_fileLines[i].erase(this->m_fileLines[i].begin());

        //Replace linebreaks with the nextline
        int lbadd = 0;
        while(m_fileLines[i].size() > 0 && m_fileLines[i].back() == '\\'){
            m_fileLines[i].pop_back();
            if(i + lbadd < (int) m_fileLines.size()){
                lineBreaks.push_back(std::make_pair(i + lbadd, (int) m_fileLines[i + lbadd].size()));
                m_fileLines[i] = m_fileLines[i] + m_fileLines[i + 1 + lbadd];
                m_fileLines[i + 1 + lbadd].clear();
            }
            lbadd++;
        }

        std::vector<std::string> stringSplit = IPLBase::split(this->m_fileLines[i]);
        if(stringSplit.size() > 0){
            if(stringSplit[0][0] == '#' && stringSplit[0].size() > 1){
                stringSplit.emplace(stringSplit.begin() + 1, stringSplit[0].substr(1));
                stringSplit[0].erase(stringSplit[0].begin() + 1, stringSplit[0].end());
            }
            if(stringSplit[0] == "#" && stringSplit.size() > 1){
                if(ifstack.size() == 0 || ifstack[ifstack.size() - 1].isTrue){
                    if(stringSplit[1] == "include" && stringSplit.size() == 3)
                        this->ProcessInclude(i, stringSplit[2], &currentFilePathStack);
                    else if(stringSplit[1] == "define" && stringSplit.size() > 2){
                        this->ProcessDefine(i, stringSplit);
                    }
                }

                if(stringSplit.size() >= 3 && stringSplit[1] == "if"){
                    bool isTrue = PreProcessor::EvalIfStringSplit(stringSplit);
                    ifstack.push_back({i, isTrue && (ifstack.size() == 0 || ifstack[ifstack.size() - 1].isTrue), isTrue});
                
                    if(m_verboseLevel >= PPV_ALL)
                        printf("Push (%d) ifstack (%d - %s - %s, %s)\n", (int) ifstack.size(), i + 1, IPLBase::fold(stringSplit).c_str(), ifstack[ifstack.size() - 1].isTrue ? "true" : "false", ifstack[ifstack.size() - 1].wasTrue ? "true" : "false");
                }
                else if(stringSplit.size() == 3 && (stringSplit[1] == "ifdef" || stringSplit[1] == "ifndef")){
                    bool isDefined = (this->m_defineMacros.find(stringSplit[2]) != this->m_defineMacros.end());
                    ifstack.push_back(IfData{i, ((stringSplit[1] == "ifdef" && isDefined) || (stringSplit[1] == "ifndef" && !isDefined)) && (ifstack.size() == 0 || ifstack[ifstack.size() - 1].isTrue),  ((stringSplit[1] == "ifdef" && isDefined) || (stringSplit[1] == "ifndef" && !isDefined))}); 
                
                    if(m_verboseLevel >= PPV_ALL)
                        printf("Push (%d) ifstack (%d - %s - %s, %s)\n", (int) ifstack.size(), i + 1, IPLBase::fold(stringSplit).c_str(), ifstack[ifstack.size() - 1].isTrue ? "true" : "false", ifstack[ifstack.size() - 1].wasTrue ? "true" : "false");
                }
                else if(stringSplit[1] == "else" && stringSplit.size() == 2){
                    this->m_fileLines[i].clear();
                    ifstack[ifstack.size() - 1].isTrue = (!ifstack[ifstack.size() - 1].isTrue && !ifstack[ifstack.size() - 1].wasTrue) && (ifstack.size() < 2 || ifstack[ifstack.size() - 2].isTrue);
                
                    if(m_verboseLevel >= PPV_ALL)
                        printf("Else ifstack (%d - %s)\n", i, ifstack[ifstack.size() - 1].isTrue ? "true" : "false");
                }
                else if(stringSplit[1] == "endif" && stringSplit.size() == 2){
                    if(m_verboseLevel >= PPV_ALL)
                        printf("Pop (%d) ifstack (%s)\n", (int) ifstack.size() - 1, this->m_fileLines[ifstack[ifstack.size() - 1].line].c_str());

                    this->m_fileLines[ifstack[ifstack.size() - 1].line].clear();
                    this->m_fileLines[i].clear();
                    ifstack.pop_back();
                }
                else if(stringSplit[1] == "elif" && stringSplit.size() >= 3){
                    this->m_fileLines[i].clear();
                    if(!ifstack[ifstack.size() - 1].isTrue){
                        bool isTrue = PreProcessor::EvalIfStringSplit(stringSplit);
                        ifstack[ifstack.size() - 1].isTrue = isTrue;
                        ifstack[ifstack.size() - 1].wasTrue = ifstack[ifstack.size() - 1].wasTrue || isTrue;
                    }
                    else{
                        ifstack[ifstack.size() - 1].isTrue = false;
                    }

                    if(m_verboseLevel >= PPV_ALL)
                        printf("Elif ifstack (%d - %s - %s, %s)\n", i + 1, IPLBase::fold(stringSplit).c_str(), ifstack[ifstack.size() - 1].isTrue ? "true" : "false", ifstack[ifstack.size() - 1].wasTrue ? "true" : "false");
                }
                
                if(ifstack.size() != 0 && !ifstack[ifstack.size() - 1].isTrue){
                    this->m_fileLines[i].clear();
                }
            }
            else if(stringSplit[0] == "__POP_INCLUDE__")
                currentFilePathStack.pop_back();
            else if(ifstack.size() > 0 && !ifstack[ifstack.size() - 1].isTrue)
                this->m_fileLines[i].clear();
        }
        i++;
    }

    for(const auto& it : this->m_defineMacros)
        this->ReplaceDefine(0, it.second);
}

void PreProcessor::ProcessStripComments(){
    bool isComment = false;
    size_t lc, lc2;
    for(int i = 0; i < (int) this->m_fileLines.size(); i++){
        lc = m_fileLines[i].find("//");
        if(lc != std::string::npos)
            m_fileLines[i].erase(m_fileLines[i].begin() + lc, m_fileLines[i].end());
    }
    for(int i = 0; i < (int) this->m_fileLines.size(); i++){
        if(isComment){
            lc = m_fileLines[i].find("*/");
            if(lc == std::string::npos){
                if(m_fileLines[i].size() > 0 && m_fileLines[i].back() == '\\'){
                    for(size_t k = 0; k < m_fileLines[i].size() - 1; k++)
                        m_fileLines[i][k] = ' ';
                }
                else
                    m_fileLines[i].clear();
            }
            else{
                isComment = false;
                for(size_t x = 0; x < lc + 2; x++)
                    m_fileLines[i][x] = ' ';
                i--;
            }
        }
        else{
            lc = m_fileLines[i].find("/*");
            if(lc != std::string::npos){
                lc2 = m_fileLines[i].find("*/");
                if(lc2 != std::string::npos && lc2 > lc + 1){
                    for(size_t x = lc; x < lc2 + 2; x++)
                        m_fileLines[i][x] = ' ';
                    i--;
                }
                else{
                    isComment = true;
                    if(m_fileLines[i].size() > 0 && m_fileLines[i].back() == '\\'){
                        for(size_t k = lc; k < m_fileLines[i].size() - 1; k++)
                            m_fileLines[i][k] = ' ';
                    }
                    else
                        m_fileLines[i].erase(m_fileLines[i].begin() + lc, m_fileLines[i].end());
                }
            }
        }
    }
}

void PreProcessor::Print(FILE *out) const{
    for(const std::string& line : this->m_fileLines)
        fprintf(out, "%s\n", line.c_str());
}

std::string PreProcessor::PrintString() const{
    std::string result;
    for(const std::string& line : this->m_fileLines)
        result = result + line + "\n";
    return result;
}

void PreProcessor::SetVerbosity(PreProcessorVerbosity ppv){
    this->m_verboseLevel = ppv;
}

void PreProcessor::RemoveCommomErrorMKDeclarations(AST *program, const std::vector<std::string>& includes){
    static const std::unordered_map<std::string, std::unordered_set<std::string>> ErrorsDeclaredInHeader {
        {"core.p4", {"NoError", "PacketTooShort", "NoMatch", "StackOutOfBounds", "HeaderTooShort", "ParserTimeout", "ParserInvalidArgument"}},
        {"v1model.p4", {}},
        {"tna.p4", {"CounterRange", "Timeout", "PhvOwner", "MultiWrite", "IbufOverflow", "IbufUnderflow"}}
    };

    static const std::unordered_map<std::string, std::unordered_set<std::string>> MatchKindsDeclaredInHeader {
        {"core.p4", {"exact", "ternary", "lpm"}},
        {"v1model.p4", {"range", "optional", "selector"}},
        {"tna.p4", {"range", "selector", "atcam_partition_index"}}
    };

    size_t c = 0;
    while(c < program->children.size()){
        if(program->children[c] != NULL && program->children[c]->nType == NT_ERROR_DECLARATION && program->children[c]->children[0] != NULL){
            AST *idList = program->children[c]->children[0];
            size_t i = 0;
            while(i < idList->children.size()){
                bool found = false;
                for(const std::string& header : includes){
                    auto it = ErrorsDeclaredInHeader.find(header);
                    if(it != ErrorsDeclaredInHeader.end()){
                        if(it->second.find(idList->children[i]->value) != it->second.end()){
                            found = true;
                        }
                    }
                }
                if(found){
                    delete idList->children[i];
                    idList->children.erase(idList->children.begin() + i);
                    i--;
                }
                i++;
            }

            if(idList->children.size() == 0){
                delete program->children[c];
                program->children.erase(program->children.begin() + c);
                c--;
            }
        }
        else if(program->children[c] != NULL && program->children[c]->nType == NT_MATCH_KIND_DECLARATION && program->children[c]->children[0] != NULL){
            AST *idList = program->children[c]->children[0];
            size_t i = 0;
            while(i < idList->children.size()){
                bool found = false;
                for(const std::string& header : includes){
                    auto it = MatchKindsDeclaredInHeader.find(header);
                    if(it != MatchKindsDeclaredInHeader.end()){
                        if(it->second.find(idList->children[i]->value) != it->second.end()){
                            found = true;
                        }
                    }
                }
                if(found){
                    delete idList->children[i];
                    idList->children.erase(idList->children.begin() + i);
                    i--;
                }
                i++;
            }

            if(idList->children.size() == 0){
                delete program->children[c];
                program->children.erase(program->children.begin() + c);
                c--;
            }
        }
        c++;
    }
}

std::string PreProcessor::Stringfy(const std::string &str){
    std::string result = "\"" + str + "\"";
    size_t i = 1;
    bool inStr = false;
    while(i < result.size()){
        if(result[i] == '\"')
            inStr = !inStr;
        if(result[i] == '\\' && inStr){
            result.insert(result.begin() + i, '\\');
            i++;
        }
        i++;
    }
    return result;
}

size_t PreProcessor::FindDefineArgInStr(const std::string& arg, const std::string& str, size_t start){
    size_t pos = str.find(arg, start);
    bool posOk = false;
    while(pos != std::string::npos && !posOk){
        posOk = true;
        size_t posEnd = pos + arg.size();
        if(pos > 0 && (isalnum(str[pos - 1]) || str[pos - 1] == '_'))
            posOk = false;
        if(posEnd < str.size() && (isalnum(str[posEnd]) || str[posEnd] == '_'))
            posOk = false;
        
        if(!posOk)
            pos = str.find(arg, posEnd);
    }
    return pos;
}

void PreProcessor::ProcessInclude(const int line, const std::string& includeFile, std::vector<std::string>* currentFilePathStack){
    std::string fileP;
    if(includeFile.size() < 2){
        fprintf(stderr, "Error: Malformed include on \"%s\" line %d:\n%s\n", currentFilePathStack->at(currentFilePathStack->size() - 1).c_str(), line + 1, m_fileLines[line].c_str());
        exit(1);
    }
    if(includeFile[0] == '"' && includeFile[includeFile.size() - 1] == '"'){
        std::vector<std::string> filePathDirChain = IPLBase::split(currentFilePathStack->at(currentFilePathStack->size() - 1), '/');
        filePathDirChain.pop_back();
        std::vector<std::string> includeFilePathDirChain = IPLBase::split(includeFile.substr(1, includeFile.size() - 2), '/');
        for(const std::string& s : includeFilePathDirChain){
            if(s == ".." && filePathDirChain.size() > 0)
                filePathDirChain.pop_back();
            else
                filePathDirChain.push_back(s);
        }
        fileP = filePathDirChain[0];
        for(int i = 1; i < (int) filePathDirChain.size(); i++)
            fileP = fileP + "/" + filePathDirChain[i];
    }
    else if(includeFile[0] == '<' && includeFile[includeFile.size() - 1] == '>'){
        FILE *fp = NULL;
        int dirIDX = 0;
        while(dirIDX < (int) this->m_p4includePaths.size() && fp == NULL){
            fileP = this->m_p4includePaths[dirIDX] + includeFile.substr(1, includeFile.size() - 2);
            fp = fopen(fileP.c_str(), "r");
            dirIDX++;
        }
        if(!fp){
            fprintf(stderr, "Error: Could not find include file: \"%s\" (Line %d - \"%s\")\n", includeFile.substr(1, includeFile.size() - 2).c_str(), line + 1, m_filePath.c_str());
            exit(1);
        }
        fclose(fp);

        bool found = false;
        for(const std::string& s : this->m_commonIncludeList){
            if(s == includeFile.substr(1, includeFile.size() - 2)){
                found = true;
                break;
            }
        }
        if(!found){
            for(size_t i = currentFilePathStack->size() - 1; i > 0; i--){
                for(const std::string& s : this->m_commonIncludeList){
                    if(currentFilePathStack->at(i).find(s) != std::string::npos){
                        found = true;
                    }
                }
            }

            if(!found)
                this->m_commonIncludeList.push_back(includeFile.substr(1, includeFile.size() - 2));
        }
    }
    else{
        fprintf(stderr, "Error: Malformed include on \"%s\" line %d:\n%s\n", currentFilePathStack->at(currentFilePathStack->size() - 1).c_str(), line + 1, m_fileLines[line].c_str());
        exit(1);
    }

    std::vector<std::string> includeFileLines = readFileLines(fileP);
    this->m_fileLines[line] = "__PUSH_INCLUDE__ \"" + fileP + "\"";
    this->m_fileLines.emplace(this->m_fileLines.begin() + line + 1, "__POP_INCLUDE__ \"" + fileP + "\"");

    currentFilePathStack->push_back(fileP);

    this->m_fileLines.resize(this->m_fileLines.size() + includeFileLines.size());
    for(int i = (int) this->m_fileLines.size() - 1; i >= (int) (line + includeFileLines.size()); i--)
        this->m_fileLines[i] = this->m_fileLines[i - includeFileLines.size()];
    for(int i = 0; i < (int) includeFileLines.size(); i++)
        this->m_fileLines[line + i + 1] = includeFileLines[i];

    this->ProcessStripComments();

    for(auto it : this->m_defineMacros)
        this->ReplaceDefine(line + 1, (int) includeFileLines.size(), it.second);

    //Updates Infered Architecture
    if(fileP.find("v1model.p4") != std::string::npos)
        this->m_p4architecture = (this->m_p4architecture == P4A_UNDEFINED || this->m_p4architecture == P4A_V1MODEL) ? P4A_V1MODEL : P4A_UNDEFINED;
    else if(fileP.find("tna.p4") != std::string::npos)
        this->m_p4architecture = (this->m_p4architecture == P4A_UNDEFINED || this->m_p4architecture == P4A_TNA) ? P4A_TNA : P4A_UNDEFINED;
    else if(fileP.find("doca_core.p4") != std::string::npos)
        this->m_p4architecture = (this->m_p4architecture == P4A_UNDEFINED || this->m_p4architecture == P4A_NVDA_DOCA) ? P4A_NVDA_DOCA : P4A_UNDEFINED;
}

void PreProcessor::ProcessDefine(const int line, const std::vector<std::string>& stringSplit){
    const std::string lineS = (line == -1) ? IPLBase::fold(stringSplit) : m_fileLines[line];

    const size_t lineStart = lineS.find("define");
    if(lineStart == std::string::npos){
        fprintf(stderr, "Error: Malformed define on \"%s\" line %d:\n%s\n", m_filePath.c_str(), line + 1, lineS.c_str());
        exit(1);
    }

    std::string lineStrip = lineS.substr(lineStart + 7);
    while(isblank(lineStrip[0]))
        lineStrip.erase(lineStrip.begin());

    size_t parenOpen = lineStrip.find("(");
    size_t parenClose = lineStrip.find(")");
    size_t firstSpace = lineStrip.find(" ");

    if(firstSpace != std::string::npos && firstSpace < parenOpen){
        parenOpen = std::string::npos;
        parenClose = std::string::npos;
    }

    if(parenClose < parenOpen){
        fprintf(stderr, "Error: Malformed define on \"%s\" line %d:\n%s\n", m_filePath.c_str(), line + 1, lineS.c_str());
        exit(1);
    }

    PPDefineMacro newM;
    newM.startline = line;
    newM.endline = -1;
    newM.name = lineStrip.substr(0, parenOpen == std::string::npos ? (lineStrip.find(" ") == std::string::npos ? lineStrip.size() : lineStrip.find(" ")) : parenOpen);
    if(parenClose != std::string::npos){
        newM.body = (parenClose + 1 < lineStrip.size()) ? lineStrip.substr(parenClose + 1) : "";
    }
    else{
        newM.body = lineStrip.substr(lineStrip.find(" ") + 1);
    }
    while(newM.body.size() > 0 && isblank(newM.body.back()))
        newM.body.pop_back();
    while(newM.body.size() > 0 && isblank(newM.body.front()))
        newM.body.erase(newM.body.begin());

    if(parenOpen != std::string::npos){
        newM.argNames = IPLBase::split(lineStrip.substr(parenOpen + 1, parenClose - parenOpen - 1), ',');
        for(size_t i = 0; i < newM.argNames.size(); i++){
            while(newM.argNames[i].size() > 0 && newM.argNames[i][newM.argNames[i].size() - 1] == ' ')
                newM.argNames[i].pop_back();
            while(newM.argNames[i].size() > 0 && newM.argNames[i][0] == ' ')
                newM.argNames[i].erase(newM.argNames[i].begin());
            if(newM.argNames[i].size() == 0){
                fprintf(stderr, "Error: Malformed define on \"%s\" line %d:\n%s\n", m_filePath.c_str(), line + 1, lineS.c_str());
                exit(1);
            }
        }
    }

    this->m_defineMacros.emplace(newM.name, newM);
    
    if(m_verboseLevel >= PPV_SIMPLE){
        printf("%s: %s - %s\n", IPLBase::fold(stringSplit).c_str(), newM.name.c_str(), newM.body.c_str());

        if(newM.argNames.size() > 0){
            printf("\n");
            for(int i = 0; i < (int) newM.argNames.size(); i++){
                printf("Arg: %s - ", newM.argNames[i].c_str());
                size_t oldPos = 0;
                size_t pos = PreProcessor::FindDefineArgInStr(newM.argNames[i], newM.body, oldPos);
                while(pos != std::string::npos){
                    while(oldPos < pos){
                        printf("%c", newM.body[oldPos]);
                        oldPos++;
                    }
                    printf("%s%s%s", C_RED, newM.argNames[i].c_str(), C_NONE);
                    oldPos = pos + newM.argNames[i].size();
                    pos = PreProcessor::FindDefineArgInStr(newM.argNames[i], newM.body, oldPos);
                }
                while(oldPos < newM.body.size()){
                    printf("%c", newM.body[oldPos]);
                    oldPos++;
                }
                printf("\n");
            }
            printf("\n");
        }
    }

    if(line >= 0){
        this->m_fileLines[line].clear();
        this->ReplaceDefine(line + 1, newM);
    }
}

void PreProcessor::ReplaceDefine(const int startLine, const int nLines, const PPDefineMacro& ppdm){
    //for(int line = startLine; line < (int) this->m_fileLines.size(); line++){
    for(int line = (startLine < (int) ppdm.startline) ? (int) ppdm.startline : startLine; line < startLine + nLines; line++){
        //int position = this->m_fileLines[line].find(ppdm.name);
        size_t position = FindDefineArgInStr(ppdm.name, this->m_fileLines[line], 0);
        size_t cur = position + ppdm.name.size() + 1;
        
        //Check if line is not another define
        bool isDefine = false;
        for(const std::string& s : IPLBase::split(this->m_fileLines[line]))
            if(s == "#define" || s == "define" || s == "#ifdef" || s == "ifdef" || s == "#ifndef" || s == "ifndef")
                isDefine = true;
        
        if(position != std::string::npos && !isDefine){
            std::string argsList;
            if(position + ppdm.name.size() < this->m_fileLines[line].size() && this->m_fileLines[line][position + ppdm.name.size()] == '('){
                int ps = 1;
                while(ps > 0 && cur < this->m_fileLines[line].size()){
                    if(!isblank(this->m_fileLines[line][cur]))
                        argsList.push_back(this->m_fileLines[line][cur]);
                    if(this->m_fileLines[line][cur] == '(')
                        ps++;
                    if(this->m_fileLines[line][cur] == ')')
                        ps--;
                    cur++;
                }
                cur++;
                if(ps != 0)
                    argsList.clear();
            }
            std::vector<std::string> argListSplit;
            if(argsList.size() > 0){
                argsList.pop_back();
                argListSplit = IPLBase::split(argsList, ',');
            }
            std::string tempBody = ppdm.body;
            if(argListSplit.size() == ppdm.argNames.size()){
                int additionalPos = 0;
                size_t lastPos = 0;

                bool done = false;
                while(!done){
                    size_t firstPos = std::string::npos;
                    size_t firstArg = std::string::npos;
                    for(size_t i = 0; i < ppdm.argNames.size(); i++){
                        size_t argPos = PreProcessor::FindDefineArgInStr(ppdm.argNames[i], ppdm.body, lastPos);
                        if(argPos != std::string::npos && (argPos < firstPos || firstPos == std::string::npos)){
                            firstPos = argPos;
                            firstArg = i;
                        }
                    }

                    if(firstArg != std::string::npos){
                        std::string arg = (firstPos > 0 && tempBody[firstPos + additionalPos - 1] == '#' && (firstPos == 1 || tempBody[firstPos + additionalPos - 2] != '#') ) ? PreProcessor::Stringfy(argListSplit[firstArg]) : argListSplit[firstArg];
                        tempBody = tempBody.substr(0, firstPos + additionalPos) + arg + tempBody.substr(firstPos + additionalPos + ppdm.argNames[firstArg].size());
                        additionalPos = additionalPos + arg.size() - ppdm.argNames[firstArg].size();
                        lastPos = firstPos + argListSplit[firstArg].size();
                    }
                    else{
                        done = true;
                    }
                }

                size_t argPos = tempBody.find("#");
                while(argPos != std::string::npos){
                    tempBody.erase(tempBody.begin() + argPos);
                    argPos = tempBody.find("#");
                }

            }
            this->m_fileLines[line] = this->m_fileLines[line].substr(0, position) + tempBody + this->m_fileLines[line].substr(cur-1);

            //Check if it is part of "defined" or "!defined"
            int startPB = position - 1;
            while(startPB >= 0 && this->m_fileLines[line][startPB] != ' ')
                startPB--;
            startPB++;
            std::string before = this->m_fileLines[line].substr(startPB, position - startPB);
            int startPA = position + tempBody.size();
            while(startPA < (int) this->m_fileLines[line].size() && this->m_fileLines[line][startPA] != ' ')
                startPA++;
            std::string after = this->m_fileLines[line].substr(position + tempBody.size(), startPA - (position + tempBody.size()));
            
            if(after == ")" && (before == "defined(" || before == "!defined(")){
                this->m_fileLines[line] = this->m_fileLines[line].substr(0, startPB) + ((before == "defined(") ? "true" : "false") + this->m_fileLines[line].substr(startPA);
            }
        }
    }
}

void PreProcessor::ReplaceDefine(const int startLine, const PPDefineMacro& ppdm){
    this->ReplaceDefine(startLine, (int) this->m_fileLines.size() - startLine, ppdm);
}

bool PreProcessor::EvalIfStringSplit(const std::vector<std::string>& stringSplit){
    bool result = false;

    if(stringSplit.size() == 1){
        if(stringSplit[0] == "false")
            result = false;
        else if(stringSplit[0] == "true")
            result = true;
        else if(stringSplit[0].find("defined(") == 0 || stringSplit[0].find("!defined(") == 0){
            const std::string macro = stringSplit[0].substr(stringSplit[0].find("(") + 1, stringSplit[0].find(")") - (stringSplit[0].find("(") + 1));
            const bool isDefined = this->m_defineMacros.find(macro) != this->m_defineMacros.end();
            result = (isDefined && stringSplit[0][0] != '!') || (!isDefined && stringSplit[0][0] == '!');
        }
    }
    else if(stringSplit.size() == 5){
        if(IPLBase::isInt(stringSplit[2].c_str()) && IPLBase::isInt(stringSplit[4].c_str())){
            int a = IPLBase::intFromStr(stringSplit[2].c_str());
            int b = IPLBase::intFromStr(stringSplit[4].c_str());
            if(stringSplit[3] == "==")
                result = a == b;
            else if(stringSplit[3] == "!=")
                result = a != b;
            else if(stringSplit[3] == ">=")
                result = a >= b;
            else if(stringSplit[3] == "<=")
                result = a <= b;
            else if(stringSplit[3] == ">")
                result = a > b;
            else if(stringSplit[3] == "<")
                result = a < b;
        }
        else{
            if(stringSplit[3] == "&&"){
                result = EvalIfStringSplit(std::vector<std::string> {stringSplit[2]}) && EvalIfStringSplit(std::vector<std::string> {stringSplit[4]});
            }
            else if(stringSplit[3] == "||"){
                result = EvalIfStringSplit(std::vector<std::string> {stringSplit[2]}) || EvalIfStringSplit(std::vector<std::string> {stringSplit[4]});
            }
        }
    }

    if(m_verboseLevel >= PPV_ALL)
        printf("Eval: %s -> %s\n", IPLBase::fold(stringSplit).c_str(), result ? "true" : "false");
    return result;
}

void PreProcessor::PrintDefineMacro(const PPDefineMacro& ppdm){
    printf("Name: %s\nArgs: (", ppdm.name.c_str());
    for(int i = 0; i < (int) ppdm.argNames.size(); i++){
        printf("%s", ppdm.argNames[i].c_str());
        if(i < (int) ppdm.argNames.size() - 1)
            printf(", ");
    }
    printf(")\nBody: %s\n\n", ppdm.body.c_str());
}

std::vector<std::string> PreProcessor::readFileLines(const std::string& filePath){
    std::vector<std::string> result;
    std::string aux;
    FILE *fp = fopen(filePath.c_str(), "r");

    if(!fp){
        fprintf(stderr, "Could not open file: %s\n", filePath.c_str());
        exit(1);
    }

    char c = fgetc(fp);
    while(!feof(fp)){
        while(c != '\n' && !feof(fp)){
            aux.push_back(c);
            c = fgetc(fp);
        }
        result.push_back(aux);
        aux.clear();
        c = fgetc(fp);
    }

    fclose(fp);
    return result;
}