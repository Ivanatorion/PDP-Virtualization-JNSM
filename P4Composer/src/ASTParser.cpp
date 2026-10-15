#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "../include/Log.h"
#include "../include/AST.h"
#include "../include/ASTParser.h"

#define STATE_DEPTH_LIMIT 10000

void ASTParser::CheckParsersStates(AST *parser, const std::string& programName){
    const std::string parserName = parser->getChildNT(NT_PARSER_TYPE_DECLARATION)->value;
    
    Log::MSG("Parser \"" + parserName + "\" (" + programName + ")", LOG_LEVEL_SHORT);
    Log::Push();

    AST *StateList = parser->getChildNT(NT_PARSER_STATE_LIST);
    if(StateList == NULL || StateList->FindStateInList("start") == NULL){
        fprintf(stderr, "Error: Parser \"%s\" in program \"%s\" does not have a start state\n", parserName.c_str(), programName.c_str());
        exit(1);
    }

    ASTParser::OrderParserStates(StateList, parser->value, programName);

    Log::Pop();
}

void ASTParser::OrderParserStates(AST *StateList, const std::string& parserName, const std::string& programName){
    std::unordered_map<AST*, int> distances;
    std::unordered_map<AST*, int> nInEdges;
    std::vector<AST*> queue;
    std::unordered_set<AST*> closed;
    std::vector<AST*> ordered;

    for(AST *state : StateList->children){
        distances.emplace(state, state->value == "start" ? 0 : STATE_DEPTH_LIMIT);
        nInEdges.emplace(state, 0);
    }

    queue.push_back(StateList->FindStateInList("start"));
    while(queue.size() > 0){
        AST *s = queue[0];
        queue.erase(queue.begin());
        if(closed.find(s) == closed.end()){
            closed.insert(s);
            ordered.push_back(s);
            const int sDistance = distances.find(s)->second;

            if(s->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
                if(s->getChildNT(NT_NAME_STATE_EXPRESSION)->value != "accept" && s->getChildNT(NT_NAME_STATE_EXPRESSION)->value != "reject"){
                    AST *nextS = StateList->FindStateInList(s->getChildNT(NT_NAME_STATE_EXPRESSION)->value);
                    Assert(nextS != NULL);

                    nInEdges.find(nextS)->second = nInEdges.find(nextS)->second + 1;

                    if(closed.find(nextS) == closed.end()){
                        queue.push_back(nextS);
                        if(distances.find(nextS)->second > sDistance + 1)
                            distances.find(nextS)->second = sDistance + 1;
                    }
                }
            }
            else{
                for(AST *sc : s->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children){
                    if(sc->value != "accept" && sc->value != "reject"){
                        AST *nextS = StateList->FindStateInList(sc->value);
                        if(nextS == NULL){
                            fprintf(stderr, "Error: State \"%s\" not found in parser \"%s\" (%s - Line %d)\n", sc->str(), parserName.c_str(), programName.c_str(), sc->lineNumber);
                            exit(1);
                        }
                        nInEdges.find(nextS)->second = nInEdges.find(nextS)->second + 1;

                        if(closed.find(nextS) == closed.end()){
                            queue.push_back(nextS);
                            if(distances.find(nextS)->second > sDistance + 1)
                                distances.find(nextS)->second = sDistance + 1;
                        }
                    }
                } 
            }
        }
    }

    for(auto d : distances){
        if(d.second == STATE_DEPTH_LIMIT){
            Log::MSG("Removed unreachable state \"" + d.first->value + "\"", LOG_LEVEL_SHORT);
            delete d.first;
        }
    }

    StateList->children.clear();
    for(AST* s : ordered)
        StateList->addChild(s);

    int itr = 0;
    while(itr < (int) StateList->children.size()){
        AST *state = StateList->children[itr];
        AST *simpleTransitionStatement = state->getChildNT(NT_NAME_STATE_EXPRESSION);
        if(simpleTransitionStatement != NULL && (simpleTransitionStatement->value != "accept" && simpleTransitionStatement->value != "reject")){
            AST *nextS = StateList->FindStateInList(simpleTransitionStatement->value);
            Assert(nextS != NULL);
            if(nInEdges.find(nextS)->second == 1){
                if(state->children[1] == NULL)
                    state->children[1] = NULL_CLONE(nextS->children[1]);
                else if(nextS->children[1] != NULL){
                    for(AST *statement : nextS->children[1]->children)
                        state->children[1]->addChild(NULL_CLONE(statement));
                }

                delete simpleTransitionStatement;
                state->children[2] = NULL_CLONE(nextS->children[2]);

                int itr2 = itr + 1;
                while(itr2 < (int) StateList->children.size() && StateList->children[itr2] != nextS)
                    itr2++;
                Assert(itr2 < (int) StateList->children.size());
                
                StateList->children.erase(StateList->children.begin() + itr2);
                delete nextS;
                itr--;
            }
        }
        itr++;
    }
}

