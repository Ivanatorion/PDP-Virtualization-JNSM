#include "../include/pcheaders.h"
#include "../include/Log.h"

#define F_NAME "log.txt"

Log::Log(){
    this->m_fp = NULL;
    //this->m_fp = stdout;
    this->m_stack.push_back(0);
    this->enable = true;
    this->m_outputDir = "output/";
    this->m_logLevel = LOG_LEVEL_ALL;
}

Log::~Log(){
    if(m_fp != NULL && m_fp != stdout)
        fclose(m_fp);
}

void Log::SetOutputDir(const std::string& outputDir){
    Log::Get().m_outputDir = outputDir;
    if(Log::Get().m_outputDir.back() != '/')
        Log::Get().m_outputDir.push_back('/');
}

void Log::SetLogLevel(const int logLevel){
    Log::Get().m_logLevel = logLevel;
}

Log& Log::Get(){
    static Log instance;
    return instance;
}

void Log::Reset(){
    Log::Get().m_stack.clear();
    Log::Get().m_stack.push_back(0);
    Log::Get().m_enableStack.clear();
    Log::Get().enable = true;
}

void Log::Push(){
    Log::Get().m_stack.push_back(0);
}

void Log::Pop(){
    Log::Get().m_stack.pop_back();
}

void Log::MSG(const std::string& msg, const int LOG_LEVEL){
    if(!Log::Get().enable || Log::Get().m_logLevel < LOG_LEVEL)
        return;

    if(Log::Get().m_fp == NULL){ //First Message
        Log::Get().m_fp = fopen((Log::Get().m_outputDir + F_NAME).c_str(), "w");
        if(!Log::Get().m_fp){
            fprintf(stderr, "Error: Could not open file \"%s\"\n", (Log::Get().m_outputDir + F_NAME).c_str());
            exit(1);
        }
    }
    
    Log::Get().m_stack[Log::Get().m_stack.size() - 1]++;
    for(int i = 0; i < (int) Log::Get().m_stack.size() - 1; i++)
        fprintf(Log::Get().m_fp, "%d.", Log::Get().m_stack[i]);
    fprintf(Log::Get().m_fp, "%d) ", Log::Get().m_stack[Log::Get().m_stack.size() - 1]);
    fprintf(Log::Get().m_fp, "%s\n", msg.c_str());
}

void Log::Save(){
    if(Log::Get().m_fp != stdout){
        fclose(Log::Get().m_fp);
        Log::Get().m_fp = fopen((Log::Get().m_outputDir + F_NAME).c_str(), "a");
    }
}

bool Log::isEnable(){
    return Log::Get().enable;
}

void Log::Enable(const bool enable){
    Log::Get().enable = enable;
}

void Log::EnablePush(const bool enable){
    Log::Get().m_enableStack.push_back(Log::Get().enable);
    Log::Get().enable = enable;
}

void Log::EnablePop(){
    if(Log::Get().m_enableStack.size() > 0){
        Log::Get().enable = Log::Get().m_enableStack[Log::Get().m_enableStack.size() - 1];
        Log::Get().m_enableStack.pop_back();
    }
}
