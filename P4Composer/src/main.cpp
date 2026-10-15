#include "../include/pcheaders.h"
#include "../include/IPLBase.hpp"
#include "parser.tab.h"
#include "../include/AST.h"
#include "../include/SymbolTable.h"
#include "../include/CFG.h"
#include "../include/Log.h"
#include "../include/PreProcessor.h"

extern AST* ast;
extern FILE *yyin;
extern void reset();
extern void set_top_file_name(std::string str);

void parseArgs(const int argc, char **argv, std::vector<std::string> *P4Programs, std::vector<std::string> *P4IncludePaths, std::vector<std::vector<std::string>> *P4ProgramsPreProcessorDefines,
               bool *help, int *logLevel, char portFile[], int *cpuPort, std::string *outputDir){
  int i = 1;
  while(i < argc){
    if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")){
      *help = true;
    }
    else if(!strcmp(argv[i], "-l")){
      i++;
      if(i < argc){
        *logLevel = IPLBase::intFromStr(argv[i]);
        if(*logLevel < 0 || *logLevel > 2){
          fprintf(stderr, "Error: Invalid log level \"%s\"\n", argv[i]);
          exit(1);
        }
      }
    }
    else if(!strcmp(argv[i], "-I")){
      i++;
      if(i < argc){
        P4IncludePaths->push_back(argv[i]);
        if(P4IncludePaths->back().back() != '/')
          P4IncludePaths->back().push_back('/');
      }
    }
    else if(!strcmp(argv[i], "-D")){
      i++;
      if(i < argc){
        std::vector<std::string> split = IPLBase::split(argv[i], ',');
        int program = IPLBase::intFromStr(split[0].c_str()) - 1;
        if(split.size() != 2 || program < 0){
          fprintf(stderr, "Malformed define macro: \"%s\"\n", argv[i]);
          exit(1);
        }
        else{
          if((int) P4ProgramsPreProcessorDefines->size() < program + 1)
            P4ProgramsPreProcessorDefines->resize(program + 1, {});
          P4ProgramsPreProcessorDefines->at(program).push_back(split[1]);
        }
      }
    }
    else if(!strcmp(argv[i], "-o")){
      i++;
      if(i < argc){
        *outputDir = std::string(argv[i]);
      }
    }
    else if(!strcmp(argv[i], "--ports")){
      i++;
      if(i < argc)
        strncpy(portFile, argv[i], BUFFER_SIZE);
    }
    else if(!strcmp(argv[i], "--cpu-port")){
      i++;
      if(i < argc){
        *cpuPort = IPLBase::intFromStr(argv[i]);
        if(*cpuPort < 0){
          fprintf(stderr, "Error: Invalid cpu-port \"%s\"\n", argv[i]);
          exit(1);
        }
      }
    }
    else if(strlen(argv[i]) > 0 && argv[i][0] == '-'){
      fprintf(stderr, "Error: Unrecognized argument \"%s\"\n", argv[i]);
      exit(1);
    }
    else
      P4Programs->push_back(std::string(argv[i]));
    i++;
  }
}

void printHelp(char *progName){
  printf("Usage: %s [Options] [P4programs, ...]\n", progName);
  printf("Options:\n");
  printf("(--ports FILE)   | Specify the file containing the assigment of ports to programs.\n");
  printf("(--cpu-port N)   | Specify CPU port (used in packet-in / packet-out).\n");
  printf("(-h, --help)     | Print this help message.\n");
  printf("(-l LEVEL)       | Set log level (0 - 2).\n");
  printf("(-o PATH)        | Output to specified directory.\n");
  printf("(-I PATH)        | Add a directory to the include search path.\n");
  printf("(-D PROGN,MACRO) | Sets a preprocessor macro for program PROGN\n");
}

std::vector<std::set<int>> getVSPorts(char fileName[]){
  std::vector<std::set<int>> result;
  std::set<int> tempS;

  FILE *fp = fopen(fileName, "r");
  if(!fp){
    fprintf(stderr, "Error: Could not open file \"%s\"\n", fileName);
    exit(1);
  }

  char buffer[BUFFER_SIZE];
  int bufferI = 0;

  while(!feof(fp)){
    char c = fgetc(fp);
    if(c == '\n' || c == ' ' || feof(fp)){
      if(bufferI > 0){
        buffer[bufferI] = '\0';
        bufferI = 0;
        int portN = IPLBase::intFromStr(buffer);
        if(portN == -1){
          fprintf(stderr, "Error: Malformed ports file \"%s\"\n", fileName);
          fclose(fp);
          exit(1);
        }
        tempS.emplace(portN);
      }
      if(c == '\n' && tempS.size() > 0){
        result.push_back(tempS);
        tempS.clear();
      }
    }
    else{
      buffer[bufferI] = c;
      bufferI++;
    }
  }

  fclose(fp);

  return result;
}