void ASTParser::WriteParserDOT(AST *parser, const std::string& fileName){
    char buffer[256];

    if(parser->nType != NT_PARSER_DECLARATION)
        return;

    FILE *fp = fopen(fileName.c_str(), "w");
    Assert(fp != NULL);
    if(!fp)
        return;

    std::unordered_map<std::string, std::set<std::string>> transitions; 

    for(AST* state : parser->getChildNT(NT_PARSER_STATE_LIST)->children){
        if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
            transitions.emplace(state->value, std::set<std::string>({state->getChildNT(NT_NAME_STATE_EXPRESSION)->value}));
        }
        else{
            transitions.emplace(state->value, std::set<std::string>());
            AST* SCL = state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST);
            for(AST *nextState : SCL->children)
                transitions.find(state->value)->second.emplace(nextState->value);
        }
    }

    fprintf(fp, "digraph AST {\n");
    for(AST *state : parser->getChildNT(NT_PARSER_STATE_LIST)->children){
        sprintf(buffer, "%s", state->value.c_str());
        fprintf(fp, "\"%s\" [ shape=record, style=\"filled\", fillcolor=cornsilk, label=\"%s\" ];\n", buffer, buffer);
    }
    fprintf(fp, "accept [ shape=record, style=\"filled\", fillcolor=cornsilk, label=\"accept\" ];\n");
    fprintf(fp, "reject [ shape=record, style=\"filled\", fillcolor=cornsilk, label=\"reject\" ];\n");
    for(AST *state : parser->getChildNT(NT_PARSER_STATE_LIST)->children){
        sprintf(buffer, "%s", state->value.c_str());
        for(const std::string& transition : transitions.find(state->value)->second){
            char buffer2[256];
            sprintf(buffer2, "%s", transition.c_str());
            fprintf(fp, "\"%s\" -> \"%s\"\n", buffer, buffer2);
        }
    }
    fprintf(fp, "}\n");
    fclose(fp);
}

bool ASTParser::BothOrNoneStart(AST* state, AST* state2){
    const std::string n1 = state->getValue().substr(state->getValue().find("_") + 1);
    const std::string n2 = state2->getValue().substr(state2->getValue().find("_") + 1);
    return ((n1 != "start" && n2 != "start") || (n1 == n2));
}

void ASTParser::MergeParserEqualStates(AST *stateList, std::vector<std::vector<int>>& statePrograms){
    char buffer[BUFFER_SIZE];
    
    std::vector<std::set<size_t>> equalStates;
    for(size_t i = 0; i < stateList->children.size() - 1; i++){
        AST *state = stateList->children[i];
        for(size_t j = i + 1; j < stateList->children.size(); j++){
            AST *state2 =  stateList->children[j];
            if(AST::Equals(state, state2) && ASTParser::BothOrNoneStart(state, state2)){
                bool found = false;
                for(std::set<size_t>& eqs : equalStates){
                    if(eqs.find(i) != eqs.end()){
                        found = true;
                        if(eqs.find(j) == eqs.end())
                            eqs.emplace(j);
                    }
                }
                if(!found){
                    equalStates.push_back({i, j});
                }
            }
        }
    }
    
    std::set<AST*> deleteStates;
    for(std::set<size_t> eqSet : equalStates){
        std::set<std::string> names;
        std::string newStateName = "P";
        std::vector<int> newStateProgs;
        for(size_t x: eqSet){
            deleteStates.emplace(stateList->children[x]);
            names.emplace(stateList->children[x]->value);
            for(int program : statePrograms[x]){
                sprintf(buffer, "s%d", program + 1);
                newStateProgs.push_back(program);
                newStateName = newStateName + std::string(buffer);
            }
        }

        int ulp = 2;
        while((*names.begin())[ulp] != '_')
            ulp++;
        newStateName = newStateName + (*names.begin()).substr(ulp);

        for(size_t i = 0; i < stateList->children.size(); i++){
            AST* state = stateList->children[i];
            if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
                if(names.find(state->getChildNT(NT_NAME_STATE_EXPRESSION)->value) != names.end()){
                    state->getChildNT(NT_NAME_STATE_EXPRESSION)->value = newStateName;
                }
            }
            else{
                for(AST *selectCase : state->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children){
                    if(names.find(selectCase->value) != names.end()){
                        selectCase->value = newStateName;
                    }
                }
            }
        }

        AST* newState = stateList->children[*eqSet.begin()]->Clone();
        newState->value = newStateName;
        stateList->addChild(newState);
        statePrograms.push_back(newStateProgs);

        std::string logM = "Merged states [ ";
        for(const std::string& name : names)
            logM = logM + name + " ";
        logM = logM + "] into state \"" + newStateName + "\"";
        Log::MSG(logM, LOG_LEVEL_SHORT);
    }

    int sc = 0;
    while(sc < (int) stateList->children.size()){
        AST* state = stateList->children[sc];
        if(deleteStates.find(state) != deleteStates.end()){
            delete state;
            stateList->children.erase(stateList->children.begin() + sc);
            statePrograms.erase(statePrograms.begin() + sc);
            sc--;
        }
        sc++;
    }

    if(equalStates.size() > 0)
        MergeParserEqualStates(stateList, statePrograms);
}