int main (int argc, char **argv)
{
  char buffer[BUFFER_SIZE];
  bool help = false;
  char portFile[BUFFER_SIZE] = "NO_FILE";
  int cpuPort = -1;
  int logLevel = LOG_LEVEL_ALL;
  std::string outputDir = "output/";
  P4Architecture programsArchitecture = P4A_UNDEFINED;
  std::vector<std::string> P4IncludePaths = {"p4include/"};
  std::vector<std::string> P4Programs;
  std::vector<std::vector<std::string>> P4ProgramsCommonIncludeLists;
  std::vector<std::vector<std::string>> P4ProgramsPreProcessorDefines;

  parseArgs(argc, argv, &P4Programs, &P4IncludePaths, &P4ProgramsPreProcessorDefines, &help, &logLevel, portFile, &cpuPort, &outputDir);
  P4ProgramsPreProcessorDefines.resize(P4Programs.size(), {});

  if(help || P4Programs.size() == 0){
    printHelp(argv[0]);
    return 0;
  }

  Log::SetLogLevel(logLevel);

  std::vector<std::set<int>> virtualSwitchPorts;
  if(!strcmp(portFile, "NO_FILE") && P4Programs.size() > 1){
    fprintf(stderr, "Error: No port file provided. Use option --ports FILE.\n");
    return 1;
  }
  else if(P4Programs.size() > 1){
    virtualSwitchPorts = getVSPorts(portFile);
    if(virtualSwitchPorts.size() != P4Programs.size()){
      fprintf(stderr, "Error: Number of programs provided (%d) is different from the port specifications from file \"%s\" (%d)\n", (int) P4Programs.size(), portFile, (int) virtualSwitchPorts.size());
      return 1;
    }
  }
  
  while(outputDir.back() == '/')
    outputDir.pop_back();
  std::filesystem::remove_all(outputDir);
  std::filesystem::create_directory(outputDir);
  if(!std::filesystem::is_directory(outputDir)){
    fprintf(stderr, "Error: Could not create output directory \"%s\"\n", outputDir.c_str());
    return 1;
  }
  std::filesystem::create_directory(outputDir + "/graphs");

  Log::SetOutputDir(outputDir);
  CFG::SetOutputDir(outputDir);

  //PreProcessor
  for(int i = 0; i < (int) P4Programs.size(); i++){
    PreProcessor pp(P4Programs[i], P4IncludePaths, P4ProgramsPreProcessorDefines[i], PPV_NONE);
    //pp.SetVerbosity(PPV_ALL);
    pp.Process();
    sprintf(buffer, "%s/ir%d.p4", outputDir.c_str(), i+1);
    FILE *fir = fopen(buffer, "w");
    pp.Print(fir);
    fclose(fir);

    if(i == 0)
      programsArchitecture = pp.getArch();
    
    if(pp.getArch() == P4A_UNDEFINED && P4Programs.size() > 1){
      fprintf(stderr, "Error: Could not detect architecture of program \"%s\"\n", P4Programs[i].c_str());
      return 1;
    }
    if(pp.getArch() != programsArchitecture){
      fprintf(stderr, "Error: Different program architectures detected\n");
      return 1;
    }

    P4ProgramsCommonIncludeLists.push_back(pp.getCommonIncludeList());
  }

  std::vector<AST*> ProgramASTs;
  for(int i = 0; i < (int) P4Programs.size(); i++){
    Log::MSG("Parsing program \"" + P4Programs[i] + "\"", LOG_LEVEL_SHORT);
    Log::Push();

    sprintf(buffer, "%s/ir%d.p4", outputDir.c_str(), i+1);
    FILE *fin = fopen(buffer, "r");
    if(!fin){
      fprintf(stderr, "Could not open file \"%s\"\n", P4Programs[i].c_str());
      return 1;
    }
    yyin = fin;
    set_top_file_name(P4Programs[i].c_str());
    int ret = yyparse();
    fclose(fin);
    if(ret != 0){
      return ret;
    }

    ast->SimplifyConstExpressions();

    ast->setValue(P4Programs[i].c_str());

    if(!ast->CheckLegal(true, ast->getValue())){
      return 1;
    }

    ast->RemoveConstantDeclarations();
    ast->RemoveEmptyLists();
    ast->SubstituteTypedef();
    ast->UnifyErrorMKDeclarations();

    Log::EnablePush(false);
    ast->InferTypes();
    ast->RemoveMergePrefixes();
    Log::EnablePop();

    sprintf(buffer, "%s/P%d-parsed.p4", outputDir.c_str(), i+1);
    FILE *fparsed = fopen(buffer, "w");
    ast->writeCode(fparsed);
    fclose(fparsed);

    Log::MSG("Removing instantiations", LOG_LEVEL_SHORT);
    Log::Push();
    ast->RemoveInstantiations();
    Log::Pop();

    Log::MSG("Checking parsers states", LOG_LEVEL_SHORT);
    Log::Push();
    ast->CheckParsersStates();
    Log::Pop();

    Log::MSG("Removing simple parser states", LOG_LEVEL_SHORT);
    Log::Push();
    ast->RemoveSimpleParserStates();
    Log::Pop();

    Log::MSG("Removing empty if / elses", LOG_LEVEL_SHORT);
    Log::Push();
    ast->RemoveEmptyIfElses();
    Log::Pop();

    ast->ExpandEmits();

    ast->SimplifyConstExpressions();

    ProgramASTs.push_back(ast->Clone());

    delete ast;
    reset();
    Log::Pop();
  }

  CFG::SetProgramNames(P4Programs);
  CFG::SetProgramASTs(ProgramASTs);
  CFG::SetArchitecture(programsArchitecture);

  for(int i = 0; i < (int) P4Programs.size(); i++){
    sprintf(buffer, "%s/graphs/P%d-preprocessed-AST.dot", outputDir.c_str(), i+1);
    FILE *fpast = fopen(buffer, "w");
    ProgramASTs[i]->GenDot(fpast);
    fclose(fpast);

    //Clone the AST to remove declarations that are common to all programs
    AST* parsedAST = ProgramASTs[i]->Clone();
    sprintf(buffer, "%s/P%d-preprocessed.p4", outputDir.c_str(), i+1);
    FILE *fpreprocessed = fopen(buffer, "w");
    
    parsedAST->RemoveErrorAndMKDeclarations();
    parsedAST->RemoveDeclarationsFromIncludesByAnotation(P4ProgramsCommonIncludeLists[i]);
    for(auto it : P4ProgramsCommonIncludeLists[i]){
      fprintf(fpreprocessed, "#include <%s>\n", it.c_str());
    }
    fprintf(fpreprocessed, "\n");
    parsedAST->writeCode(fpreprocessed);
    fclose(fpreprocessed);

    std::vector<std::pair<AST*, AST*>> newHeaders = parsedAST->GetHeadersWithRemovedUnusedFields();
    for(auto p : newHeaders){
      ProgramASTs[i]->SubstituteAndDeleteChildren(p.first->getValue(), p.second);
    }

    delete parsedAST;
  }

  AST *merged = NULL;

  if(ProgramASTs.size() > 1){
    bool mergeOk = false;
    while(!mergeOk){
      mergeOk = true;

      std::vector<AST*> tempProgramClones(ProgramASTs.size());
      for(size_t i = 0; i < ProgramASTs.size(); i++)
        tempProgramClones[i] = ProgramASTs[i]->Clone();
    
      try{
        merged = AST::MergeAST(tempProgramClones, virtualSwitchPorts, programsArchitecture, cpuPort);
      }
      catch(ExceptionWrongEmitOrder eweo){
        mergeOk = false;
        ProgramASTs[eweo.program]->AddPreventMergeAnnotationBasedOnMergedEmit(eweo.emitNum, eweo.deparserName);
      }

      for(size_t i = 0; i < tempProgramClones.size(); i++)
        delete tempProgramClones[i];
    }
    merged->RemoveErrorAndMKDeclarations();

    FILE *fp = fopen((outputDir + "/graphs/merged.dot").c_str(), "w");
    merged->GenDot(fp);
    fclose(fp);

    FILE *fmerged = fopen((outputDir + "/merged.p4").c_str(), "w");
    
    CFG::GenerateP4Infos(merged);

    std::vector<std::string> commonIncludes = P4ProgramsCommonIncludeLists[0];
    for(size_t i = 1; i < P4ProgramsCommonIncludeLists.size(); i++){
      for(size_t j = 0; j < P4ProgramsCommonIncludeLists[i].size(); j++){
        bool found = false;
        for(size_t k = 0; k < commonIncludes.size(); k++){
          if(P4ProgramsCommonIncludeLists[i][j] == commonIncludes[k]){
            found = true;
            break;
          }
        }
        if(!found)
          commonIncludes.push_back(P4ProgramsCommonIncludeLists[i][j]);
      }
    }
    merged->RemoveDeclarationsFromIncludesByAnotation(commonIncludes);
    for(auto it : commonIncludes){
      fprintf(fmerged, "#include <%s>\n", it.c_str());
    }
    fprintf(fmerged, "\n");
    
    if(cpuPort >= 0){
      if(cpuPort > 511 && (programsArchitecture == P4A_V1MODEL || programsArchitecture == P4A_TNA))
        fprintf(stderr, "Warning: cpu-port does not fit in 9 bits as specified in architecture (%d)\n", cpuPort);
      fprintf(fmerged, "#define CPU_PORT %d\n\n", cpuPort);
    }
    
    merged->writeCode(fmerged);
    fclose(fmerged);

    for(AST *declaration : merged->getChildren()){
      if(declaration->getNodeType() == NT_PARSER_DECLARATION){
        declaration->WriteParserDOT(outputDir + "/graphs/" + declaration->getChildNT(NT_PARSER_TYPE_DECLARATION)->getValue() + "-States.dot");
        declaration->GenDot(outputDir + "/graphs/" + declaration->getChildNT(NT_PARSER_TYPE_DECLARATION)->getValue() + ".dot");
      }
      else if(declaration->getNodeType() == NT_CONTROL_DECLARATION)
        declaration->GenDot(outputDir + "/graphs/" + declaration->getChildNT(NT_CONTROL_TYPE_DECLARATION)->getValue() + ".dot");
    }
  }
  else{
    CFG::GenerateP4Infos(NULL);
  }

  NullDelete(merged);

  for(AST *ast : ProgramASTs)
    delete ast;

  return 0;
}