bool ASTParser::MergeParserSemiEqualStates(AST *stateList, std::vector<std::vector<int>>& statePrograms, const std::vector<std::vector<std::pair<int, int>>>& continuousPorts, AST* IG_PORT_EXP, std::vector<AST*> valueSetDeclarations){
    char buffer[BUFFER_SIZE];
    
    std::vector<std::set<int>> semiEqualStates;
    for(int i = 0; i < (int) stateList->children.size() - 1; i++){
        AST *state = stateList->children[i];
        for(int j = i + 1; j < (int) stateList->children.size(); j++){
            AST *state2 =  stateList->children[j];
            if(AST::Equals(state->getChildNT(NT_PARSER_STATEMENT_LIST), state2->getChildNT(NT_PARSER_STATEMENT_LIST)) && ASTParser::BothOrNoneStart(state, state2)){
                bool canMerge = false;
                if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL || state2->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL)
                    canMerge = true;
                //Expression lists must be the same
                else if(AST::Equals(state->getChildNT(NT_SELECT_EXPRESSION)->children[0], state2->getChildNT(NT_SELECT_EXPRESSION)->children[0])){
                    //Must not use value sets as keys
                    canMerge = true;
                    for(AST* selectCase : state->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST)->children){
                        Assert(selectCase->children.size() == 1);
                        for(AST *VSD : valueSetDeclarations){
                            if(selectCase->children[0]->value == VSD->value){
                                canMerge = false;
                            }
                        }
                    }
                }

                if(canMerge){
                    bool found = false;
                    for(std::set<int>& s : semiEqualStates){
                        if(s.find(i) != s.end()){
                            if(s.find(j) == s.end())
                                s.emplace(j);
                            found = true;
                        }
                    }
                    if(!found){
                        semiEqualStates.push_back({i, j});
                    }
                }
            }
        }
    }

    for(const std::set<int>& ses : semiEqualStates){
        std::vector<AST*> states;
        std::set<std::string> names;
        AST* newState;

        for(int i : ses){
            states.push_back(stateList->children[i]);
            names.emplace(stateList->children[i]->value);
        }
        
        int firstSE = 0;
        while(firstSE < (int) states.size() && states[firstSE]->getChildNT(NT_SELECT_EXPRESSION) == NULL)
            firstSE++;
        
        if(firstSE == (int) states.size()){ //Only transitions without select
            newState = states[0]->Clone();
            delete newState->children[2];
            AST *newTransition = new AST("select", NT_SELECT_EXPRESSION);
            newState->children[2] = newTransition;
            AST *newExpList = new AST("", NT_EXPRESSION_LIST);
            if(IG_PORT_EXP != NULL)
                newExpList->addChild(IG_PORT_EXP->Clone());
            newTransition->addChild(newExpList);
            AST *newSelectCaseList = new AST("", NT_SELECT_CASE_LIST);
            newTransition->addChild(newSelectCaseList);

            for(int stateN : ses){
                for(int program : statePrograms[stateN]){
                    for(const std::pair<int, int>& pr : continuousPorts[program]){
                        AST *newSCCase = new AST(stateList->children[stateN]->getChildNT(NT_NAME_STATE_EXPRESSION)->value, NT_SELECT_CASE);
                        if(pr.first == pr.second){
                            sprintf(buffer, "%d", pr.first);
                            newSCCase->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                        }
                        else{
                            AST *rangeAST = new AST("..", NT_RANGE);
                            sprintf(buffer, "%d", pr.first);
                            rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                            sprintf(buffer, "%d", pr.second);
                            rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                            newSCCase->addChild(rangeAST);
                        }
                        newSelectCaseList->addChild(newSCCase);
                    }
                }
            }
        }
        else{
            newState = states[firstSE]->Clone();
            AST* newExpList = newState->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_EXPRESSION_LIST);
            if(IG_PORT_EXP != NULL)
                newExpList->addChildStart(IG_PORT_EXP->Clone());

            std::vector<AST*> selectCaseLists(states.size());

            AST *newSelectCaseList = new AST("", NT_SELECT_CASE_LIST);
            for(int stateN : ses){
                if(stateList->children[stateN]->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
                    for(int program : statePrograms[stateN]){
                        for(const std::pair<int, int>& portRange : continuousPorts[program]){
                            AST *newSC = new AST(stateList->children[stateN]->getChildNT(NT_NAME_STATE_EXPRESSION)->value, NT_SELECT_CASE);
                            newSC->addChild(new AST("", NT_TUPLE_KEYSET_EXPRESSION));
                            newSC->children[0]->addChild(new AST("", NT_SIMPLE_EXPRESSION_LIST));
                            if(portRange.first == portRange.second){
                                sprintf(buffer, "%d", portRange.first);
                                newSC->children[0]->children[0]->addChildStart(new AST(std::string(buffer), NT_EXPRESSION));
                            }
                            else{
                                AST *rangeAST = new AST("..", NT_RANGE);
                                sprintf(buffer, "%d", portRange.first);
                                rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                                sprintf(buffer, "%d", portRange.second);
                                rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                                newSC->children[0]->children[0]->addChildStart(rangeAST);
                            }
                            for(int i = 1; i < (int) newState->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_EXPRESSION_LIST)->children.size(); i++){
                                newSC->children[0]->children[0]->addChild(new AST("_", NT_SIMPLE_KEYSET_DONTCARE));
                            }
                            newSelectCaseList->addChild(newSC);
                        }
                    }
                }
                else{
                    for(int program : statePrograms[stateN]){
                        for(AST *sCase : stateList->children[stateN]->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_SELECT_CASE_LIST)->children){
                            AST *newSC = sCase->Clone();
                            AST *aux = newSC->children[0];

                            newSC->children[0] = new AST("", NT_TUPLE_KEYSET_EXPRESSION);
                            newSC->children[0]->addChild(new AST("", NT_SIMPLE_EXPRESSION_LIST));
                            
                            
                            
                            if(aux->nType == NT_TUPLE_KEYSET_EXPRESSION){
                                Assert(aux->children.size() == 1 && aux->children[0]->nType == NT_SIMPLE_EXPRESSION_LIST);
                                for(AST *expression : aux->children[0]->children)
                                    newSC->children[0]->children[0]->addChild(expression);
                                aux->children[0]->children.clear();
                                delete aux->children[0];
                            }
                            else if(aux->nType == NT_SIMPLE_KEYSET_DEFAULT){
                                if(stateList->children[stateN]->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_EXPRESSION_LIST)->children.size() > 1){
                                    delete aux;
                                    for(size_t i = 0; i < stateList->children[stateN]->getChildNT(NT_SELECT_EXPRESSION)->getChildNT(NT_EXPRESSION_LIST)->children.size(); i++){
                                        newSC->children[0]->children[0]->addChild(new AST("_", NT_SIMPLE_KEYSET_DONTCARE));
                                    }
                                }
                                else{
                                    aux->nType = NT_SIMPLE_KEYSET_DONTCARE;
                                    aux->value = "_";
                                    newSC->children[0]->children[0]->addChild(aux);
                                }
                            }
                            else 
                                newSC->children[0]->children[0]->addChild(aux);

                            for(const std::pair<int, int>& portRange : continuousPorts[program]){
                                AST *newSCCase = newSC->Clone();
                                if(portRange.first == portRange.second){
                                    sprintf(buffer, "%d", portRange.first);
                                    newSCCase->children[0]->children[0]->addChildStart(new AST(std::string(buffer), NT_EXPRESSION));
                                }
                                else{
                                    AST *rangeAST = new AST("..", NT_RANGE);
                                    sprintf(buffer, "%d", portRange.first);
                                    rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                                    sprintf(buffer, "%d", portRange.second);
                                    rangeAST->addChild(new AST(std::string(buffer), NT_EXPRESSION));
                                    newSCCase->children[0]->children[0]->addChildStart(rangeAST);
                                }
                                newSelectCaseList->addChild(newSCCase);
                            }
                            delete newSC;
                        }
                    }
                }
            }

            delete newState->children[2]->children[1];
            newState->children[2]->children[1] = newSelectCaseList;
        }

        stateList->addChild(newState);
        statePrograms.push_back({});

        std::string newStateName = "P";
        for(int stateN : ses){
            for(int program : statePrograms[stateN]){
                sprintf(buffer, "s%d", program + 1);
                newStateName = newStateName + std::string(buffer);
                statePrograms[statePrograms.size() - 1].push_back(program);
            }
        }
        newStateName = newStateName + newState->value.substr(newState->value.find("_"));

        for(int i = 0; i < (int) stateList->children.size(); i++){
            AST* state = stateList->children[i];
            if(state->getChildNT(NT_NAME_STATE_EXPRESSION) != NULL){
                if(names.find(state->getChildNT(NT_NAME_STATE_EXPRESSION)->value) != names.end()){
                    state->getChildNT(NT_NAME_STATE_EXPRESSION)->value = newStateName;
                }
            }
            else{
                for(AST *selectCase : state->getChildNT(NT_SELECT_EXPRESSION)->children[1]->children){
                    if(names.find(selectCase->value) != names.end()){
                        selectCase->value = newStateName;
                    }
                }
            }
        }
        newState->value = newStateName;

        std::string LogM = "Merged states [";
        for(std::string name : names)
            LogM = LogM + " " + name;
        LogM = LogM + " ] into state \"" + newStateName + "\"";
        Log::MSG(LogM, LOG_LEVEL_SHORT);
    }

    std::set<int> toDelete;
    for(const std::set<int>& ses : semiEqualStates)
        for(int s : ses)
            toDelete.emplace(s);

    int dc = 0;
    int sc = 0;
    while(sc < (int) stateList->children.size()){
        if(toDelete.find(sc + dc) != toDelete.end()){
            delete stateList->children[sc];
            stateList->children.erase(stateList->children.begin() + sc);
            statePrograms.erase(statePrograms.begin() + sc);
            sc--;
            dc++;
        }
        sc++;
    }

    return (semiEqualStates.size() > 0);
}